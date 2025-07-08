#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <misc/string.h>

/* Halt and catch fire function (obvious) */
static inline __attribute__((unused)) void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void display_info(void);
void vfs_test(void);

/* Helper function to convert integer to string (itoa) */
static inline char* itoa(int num, char *str, int base) {
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitly, otherwise empty string is printed */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    /* Handle negative numbers only if base is 10 */
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    /* Process individual digits */
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    /* Append negative sign for negative numbers */
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    /* Reverse the string */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    return str;
}

static __attribute__((unused)) size_t build_string(char *buffer, size_t size, ...) {
    va_list args;
    va_start(args, size);

    size_t pos = 0;
    char num_buf[32];

    while (pos < size - 1) {
        char *str = va_arg(args, char*);
        if (str == NULL) break;

        if (strcmp(str, "%d") == 0) {
            int num = va_arg(args, int);
            itoa(num, num_buf, 10);
            size_t num_len = strlen(num_buf);
            if (pos + num_len < size) {
                strcpy(buffer + pos, num_buf);
                pos += num_len;
            } else {
                break;
            }
        } else if (strcmp(str, "%d.%02d") == 0) {
            int num1 = va_arg(args, int);
            int num2 = va_arg(args, int);

            itoa(num1, num_buf, 10);
            size_t num_len = strlen(num_buf);
            if (pos + num_len < size) {
                strcpy(buffer + pos, num_buf);
                pos += num_len;
            }
            if (pos + 1 < size) {
                buffer[pos++] = '.';
            }
            num_buf[0] = (num2 / 10) + '0';
            num_buf[1] = (num2 % 10) + '0';
            num_buf[2] = '\0';
            if (pos + 2 < size) {
                strcpy(buffer + pos, num_buf);
                pos += 2;
            }
        } else if (strcmp(str, "%s") == 0) {
            char *insert_str = va_arg(args, char*);
            if (insert_str == NULL) continue;
            size_t str_len = strlen(insert_str);
            if (pos + str_len < size) {
                strcpy(buffer + pos, insert_str);
                pos += str_len;
            } else {
                break;
            }
        } else {
            size_t len = strlen(str);
            if (pos + len < size) {
                strcpy(buffer + pos, str);
                pos += len;
            } else {
                break;
            }
        }
    }
    buffer[pos] = '\0';
    va_end(args);
    return pos;
}

static __attribute__((unused)) unsigned int parse_hex(const char *s) {
    unsigned int result = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9')
            result = (result << 4) | (c - '0');
        else if (c >= 'a' && c <= 'f')
            result = (result << 4) | (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            result = (result << 4) | (c - 'A' + 10);
        else
            break;
    }
    return result;
}

void panic(const char *fmt, ...);
void print_logo_and_tagline();
