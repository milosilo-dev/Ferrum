use std::ffi::OsStr;
use std::fs;
use std::process::Command;

const ASM: &str = "nasm";

fn remove_files_with_extension(dir_path: &str, extension: &str) -> Result<(), Box<dyn std::error::Error>> {
    for entry in fs::read_dir(dir_path)? {
        let path = entry?.path();
        // Check if the file has the specific extension
        if path.extension() == Some(OsStr::new(extension)) {
            if path.is_file() {
                fs::remove_file(&path)?;
            }
        }
    }
    Ok(())
}

fn main() {
    let _ = remove_files_with_extension("guest", "bin");

    let entries = fs::read_dir("guest").expect("failed to read guest directory");

    for entry in entries {
        let entry = entry.expect("failed to read entry");
        let path = entry.path();
        println!("Compiled {}", path.to_str().unwrap());

        if path.extension().and_then(|e| e.to_str()) != Some("asm") {
            continue;
        }

        let input = path.to_str().expect("invalid path");
        let output = path.with_extension("bin");
        let output = output.to_str().expect("invalid output path");

        let status = Command::new(ASM)
            .args(["-f", "bin", input, "-o", output])
            .status()
            .expect(&format!("failed to run: {}", ASM));

        if !status.success() {
            panic!("{} failed on {}", ASM, input);
        }

        println!("cargo:rerun-if-changed={}", input);
    }
}
