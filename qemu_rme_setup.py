import shutil
import os
import sys
import tarfile
import subprocess
import socket
import time
import argparse
from urllib import request
from pathlib import Path
from threading import Thread


class cd:
    def __init__(self, new_path):
        self.new_path = os.path.expanduser(new_path.as_posix())

    def __enter__(self):
        self.prev_home = os.getcwd()
        os.chdir(self.new_path)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.prev_home)

class RmeStackComponentX:
    def __init__(self, _url, _commands, _env=None, _script=False, _branch=None):
        self.url= _url
        self.commands = _commands
        self.script = _script
        self.env = _env
        self.branch = _branch

    def component_setup(self, root_path, script_path):
        thread_cwd = root_path
        comp_path = root_path 
        if self.script is False:
            if "git" not in self.url:
                comp_path = comp_path.joinpath(self.url)
                if comp_path.exists() == False:
                    comp_path.mkdir(parents=True)
            else:
                path_comp = self.url.split('/')
                comp_name = path_comp[-1].removesuffix('.git')
                comp_path = comp_path.joinpath(comp_name)
                if comp_path.exists() == False:
                    git_branch = ''
                    if self.branch:
                        git_branch = f'-b {self.branch}'
                    subprocess.run(f'git clone {self.url} {git_branch}', shell=True, check=True, executable='/bin/bash')
            thread_cwd = comp_path
        else:
            comp_path = comp_path.joinpath(self.url)
            if root_path.as_posix() != script_path.as_posix() and comp_path.exists() == False:
                shutil.copyfile(script_path.joinpath(self.url).as_posix(), comp_path.as_posix())
        if self.env is not None:
            for env in self.env:
                os.environ[env[0]] = env[1]
        print(f'>> Component:{comp_path.as_posix()} - cwd:{thread_cwd.as_posix()}')
        bash_cmd = ' && '.join(self.commands)
        subprocess.run(bash_cmd, shell=True, check=True, executable='/bin/bash', cwd=thread_cwd.as_posix())

def run_tasks(root_path, script_path, task_lists):
    with cd(root_path):
        thread_lists = [[]] * len(task_lists)
        i = 0
        for task_list in task_lists:
            for task in task_list:
                thread = Thread(target=task.component_setup(root_path, script_path))
                thread_lists[i].append(thread)
                thread.start()
            for thread in thread_lists[i]:
                thread.join()
            i = i + 1
    print(">> All needed componets are now built")

