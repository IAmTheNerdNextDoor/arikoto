#include <misc/kernel.h>
#include <misc/print.h>
#include <misc/shell.h>

void panic(const char *fmt, ...) {
    asm volatile ("cli");
    screen_clear();
    // printk(COLOR_RED,
    //     "+======================================================================================================+\n"
    //     "| ##    ## ######## ########  ##    ## ######## ##          ########     ###    ##    ## ####  ######  |\n"
    //     "| ##   ##  ##       ##     ## ###   ## ##       ##          ##     ##   ## ##   ###   ##  ##  ##    ## |\n"
    //     "| ##  ##   ##       ##     ## ####  ## ##       ##          ##     ##  ##   ##  ####  ##  ##  ##       |\n"
    //     "| #####    ######   ########  ## ## ## ######   ##          ########  ##     ## ## ## ##  ##  ##       |\n"
    //     "| ##  ##   ##       ##   ##   ##  #### ##       ##          ##        ######### ##  ####  ##  ##       |\n"
    //     "| ##   ##  ##       ##    ##  ##   ### ##       ##          ##        ##     ## ##   ###  ##  ##    ## |\n"
    //     "| ##    ## ######## ##     ## ##    ## ######## ########    ##        ##     ## ##    ## ####  ######  |\n"
    //     "+======================================================================================================+\n\n"); /* Will also reintroduced this at some point */


    printk(COLOR_WHITE, "Arikoto has encountered a fatal exception.\n\n");
    char panic_buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(panic_buffer, sizeof(panic_buffer), fmt, args);
    va_end(args);
    printk(COLOR_WHITE, "%s\n", panic_buffer);

    cmd_minimal_uptime();

    printk(COLOR_WHITE, "\nSystem halted.");

    hcf();
}
