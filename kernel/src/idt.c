#include <isr.h>
#include <irq.h>
#include <serial.h>

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
    idt_set_gate(32, (uint64_t)do_irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)do_irq1, 0x08, 0x8E);

    asm volatile("lidt (%0)" : : "r" (&idtp));
    asm volatile("sti");
}
