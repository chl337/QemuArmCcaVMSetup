#!/bin/bash

# This shares the current directory with the host, providing the files needed
# to launch the guest:
#        -device virtio-9p-device,fsdev=shr0,mount_tag=shr0 \
#        -fsdev local,security_model=none,path=.,id=shr0 \
#
# Debug: -s
#

# The following parameters allow to use separate consoles for host (54324) and RMM (54324).

#        -serial tcp:localhost:54321 \
#        -chardev socket,mux=on,id=hvc1,port=54323,host=localhost \
#        -device virtio-serial-device \
#        -device virtconsole,chardev=hvc1 \
QEMUDIR=$(realpath ./qemu/build)
BIN=$(realpath ./bin)
TFA=${BIN}/trusted-firmware-a
LINUX_CCA=${BIN}/linux-cca
OS_IMG=./ubuntu/ubuntu22.img
SHDIR="$1"

if [ -z "$SHDIR" ]; then
        SHDIR="./shdir"
fi

echo "Start GuestPlaftform with Arm CCA support"
echo "GuestPlaftform: Shared directory: $(realpath ${SHDIR}) can be mount with 'mount -t 9p shr0 /mnt'"
echo "Host: after booting GuestPlaftform use: ssh -p 50022 realm@localhost"

${QEMUDIR}/qemu-system-aarch64 -M virt,virtualization=on,secure=on,gic-version=3 \
        -M acpi=off -cpu max,x-rme=on -m 8G -smp 4 \
        -nographic \
        -bios ${TFA}/flash.bin \
        -kernel ${LINUX_CCA}/Image \
        -drive format=raw,if=none,file=${OS_IMG},id=hd0 \
        -device virtio-blk-pci,drive=hd0 \
        -nodefaults \
        -serial tcp:localhost:54323 \
        -chardev socket,mux=on,id=hvc0,port=54324,host=localhost \
        -device virtio-serial-device \
        -device virtconsole,chardev=hvc0 \
        -append "root=/dev/vda rw console=hvc0" \
        -device virtio-net-pci,netdev=net0 \
        -netdev user,net=192.168.1.0/24,dhcpstart=192.168.1.109,hostfwd=tcp:127.0.0.1:50022-192.168.1.101:22,id=net0 \
        -device virtio-9p-device,fsdev=shr0,mount_tag=shr0 \
        -fsdev local,security_model=none,path=${SHDIR},id=shr0
