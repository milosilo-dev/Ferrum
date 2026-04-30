use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize, Debug)]
pub struct MemMap{
    pub base: u64,
    pub length: u64,
    pub mem_type: u32,
}