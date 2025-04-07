#include <kernel.h>
#include <print.h>
#include <shell.h>

static void panic_vfprintf(const char *fmt, va_list args) {
    char buffer[256];
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
                    }
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char *);
                    size_t str_len = strlen(str);
                    if(pos + str_len < sizeof(buffer)){
                       strcpy(buffer + pos, str);
                       pos += str_len;
                    }

                    break;
                }
                case 'c': {
                  if(pos + 1 < sizeof(buffer)) {
                      buffer[pos++] = (char)va_arg(args, int);
                  }
                    break;
                }
                case '%': {
                    if (pos + 1 < sizeof(buffer)) {
                         buffer[pos++] = '%';
                    }

                    break;
                }
                default:
                    break;
            }
        } else {
            buffer[pos++] = *fmt;
        }
        fmt++;
    }

      if (pos < sizeof(buffer)) {
        buffer[pos] = '\0';
    }

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
