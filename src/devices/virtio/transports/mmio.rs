use crate::{device_maps::mmio::MMIODevice, devices::virtio::virtio::{VirtioDevice, VirtioQueue}};

const MAGIC_NUMBER: u32 = 0x74726976;
const VERSION: u32 = 0x2;
const VENDOR_ID: u32 = 0x56484B53;

pub struct MMIOTransport {
    device: Box<dyn VirtioDevice + Sync + Send>,
    queue: Vec<VirtioQueue>,

    queue_sel: usize,
    status: u32,
    interrupt_status: u32,
}

impl MMIOTransport {
    pub fn new(device: Box<dyn VirtioDevice + Sync + Send>) -> Self{
        Self {
            device,
            queue: vec![VirtioQueue::new()],
            queue_sel: 0,
            status: 0,
            interrupt_status: 0,
        }
    }
}

impl MMIODevice for MMIOTransport {
    fn read(&mut self, addr: u64, length: usize) -> Vec<u8> {
        let value = (match addr {
            0x000 => MAGIC_NUMBER,
            0x004 => VERSION,
            0x008 => self.device.virtio_type(),
            0x00C => VENDOR_ID,
            0x010 => self.device.features(),
            0x034 => self.queue_sel as u32,
            0x038 => self.queue[self.queue_sel].size as u32,
            0x044 => self.queue[self.queue_sel].ready as u32,
            0x070 => self.status,
            0x060 => self.interrupt_status,
            _ => 0,
        } as u64).to_le_bytes();
        value[..length].to_vec()
    }

    fn write(&mut self, addr: u64, data: &[u8]) {
        todo!()
    }

    fn irq_handler(&mut self, irq_handler: std::sync::Arc<std::sync::Mutex<crate::irq_handler::IRQHandler>>) {
        todo!()
    }

    fn tick(&mut self) {
        todo!()
    }
}