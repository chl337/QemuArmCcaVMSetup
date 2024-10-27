use bitfield_struct::bitfield;
use byteorder::{ByteOrder, LittleEndian};
use kvm_ioctls::VcpuExit;
use kvm_ioctls::{Kvm, DeviceFd, VcpuFd, VmFd};

use core::panic;
use std::fs::File;
use std::io::Read;
use std::io::Write;

use libc::EFD_NONBLOCK;
use std::{slice, u32, u128};
use nix::sched::{sched_setaffinity,sched_getcpu, CpuSet};
use nix::unistd::Pid;

use std::thread;
use vmm_sys_util::ioctl_ioc_nr;
use vmm_sys_util::eventfd::EventFd;

const KVM_CAP_USER_MEMORY2: u32 = 231;
const KVM_CAP_MEMORY_ATTRIBUTES: u32 = 233;
const KVM_CAP_GUEST_MEMFD: u32 = 234;
const KVM_MEMORY_ATTRIBUTE_PRIVATE: u64 = 1 << 3;
const KVM_MEM_GUEST_MEMFD: u32 = 1 << 2;

/* KVM_CAP_ARM_RME kvm_enable_cap->args[0] points to this */
const KVM_CAP_ARM_RME_CONFIG_REALM: u64 = 0;
const KVM_CAP_ARM_RME_CREATE_RD: u64 = 1;
const KVM_CAP_ARM_RME_INIT_IPA_REALM: u64 = 2;
const KVM_CAP_ARM_RME_POPULATE_REALM: u64 = 3;
const KVM_CAP_ARM_RME_ACTIVATE_REALM: u64 = 4;
const KVM_CAP_ARM_VM_SIZE: u64 = 165;

const KVM_CAP_ARM_RME_POPULATE_FLAGS_MEASURE: u32 = 1u32 << 0;
const KVM_CAP_ARM_RME_MEASUREMENT_ALGO_SHA256: u32 = 0;
const KVM_CAP_ARM_RME_MEASUREMENT_ALGO_SHA512: u32 = 1;

const KVM_CAP_ARM_RME_RPV_SIZE: u64 = 64;

/* List of configuration items accepted for KVM_CAP_ARM_RME_CONFIG_REALM */

const KVM_CAP_ARM_RME_CFG_RPV: u64 = 0;
const KVM_CAP_ARM_RME_CFG_HASH_ALGO: u64 = 1;
const KVM_ARM_VCPU_PTRAUTH_ADDRESS : u64 = 5; /* VCPU uses address authentication */
const KVM_ARM_VCPU_PTRAUTH_GENERIC : u64 = 6;
const KVM_ARM_VCPU_HAS_EL2: u64 = 7; //Support nested virtualization
const KVM_ARM_VCPU_REC: u64 = 8; //vCPU Rec state as part of Realm
const KVM_CAP_ARM_RME: u32 = 300;

const KVM_ARM_VCPU_SVE: u32 = 4;

const KVMIO: u32 = 0xAE;
const KVM_ENABLE_CAP: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0xA3,
    std::mem::size_of::<kvm_bindings::kvm_enable_cap>() as u32,
) as u64;

const KVM_ARM_VCPU_FINALIZE: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0xc2,
    std::mem::size_of::<libc::c_int>() as u32,
) as u64;
const KVM_RUN: u64 =
    vmm_sys_util::ioctl::ioctl_expr(vmm_sys_util::ioctl::_IOC_NONE, KVMIO, 0x80, 0) as u64;

const KVM_GET_DEVICE_ATTR: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0xe2,
    std::mem::size_of::<kvm_bindings::kvm_device_attr>() as u32,
) as u64;

const KVM_SET_DEVICE_ATTR: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0xe1,
    std::mem::size_of::<kvm_bindings::kvm_device_attr>() as u32,
) as u64;

const KVM_CREATE_GUEST_MEMFD: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE | vmm_sys_util::ioctl::_IOC_READ,
    KVMIO,
    0xd4,
    std::mem::size_of::<KvmCreateGuestMemFd>() as u32
) as u64;

const KVM_SET_USER_MEMORY_REGION2: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0x49,
    std::mem::size_of::<KvmUserspaceMemoryRegion2>() as u32
) as u64;

vmm_sys_util::ioctl_io_nr!(KVM_CHECK_EXTENSION, KVMIO, 0x03);

const KVM_ARM_VCPU_PSCI_0_2: u32 = 2;
const KVM_CAP_ARM_PSCI_0_2: u64 = 102;

const KVM_VM_TYPE_ARM_SHIFT: u64 = 8;
const KVM_VM_TYPE_ARM_MASK: u64 = (0xf as u64) << KVM_VM_TYPE_ARM_SHIFT;
const KVM_VM_TYPE_ARM_IPA_SIZE_MASK: u64 = 0xff as u64;

macro_rules! KVM_VM_TYPE_ARM {
    ($_type:expr) => {{
        ((($_type) << KVM_VM_TYPE_ARM_SHIFT) & KVM_VM_TYPE_ARM_MASK)
    }};
}

macro_rules! KVM_VM_TYPE_ARM_IPA_SIZE {
    ($a:expr) => {{
        $a & KVM_VM_TYPE_ARM_IPA_SIZE_MASK
    }};
}

const KVM_VM_TYPE_ARM_NORMAL: u64 = KVM_VM_TYPE_ARM!(0);
const KVM_VM_TYPE_ARM_REALM: u64 = KVM_VM_TYPE_ARM!(1);

#[repr(C, align(1))]
union KvmCapArmRmeConfigField {
    rpv: [u8; KVM_CAP_ARM_RME_RPV_SIZE as usize], /* cfg == KVM_CAP_ARM_RME_CFG_RPV */
    hash_algo: u32,                               /* cfg == KVM_CAP_ARM_RME_CFG_HASH_ALGO */
    reserved: [u8; 256],                          /* Fix the size of the union */
}

#[repr(C, align(1))]
struct KvmCapArmRmeConfigItem {
    cfg: u32,
    u_field: KvmCapArmRmeConfigField,
}

#[repr(C, align(1))]
struct KvmCapArmRmeConfigHash {
    cfg: u32,
    hash_algo: u32,
    pad: [u8; 256 - 4],
}

#[repr(C)]
struct KvmCapArmRmePopulateRealmArgs {
    populate_ipa_base: u64,
    populate_ipa_size: u64,
    flags: u32,
    __pad: [u32; 3],
}

#[repr(C)]
struct KvmCapArmRmeInitIpaArgs {
    init_ipa_base: u64,
    init_ipa_size: u64,
    __pad: [u32; 4],
}

#[repr(C)]
struct KvmCapArmRmeInitIPAargs {
    init_ipa_base: u64,
    init_ipa_size: u64,
}

