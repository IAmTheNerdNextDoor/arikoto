#include <misc/kernel.h>
#include <stdint.h>
#include <misc/print.h>
#include <arch/x86_64/vmm.h>
#include <arch/x86_64/memory.h>

void isr0() {
    panic("PANIC: Division by zero exception\n");
}

void isr1() {
    panic("PANIC: Debug exception\n");
}

void isr2() {
    panic("PANIC: Non-maskable interrupt exception\n");
}

void isr3() {
    panic("PANIC: Breakpoint exception\n");
}

void isr4() {
    panic("PANIC: Into overflow exception\n");
}

void isr5() {
    panic("PANIC: Out of bounds exception\n");
}

void isr6() {
    panic("PANIC: Invalid opcode exception\n");
}

void isr7() {
    panic("PANIC: No coprocessor exception\n");
}

void isr8() {
    panic("PANIC: Double fault exception\n");
}

void isr9() {
    panic("PANIC: Coprocessor segment overrun exception\n");
}

void isr10() {
    panic("PANIC: Bad TSS exception\n");
}

void isr11() {
    panic("PANIC: Segment not present exception\n");
}

void isr12() {
    panic("PANIC: Stack fault exception\n");
}

void isr13() {
    panic("PANIC: General protection fault exception\n");
}

void page_fault_handler(uint64_t fault_addr, uint64_t error_code, uint64_t rip, uint64_t cr2) {
    printk(COLOR_RED, "PAGE FAULT at 0x%lx (cr2=0x%lx), error=0x%lx, rip=0x%lx\n",
        fault_addr, cr2, error_code, rip);

    printk(COLOR_WHITE, "  %s, %s, %s, %s\n",
        (error_code & 1) ? "protection violation" : "non-present page",
        (error_code & 2) ? "write" : "read",
        (error_code & 4) ? "user" : "kernel",
        (error_code & 8) ? "reserved bit" : "no reserved bit"
    );

    if ((fault_addr >= 0xffffffff80000000ULL) && (fault_addr < 0xffffffffc0000000ULL)) {
        void *phys = allocate_page();
        if (phys) {
            vmm_map_page(kernel_pagemap, fault_addr & ~(PAGE_SIZE-1), (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE);
            printk(COLOR_GREEN, "Demand-paged kernel page at 0x%lx\n", fault_addr & ~(PAGE_SIZE-1));
            return;
        }
    }
    hcf();
}

void isr14(void) {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    uint64_t rip, err;
    asm volatile("mov 8(%%rsp), %0" : "=r"(err));
    asm volatile("mov 16(%%rsp), %0" : "=r"(rip));
    page_fault_handler(cr2, err, rip, cr2);
}

void isr15() {
    panic("PANIC: Unknown interrupt exception\n");
}

void isr16() {
    panic("PANIC: Coprocessor fault exception\n");
}

void isr17() {
    panic("PANIC: Alignment check exception\n");
}

void isr18() {
    panic("PANIC: Machine check exception\n");
}

void isr_reserved() {
    panic("PANIC: Reserved exception");
}
