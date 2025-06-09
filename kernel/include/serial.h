#pragma once

#include <stdint.h>
#include <stdbool.h>

/* IO Port functions */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Serial port functions */
void init_serial(void);
void serial_putchar(char a);
void serial_puts(const char *s);
bool is_serial_initialized(void);
char serial_try_getchar();
