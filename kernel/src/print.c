#include <print.h>
#include <kernel.h>
#include <request.h>
#include <vmm.h>

extern uint8_t _binary_matrix_psf_start;
extern uint8_t _binary_matrix_psf_size;

int screencolor;

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

char* u64toa_hex(uint64_t num, char *str) {
    char buffer[17];
    int i = 15;
    buffer[16] = '\0';

    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    while (num > 0 && i >= 0) {
        int rem = num % 16;
        buffer[i--] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        num /= 16;
    }

    strcpy(str, &buffer[i + 1]);
    return str;
}

void setup_framebuffer(uint32_t *fb, size_t p, size_t bpp_val, size_t width, size_t height) {
    framebuffer = fb;
    pitch = p;
    bpp = bpp_val;
    screen_width = width;
    screen_height = height;
    cursor_x = 0;
    cursor_y = 0;
}

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
                    framebuffer[pixel_index] = screencolor;
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
            framebuffer[pixel_index] = screencolor;
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
    if (!buffer || size == 0) return 0;

    char *buf_start = buffer;
    char *buf = buffer;
    char *end = buffer + size - 1;

    while (*fmt && buf < end) {
        if (*fmt != '%') {
            *buf++ = *fmt++;
            continue;
        }

        fmt++;
        if (*fmt == '\0') break;

        if (*fmt == '%') {
            *buf++ = *fmt++;
            continue;
        }

        bool is_long_long = false;
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                is_long_long = true;
                fmt++;
            }
        }

        if (*fmt == 'd' || *fmt == 'i') {
            long long num_ll = is_long_long ? va_arg(args, long long) : va_arg(args, int);
            unsigned long long u_num;
            bool is_negative = false;

            if (num_ll < 0) {
                is_negative = true;
                u_num = (unsigned long long)-num_ll;
            } else {
                u_num = (unsigned long long)num_ll;
            }

            char tmp[21];
            int i = 0;
            if (u_num == 0) {
                tmp[i++] = '0';
            } else {
                while (u_num > 0 && i < 20) {
                    tmp[i++] = (u_num % 10) + '0';
                    u_num /= 10;
                }
            }

            if (is_negative && buf < end) {
                *buf++ = '-';
            }
            while (i-- > 0 && buf < end) {
                *buf++ = tmp[i];
            }
        } else if (*fmt == 'u') {
            unsigned long long u_num = is_long_long ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
            char tmp[21];
            int i = 0;
            if (u_num == 0) {
                tmp[i++] = '0';
            } else {
                while (u_num > 0 && i < 20) {
                    tmp[i++] = (u_num % 10) + '0';
                    u_num /= 10;
                }
            }
            while (i-- > 0 && buf < end) {
                *buf++ = tmp[i];
            }
        } else if (*fmt == 'x') {
             unsigned long long u_num = is_long_long ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
             char tmp[17];
             int i = 0;
             if (u_num == 0) {
                tmp[i++] = '0';
             } else {
                while (u_num > 0 && i < 16) {
                    tmp[i++] = "0123456789abcdef"[u_num % 16];
                    u_num /= 16;
                }
             }
             while (i-- > 0 && buf < end) {
                *buf++ = tmp[i];
             }
        } else if (*fmt == 'X') {
             unsigned long long u_num = is_long_long ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
             char tmp[17];
             int i = 0;
             if (u_num == 0) {
                tmp[i++] = '0';
             } else {
                 while (u_num > 0 && i < 16) {
                    tmp[i++] = "0123456789ABCDEF"[u_num % 16];
                    u_num /= 16;
                 }
             }
             while (i-- > 0 && buf < end) {
                 *buf++ = tmp[i];
             }
        } else if (*fmt == 'p') {
            uintptr_t ptr_val = (uintptr_t)va_arg(args, void *);
            char ptr_buf[17];
            char full_buf[19];

            u64toa_hex((uint64_t)ptr_val, ptr_buf);
            strcat(full_buf, ptr_buf);

            char *p = full_buf;
            while (*p && buf < end) {
                *buf++ = *p++;
            }
        } else if (*fmt == 's') {
            char *s = va_arg(args, char *);
            if (s == NULL) {
                s = "(null)";
            }
            while (*s && buf < end) {
                *buf++ = *s++;
            }
        } else if (*fmt == 'c') {
            *buf++ = (char)va_arg(args, int);
        }
         else {
            *buf++ = '%';
            if (buf < end) {
                *buf++ = *fmt;
            }
         }
        fmt++;
    }
    *buf = '\0';
    return buf - buf_start;
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
        framebuffer[i] = screencolor;
    }

    cursor_x = 0;
    cursor_y = 0;
}

void screen_color(int color) {
    if (framebuffer == NULL) {
        return;
    }

    size_t total_pixels = (pitch / (bpp / 8)) * screen_height;

    for (size_t i = 0; i < total_pixels; i++) {
        framebuffer[i] = color;
    }

    screencolor = color;

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

    if ((uintptr_t)fb < VMM_HIGHER_HALF) {
       fb = (uint32_t *)((uintptr_t)fb + VMM_HIGHER_HALF);
    }

    setup_framebuffer(fb, pitch, bpp, max_width, max_height);

    screen_color(0x182028);
}
