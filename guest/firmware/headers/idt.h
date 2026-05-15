#include <stdint.h>
#include "serial.h"

#define IDT_ENTRIES 32
#define ISR_LIST \
    X(0)  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  \
    X(8)  X(9)  X(10) X(11) X(12) X(13) X(14) X(15)  \
    X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23)  \
    X(24) X(25) X(26) X(27) X(28) X(29) X(30) X(31)

#define X(n) extern void isr##n(void);
ISR_LIST
#undef X

typedef struct {
    uint16_t offset_low;    // handler address [15:0]
    uint16_t selector;      // code segment selector (0x18 — your 64-bit CS)
    uint8_t  ist;           // interrupt stack table, 0 = none
    uint8_t  type_attr;     // 0x8E = present, ring-0, interrupt gate
    uint16_t offset_mid;    // handler address [31:16]
    uint32_t offset_high;   // handler address [63:32]
    uint32_t reserved;      // must be zero
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t size;
    uint64_t base;
} __attribute__((packed)) IDTPointer;

static IDTEntry idt[IDT_ENTRIES];
static IDTPointer idtp;

static inline void idt_set_entry(int n, uint64_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x18;        // your 64-bit code segment
    idt[n].ist         = 0;
    idt[n].type_attr   = 0x8E;        // present, ring-0, interrupt gate
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].reserved    = 0;
}

static inline void idt_init(void) {
    idtp.size = sizeof(idt) - 1;
    idtp.base = (uint64_t)idt;

    #define X(n) idt_set_entry(n, (uint64_t)isr##n);
    ISR_LIST
    #undef X

    // double fault must use IST1 so it has a known-good stack
    idt[8].ist = 1;

    __asm__ volatile("lidt %0" :: "m"(idtp));
}

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10;
    uint64_t r9, r8, rdi, rsi, rdx, rcx, rbx, rax, rbp;
    uint64_t exception_num, error_code, rip, cs, rflags;
} ExceptionContext;

void exception_handler(ExceptionContext* ctx) {
    serial_puts("EXCEPTION: ");
    serial_putx(ctx->exception_num);
    serial_puts(" ERR: ");
    serial_putx(ctx->error_code);
    serial_puts(" RIP: ");
    serial_putx(ctx->rip);
    serial_puts("\n  CODE: ");
    for (int i = 0; i < 16; i++) {
        serial_putx(((uint8_t*)ctx->rip)[i]);
        serial_putc(' ');
    }
    uint64_t old_rsp = (uint64_t)(&ctx->rflags) + 8;
    serial_puts("\n  OLD_RSP: ");
    serial_putx(old_rsp);
    serial_puts("\n  OLD_STACK: ");
    for (int i = 0; i < 20; i++) {
        serial_putx(((uint64_t*)old_rsp)[i]);
        serial_putc(' ');
    }
    serial_puts("\n  PRE_CODE @ rip-32: ");
    for (int i = 0; i < 32; i++) {
        serial_putx(((uint8_t*)(ctx->rip - 32))[i]);
        serial_putc(' ');
    }
    serial_puts("\n  REGS: ");
    serial_puts("\n  rax="); serial_putx(ctx->rax);
    serial_puts(" rbx="); serial_putx(ctx->rbx);
    serial_puts(" rcx="); serial_putx(ctx->rcx);
    serial_puts(" rdx="); serial_putx(ctx->rdx);
    serial_puts(" rsi="); serial_putx(ctx->rsi);
    serial_puts(" rdi="); serial_putx(ctx->rdi);
    serial_puts("\n  r8="); serial_putx(ctx->r8);
    serial_puts(" r9="); serial_putx(ctx->r9);
    serial_puts(" r10="); serial_putx(ctx->r10);
    serial_puts(" r11="); serial_putx(ctx->r11);
    serial_puts("\n  r12="); serial_putx(ctx->r12);
    serial_puts(" r13="); serial_putx(ctx->r13);
    serial_puts(" r14="); serial_putx(ctx->r14);
    serial_puts(" r15="); serial_putx(ctx->r15);
    serial_puts("\n  rbp="); serial_putx(ctx->rbp);
    serial_puts("\n  rflags="); serial_putx(ctx->rflags);
    serial_puts("\n  cs="); serial_putx(ctx->cs);
    serial_puts("\n  CODE_RA:");
    for (int s = 0; s < 20; s++) {
        uint64_t addr = ((uint64_t*)old_rsp)[s];
        if (addr >= 0x1200000 && addr <= 0x1310000) {
            serial_puts("\n    stk["); serial_putx((uint64_t)s); serial_puts("]@");
            serial_putx(addr); serial_puts("-8: ");
            for (int i = -8; i < 20; i++) {
                serial_putx(((uint8_t*)addr)[i]); serial_putc(' ');
            }
        }
    }
    serial_puts("\n  ALLOCPOOL_BUF @0x30605D8: ");
    for (int i = 0; i < 27; i++) {
        serial_putx(((uint64_t*)0x30605D8)[i]);
        serial_putc(' ');
    }
    serial_puts("\n");
    for(;;) outb(0x500, 0);
}