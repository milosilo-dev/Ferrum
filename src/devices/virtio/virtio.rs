pub trait VirtioDevice {
    fn virtio_type(&self) -> u32;
    fn features(&self) -> u32;
}

pub struct VirtioQueue {
    pub size: u16,
    pub ready: bool,

    pub desc_addr: u64,
    pub avail_addr: u64,
    pub used_addr: u64,
}

impl VirtioQueue {
    pub fn new() -> Self{
        Self{
            size: 0,
            ready: false,
            desc_addr: 0,
            avail_addr: 0,
            used_addr: 0,
        }
    }
}