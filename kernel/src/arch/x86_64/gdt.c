#include <arch/x86_64/gdt.h>
#include <misc/string.h>
#include <misc/print.h>

__attribute__((aligned(16)))
uint8_t kernel_stack[KERNEL_STACK_SIZE];

__attribute__((aligned(16)))
uint8_t user_stack[USER_STACK_SIZE];

struct __attribute__((packed)) TSS {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3, io_map_base;
} tss;

struct GDTEntry {
    uint16_t lo_lim, lo_base;
    uint8_t mid_base, access, flags_hi, hi_base;
} __attribute__((packed));

struct GDTPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct GDTEntry gdt[7];
static struct GDTPtr gdtp;

static void gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[i].lo_base   = base & 0xFFFF;
    gdt[i].mid_base  = (base >> 16) & 0xFF;
    gdt[i].hi_base   = (base >> 24) & 0xFF;
    gdt[i].lo_lim    = limit & 0xFFFF;
    gdt[i].flags_hi  = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].access    = access;
}

static void gdt_set_tss(int i, uint64_t base, uint32_t limit) {
    gdt_set(i, base, limit, 0x89, 0x00);
    uint32_t *hi = (uint32_t*)&gdt[i + 1];
    hi[0] = base >> 32;
    hi[1] = 0;
}

void init_gdt_tss(void) {
    memset(&gdt, 0, sizeof(gdt));
    memset(&tss, 0, sizeof(tss));

    gdt_set(0, 0, 0, 0, 0);
    gdt_set(1, 0, 0xFFFFF, 0x9A, 0xA0);
    gdt_set(2, 0, 0xFFFFF, 0x92, 0xC0);
    gdt_set_tss(3, (uint64_t)&tss, sizeof(tss)-1);
    gdt_set(5, 0, 0xFFFFF, 0xFA, 0xA0);
    gdt_set(6, 0, 0xFFFFF, 0xF2, 0xC0);

    tss.rsp0 = (uintptr_t)&kernel_stack[KERNEL_STACK_SIZE];

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint64_t)&gdt;

    asm volatile("lgdt %0" :: "m"(gdtp));

    asm volatile(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "pushq $0x08\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "retfq\n\t"
        "1:\n\t"
        :::"rax"
    );

    printk(COLOR_GREEN, "GDT installed\n");

    asm volatile("ltr %%ax" :: "a"(0x18));

    printk(COLOR_GREEN, "TSS installed\n");
}
