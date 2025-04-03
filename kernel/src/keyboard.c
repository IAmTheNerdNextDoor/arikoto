#include <keyboard.h>
#include <stdint.h>
#include <stdbool.h>
#include <serial.h>
#include <idt.h>
#include <pic.h>
#include <print.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool extended_key = false;

static char key_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_start = 0;
static volatile int buffer_end = 0;

static const char scan_code_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scan_code_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static void keyboard_buffer_add(char c) {
    asm volatile("cli");

    if ((buffer_end + 1) % KEYBOARD_BUFFER_SIZE != buffer_start) {
        key_buffer[buffer_end] = c;
        buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    }

    asm volatile("sti");
}

void init_keyboard() {
    while (inb(0x64) & 0x2);

    outb(0x64, 0xAD);
    io_wait();
    outb(0x64, 0xAE);
    io_wait();

    inb(0x60);
    io_wait();

    outb(0x64, 0x20);
    io_wait();
    uint8_t config = inb(0x60);
    io_wait();
    config |= 1;
    outb(0x64, 0x60);
    io_wait();
    outb(0x60, config);
    io_wait();
}

void keyboard_callback() {
    uint8_t scan_code = inb(PS2_DATA_PORT);

    if (scan_code == KEY_EXTENDED) {
        extended_key = true;
        return;
    }

    if (scan_code & KEY_RELEASE) {
        scan_code &= ~KEY_RELEASE;

        if (scan_code == KEY_LSHIFT || scan_code == KEY_RSHIFT) {
            shift_pressed = false;
        } else if (scan_code == KEY_LCTRL) {
            ctrl_pressed = false;
        } else if (scan_code == KEY_LALT) {
            alt_pressed = false;
        }

        extended_key = false;
        return;
    }

    if (extended_key) {
        extended_key = false;
        return;
    }

    if (scan_code == KEY_LSHIFT || scan_code == KEY_RSHIFT) {
        shift_pressed = true;
        return;
    } else if (scan_code == KEY_LCTRL) {
        ctrl_pressed = true;
        return;
    } else if (scan_code == KEY_LALT) {
        alt_pressed = true;
        return;
    } else if (scan_code == KEY_CAPSLOCK) {
        caps_lock = !caps_lock;
        return;
    }

    if (scan_code < sizeof(scan_code_ascii)) {
        char ascii;

        if (shift_pressed) {
            ascii = scan_code_shift[scan_code];
        } else {
            ascii = scan_code_ascii[scan_code];
        }

        if (caps_lock && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 'A';
        } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
            ascii = ascii - 'A' + 'a';
        }

        if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 1;
        }

        if (ascii) {
            keyboard_buffer_add(ascii);
        }
    }
}

char keyboard_read() {
    if (buffer_start != buffer_end) {
        char c = key_buffer[buffer_start];
        buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
        return c;
    }

    return '\0';
}

bool keyboard_has_key() {
    return buffer_start != buffer_end;
}

char* keyboard_get_buffer() {
    return key_buffer;
}

void keyboard_clear_buffer() {
    buffer_start = 0;
    buffer_end = 0;
}
