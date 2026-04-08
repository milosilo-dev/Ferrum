use crate::device_maps::io::IODevice;

pub struct Serial{}

impl IODevice for Serial {
    fn input(&mut self, _port: u16, length: usize) -> Vec<u8> {
        vec![0; length]
    }

    fn output(&mut self, _port: u16, data: &[u8]) {
        for i in 0..data.len(){
            print!("{}", data[i] as char);
        }
    }
}