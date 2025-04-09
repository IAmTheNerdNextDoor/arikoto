#include <print.h>
#include <kernel.h>
#include <request.h>

extern uint8_t _binary_matrix_psf_start;
extern uint8_t _binary_matrix_psf_size;

/* PSF1 font header structure. */
struct psf1_header {
    uint8_t magic[2];
    uint8_t mode;
    uint8_t charsize;
};

static uint32_t *framebuffer = NULL;
static size_t pitch = 0;
static size_t bpp = 0;
static size_t cursor_x = 0;
static size_t cursor_y = 0;
static size_t screen_width = 0;
static size_t screen_height = 0;

void setup_framebuffer(uint32_t *fb, size_t p, size_t bpp_val, size_t width, size_t height) {
    framebuffer = fb;
    pitch = p;
    bpp = bpp_val;
    screen_width = width;
    screen_height = height;
    cursor_x = 0;
    cursor_y = 0;
}

/* Function to render a single character */
void putchar(char c, uint32_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 16;
        if (cursor_y + 16 >= screen_height) {
            cursor_y = 0;
        }
        return;
    } else if (c == '\b') {
        if (cursor_x >= 8) {
            cursor_x -= 8;

            for (size_t py = 0; py < 16; py++) {
                for (size_t px = 0; px < 8; px++) {
                    size_t pixel_index = (cursor_y + py) * (pitch / (bpp / 8)) + (cursor_x + px);
                    framebuffer[pixel_index] = COLOR_BLACK;
                }
            }
        }
        return;
    }

    struct psf1_header *font = (struct psf1_header *)&_binary_matrix_psf_start;
    uint8_t *glyphs = (uint8_t *)(&_binary_matrix_psf_start + sizeof(struct psf1_header));
    uint8_t *glyph = glyphs + (c * font->charsize);

    for (size_t py = 0; py < font->charsize; py++) {
        for (size_t px = 0; px < 8; px++) {
            size_t pixel_index = (cursor_y + py) * (pitch / (bpp / 8)) + (cursor_x + px);
            framebuffer[pixel_index] = COLOR_BLACK;
        }
    }

    for (size_t py = 0; py < font->charsize; py++) {
        for (size_t px = 0; px < 8; px++) {
            if (glyph[py] & (0x80 >> px)) {
                size_t pixel_index = (cursor_y + py) * (pitch / (bpp / 8)) + (cursor_x + px);
                framebuffer[pixel_index] = color;
            }
        }
    }

    cursor_x += 8;

    if (cursor_x + 8 >= screen_width) {
        cursor_x = 0;
        cursor_y += 16;
    }
    if (cursor_y + 16 >= screen_height) {
        cursor_y = 0;
    }
}

int vsnprintf(char *buffer, size_t size, const char *fmt, va_list args) {
    char *buf = buffer;
    char *end = buffer + size - 1;

    while (*fmt && buf < end) {
        if (*fmt != '%') {
            *buf++ = *fmt++;
            continue;
        }

        fmt++;

        if (*fmt == '%') {
            *buf++ = *fmt++;
            continue;
        }

        if (*fmt == 'd') {
            int num = va_arg(args, int);
            unsigned int u_num;
            bool is_negative = false;

            if (num < 0) {
                is_negative = true;
                u_num = (unsigned int)-num;
            } else {
                u_num = (unsigned int)num;
            }
            if (is_negative && buf < end) {
                *buf++ = '-';
            }
            char tmp[11];
            int i = 0;
            if (u_num == 0) {
                tmp[i++] = '0';
            } else {
                while (u_num > 0 && i < 10) {
                    tmp[i++] = (u_num % 10) + '0';
                    u_num /= 10;
                }
            }
            while (i-- > 0 && buf < end) {
                *buf++ = tmp[i];
            }
        }
        else if (*fmt == 'x') {
            unsigned int num = va_arg(args, unsigned int);
            char tmp[9];
            int i = 0;
            int base = 16;

            if (num == 0) {
                tmp[i++] = '0';
            } else {
                 while (num > 0 && i < 8) {
                    tmp[i++] = "0123456789abcdef"[num % base];
                    num /= base;
                }
            }
            while (i-- > 0 && buf < end) {
                *buf++ = tmp[i];
            }
        }
        else if (*fmt == 's') {
            char *s = va_arg(args, char *);
            if (s == NULL) {
                s = "(null)";
            }
            while (*s && buf < end) {
                *buf++ = *s++;
            }
        }
         else if (*fmt == 'c') {
            *buf++ = (char)va_arg(args, int);
         }
        else if (*fmt == 'C') {
            if (buf < end - 1) {
                *buf++ = '\x01';
                *buf++ = (char)va_arg(args, int);
            } else {
                (void)va_arg(args, int);
            }
        }
        else {
            if(buf < end) {
                *buf++ = *fmt;
            }
        }
        fmt++;
    }
    *buf = '\0';
    return buf - buffer;
}

void printk(uint32_t default_color, const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    uint32_t current_color = default_color;
    for (char *c = buffer; *c != '\0'; c++) {
        if (*c == '\x01') {
            c++;
            if (*c != '\0') {
                current_color = (uint32_t)(unsigned char)*c;
            }
            continue;
        }
        putchar(*c, current_color);
    }
}

void screen_clear(void) {
    if (framebuffer == NULL) {
        return;
    }

    size_t total_pixels = (pitch / (bpp / 8)) * screen_height;

    for (size_t i = 0; i < total_pixels; i++) {
        framebuffer[i] = COLOR_BLACK;
    }

    cursor_x = 0;
    cursor_y = 0;
}

void init_framebuffer() {
    if (framebuffer_info.response == NULL || framebuffer_info.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_info.response->framebuffers[0];
    uint32_t *fb = framebuffer->address;
    size_t pitch = framebuffer->pitch;
    size_t bpp = framebuffer->bpp;
    size_t max_width = framebuffer->width;
    size_t max_height = framebuffer->height;

    setup_framebuffer(fb, pitch, bpp, max_width, max_height);
}