#[repr(C)]
struct KvmEnableCap {
    cap: u32,
    args: [u64; 8],
}

#[repr(C)]
struct KvmCreateGuestMemFd {
    size: u64,
    flags: u64,
    __pad: [u64; 6],
}

#[repr(C)]
struct KvmUserspaceMemoryRegion2 {
    slot: u32,
    flags: u32,
    guest_phys_addr: u64,
    memory_size: u64,
    userspace_addr: u64,
    guest_memfd_offset: u64,
    guest_memfd: u32,
    __pad1: u32,
    __pad2: [u64; 14],
}


//
// Test EXIT_HYPERCALL
//
const KVM_HAS_DEVICE_ATTR: u64 = vmm_sys_util::ioctl::ioctl_expr(
    vmm_sys_util::ioctl::_IOC_WRITE,
    KVMIO,
    0xE3,
    std::mem::size_of::<kvm_bindings::kvm_device_attr>() as u32,
) as u64;

const KVM_ARM_VM_SMCCC_CTRL: u32 = 0;
const KVM_ARM_VM_SMCCC_FILTER: u64 = 0;
#[repr(C)]
struct KvmDeviceAttr {
    flags: u32,
    group: u32,
    attr: u64,
    addr: u64
}

const PSCI_0_2_FN_PSCI_VERSION: u32 = 0x8400_0000;
const KVM_SMCCC_FILTER_FWD_TO_USER: u8 = 2;
#[repr(C)]
struct KvmSmcccFilter {
    base: u32,
    nr_functions: u32,
    action: u8,
    __pad: [u8; 15]
}

/* end of kvm api for arm cca */

use kvm_bindings::{
    kvm_device_type_KVM_DEV_TYPE_ARM_VGIC_V3, KVM_DEV_ARM_VGIC_CTRL_INIT,
    KVM_DEV_ARM_VGIC_GRP_ADDR, KVM_DEV_ARM_VGIC_GRP_CTRL, KVM_VGIC_V3_ADDR_TYPE_DIST,
    KVM_VGIC_V3_ADDR_TYPE_REDIST, RegList
};
use libc::{self};
use libc::{c_char, c_int, c_ulong, c_void, mode_t, off_t, size_t};
use std::io::{self, Error, ErrorKind};
use std::result::Result;
use std;
use std::ptr::NonNull;
use std::str;

use kvm_bindings;
use kvm_bindings::kvm_userspace_memory_region;

#[bitfield(u16)]
#[derive(PartialEq, Eq)]
struct SystemRegister {
    #[bits(3)]
    op2: u16,
    #[bits(4)]
    crm: u16,
    #[bits(4)]
    crn: u16,
    #[bits(3)]
    op1: u16,
    #[bits(2)]
    op0: u16,
}

const HVC_OPEN: u64 = 0x8001;
const HVC_READ: u64 = 0x8002;
const HVC_WRITE: u64 = 0x8003;
const HVC_LSEEK: u64 = 0x8004;
const HVC_CLOSE: u64 = 0x8005;
const HVC_EXIT: u64 = 0x8006;

const UART_SYS_BASE_ADDRESS: u64 = 0x9001000;
const UART_SYS_DR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x00;
const UART_SYS_ECR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x04;
const UART_SYS_RSR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x04;
const UART_SYS_FR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x18;
const UART_SYS_BRD_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x24;
const UART_SYS_FBRD_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x28;
const UART_SYS_LCRH_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x2c;
const UART_SYS_CR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x30;
const UART_SYS_IFLS_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x34;
const UART_SYS_IMSC_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x38;
const UART_SYS_RIS_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x3c;
const UART_SYS_MIS_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x40;
const UART_SYS_ICR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x44;
const UART_SYS_DMACR_ADDRESS: u64 = UART_SYS_BASE_ADDRESS | 0x48;

const UART0_BASE_ADDRESS: u64 = 0x9000000;
const UART0_DR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x00;
const UART0_ECR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x04;
const UART0_RSR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x04;
const UART0_FR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x18;
const UART0_BRD_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x24;
const UART0_FBRD_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x28;
const UART0_LCRH_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x2c;
const UART0_CR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x30;
const UART0_IFLS_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x34;
const UART0_IMSC_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x38;
const UART0_RIS_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x3c;
const UART0_MIS_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x40;
const UART0_ICR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x44;
const UART0_DMACR_ADDRESS: u64 = UART0_BASE_ADDRESS | 0x48;

struct UartDevice {
    data_reg: u16,
    flag_reg: u16,
    status_reg: u16,
    ibrd_reg: u8,
    fbrd_reg: u8,
    lcrh_reg: u16,
    cr_reg: u16,
    recv_buffer: Vec<u8>,
}

impl UartDevice {
    fn handle_write_request(&mut self, addr: u64, data: &[u8]) -> u64 {
        match addr {
            UART0_DR_ADDRESS => {
                self.data_reg = data[0] as u16;
                print!("{}", data[0] as char);
                io::stdout().flush().unwrap();
            }
    
            UART_SYS_DR_ADDRESS => {
                self.data_reg = data[0] as u16;
                self.recv_buffer.push(data[0] as u8);
                println!("{:?}", data);
                return self.recv_buffer.len() as u64;
            }
            _ => {
                println!("write to unknown uart address {:#x}", addr);
            }
        }
        return 0;
    }
    
    fn handle_read_request(&mut self, addr: u64, data: &mut [u8]) -> bool {
        match addr {
            UART0_FR_ADDRESS | UART_SYS_FR_ADDRESS => {
                data[0] = 0x00;
                //println!("read register {:#x}", addr);
                return true;
            }
            _ => {
                println!("unknown read register {:#x}", addr);
                return false;
            }
        }
    }

   fn clear(&mut self) {
        self.recv_buffer.clear();
    }
}

struct MemConfig {
    slot_ram_size: u64,
    guest_addr: u64,
    slot: u32,
    load_addr: u64,
    flags: u32,
}

struct VMConfig<'a> {
    size: u64,
    path: &'a str,
    buffer: Vec<u8>,
    ram: &'a mut MemConfig,
    boot: &'a mut MemConfig,
}

impl VMConfig<'_> {
    fn kernel_size(&mut self) -> u64 {
        let metadata =
            &std::fs::metadata(self.path).expect("error while loading stating kernel file");
        self.size = metadata.len();
        self.size
    }

    fn read_kernel(&mut self) -> io::Result<()> {
        let mut k_file = File::open(self.path)?;
        k_file.read_to_end(&mut self.buffer)?;
        Ok(())
    }
}

const SHARED_MEM_PATH: *const c_char = b"vm_shared_memory\0".as_ptr() as *const c_char;

struct VmMemoryRegion {
    guest_phys_addr: u64,
    memory_size: u64,
}

