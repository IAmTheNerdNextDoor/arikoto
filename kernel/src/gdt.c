#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;          /* Lower 16 bits of the segment limit */
    uint16_t base_low;           /* Lower 16 bits of the segment base address */
    uint8_t base_middle;         /* Next 8 bits of the segment base address */
    uint8_t access;              /* Access flags (read/write, execute, etc) */
    uint8_t granularity;         /* Granularity flags (limit size, etc) */
    uint8_t base_high;           /* Upper 8 bits of the segment base address */
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gdtp;

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char granularity) {
    gdt[num].base_low = base & 0xFFFF;                   /* Lower 16 bits of base */
    gdt[num].base_middle = (base >> 16) & 0xFF;          /* Middle 8 bits of base */
    gdt[num].base_high = (base >> 24) & 0xFF;            /* Upper 8 bits of base */
    gdt[num].limit_low = limit & 0xFFFF;                 /* Lower 16 bits of limit */
    gdt[num].granularity = (limit >> 16) & 0x0F;         /* Higher 4 bits of limit */
    gdt[num].granularity |= granularity & 0xF0;          /* Granularity flags */
    gdt[num].access = access;                            /* Access flags (code/data/privilege) */
}

void init_gdt() {
    gdtp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdtp.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                     /* Null descriptor */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);      /* 64-bit code segment (L flag set, executable) */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xC0);      /* 64-bit data segment (non-executable) */

    /*
    * gdt_flush I had originally planned but since nasm wasn't
    * cooperating with me, I moved it to inline assembly instead.
    */
    asm volatile (
        "lea %[gdtp], %%rax\n\t"
        "lgdt (%%rax)\n\t"

        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"

        "pushq $0x08\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "retfq\n\t"

        "1:\n\t"
        :
        : [gdtp] "m"(gdtp)
        : "rax", "memory"
    );
}
