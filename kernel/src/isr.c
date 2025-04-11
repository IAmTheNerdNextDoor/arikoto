#include <kernel.h>

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

void isr14() {
    panic("PANIC: Page fault exception\n");
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