struct Machine {
    uart_devices: Vec<UartDevice>,
    memory_regions: Vec<VmMemoryRegion>,
    kvm: Kvm,
    vm: VmFd,
    vcpu: VcpuFd,
    is_realm: bool,
    vgic: DeviceFd,
    its: DeviceFd,
    event_fds : Vec<EventFd>,
}

impl Machine {
    const VGIC_DIST_SIZE: u64 = 0x10000;
    const VGIC_CPUI_SIZE: u64 = 0x20000;
    const VGIC_REDIST_SIZE: u64 = 0x20000;
    const VGIC_ITS_SIZE: u64 = 0x20000;
    const VGIC_GIC_END: u64 = 0x200_0000;
    const VGIC_DIST_BASE: u64 = Machine::VGIC_GIC_END - Machine::VGIC_DIST_SIZE * 2;
    const VGIC_REDIST_BASE: u64 = Machine::VGIC_DIST_BASE - Machine::VGIC_REDIST_SIZE; //only 1 cpu
    const VGIC_ITS_BASE: u64 = Machine::VGIC_REDIST_BASE - Machine::VGIC_ITS_SIZE; //only 1 cpu
    
    fn realm_init_ipa_range(vm: &mut VmFd, start: u64, size: u64) {
        unsafe {
            let mut rme_init_ipa_realm = KvmCapArmRmeInitIpaArgs {
                init_ipa_base: start,
                init_ipa_size: size,
                __pad: [0u32; 4],
            };
    
            let mut config_enable_cap: kvm_bindings::kvm_enable_cap = Default::default();
    
            config_enable_cap.cap = KVM_CAP_ARM_RME as u32;
            config_enable_cap.args[0] = KVM_CAP_ARM_RME_INIT_IPA_REALM;
            config_enable_cap.args[1] =
                &mut rme_init_ipa_realm as *mut KvmCapArmRmeInitIpaArgs as *mut u64 as u64;
    
            let ret =
                vmm_sys_util::ioctl::ioctl_with_mut_ref(vm, KVM_ENABLE_CAP, &mut config_enable_cap);
            if ret < 0 {
                eprintln!(
                    "cannot initialize ipa range {:#x} {:#x} {:#x}, {}",
                    start,
                    start + size,
                    size,
                    io::Error::last_os_error(),
                );
            }
        }
    }

    fn realm_populate(vm: &mut VmFd, start: u64, size: u64) {
        let mut rme_populate_realm: kvm_bindings::kvm_enable_cap = Default::default();
        let mut rme_init_ipa_realm = KvmCapArmRmePopulateRealmArgs {
            populate_ipa_base: start,
            populate_ipa_size: size,
            flags: KVM_CAP_ARM_RME_POPULATE_FLAGS_MEASURE,
            __pad: [0u32; 3],
        };

        rme_populate_realm.cap = KVM_CAP_ARM_RME as u32;
        rme_populate_realm.flags = 0;
        rme_populate_realm.args[0] = KVM_CAP_ARM_RME_POPULATE_REALM;
        rme_populate_realm.args[1] =
            &mut rme_init_ipa_realm as *mut KvmCapArmRmePopulateRealmArgs as *mut u64 as u64;

        let ret = unsafe {
            vmm_sys_util::ioctl::ioctl_with_mut_ref(vm, KVM_ENABLE_CAP, &mut rme_populate_realm)
        };
        if ret < 0i32 {
            let os_error = std::io::Error::last_os_error();
            panic!(
                "cannot populate ipa range start:{:#x}, end:{:#x}, size:{:#x} with error:{os_error:?}",
                start,
                start + size,
                size
            );
        }
    }

    fn realm_activate_realm(vm: &mut VmFd) {
        let mut activate_realm: kvm_bindings::kvm_enable_cap = Default::default();
        activate_realm.cap = KVM_CAP_ARM_RME as u32;
        activate_realm.args[0] = KVM_CAP_ARM_RME_ACTIVATE_REALM;

        let ret = unsafe {
            vmm_sys_util::ioctl::ioctl_with_mut_ref(vm, KVM_ENABLE_CAP, &mut activate_realm)
        };
        if ret < 0 {
            panic!("could not activate realm");
        }
    }
    fn initialize_vcpu(vcpu_fd: &mut VcpuFd, vm: &mut VmFd) {
        //initialize cpu
        let mut kvi = kvm_bindings::kvm_vcpu_init::default();
        //set vcpu features

        vm.get_preferred_target(&mut kvi).unwrap();

        if std::arch::is_aarch64_feature_detected!("sve2") {
            println!("detected sve2 ");
        }
        if std::arch::is_aarch64_feature_detected!("sve") {
            println!("detected sve ");
        }

        kvi.features[0] |= (0x01 << KVM_ARM_VCPU_PSCI_0_2);
        kvi.features[0] |= (0x01 << KVM_ARM_VCPU_PTRAUTH_ADDRESS);
        kvi.features[0] |= (0x01 << KVM_ARM_VCPU_PTRAUTH_GENERIC);
        //	kvi.features[0]  |= (0x01 << kvm_bindings::KVM_ARM_VCPU_SVE);

        println!("requesting vcpu with features {:?} ", kvi);
        if vcpu_fd.vcpu_init(&kvi).is_err() {
            eprintln!(
                "vcpu init failed to initialize vcpu {:?}",
                io::Error::last_os_error()
            );
        }

    }

    fn vm_map_guest_memory(base_address: u64, size: u64, realm: bool) -> Option<u64> {
        let load_addr: u64;
        println!("VMM: map host memory for guest VM - base:{:#x}, size:{} MB",
            base_address, size >> 20);
        if realm {
            load_addr = unsafe {
                libc::mmap(base_address as *mut libc::c_void, size.try_into().unwrap(),
                    libc::PROT_READ | libc::PROT_WRITE, libc::MAP_ANONYMOUS | libc::MAP_FIXED
                    | libc::MAP_PRIVATE | libc::MAP_LOCKED,
                    -1, 0) as *mut u8 as u64
            };
            assert_eq!(load_addr, base_address, "VMM: Failed to mmap on requested base");

            let err = unsafe {
                libc::mlock2(load_addr as *const libc::c_void, size.try_into().unwrap(),
                    libc::MLOCK_ONFAULT.try_into().unwrap())
            };
            if err < 0 {
                println!("VMM: failed to map memory for guest - error:{:?}.",
                    std::io::Error::last_os_error());
                return None;
            }
        } else {
            let shared_fd = unsafe {
                libc::shm_open(SHARED_MEM_PATH, libc::O_RDWR | libc::O_CREAT,
                    libc::S_IRUSR | libc::S_IWUSR)   
            };
            let err = unsafe {
                libc::ftruncate(shared_fd, size as libc::off_t)
            };
            if err < 0 {
                println!("VMM: failed to map memory for guest - error:{:?}.",
                    std::io::Error::last_os_error());
                return None;
            }
            load_addr = unsafe {
               libc::mmap(base_address as *mut libc::c_void, size.try_into().unwrap(),
                    libc::PROT_READ | libc::PROT_WRITE, libc::MAP_SHARED | libc::MAP_FIXED
                    | libc::MAP_NORESERVE | libc::MAP_POPULATE, shared_fd, 0) as *mut u8 as u64 
            };
            assert_eq!(load_addr, base_address, "VMM: Failed to mmap on requested base");
            unsafe {
                libc::close(shared_fd);
            }
        }

        Some(load_addr)
    }

