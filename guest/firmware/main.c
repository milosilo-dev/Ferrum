#include "serial.h"
#include "rng.c"
#include "counter.c"
#include "blk.c"

void c_main(void) {
    serial_init();
    virtio_rng_init();
    virtio_cnt_init();
    virtio_blk_init();

    uint8_t rnd_buf[16] = {0};
    uint32_t written = virtio_rng_read(rnd_buf, 16);

    serial_puts("bytes written: "); serial_putx(written); serial_puts("\n");
    serial_puts("random bytes: ");
    for (int i = 0; i < 16; i++) {
        serial_putx(rnd_buf[i]);
        serial_putc(' ');
    }
    serial_puts("\n");

    uint32_t increament = virtio_cnt(0x20);
    serial_puts("counter: ");
    serial_putx(increament);
    serial_puts("\n");

    uint8_t blk_buf_write[10] = {0xFF};
    uint8_t status = virtio_blk_write(0, 10, blk_buf_write);
    serial_puts("disk write status: ");serial_putx(status);serial_puts("\n");

    uint8_t blk_buf_read[20] = {0};
    uint8_t status2 = virtio_blk_read(0, 20, blk_buf_read);
    serial_puts("disk read status: ");serial_putx(status2);serial_puts("\n");
    serial_puts("read from disk: ");
    for (int i = 0; i < 20; i++) {
        serial_putx(blk_buf_read[i]);
        serial_putc(' ');
    }
    serial_puts("\n");

    // spin forever
    while (1) {
        __asm__ volatile("hlt");
    }
}