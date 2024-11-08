#!/bin/bash

# Build the big files

#Qemu-CCA
echo "Build Qemu-CCA"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get install -yq git python3-venv python3-pyelftools \
     python3-tomli libglib2.0-dev acpica-tools libssl-dev \
     libpixman-1-dev device-tree-compiler flex bison rsync curl \
     ninja-build meson python3-sphinx
git clone -b cca/v3 --single-branch https://git.codelinaro.org/linaro/dcap/qemu
pushd qemu/
./configure --target-list=aarch64-softmmu --enable-slirp --disable-alsa --disable-coreaudio \
        --disable-dsound --disable-jack --disable-pa --disable-bsd-user --disable-xen
make -j$(nproc -1)
popd

#Minimal Ubuntu22.04 img
echo "Create Ubuntu22.04 Image for aarch64"
sudo ./ubuntu/create_img.sh ./ubuntu/ubuntu22.img
unset DEBIAN_FRONTEND

#Unpuck prebuilds
echo "Unpack TF-A and linux-cca in bin/"
tar -xvf prebuildfw.tar.gz
