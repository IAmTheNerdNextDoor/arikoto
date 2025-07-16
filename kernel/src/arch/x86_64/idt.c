#include <arch/x86_64/isr.h>
#include <arch/x86_64/irq.h>
#include <misc/serial.h>
#include <misc/print.h>

struct idt_entry {
    uint16_t isr_low;   /* Lower 16 bits of ISR */
    uint16_t selector;  /* Code segment selector */
    uint8_t ist;        /* IST offset */
    uint8_t flags;      /* Type and attribute */
    uint16_t isr_mid;   /* Middle 16 bits of ISR */
    uint32_t isr_high;  /* Upper 32 bits of ISR */
    uint32_t reserved;  /* Reserved at 0 */
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;     /* IDT size but in bytes */
    uint64_t base;      /* Address of first entry */
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(int num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].isr_low = base & 0xFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0; /* Kept at 0 for now */
    idt[num].flags = flags;
    idt[num].isr_mid = (base >> 16) & 0xFFFF;
    idt[num].isr_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved = 0;
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt;

    idt_set_gate(0, (uint64_t)do_isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)do_isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint64_t)do_isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint64_t)do_isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint64_t)do_isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint64_t)do_isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint64_t)do_isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint64_t)do_isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)do_isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint64_t)do_isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint64_t)do_isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)do_isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)do_isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)do_isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)do_isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)do_isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)do_isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint64_t)do_isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)do_isr18, 0x08, 0x8E);
    for (int i = 19; i < 31; i++)
    {
        idt_set_gate(i, (uint64_t)do_isr_reserved, 0x08, 0x8E);
    }
    idt_set_gate(32, (uint64_t)do_irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)do_irq1, 0x08, 0x8E);

    asm volatile("lidt (%0)" : : "r" (&idtp));
    printk(COLOR_GREEN, "IDT installed\n");
    asm volatile("sti");
    printk(COLOR_GREEN, "Interrupts enabled\n");
}