    fn kvm_supports_attribute(kvm_fd: &Kvm, kvm_cap: u32) -> Result<bool, std::io::Error> {
        let err = unsafe {
            vmm_sys_util::ioctl::ioctl_with_val(kvm_fd, KVM_CHECK_EXTENSION(), kvm_cap as c_ulong)
        };
        if err <= 0 {
            if err < 0 {
                panic!("KVM: ioctl failed with error - {:?}", std::io::Error::last_os_error());
            }
            return Ok(false);
        }
        Ok(true)
    }

    fn kvm_vm_supports_attribute(vm_fd: &VmFd, kvm_cap: u32, check_bit: Option<u32>)
        -> Result<bool, std::io::Error> {
        let err: i32;
        let mut res: bool = true;
        err = unsafe {
            vmm_sys_util::ioctl::ioctl_with_val(vm_fd, KVM_CHECK_EXTENSION(), kvm_cap as c_ulong)
        };
        if err < 0 {
            panic!("KVM: ioctl failed with error - {:?}", std::io::Error::last_os_error());
        } else if check_bit.is_some() && (err as u64 & kvm_cap as u64 == 0) {
            println!("KVM: vm does not support capability");
            res = false;
        }
        Ok(res)
    }

    fn realm_personalize(
        vm: &mut VmFd,
        hash_algo: u32,
        rpv_token: &[u8; KVM_CAP_ARM_RME_RPV_SIZE as usize],
    ) {
        let mut rme_config: kvm_bindings::kvm_enable_cap = Default::default();

        let mut config = KvmCapArmRmeConfigHash {
            cfg: KVM_CAP_ARM_RME_CFG_HASH_ALGO as u32,
            hash_algo: hash_algo.try_into().unwrap(),
            pad: [0u8; 252],
        };

        rme_config.cap = KVM_CAP_ARM_RME as u32;
        rme_config.args[0] = KVM_CAP_ARM_RME_CONFIG_REALM;
        rme_config.args[1] = &mut config as *mut KvmCapArmRmeConfigHash as *mut u64 as u64;

        let mut ret =
            unsafe { vmm_sys_util::ioctl::ioctl_with_mut_ref(vm, KVM_ENABLE_CAP, &mut rme_config) };

        if ret < 0 {
            panic!(
                "could not set realm personalization value {:#?} ",
                io::Error::last_os_error()
            );
        }
    }

    fn realm_finalize_cpu(vcpu: &VcpuFd) {
        let feature = KVM_ARM_VCPU_REC;
        let ret =
            unsafe { vmm_sys_util::ioctl::ioctl_with_ref(vcpu, KVM_ARM_VCPU_FINALIZE, &feature) };
        if ret < 0 {
            eprintln!("vcpu failed to REC {:?}", io::Error::last_os_error());
        }
    }

    fn create_realm_descriptor(vm: &VmFd) {
        let mut enable_descriptor: kvm_bindings::kvm_enable_cap = Default::default();
        enable_descriptor.cap = KVM_CAP_ARM_RME;
        enable_descriptor.args[0] = KVM_CAP_ARM_RME_CREATE_RD;

        let ret = unsafe {
            vmm_sys_util::ioctl::ioctl_with_mut_ref(vm, KVM_ENABLE_CAP, &mut enable_descriptor)
        };
        if ret < 0 {
            panic!(
                "failed to create realm RD: {}  {:#?}",
                ret,
                io::Error::last_os_error()
            );
        }
    }

