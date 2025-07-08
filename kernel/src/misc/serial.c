#include <misc/serial.h>
#include <arch/x86_64/pic.h>
#include <stdbool.h>

#define COM1_PORT 0x3F8

static bool g_serial_initialized = false;

static int serial_is_transmit_empty() {
   return inb(COM1_PORT + 5) & 0x20;
}

void serial_putchar(char a) {
   if (!g_serial_initialized) return;
   while (serial_is_transmit_empty() == 0) {
       io_wait();
   }
   outb(COM1_PORT, a);
}

void serial_puts(const char *s) {
    if (!g_serial_initialized) return;
    for (int i = 0; s[i] != '\0'; ++i) {
        serial_putchar(s[i]);
    }
}

void init_serial() {
   outb(COM1_PORT + 1, 0x00);
   io_wait();
   outb(COM1_PORT + 3, 0x80);
   io_wait();
   outb(COM1_PORT + 0, 0x03);
   io_wait();
   outb(COM1_PORT + 1, 0x00);
   io_wait();
   outb(COM1_PORT + 3, 0x03);
   io_wait();
   outb(COM1_PORT + 2, 0xC7);
   io_wait();
   outb(COM1_PORT + 4, 0x0B);
   io_wait();

   g_serial_initialized = true;
}

bool is_serial_initialized(void) {
    return g_serial_initialized;
}

static int serial_received() {
    return inb(COM1_PORT + 5) & 1;
}

char serial_getchar() {
    if (!g_serial_initialized) return 0;

    while (serial_received() == 0) {
        io_wait();
    }

    return inb(COM1_PORT);
}

char serial_try_getchar() {
    if (!g_serial_initialized) return 0;
    if (inb(COM1_PORT + 5) & 1) {
        return inb(COM1_PORT);
    }
    return 0;
}
