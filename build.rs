use std::process::Command;

const FIRMWARE_CODE: &str = "guest/firmware.asm";
const FIRMWARE_OUTPUT: &str = "guest/firmware.bin";

const ASM: &str = "nasm";

fn main() {

    let status = Command::new(ASM)
        .args([
            "-f", "bin",
            FIRMWARE_CODE,
            "-o",
            FIRMWARE_OUTPUT,
        ])
        .status()
        .expect(format!("failed to run: {}", ASM).as_str());

    if !status.success() {
        panic!("{} failed", ASM);
    }

    println!("cargo:rerun-if-changed=asm/guest.asm");
}