    fn new(config: &mut VMConfig, entry: u64, debug: bool, is_realm: bool) -> io::Result<Self> {
        //personalize realm
        let rpv_token: [u8; KVM_CAP_ARM_RME_RPV_SIZE as usize] = [
            0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xAD, 0xCE, 0xED, 0xDA, 0xBA, 0xEE, 0xEA, 0xDE, 0xAD,
            0xBE, 0xEF, 0xBE, 0xAD, 0xCE, 0xED, 0xDA, 0xBA, 0xEE, 0xEA, 0xDE, 0xAD, 0xBE, 0xEF,
            0xBE, 0xAD, 0xCE, 0xED, 0xDA, 0xBA, 0xEE, 0xEA, 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xAD,
            0xCE, 0xED, 0xDA, 0xBA, 0xEE, 0xEA, 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xAD, 0xCE, 0xED,
            0xDA, 0xBA, 0xEE, 0xEA, 0, 0, 0, 0,
        ];

        let mut devices: Vec<UartDevice> = Vec::new();
        let uart_device = UartDevice {
            data_reg: 0,
            flag_reg: 0,
            status_reg: 0,
            ibrd_reg: 0,
            fbrd_reg: 0,
            lcrh_reg: 0,
            cr_reg: 0,
            recv_buffer: Vec::<u8>::new(),
        };

        let syscall_device = UartDevice {
            data_reg: 0,
            flag_reg: 0,
            status_reg: 0,
            ibrd_reg: 0,
            fbrd_reg: 0,
            lcrh_reg: 0,
            cr_reg: 0,
            recv_buffer: Vec::<u8>::new(),
        };

        devices.push(uart_device);
        devices.push(syscall_device);

        let mut kvm = Kvm::new().unwrap();
        let mut ipa_size: u64 = 33; // RIPAS_bit = ipa_size - 1
        let mut load_addr;
        let mut vm;



        if is_realm {
            if !kvm.check_extension(kvm_ioctls::Cap::ArmVmIPASize) {
                panic!("KVM: Does not support setting IPA size:{:#?}",
                    io::Error::last_os_error());
            }
            let max_ipa = kvm.get_host_ipa_limit() as u64;
            println!("KVM: Current IPA limit is {:?}", max_ipa);
            if max_ipa < ipa_size {
                panic!("VMM: Current IPA:{} bigger than supported limit:{}", ipa_size, max_ipa);
            }
            let vm_type = KVM_VM_TYPE_ARM_REALM | KVM_VM_TYPE_ARM_IPA_SIZE!(ipa_size);
            vm = kvm.create_vm_with_type(vm_type).unwrap();
            println!("KVM: vm_fd created with type: Realm - IPA: {} Bits.", ipa_size);

            if (Self::kvm_supports_attribute(&kvm, KVM_CAP_MEMORY_ATTRIBUTES).unwrap()
                || Self::kvm_vm_supports_attribute(&vm, KVM_CAP_MEMORY_ATTRIBUTES,
                Some(KVM_MEMORY_ATTRIBUTE_PRIVATE as u32)).unwrap()) == false {
                panic!("VMM: Can not create vm - missing cap:KVM_CAP_MEMORY_ATTRIBUTES");
            }
        } else {
            vm = kvm.create_vm().unwrap();
        }

        unsafe {
            let mut err; 
            println!("VMM - Set filter for EXIT_HYPERCALL - RSI_HOST_CALL ");
            let psci_filter: KvmSmcccFilter = KvmSmcccFilter {
                base: PSCI_0_2_FN_PSCI_VERSION,
                nr_functions: 1u32,
                action: KVM_SMCCC_FILTER_FWD_TO_USER,
                __pad: [0u8;15]
            };

            let device_filter: KvmDeviceAttr = KvmDeviceAttr {
                group: KVM_ARM_VM_SMCCC_CTRL,
                attr: KVM_ARM_VM_SMCCC_FILTER,
                flags: 0,
                addr: &psci_filter as *const _ as u64,
            };

            let rhc_filter: KvmSmcccFilter = KvmSmcccFilter {
                base: 0xC400_0199,
                nr_functions: 1u32,
                action: KVM_SMCCC_FILTER_FWD_TO_USER,
                __pad: [0u8;15]
            };
            let device_rhc_filter: KvmDeviceAttr = KvmDeviceAttr {
                group: KVM_ARM_VM_SMCCC_CTRL,
                attr: KVM_ARM_VM_SMCCC_FILTER,
                flags: 0,
                addr: &rhc_filter as *const _ as u64,
            };

            let device_attr: kvm_bindings::kvm_device_attr = kvm_bindings::kvm_device_attr{
                flags: 0,
                group: KVM_ARM_VM_SMCCC_CTRL,
                attr: KVM_ARM_VM_SMCCC_FILTER,
                addr: 0,
            };
            err = vmm_sys_util::ioctl::ioctl_with_ref(&vm, KVM_HAS_DEVICE_ATTR, &device_attr);
            if err < 0 {
                panic!("VM: Failed to Get DeviceAttribute - SmcccFilter : err:{:?}", std::io::Error::last_os_error());
            }
            if err == 0 {
                println!("VM: Does have DeviceAttribute - SmcccFilter : res:{:?}", err);
            }
            err = vmm_sys_util::ioctl::ioctl_with_ref(&vm, KVM_SET_DEVICE_ATTR, &device_filter);
            if err < 0 {
                panic!("VM: Failed to set DeviceAttribute - SmcccFilter-PSCI : err:{:?}", std::io::Error::last_os_error());
            } else {
                println!("VM: Set DeviceAttribute - SmmcccFilter-PSCI : ret:{:?}.", err);
            }
           err = vmm_sys_util::ioctl::ioctl_with_ref(&vm, KVM_SET_DEVICE_ATTR, &device_rhc_filter);
           if err < 0 {
               panic!("VM: Failed to set DeviceAttribute - SmcccFilter-RHC : err:{:?}", std::io::Error::last_os_error());
           } else {
               println!("VM: Set DeviceAttribute - SmmcccFilter-RHC : ret:{:?}.", err);
           }
        }

        //configure the vm at least once
        Machine::realm_personalize(&mut vm, KVM_CAP_ARM_RME_MEASUREMENT_ALGO_SHA256, &rpv_token);
        //create realm descriptor
        if is_realm {
            Machine::create_realm_descriptor(&vm);
        }
        //create_gic
        let mut vgic = Machine::create_irqchip(&mut vm).unwrap();
        let mut its = Machine::create_its_device(&mut vm).unwrap();

        //Reserve backend memory for the VM
        load_addr = Self::vm_map_guest_memory(config.ram.guest_addr, 
            config.ram.slot_ram_size, is_realm).expect("VMM: guest ram base address.");

        //add memory region to kvm
        eprintln!(
            "value of load address is {:#x} {}, {:#x}",
            load_addr as *mut u64 as u64,
            io::Error::last_os_error(),
            config.ram.guest_addr as u64
        );

        let mut config_enable_cap: kvm_bindings::kvm_enable_cap = Default::default();

        let mut guest_memfd_reg0 = KvmCreateGuestMemFd {
            size: 0x1200000,
            flags: 0,
            __pad: [0u64; 6]
        };

        let fd_reg0 = unsafe {
            vmm_sys_util::ioctl::ioctl_with_mut_ref(&mut vm, KVM_CREATE_GUEST_MEMFD, &mut guest_memfd_reg0)
        };
        if fd_reg0 < 0 {
            panic!("VM: Failed to create guest_memfd_reg0 : err:{:?}", std::io::Error::last_os_error());
        }

        let mut guest_memfd_reg2 = KvmCreateGuestMemFd {
            size: config.ram.slot_ram_size - 0x1400000,
            flags: 0,
            __pad: [0u64; 6]

        };
        let fd_reg2 = unsafe {
            vmm_sys_util::ioctl::ioctl_with_mut_ref(&mut vm, KVM_CREATE_GUEST_MEMFD, &mut guest_memfd_reg2)
        };
        if fd_reg2 < 0 {
            panic!("VM: Failed to create guest_memfd_reg2 : err:{:?}", std::io::Error::last_os_error());
        }

        let mut memory_regions: Vec<VmMemoryRegion> = Vec::new();

        let mut memory_region_0 = KvmUserspaceMemoryRegion2 {
            slot: 0,
            flags: KVM_MEM_GUEST_MEMFD,
            guest_phys_addr: config.ram.guest_addr,
            memory_size: 0x1200000 as u64,
            userspace_addr: load_addr as u64,
            guest_memfd_offset: 0,
            guest_memfd: fd_reg0 as u32,
            __pad1: 0u32,
            __pad2: [0u64; 14],
        };

        let mut memory_region_2 = KvmUserspaceMemoryRegion2 {
            slot: 2,
            flags: KVM_MEM_GUEST_MEMFD,
            guest_phys_addr: config.ram.guest_addr + 0x1400000,
            memory_size: 0x200000 as u64,
            userspace_addr: (load_addr as u64) + 0x1400000,
            guest_memfd_offset: 0,
            guest_memfd: fd_reg2 as u32,
            __pad1: 0u32,
            __pad2: [0u64; 14],
        };

        let memory_region_1 = kvm_userspace_memory_region {
            slot: config.ram.slot + 1,
            guest_phys_addr: config.ram.guest_addr + 0x1200000,
            memory_size: 0x200000 as u64,
            userspace_addr: (load_addr as u64) + 0x1200000,
            flags: 0,
        };

        unsafe {
            let err =
                vmm_sys_util::ioctl::ioctl_with_mut_ref(&mut vm, KVM_SET_USER_MEMORY_REGION2, &mut memory_region_0);
            if err < 0 {
                panic!("VM: Failed to set memory_region_0 - err:{:?}", std::io::Error::last_os_error());
            }
            vm.set_user_memory_region(memory_region_1).unwrap();
            let err =
                vmm_sys_util::ioctl::ioctl_with_mut_ref(&mut vm, KVM_SET_USER_MEMORY_REGION2, &mut memory_region_2);
            if err < 0 {
                panic!("VM: Failed to set memory_region_2 - err:{:?}", std::io::Error::last_os_error());
            }
        }
        //load kernel
        unsafe {
            let kernel_size = config.kernel_size();
	        if kernel_size > config.ram.slot_ram_size {
		        panic!("kernel size is larger");
	        }
            //let mut slice = slice::from_raw_parts_mut((load_addr as u64 + 0x200_0000) as *mut u8, kernel_size as usize);
            let mut slice =
                slice::from_raw_parts_mut((load_addr as u64) as *mut u8, kernel_size as usize);
            slice.write(&config.buffer).unwrap();
        }
	if is_realm {
            eprintln!("Init Realm-IPA range: region0");
            Machine::realm_init_ipa_range(&mut vm, memory_region_0.guest_phys_addr, memory_region_0.memory_size);
            eprintln!("Init Realm-IPA range: region2");
            Machine::realm_init_ipa_range(&mut vm, memory_region_2.guest_phys_addr, memory_region_2.memory_size);
            eprintln!("done with Realm-IPA range initialization.");

            eprintln!("Populate Realm-Ram: region0");
            Machine::realm_populate(&mut vm, memory_region_0.guest_phys_addr, memory_region_0.memory_size);
            eprintln!("Populate Realm-Ram: region2");
            Machine::realm_populate(&mut vm, memory_region_2.guest_phys_addr, memory_region_2.memory_size);
            eprintln!("done with Realm-Ram populate.");
	}

        memory_regions.push(
            VmMemoryRegion {
                guest_phys_addr: memory_region_0.guest_phys_addr,
                memory_size: memory_region_0.memory_size
            });
        memory_regions.push(
            VmMemoryRegion {
                guest_phys_addr: memory_region_1.guest_phys_addr,
                memory_size: memory_region_1.memory_size,
            });
        memory_regions.push(
            VmMemoryRegion {
                guest_phys_addr: memory_region_2.guest_phys_addr,
                memory_size: memory_region_2.memory_size});

        //create cpu
        let mut vcpu_fd = vm.create_vcpu(0).unwrap();
        eprintln!("vcpu_fd create {:?}", io::Error::last_os_error());

        Machine::initialize_vcpu(
            &mut vcpu_fd,
            &mut vm
        );

	Machine::finish_gic_init(&vgic);

        let core_reg_base: u64 = 0x6030_0000_0010_0000;
        //let mut _mmio_addr: u64 = (config.ram.guest_addr + 0x64);
        let mut _mmio_addr: u64 = (config.ram.guest_addr + 0x1240);
	eprintln!("the address of pc is {:#x?}",_mmio_addr);
        //set pc
        vcpu_fd.set_one_reg(core_reg_base + 2 * 32, _mmio_addr as u128 as u128).unwrap();
	    _mmio_addr = 0;
        vcpu_fd.set_one_reg(core_reg_base + 2 * 0, _mmio_addr as u128 as u128).unwrap();//x0
        vcpu_fd.set_one_reg(core_reg_base + 2 * 1, _mmio_addr as u128 as u128).unwrap();//x1
        vcpu_fd.set_one_reg(core_reg_base + 2 * 2, _mmio_addr as u128 as u128).unwrap();//x2
        vcpu_fd.set_one_reg(core_reg_base + 2 * 3, _mmio_addr as u128 as u128).unwrap();//x3

        if is_realm {
            //activate the cpu
            Machine::realm_finalize_cpu(&vcpu_fd);
            eprintln!("done finalize cpu");

            //finalize the vm
            Machine::realm_activate_realm(&mut vm);
            eprintln!("done activate realm");
        }

	let mut event_fds : Vec<EventFd> = Vec::new();
	let vtimer_irqfd = EventFd::new(EFD_NONBLOCK).unwrap();
	let vtimer_samplefd = EventFd::new(EFD_NONBLOCK).unwrap();

	let pvtimer_irqfd = EventFd::new(EFD_NONBLOCK).unwrap();
	let pvtimer_samplefd = EventFd::new(EFD_NONBLOCK).unwrap();
	
	event_fds.push(vtimer_irqfd);	
	event_fds.push(vtimer_samplefd);	
	event_fds.push(pvtimer_irqfd);	
	event_fds.push(pvtimer_samplefd);
        Ok(Machine {
            //vmconfig: config,
            uart_devices: devices,
            memory_regions: memory_regions,
            kvm: kvm,
            vm: vm,
            vcpu: vcpu_fd,
            is_realm: is_realm,
	    vgic,
	    its,
	    event_fds,
        })
    }

