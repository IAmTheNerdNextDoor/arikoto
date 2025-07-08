#include <arch/x86_64/pic.h>
#include <misc/serial.h>

void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(int offset1, int offset2) {
    uint8_t mask1, mask2;

    mask1 = inb(PIC1_DATA);
    io_wait();
    mask2 = inb(PIC2_DATA);
    io_wait();

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();

    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    io_wait();
    outb(PIC2_DATA, mask2);
    io_wait();

    pic_unmask_irq(1);
    pic_unmask_irq(2);
}

void pic_mask_irq(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if(irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = inb(port);
    io_wait();
    value |= (1 << irq_line);
    outb(port, value);
    io_wait();
}

void pic_send_eoi(uint8_t irq) {
    if(irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_unmask_irq(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if(irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;

        value = inb(PIC1_DATA);
        io_wait();
        outb(PIC1_DATA, value & ~(1 << 2));
        io_wait();
    }

    value = inb(port);
    io_wait();
    value &= ~(1 << irq_line);
    outb(port, value);
    io_wait();
}
