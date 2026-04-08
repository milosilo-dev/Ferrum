use kvm_bindings::{
    kvm_regs, 
    kvm_userspace_memory_region
};

use kvm_ioctls::{
    Kvm, 
    VcpuExit, 
    VcpuFd, 
    VmFd
};

use skhv::{device_maps::{
    io::{
        IODeviceMap, 
        IODeviceRegion
    }, 
    mmio::{
        MMIODeviceMap, 
        MMIODeviceRegion
    }
}, devices::serial::Serial};
use std::{fs, ptr};
use libc::{MAP_ANONYMOUS, MAP_PRIVATE, PROT_READ, PROT_WRITE, mmap};

const MEM_SIZE: usize = 0x1000;

pub enum CrashReason {
    Hlt,
    FailedEntry,
    UnhandledExit,
    NoIODataReturned,
    IncorrectIOInputLength,
    NoMMIODataReturned,
    IncorrectMMIOReadLength,
}

pub struct VCPU {
    pub fd: VcpuFd
}

impl VCPU{
    pub fn new(vm: VmFd) -> Self{
        let vcpu = vm.create_vcpu(0).unwrap();

        let mut sregs = vcpu.get_sregs().unwrap();
        sregs.cs.base = 0;
        sregs.cs.selector = 0;
        sregs.ds.base = 0;
        sregs.es.base = 0;
        sregs.fs.base = 0;
        sregs.gs.base = 0;
        sregs.ss.base = 0;
        vcpu.set_sregs(&sregs).unwrap();

        let mut regs = kvm_regs::default();
        regs.rip = 0x1000;
        regs.rax = 2;
        regs.rbx = 2;
        regs.rflags = 0x2;
        vcpu.set_regs(&regs).unwrap();

        Self {
            fd: vcpu 
        }
    }

    pub fn run(&mut self) -> VcpuExit<'_> {
        self.fd.run().expect("run failed")
    }
}

pub struct VirtualMachine{
    vcpu: VCPU,
    io_map: IODeviceMap,
    mmio_map: MMIODeviceMap,
}

impl VirtualMachine{
    pub fn new(init_mem_image: Vec<u8>) -> Self{
        let kvm: Kvm = Kvm::new().unwrap();
        let vm = kvm.create_vm().unwrap();

        let raw_ptr = unsafe {
            mmap(
                std::ptr::null_mut(),
                MEM_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0,
            )
        };

        if raw_ptr == libc::MAP_FAILED {
            panic!("mmap failed");
        }

        let userspace_mem = raw_ptr as *mut u8;
        unsafe { ptr::copy_nonoverlapping(init_mem_image.as_ptr(), userspace_mem, init_mem_image.len()); }
        let memory_region = kvm_userspace_memory_region{
            slot: 0,
            flags: 0,
            guest_phys_addr: 0x1000,
            memory_size: 0x1000,
            userspace_addr: userspace_mem as u64
        };

        let _mem = unsafe { vm.set_user_memory_region(memory_region) }.unwrap();

        let io_map = IODeviceMap::new();
        let mmio_map = MMIODeviceMap::new();
        Self {
            vcpu: VCPU::new(vm),
            io_map,
            mmio_map
        }
    }

    pub fn register_io_device(&mut self, region: IODeviceRegion) {
        self.io_map.register(region);
    }

    pub fn register_mmio_device(&mut self, region: MMIODeviceRegion) {
        self.mmio_map.register(region);
    }

    pub fn run(&mut self) -> Result<(), CrashReason> {
        match self.vcpu.run() {
            VcpuExit::Hlt => {
                println!("KVM_EXIT_HLT");
                return Err(CrashReason::Hlt);
            }
            VcpuExit::IoOut(port, data) => {
                self.io_map.output(port, data);
            }
            VcpuExit::IoIn(port, data) => {
                let ret = self.io_map.input(port, data.len());
                if ret.is_none() {
                    return Err(CrashReason::NoIODataReturned);
                }
                let ret = ret.unwrap();

                if ret.len() != data.len() {
                    return Err(CrashReason::IncorrectIOInputLength);
                }
                data.copy_from_slice(&ret);
            }
            VcpuExit::MmioWrite(addr, data) => {
                self.mmio_map.write(addr, data);
            }
            VcpuExit::MmioRead(addr, data) => {
                let ret = self.mmio_map.read(addr, data.len());
                if ret.is_none() {
                    return Err(CrashReason::NoMMIODataReturned);
                }
                let ret = ret.unwrap();

                if ret.len() != data.len() {
                    return Err(CrashReason::IncorrectMMIOReadLength);
                }
                data.copy_from_slice(&ret);
            }
            VcpuExit::FailEntry(reason, ..) => {
                eprintln!(
                    "KVM_EXIT_FAIL_ENTRY: reason = {:#x}",
                    reason
                );
                return Err(CrashReason::FailedEntry);
            }
            exit_reason => {
                println!("Unhandled exit: {:?}", exit_reason);
                return Err(CrashReason::UnhandledExit);
            }
        }
        Ok(())
    }
}

fn main() {
    let init_mem_image = fs::read("guest/firmware.bin").unwrap();
    let mut vm = VirtualMachine::new(Vec::from(init_mem_image));

    let serial_device = Box::new(Serial{});
    vm.register_io_device(IODeviceRegion::new(0x3f8..=0x3f8, serial_device));

    loop {
        let ret = vm.run();
        if ret.is_err() {
            break;
        }
    }
}