#include <kernel.h>
#include <print.h>
#include <shell.h>

void panic(const char *fmt, ...) {
    asm volatile ("cli");
    screen_clear();
    printk(COLOR_RED, "****************************************\n");
    printk(COLOR_RED, "              KERNEL PANIC\n");
    printk(COLOR_RED, "****************************************\n");


    printk(COLOR_WHITE, "Arikoto has panicked. If this occurs frequently and you didn't cause this intentionally, please submit it on GitHub.\n\n");
    char panic_buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(panic_buffer, sizeof(panic_buffer), fmt, args);
    va_end(args);
    printk(COLOR_WHITE, "%s\n", panic_buffer);

    cmd_minimal_uptime();

    printk(COLOR_WHITE, "\nSystem halted.");

    hcf();
}
