#include <serial.h>
#include <pic.h>
#include <pit.h>
#include <multitask.h>

volatile uint64_t pit_ticks = 0;

void pit_init(uint32_t frequency) {
    pit_frequency = frequency;

    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    uint8_t cmd = PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_BOTH | PIT_CMD_MODE3 | PIT_CMD_BINARY;

    outb(PIT_COMMAND, cmd);
    io_wait();

    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xFF);
    io_wait();

    pic_unmask_irq(0);
}

void pit_set_frequency(uint32_t frequency) {
    pit_init(frequency);
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

uint64_t pit_get_elapsed_ms(void) {
    return (pit_ticks * 1000) / pit_frequency;
}

void pit_irq_handler(void) {
    pit_ticks++;
    task_timer_tick();
}

void pit_sleep_ms(uint32_t ms) {
    uint64_t target_ticks = pit_ticks + ((uint64_t)ms * pit_frequency) / 1000;
    while (pit_ticks < target_ticks) {
        asm volatile("hlt");
    }
}
