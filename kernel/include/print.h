#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* Predefined color constants (RGB with 8-bit per channel) */
#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_CYAN    0x00FFFF
#define COLOR_MAGENTA 0xFF00FF

extern uint8_t _binary_matrix_psf_start;
extern uint8_t _binary_matrix_psf_size;

static __attribute__((unused)) size_t cursor_x;
static __attribute__((unused)) size_t cursor_y;

char* u64toa_hex(uint64_t num, char *str);
void init_framebuffer();
void putchar(char c, uint32_t color);
void screen_clear(void);
void screen_color(int color);
int vsnprintf(char *buffer, size_t size, const char *fmt, va_list args);
void printk(uint32_t color, const char *fmt, ...);
