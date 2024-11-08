#!/bin/bash

root=$(dirname $(realpath $0))
img_name="$1"
directory=$root/"ubuntu_fs"
realm_user="realm"
realm_password="realm"
arch=$(uname -i)
if [[ $EUID -ne 0 ]]; then
   echo "Need to run with sudo"
   exit 1
fi

# Check if the img file name parameter is provided
if [ -z "$img_name" ]; then
    echo "Please provide the generated img file name as the first parameter!"
    exit 1
fi

#Generate a 4GB img file for guest
echo "Generating 4GB guest img file $img_name ..."
dd if=/dev/zero of=$img_name bs=1GB count=4 || exit 1

# Format the img file as ext4 file system
echo "Formatting the img file as ext4 file system..."
mkfs.ext4 -F "$img_name" || exit 1

# Check if the directory exists
if [ -d "$directory" ]; then
    echo "Directory $directory exists. Deleting its contents..."
    rm -rf "$directory"/*
else
    echo "Directory $directory does not exist. Creating the directory..."
    mkdir "$directory"
fi

# Mount the img file to the ubuntu_fs directory
echo "Mounting the img file to directory $directory..."
mount -o loop "$img_name" "$directory" || exit 1

# Download the file
echo "Downloading file $archive_file ..."
archive_url="https://cdimage.ubuntu.com/ubuntu-base/releases/22.04.4/release/ubuntu-base-22.04.4-base-arm64.tar.gz"
archive_file="ubuntu-base-22.04.4-base-arm64.tar.gz"
wget "$archive_url" -P "$directory"

# Extract the file
echo "Extracting file $archive_file to directory $directory..."
tar -xf "$directory/$archive_file" -C "$directory"

# Remove the downloaded archive file
echo "Removing downloaded archive file $archive_file ..."
rm "$directory/$archive_file"

# Write nameserver to resolv.conf file
echo "Writing nameserver to $directory/etc/resolv.conf file..."
echo "nameserver 8.8.8.8" > "$directory/etc/resolv.conf"

cat > "$directory/etc/apt/sources.list" << EOF
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy main restricted
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-updates main restricted
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy universe
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-updates universe
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy multiverse
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-updates multiverse
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-backports main restricted universe multiverse
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-security main restricted
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-security universe
deb http://nova.clouds.ports.ubuntu.com/ubuntu-ports/ jammy-security multiverse
EOF

#Arm64 chroot
if [[ $arch != a* ]]; then
        sudo apt-get install -yq binfmt-support qemu-user-static
        sudo cp -av /usr/bin/qemu-aarch64-static $directory/usr/bin
fi

# Switch to chroot environment and execute apt command
echo "Switching to chroot environment and executing apt command..."
mount -t proc /proc $directory/proc
mount -t sysfs /sys $directory/sys
mount -o bind /dev $directory/dev
mount -o bind /dev/pts $directory/dev/pts
chroot "$directory" /bin/bash -c "apt update -y" || exit 1

# Create a new user with sudo privileges
echo "Creating user $realm_user with sudo privileges..."
chroot "$directory" /bin/bash -c "useradd -m -s /bin/bash -G sudo $realm_user" || exit 1

# Set the password for the new user
echo "Setting password for user $realm_user..."
echo "$realm_user:$realm_password" | chroot "$directory" /bin/bash -c "chpasswd" || exit 1

chroot "$directory" /bin/bash -c "chmod 1777 /tmp" || exit 1

echo "Generate the init file"
cat > "$directory/init" << EOF
#!/bin/sh

[ -d /dev ] || mkdir -m 0755 /dev
[ -d /root ] || mkdir -m 0700 /root
[ -d /sys ] || mkdir /sys
[ -d /proc ] || mkdir /proc
[ -d /tmp ] || mkdir /tmp
mkdir -p /var/lock
mount -t sysfs -o nodev,noexec,nosuid sysfs /sys
mount -t proc -o nodev,noexec,nosuid proc /proc
# Some things don't work properly without /etc/mtab.
ln -sf /proc/mounts /etc/mtab

grep -q '\<quiet\>' /proc/cmdline || echo "Loading, please wait..."

# Note that this only becomes /dev on the real filesystem if udev's scripts
# are used; which they will be, but it's worth pointing out
if ! mount -t devtmpfs -o mode=0755 udev /dev; then
        echo "W: devtmpfs not available, falling back to tmpfs for /dev"
        mount -t tmpfs -o mode=0755 udev /dev
        [ -e /dev/console ] || mknod -m 0600 /dev/console c 5 1
        [ -e /dev/null ] || mknod /dev/null c 1 3
fi
mkdir /dev/pts
mount -t devpts -o noexec,nosuid,gid=5,mode=0620 devpts /dev/pts || true
mount -t tmpfs -o "noexec,nosuid,size=10%,mode=0755" tmpfs /run
mkdir /run/initramfs
# compatibility symlink for the pre-oneiric locations
ln -s /run/initramfs /dev/.initramfs 

# Set modprobe env
export MODPROBE_OPTIONS="-qb"

# mdadm needs hostname to be set. This has to be done before the udev rules are called!
if [ -f "/etc/hostname" ]; then
        /bin/hostname -b -F /etc/hostname 2>&1 1>/dev/null
fi

exec /sbin/init
EOF

chmod +x $directory/init || exit 1

chroot "$directory" /bin/bash -c "apt install systemd iptables -y" || exit 1
chroot "$directory" /bin/bash -c "ln -s /lib/systemd/systemd /sbin/init" || exit 1

echo "Install other essential components, in case of booting blocking at /dev/hvc0 failed to bring up"
chroot "$directory" /bin/bash -c "apt install build-essential network-manager git vim bash-completion \
        net-tools iputils-ping ifupdown ethtool ssh rsync udev htop rsyslog curl openssh-server \
        apt-utils dialog nfs-common psmisc language-pack-en-base \
        sudo kmod apt-transport-https libcap-dev -y" || exit 1
chroot "$directory" /bin/bash -c "systemctl enable systemd-networkd.service" || exit 1

cat > "$directory/etc/hosts" << EOF
127.0.0.1       localhost.localdomain localhost
127.0.1.1       arm1
EOF

cat >> "$directory/etc/network/interfaces.d/05-eth0.network" << EOF
auto lo
iface lo inet loopback
gateway 192.168.1.2
auto enp0s2
iface enp0s2 inet static
        address 192.168.1.101
        netmask 255.255.255.0
        network 192.168.1.0
        broadcast 192.168.1.255
        gateway 192.168.1.2
        dns-nameservers 8.8.8.8 8.8.4.4
EOF

mkdir -p $directory/etc/systemd/system/systemd-networkd-wait-online.service.d
cat >> "$directory/etc/systemd/system/systemd-networkd-wait-online.service.d/waitany.conf" << EOF
[Service]
ExecStart=
ExecStart=/lib/systemd/systemd-networkd-wait-online --any -i lo:missing -i enp0s2:missing
EOF

# Unmount the mounted directory
echo "Unmounting the mounted directory $directory ..."
umount $directory/proc
umount $directory/sys
umount $directory/dev/pts
umount $directory/dev
umount "$directory"

echo "Operation completed!"