    fn runvm(&mut self) {
        self.runloop();
    }

    fn runloop(&mut self) -> i32 {
        loop {
            match self.vcpu.run().unwrap_or_else(|x| { eprintln!("run failed:{} {:?}", x, io::Error::last_os_error()); VcpuExit::Hlt
            }) {
                VcpuExit::MmioRead(physical_address, data) => {
                    if physical_address == UART0_FR_ADDRESS {
                        self.uart_devices[0].handle_read_request(physical_address, data);
                        continue;
                    }
                    if physical_address == UART_SYS_FR_ADDRESS {
                        self.uart_devices[1].handle_read_request(physical_address, data);
                        continue;
                    }

                    println!(
                        "Received an MMIO Read Request for the address {:#x} data {:#x}.",
                        physical_address, data[0]
                    );
                }

                VcpuExit::MmioWrite(physical_address, data) => {
                    if physical_address == UART0_BASE_ADDRESS {
                        self.uart_devices[0].handle_write_request(physical_address, data);
                        continue;
                    }
                    if physical_address == UART_SYS_BASE_ADDRESS {
                        if self.uart_devices[1].handle_write_request(physical_address, data) < 8 {
                            continue;
                        }
                        let syscall_buffer: u64 =
                            LittleEndian::read_u64(&self.uart_devices[1].recv_buffer);
                        println!("syscall buffer address is {:#x?}", syscall_buffer);
                        handle_hypercall(syscall_buffer);
                        self.uart_devices[1].clear();
                        continue;
                    }
                    println!(
                        "Received an MMIO Write Request to the address {:#x} data {:#x}.",
                        physical_address, data[0]
                    );
                }
                VcpuExit::Hlt => {
                    break;
                }
                VcpuExit::Debug(kvm_debug_info) => {
                    println!("kvm_debug_exit_arch {:#?}", kvm_debug_info);
                    break;
                }
		        VcpuExit::SystemEvent(type_ , flags) => {
                    println!("received system event {:#?} {:#?} ", type_, flags);
                    return 0;
		        },
                VcpuExit::Hypercall => {
                    println!("Exit from Hypercall");
                    let arm_reg_base: u64 = 0x6030_0000_0010_0000;
                    let x0 = arm_reg_base;
                    let fid = self.vcpu.get_one_reg(x0).expect("Did not get X0");
                    println!(" - Exit number is: {:#10x}", fid);
                    let mut reg;
                    let mut val: u64;
                    for i in 0..8 {
                        reg = arm_reg_base + (0x2 * i);
                        val = self.vcpu.get_one_reg(reg).expect("Did not get Reg") as u64;
                        println!(" - vCPU-reg{} - {:#018x}", i, val);

                    }
                    let pc_reg_addr = arm_reg_base + 0x40;
                    let pc_val = self.vcpu.get_one_reg(pc_reg_addr).expect("Did not get PC");
                    println!(" - Guest PC:{:#018x}", pc_val);
                },
                r => panic!("Unexpected exit reason: {:?}", r),
            }
        }
        return 0;
    }

