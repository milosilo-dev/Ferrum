#include "serial.h"

void c_main_64(void) {
    serial_puts("Long mode!\n");

    // spin forever
    while (1) {
        __asm__ volatile("hlt");
    }
}