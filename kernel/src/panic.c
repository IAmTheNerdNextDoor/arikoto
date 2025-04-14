#include <kernel.h>
#include <print.h>
#include <shell.h>

static void panic_vfprintf(const char *fmt, va_list args) {
    char buffer[512];
    size_t pos = 0;

    while (*fmt && pos < sizeof(buffer) - 1) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int num = va_arg(args, int);
                    char num_buf[32];
                    itoa(num, num_buf, 10);
                    size_t num_len = strlen(num_buf);
                    if (pos + num_len < sizeof(buffer)) {
                        strcpy(buffer + pos, num_buf);
                        pos += num_len;
                    } else {
                         goto end_loop;
                    }
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char num_buf[32];
                    itoa(num, num_buf, 16);
                    size_t num_len = strlen(num_buf);
                    if (pos + num_len < sizeof(buffer)) {
                        strcpy(buffer + pos, num_buf);
                        pos += num_len;
                    } else {
                        goto end_loop;
                    }
                    break;
                }
                case 'p': {
                    uintptr_t ptr_val = (uintptr_t)va_arg(args, void *);
                    char ptr_buf[19];
                    u64toa_hex((uint64_t)ptr_val, ptr_buf);
                    size_t ptr_len = strlen(ptr_buf);
                    if (pos + ptr_len < sizeof(buffer)) {
                        strcpy(buffer + pos, ptr_buf);
                        pos += ptr_len;
                    } else {
                        goto end_loop;
                    }
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char *);
                    if (str == NULL) str = "(null)";
                    size_t str_len = strlen(str);
                    if (pos + str_len < sizeof(buffer)) {
                       strcpy(buffer + pos, str);
                       pos += str_len;
                    } else {
                         goto end_loop;
                    }
                    break;
                }
                case 'c': {
                    if (pos + 1 < sizeof(buffer)) {
                        buffer[pos++] = (char)va_arg(args, int);
                    } else {
                         goto end_loop;
                    }
                    break;
                }
                case '%': {
                    if (pos + 1 < sizeof(buffer)) {
                         buffer[pos++] = '%';
                    } else {
                         goto end_loop;
                    }
                    break;
                }
                default:
                    if (pos + 1 < sizeof(buffer)) {
                         buffer[pos++] = '%';
                    } else {
                         goto end_loop;
                    }
                    if (*fmt && pos + 1 < sizeof(buffer)) {
                        buffer[pos++] = *fmt;
                    } else if (*fmt) {
                        goto end_loop;
                    }
                    break;
            }
        } else {
            if (pos + 1 < sizeof(buffer)) {
                buffer[pos++] = *fmt;
            } else {
                 goto end_loop;
            }
        }
        if (*fmt) {
             fmt++;
        } else {
             break;
        }
    }

end_loop:
    buffer[pos] = '\0';

    printk(COLOR_WHITE, "%s\n", buffer);
}

void panic(const char *fmt, ...) {
    asm volatile ("cli");
    screen_clear();
    printk(COLOR_RED, "****************************************\n");
    printk(COLOR_RED, "              KERNEL PANIC\n");
    printk(COLOR_RED, "****************************************\n");


    printk(COLOR_WHITE, "Arikoto has panicked. If this occurs frequently and you didn't cause this intentionally, please submit it on GitHub.\n\n");
    va_list args;
    va_start(args, fmt);
    panic_vfprintf(fmt, args);
    va_end(args);

    cmd_minimal_uptime();

    printk(COLOR_WHITE, "\nSystem halted.");

    hcf();
}