    fn create_its_device(vm: &VmFd) -> Result<DeviceFd,()>{
        let mut its_dev_address = Machine::VGIC_ITS_BASE as u64;
        let mut its_device = kvm_bindings::kvm_create_device {
            type_: kvm_bindings::kvm_device_type_KVM_DEV_TYPE_ARM_VGIC_ITS,
            fd: 0,
            flags: 0,
        };

        let its_fd = vm
            .create_device(&mut its_device).unwrap();
       //     .expect("device could not create ITS device");

        let its_attr = kvm_bindings::kvm_device_attr {
            group: KVM_DEV_ARM_VGIC_GRP_ADDR,
            attr: kvm_bindings::KVM_VGIC_ITS_ADDR_TYPE.try_into().unwrap(),
            addr: &its_dev_address as *const u64 as u64,
            flags: 0,
        };

        let _ = its_fd.set_device_attr(&its_attr).unwrap();
	//map_err(|x| { eprintln!( "could not set ITS attribute {x:?} {}",
	// std::io::Error::last_os_error()); });

        let its_init = kvm_bindings::kvm_device_attr {
            group: KVM_DEV_ARM_VGIC_GRP_CTRL,
            attr: KVM_DEV_ARM_VGIC_CTRL_INIT.try_into().unwrap(),
            addr: 0,
            flags: 0,
        };

        its_fd.set_device_attr(&its_init).unwrap();// .expect("could not do its initialization");
	    Ok(its_fd)
    }

    fn get_device_attribute(vcpufd: &VcpuFd) -> (u64,u64) {
        let mut timer_value: u64 = 0;
        let mut timer_attr = kvm_bindings::kvm_device_attr {
            group: kvm_bindings::KVM_ARM_VCPU_TIMER_CTRL,
            attr: kvm_bindings::KVM_ARM_VCPU_TIMER_IRQ_VTIMER
                .try_into()
                .unwrap(),
            addr: &mut timer_value as *mut u64 as u64,
            flags: 0,
        };
        unsafe {
            let ret = vmm_sys_util::ioctl::ioctl_with_mut_ref(
                vcpufd,
                KVM_GET_DEVICE_ATTR,
                &mut timer_attr,
            );
            if ret < 0 {
                eprintln!("get timer level",);
            }

            eprintln!("TIMER LEVEL VTIMER irq number is {:?}", timer_value);
        }

	let old_value = timer_value;

        timer_attr = kvm_bindings::kvm_device_attr {
            group: kvm_bindings::KVM_ARM_VCPU_TIMER_CTRL,
            attr: kvm_bindings::KVM_ARM_VCPU_TIMER_IRQ_PTIMER
                .try_into()
                .unwrap(),
            addr: &mut timer_value as *mut u64 as u64,
            flags: 0,
        };
        unsafe {
            let ret = vmm_sys_util::ioctl::ioctl_with_mut_ref(
                vcpufd,
                KVM_GET_DEVICE_ATTR,
                &mut timer_attr,
            );
            if ret < 0 {
                eprintln!("get timer level",);
            }

            eprintln!("TIMER LEVEL PV irq number is {:#x?}", timer_value);
        }
	    (old_value, timer_value)
    }

    fn create_irqchip(vm: &mut VmFd) -> Result<DeviceFd,()>{
        //kvm_create_irqchip() creates a vgicv2 controller
        //other versions have to do it this way.
        let mut vgic_dev_address;
        let mut gic_device = kvm_bindings::kvm_create_device {
            type_: kvm_device_type_KVM_DEV_TYPE_ARM_VGIC_V3,
            fd: 0,
            flags: 0,
        };
        let vgic_fd = vm
            .create_device(&mut gic_device).unwrap();
            //.expect("device could not create VGIC device vGIC3");


        vgic_dev_address = Machine::VGIC_REDIST_BASE;
        let redist = kvm_bindings::kvm_device_attr {
            group: KVM_DEV_ARM_VGIC_GRP_ADDR,
            attr: KVM_VGIC_V3_ADDR_TYPE_REDIST.try_into().unwrap(),
            addr: &vgic_dev_address as *const u64 as u64,
            flags: 0,
        };

        vgic_fd
            .set_device_attr(&redist).unwrap();
            //.expect("could not create gicv3 distributor");

        vgic_dev_address = Machine::VGIC_DIST_BASE;
        let dist = kvm_bindings::kvm_device_attr {
            group: KVM_DEV_ARM_VGIC_GRP_ADDR,
            attr: KVM_VGIC_V3_ADDR_TYPE_DIST.try_into().unwrap(),
            addr: &vgic_dev_address as *const u64 as u64,
            flags: 0,
        };

        let _ = vgic_fd.set_device_attr(&dist).unwrap();//.map_err(|x| {
//            eprintln!( "could not create gicv3 redistributor {x:?} value: {}",
//                std::io::Error::last_os_error()); });

	Ok(vgic_fd)
}
	fn finish_gic_init(vgic_fd : &DeviceFd)  {
        let irq_lines = 64;
        let vgic_init = kvm_bindings::kvm_device_attr {
            group: kvm_bindings::KVM_DEV_ARM_VGIC_GRP_NR_IRQS,
            attr: 0,
            addr: &irq_lines as *const i32 as u64,
            flags: 0,
        };

        vgic_fd
            .set_device_attr(&vgic_init)
            .expect("could not irq lines");

       let vgic_init = kvm_bindings::kvm_device_attr {
           group: KVM_DEV_ARM_VGIC_GRP_CTRL,
           attr: KVM_DEV_ARM_VGIC_CTRL_INIT.try_into().unwrap(),
           addr: 0,
           flags: 0,
       };

       vgic_fd
           .set_device_attr(&vgic_init)
           .expect("could not create gicv3 distributor");


    }
}

