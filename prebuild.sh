#!/bin/bash

QEMU=
UBUNTU=
PREBUILD=
ALL=

# Build the big files
while getopts qupa flags
do
        case ${flags} in
                q) QEMU="y";;
                u) UBUNTU="y";;
                p) PREBUILD="y";;
                a) ALL="y";;
                ?) echo "Use -a: create all; -q: build Qemu; -u: create Ubuntu image"
                        exit 1;;
        esac
done

if [[ -n ${QEMU} ]] || [[ -n ${UBUNTU} ]] || [[ -n ${PREBUILD} ]]; then
        if [[ -n ${ALL} ]]; then
                unset ALL
                echo "Flag '-a' will be ignored"
        fi
fi

export DEBIAN_FRONTEND=noninteractive

#Qemu-CCA
if [[ -n ${QEMU} ]] || [[ -n ${ALL} ]]; then
        echo "Build Qemu-CCA"
        if [[ ! -d ./qemu ]]; then
                echo " Qemu-CCA not found - install"
                sudo apt-get install -yq git python3-venv python3-pyelftools \
                     python3-tomli libglib2.0-dev acpica-tools libssl-dev \
                     libpixman-1-dev device-tree-compiler flex bison rsync curl \
                     ninja-build meson python3-sphinx
                     git clone -b cca/v3 --single-branch https://git.codelinaro.org/linaro/dcap/qemu
        else
                echo "Qemu-CCA found: make clean"
                make clean -C qemu
        fi
        echo "Build qemu"
        pushd qemu/
        ./configure --target-list=aarch64-softmmu --enable-slirp --disable-alsa \
                    --disable-coreaudio --disable-dsound --disable-jack --disable-pa \
                    --disable-bsd-user --disable-xen
        make -j$(nproc -1)
        popd
fi

#Minimal Ubuntu22.04 img
if [[ -n ${UBUNTU} ]] || [[ -n ${ALL} ]]; then
        echo "Create Ubuntu22.04 Image for aarch64"
        if [[ -f ./ubuntu/ubuntu22.img ]]; then
                echo "Old Img-file exists - remove"
                sudo rm ./ubuntu/ubuntu22.img
        fi
        sudo ./ubuntu/create_img.sh ./ubuntu/ubuntu22.img
fi
unset DEBIAN_FRONTEND

#Unpack prebuilds
if [[ -n ${PREBUILD} ]] || [[ -n ${ALL} ]]; then
        echo "Unpack TF-A and linux-cca in bin/"
        if [[ -d ./bin ]]; then
                echo "Remove previous ./bin from unpacked prebuildfw.tar.gz"
                rm -rf ./bin
        fi
        tar -xvf prebuildfw.tar.gz
fi
