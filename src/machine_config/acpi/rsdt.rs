use crate::machine_config::binary::Binary;

pub fn build_rsdt() -> Binary {
    let mut rsdt = vec![0; 0];

    Binary::new(
        rsdt,
        0x0
    )
}