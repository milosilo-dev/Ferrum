#pragma once

#include "serial.h"

__attribute__((noreturn))
static void hang(void) {
    for (;;) {
        outb(0x500, 0);
    }
}