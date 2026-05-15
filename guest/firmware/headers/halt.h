#pragma once

__attribute__((noreturn))
static void hang(void) {
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}