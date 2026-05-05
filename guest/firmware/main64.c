#include "headers/serial.h"
#include "virtio/blk.c"
#include "headers/idt.h"
#include "headers/sector_range.h"
#include "disk/esp.c"
#include "disk/fat32.c"
#include "memmap.c"

void c_main_64(void) {
    serial_puts("=-- Long mode --=\n");
    idt_init();
    init_memmap();
    virtio_blk_init();

    SectorRange sec_range;
    int status = load_part_table(&sec_range);
    if (status != 0) return;

    Fat32_Handle fs;
    status = open_fat32(&sec_range, &fs);
    if (status != 0) return;
    status = open_root_dir(&fs);
    if (status != 0) return;

    // spin forever
    while (1) {
        __asm__ volatile("hlt");
    }
}