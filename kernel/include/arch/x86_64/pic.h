#pragma once

#include <stdint.h>

#define PIC1_VECTOR_OFFSET 0x20
#define PIC2_VECTOR_OFFSET 0x28

#define PIC1         0x20  /* IO base address for master PIC */
#define PIC2         0xA0  /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2+1)

#define PIC_EOI      0x20

#define ICW1_ICW4      0x01  /* ICW4 is present */
#define ICW1_SINGLE    0x02  /* Single cascade mode */
#define ICW1_INTERVAL4 0x04  /* Call address interval 4 */
#define ICW1_LEVEL     0x08  /* Level triggered edge mode */
#define ICW1_INIT      0x10  /* Initialization */

#define ICW4_8086      0x01  /* 8086/88 mode */
#define ICW4_AUTO      0x02  /* Auto EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM      0x10  /* Special fully nested */

void pic_remap(int offset1, int offset2);
void pic_mask_irq(uint8_t irq_line);
void pic_unmask_irq(uint8_t irq_line);
void io_wait(void);
