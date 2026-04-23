#include "serial.h"
#include "rng.c"
#include "counter.c"

void c_main(void) {
    serial_init();
    virtio_rng_init();
    virtio_cnt_init();

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

    // spin forever
    while (1) {
        __asm__ volatile("hlt");
    }
}