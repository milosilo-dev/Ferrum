#include "serial.h"
#include "blk.c"

void c_main_64(void) {
    serial_puts("Long mode!\n");

    uint8_t sector[512];
    uint32_t status = virtio_blk_read(0, 512, sector);
    serial_puts("status: "); serial_putx(status); serial_puts("\n");
    serial_puts("MBR sig: ");
    serial_putx(sector[510]); serial_putc(' ');
    serial_putx(sector[511]); serial_puts("\n");

    // spin forever
    while (1) {
        __asm__ volatile("hlt");
    }
}