def main():
    print(">> Setup Qemu-RME stack")
    script_path = Path(os.path.abspath(__file__)).parent
    argpars = argparse.ArgumentParser()
    argpars.add_argument('--dir', help='Abspath to root directory where to build RME. \
    \nDefault {path}'.format(path=script_path), type=Path, default=Path(script_path))
    args = argpars.parse_args()
    root_path = args.dir
    print(f'>> Script path:{script_path.as_posix()} - project root:{root_path.as_posix()}')
    #[*]
    tf_rmm = RmeStackComponentX('https://git.codelinaro.org/linaro/dcap/rmm.git',
                                ['git submodule update --init --recursive',
                                 'cmake -DCMAKE_BUILD_TYPE=Debug '
                                 '-DRMM_CONFIG=qemu_virt_defcfg -B build-qemu',
                                 'cmake --build build-qemu'],
                                 [('CROSS_COMPILE','aarch64-none-elf-')],
                                _branch='cca/v3')
    #[*]
    edk2_host = RmeStackComponentX('https://github.com/tianocore/edk2',
                                   ['git submodule update --init --recursive',
                                    'source edksetup.sh',
                                    'make -j30 -C BaseTools',
                                    'build -b RELEASE -a AARCH64 -t GCC5 '
                                    '-p ArmVirtPkg/ArmVirtQemuKernel.dsc',
                                    'mv ../edk2 ../edk2_host'],
                                   [('GCC5_AARCH64_PREFIX','aarch64-linux-gnu-')])
    #[]
    tf_a = RmeStackComponentX('https://git.codelinaro.org/linaro/dcap/tf-a/'
                              'trusted-firmware-a',
                              ['make -j CROSS_COMPILE=aarch64-linux-gnu- '
                              'PLAT=qemu ENABLE_RME=1 DEBUG=1 LOG_LEVEL=40 '
                              'QEMU_USE_GIC_DRIVER=QEMU_GICV3 '
                              'RMM=../rmm/build-qemu/Debug/rmm.img '
                              'BL33=../edk2_host/Build/ArmVirtQemuKernel-AARCH64/RELEASE_GCC5/'
                              'FV/QEMU_EFI.fd all fip',
                              'dd if=build/qemu/debug/bl1.bin of=flash.bin',
                              'dd if=build/qemu/debug/fip.bin of=flash.bin seek=64 bs=4096'],
                              _branch='cca/v3')
    #[*]
    linux_cca = RmeStackComponentX('https://gitlab.arm.com/linux-arm/linux-cca',
                                   ['make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 defconfig',
                                    'scripts/config -e VIRT_DRIVERS -e ARM_CCA_GUEST',
                                    'make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 -j30'],
                                   _branch='cca-full/v3')
    qemu_platform_emul = RmeStackComponentX('https://git.codelinaro.org/linaro/dcap/qemu',
                                  ['./configure --target-list=aarch64-softmmu --enable-slirp',
                                   'make -j30'], _branch='cca/v3')
    #[*]
    ubuntu_fs = RmeStackComponentX('ubuntu_img/ubuntu_fs.sh',
                                   ['chmod +x ./ubuntu_img/ubuntu_fs.sh',
                                    'sudo ./ubuntu_img/ubuntu_fs.sh ubuntu_img/ubuntu22.img',
                                    'sudo chmod 666 ubuntu_img/ubuntu22.img'],
                                   _script=True)

    print(">> Check if 'aarch64-none-elf-' toolchain is installed")
    toolchain_path = shutil.which("aarch64-none-elf-gcc")
    if toolchain_path is None:
        _rpath  = root_path
        tool_dir = _rpath.joinpath('tools')
        if tool_dir.exists() == False:
            os.makedirs(tool_dir.as_posix())
        with cd(tool_dir):
            toolchain = 'arm-gnu-toolchain-13.3.rel1-aarch64-aarch64-none-elf'
            url = f'https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/{toolchain}.tar.xz'
            print(f'>> Get \'aarch64-none-elf-\' toolchain from:{url}')
            response = request.urlopen(request.Request(url))
            if response.status == 200:#Ok
                with open(toolchain+'.tar.xz', 'w+b') as tar:
                    tar.write(response.read())
                    with tarfile.open(toolchain+'.tar.xz', 'r:xz') as f:
                        f.extractall(path='.')
                        toolchain_path = os.path.abspath(f'./{toolchain}/bin')
                        os.environ['PATH'] += ':'+toolchain_path
            else:
                sys.exit('>> Failed to get {}'.format(toolchain))
    else:
        print('>> Found toolchain')
    print(f'>> Path to toolchain:{toolchain_path}')
    task_batch_0 = [tf_rmm, edk2_host, linux_cca, qemu_platform_emul]
    task_batch_1 = [tf_a, ubuntu_fs]
    run_tasks(root_path, script_path, [task_batch_0, task_batch_1])
    print(">> Create 2 additional terminal windows and start in each a TCP lister: nc -l 5432{3,4}")
    exit(">> If done call ./qemu_system_launcher_host.sh to start the guest-platform.")

# TODO:
   # A) Wait for the tcp servers
   # B) Call qemu qemu_system_launcher_host
   # ports = [54320, 54322, 54321, 54324]
   # start_wait = time.time()
   # opened = False
   # while time.time() - start_wait < 60*3:
   #     _open = 1
   #     for port in ports:
   #         with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
   #             print(f'>> Check port:{port}')
   #             if sock.connect_ex(('127.0.0.1', port)) != 0:
   #                 print(f'>> Check port:{port} - CLOSED')
   #                 _open = 0
   #             else:
   #                 print(f'>> Check port:{port} - OPEND')
   #     if _open == 1:
   #         opened = True
   #         break

   # if not opened:
   #     exit('Failed to connect to TCP servers')
   # B) Call qemu qemu_system_launcher_host

if __name__ == "__main__":
    main()
