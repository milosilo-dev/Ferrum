use std::{
    alloc::System,
    time::{SystemTime, UNIX_EPOCH},
};

use chrono::{Timelike, Utc};

use crate::device_maps::io::IODevice;

#[derive(Debug, Clone)]
enum CmosRegister {
    Unset,
    Seconds,
    Minute,
    Hour,
    SecondAlarm,
    MinuteAlarm,
    HourAlarm,
    DayOfWeek,
    DayOfMonth,
    Month,
    Year,
    StatusA,
    StatusB,
    StatusC,
    StatusD,
}

pub struct Cmos {
    reg: CmosRegister,
}

impl Cmos {
    pub fn new() -> Self {
        Self {
            reg: CmosRegister::Unset,
        }
    }
}

// IO device mapped 0x70 - 0x71
impl IODevice for Cmos {
    fn input(&mut self, port: u16, length: usize) -> Vec<u8> {
        match port {
            1 => match self.reg {
                CmosRegister::Seconds => {
                    let now = Utc::now();
                    vec![now.second() as u8; length]
                }
                CmosRegister::Minute => {
                    let now = Utc::now();
                    vec![now.minute() as u8; length]
                }
                CmosRegister::Hour => {
                    let now = Utc::now();
                    vec![now.hour() as u8; length]
                }
                _ => vec![0; length],
            },
            _ => vec![0; length],
        }
    }

    fn output(&mut self, port: u16, data: &[u8]) {
        match port {
            0 => {
                self.reg = match data[data.len() - 1] {
                    0x00 => CmosRegister::Seconds,
                    0x01 => CmosRegister::SecondAlarm,
                    0x02 => CmosRegister::Minute,
                    0x03 => CmosRegister::MinuteAlarm,
                    0x04 => CmosRegister::Hour,
                    0x05 => CmosRegister::HourAlarm,
                    0x06 => CmosRegister::DayOfWeek,
                    0x07 => CmosRegister::DayOfMonth,
                    0x08 => CmosRegister::Month,
                    0x09 => CmosRegister::Year,
                    0x0A => CmosRegister::StatusA,
                    0x0B => CmosRegister::StatusB,
                    0x0C => CmosRegister::StatusC,
                    0x0D => CmosRegister::StatusD,
                    _ => self.reg.clone(),
                }
            }
            1 => match self.reg {
                _ => {}
            },
            _ => {}
        }
    }

    fn irq_handler(
        &mut self,
        _irq_handler: std::sync::Arc<std::sync::Mutex<crate::irq_handler::IRQHandler>>,
    ) {
    }

    fn tick(&mut self) {}
}
