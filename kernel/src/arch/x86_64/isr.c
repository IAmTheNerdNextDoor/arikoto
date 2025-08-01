#include <misc/kernel.h>
#include <stdint.h>
#include <misc/print.h>
#include <arch/x86_64/vmm.h>
#include <arch/x86_64/memory.h>
#include <arch/x86_64/request.h>

extern uint8_t _bss_end[];

void isr0() {
    panic("Division by zero exception\n");
}

void isr1() {
    panic("Debug exception\n");
}

void isr2() {
    panic("Non-maskable interrupt exception\n");
}

void isr3() {
    panic("Breakpoint exception\n");
}

void isr4() {
    panic("Into overflow exception\n");
}

void isr5() {
    panic("Out of bounds exception\n");
}

void isr6() {
    panic("Invalid opcode exception\n");
}

void isr7() {
    panic("No coprocessor exception\n");
}

void isr8() {
    panic("Double fault exception\n");
}

void isr9() {
    panic("Coprocessor segment overrun exception\n");
}

void isr10() {
    panic("Bad TSS exception\n");
}

void isr11() {
    panic("Segment not present exception\n");
}

void isr12() {
    panic("Stack fault exception\n");
}

void isr13() {
    panic("General protection fault exception\n");
}

void isr14(uint64_t *stack_frame) {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    uint64_t err = stack_frame[15];
    uint64_t rip = stack_frame[16];

    printk(COLOR_RED, "PAGE FAULT at 0x%lx, error=0x%lx, rip=0x%lx\n", cr2, err, rip);

    printk(COLOR_WHITE, "  %s, %s, %s, %s\n",
        (err & 1) ? "protection violation" : "non-present page",
        (err & 2) ? "write" : "read",
        (err & 4) ? "user" : "kernel",
        (err & 8) ? "reserved bit" : "no reserved bit"
    );

    uintptr_t KVIRT_BASE = kernel_address_request.response->virtual_base;
    uintptr_t KERNEL_TOP = ALIGN_UP((uintptr_t)_bss_end, PAGE_SIZE);

    if (cr2 >= KVIRT_BASE && cr2 < KERNEL_TOP) {
        void *phys = allocate_page();
        if (phys) {
            uintptr_t page = cr2 & ~(PAGE_SIZE - 1);
            vmm_map_page(kernel_pagemap, page, (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE);
            printk(COLOR_GREEN, "Demand-paged kernel page at 0x%lx\n", page);
            return;
        }
    }
    hcf();
}

void isr15() {
    panic("Unknown interrupt exception\n");
}

void isr16() {
    panic("Coprocessor fault exception\n");
}

void isr17() {
    panic("Alignment check exception\n");
}

void isr18() {
    panic("Machine check exception\n");
}

void isr_reserved() {
    panic("Reserved exception");
}