use clap::Parser;
#[derive(Parser)]
#[command(author, version, about, long_about = None)]
struct Cli {
    #[arg(short, long, value_name = "address", action = clap::ArgAction::Set, default_value_t = 0x4200_0000)]
    address: u64,
    #[arg(short, long, action = clap::ArgAction::SetTrue, default_value_t = false)]
    debug: bool,
    #[arg(short, long, action = clap::ArgAction::SetTrue, default_value_t = false)]
    realm: bool,
}

fn main() {
    let args = Cli::parse();
    let mut is_realm = args.realm;
    let mut is_debug = args.debug;
    let mut guest_addr = args.address;

    if is_realm && is_debug {
        eprintln!("a realm enclave cannot be debugged at the moment");
        return;
    }

    let mut vmconfig = VMConfig {
        ram: &mut MemConfig {
            slot_ram_size: 0x200_0000,
            guest_addr: guest_addr,
            slot: 0,
            load_addr: 0,
            flags: 0,
        },

        boot: &mut MemConfig {
            slot_ram_size: 0x10_000,
            guest_addr: 0xff_000,
            slot: 1,
            load_addr: 0,
            flags: 0,
        },

        size: 0,
        //TODO: FIXME -pass as argument or default
        path: "./src/kernel/kernel_start.bin",
        buffer: Vec::new(),
    };

    vmconfig.kernel_size();
    let _ = vmconfig.read_kernel();

    let mut machine = Machine::new(&mut vmconfig, 0x11e0, is_debug, is_realm).unwrap();

	if ! machine.is_realm {
    		return machine.runvm();
	}

	let thread_join_handle = thread::spawn(move || 
	{
        	let mut cpu_set = CpuSet::new();
        	cpu_set.set(sched_getcpu().unwrap() ).unwrap();
        	sched_setaffinity(Pid::from_raw(0), &cpu_set).unwrap();
		eprintln!("running the vcpu");
		machine.runvm()
	}
	);

	let _ = thread_join_handle.join();
}

fn handle_hypercall(address: u64) -> u64 {
    let hvc_args;
    unsafe {
        hvc_args = slice::from_raw_parts_mut(address as *mut u64, 8);
        println!("hvc  {:#x?}", hvc_args[0]);
        println!("arg0 {:#x?}", hvc_args[1]);
        println!("arg1 {:#x?}", hvc_args[2]);
        println!("arg2 {:#x?}", hvc_args[3]);
        println!("arg3 {:#x?}", hvc_args[4]);
        println!("arg4 {:#x?}", hvc_args[5]);
    }
    match hvc_args[0] {
        HVC_OPEN => {
            hvc_args[0] = hypercall_open(
                hvc_args[1] as *const c_char,
                hvc_args[2] as c_int,
                hvc_args[3] as mode_t,
            ) as u64;
        }
        HVC_READ => {
            hvc_args[0] =
                hypercall_read(hvc_args[1] as c_int, hvc_args[2] as *mut u8, hvc_args[3]) as u64;
        }
        HVC_WRITE => {
            hvc_args[0] = hypercall_write(
                hvc_args[1] as c_int,
                hvc_args[2] as *mut u8,
                hvc_args[3] as u64,
            )
            .unwrap() as u64;
        }
        HVC_LSEEK => unsafe {
            hvc_args[0] = libc::lseek(
                hvc_args[1] as c_int,
                hvc_args[2] as off_t,
                hvc_args[3] as c_int,
            ) as u64;
        },
        HVC_CLOSE => {
            hvc_args[0] = hypercall_close(hvc_args[2] as c_int) as u64;
        }
        HVC_EXIT => {
            println!("Exiting from the VMM\n");
            unsafe {
                hvc_args[0] = libc::exit(hvc_args[2] as c_int) as u64;
            }
            return hvc_args[1];
        }
        _r => {
            println!("unknown hypercall {:#x?} {:#x?}", _r, hvc_args[1]);
            hvc_args[0] = u64::MAX;
            return 1;
        }
    }
    return 0;
}

fn hypercall_open(filename: *const c_char, flags: c_int, mode: mode_t) -> i32 {
    let result = unsafe { libc::open(filename as *const u8, flags as i32, mode as c_ulong) };

    result as i32
}

fn hypercall_close(fd: i32) -> i32 {
    let result = unsafe { libc::close(fd as i32) };
    result as i32
}

fn hypercall_write(fd: c_int, buf: *mut u8, mut len: u64) -> io::Result<u64> {
    let mut buf_ptr: NonNull<u8> =
        NonNull::new(buf).ok_or(Error::new(ErrorKind::Other, "failed to get buffer pointer"))?;
    let count = len;
    println!("handling hypercall write\n");
    io::stdout().flush().unwrap();
    while len > 0 {
        let ret = unsafe {
            libc::write(
                fd,
                buf_ptr.as_ptr() as *const libc::c_void,
                len.min(isize::max_value() as u64) as size_t,
            )
        };

        if ret < 0 {
            return Err(Error::last_os_error());
        } else {
            len -= ret as u64;
            buf_ptr = unsafe {
                NonNull::new(buf_ptr.as_ptr().add(ret as usize)).ok_or(Error::new(
                    ErrorKind::Other,
                    "failed to advance buffer pointer",
                ))?
            }
        }
    }

    Ok(count)
}

fn hypercall_read(fd: c_int, _buf: *mut u8, len: u64) -> i64 {
    let mut total = 0;
    let mut remaining = len;
    let buf;
    unsafe {
        buf = slice::from_raw_parts_mut(_buf, len.try_into().unwrap());
    }

    println!("handling hypercall read\n");
    io::stdout().flush().unwrap();
    while remaining > 0 {
        let result = unsafe {
            libc::read(
                fd,
                buf[total..].as_mut_ptr() as *mut c_void,
                remaining.try_into().unwrap(),
            )
        };

        if result == -1 {
            if remaining < len {
                return (len - remaining) as i64;
            }
            return result as i64;
        }

        let amount_read = result as u64;
        total += amount_read as usize;
        remaining -= amount_read;
    }

    total as i64
}
