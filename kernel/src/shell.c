#include <shell.h>
#include <keyboard.h>
#include <print.h>
#include <string.h>
#include <memory.h>
#include <vfs.h>
#include <kernel.h>
#include <serial.h>

#define MAX_COMMANDS 32
#define MAX_ARGS 16

static char line_buffer[SHELL_BUFFER_SIZE];

static const char *PROMPT = "arikoto> ";

char *shell_readline(const char *prompt) {
    while (*prompt) {
        putchar(*prompt++, COLOR_WHITE);
    }

    memset(line_buffer, 0, SHELL_BUFFER_SIZE);
    int pos = 0;

    while (1) {
        char c = keyboard_read();

        if (c == '\n') {
            line_buffer[pos] = '\0';
            putchar('\n', COLOR_WHITE);
            return line_buffer;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                putchar('\b', COLOR_WHITE);
            }
        } else if (c >= ' ' && c <= '~') {
            if (pos < SHELL_BUFFER_SIZE - 1) {
                line_buffer[pos++] = c;
                putchar(c, COLOR_WHITE);
            }
        }
    }
}

typedef struct {
    char *name;
    int (*func)(int argc, char **argv);
} command_t;

static command_t commands[MAX_COMMANDS];
static int command_count = 0;

static int cmd_help();
static int cmd_echo(int argc, char **argv);
static int cmd_clear();
static int cmd_ls();
static int cmd_cat(int argc, char **argv);
static int cmd_mem();
static int cmd_reboot();

void shell_init() {
    shell_register_command("help", cmd_help);
    shell_register_command("echo", cmd_echo);
    shell_register_command("clear", cmd_clear);
    shell_register_command("ls", cmd_ls);
    shell_register_command("cat", cmd_cat);
    shell_register_command("mem", cmd_mem);
    shell_register_command("reboot", cmd_reboot);
}

static int parse_args(char *input, char **argv) {
    int argc = 0;
    char *token;
    char *rest = input;

    while ((token = strtok_r(rest, " \t", &rest)) != NULL && argc < MAX_ARGS) {
        argv[argc++] = token;
    }

    argv[argc] = NULL;
    return argc;
}

void shell_run() {
    printk(COLOR_WHITE, "Arikoto Shell started.\n");

    while (1) {
        char *input = shell_readline(PROMPT);

        if (strcmp(input, "exit") == 0) {
            printk(COLOR_YELLOW, "Shell exiting...\n");
            return;
        }

        if (strlen(input) > 0) {
            shell_execute(input);
        }
    }
}

int shell_execute(const char *command) {
    char cmd_buffer[SHELL_BUFFER_SIZE];
    strcpy(cmd_buffer, command);

    char *argv[MAX_ARGS];
    int argc = parse_args(cmd_buffer, argv);

    if (argc == 0) {
        return 0;
    }

    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            return commands[i].func(argc, argv);
        }
    }

    printk(COLOR_RED, "Unknown command: %s\n", argv[0]);
    return -1;
}

void shell_register_command(const char *name, int (*func)(int argc, char **argv)) {
    if (command_count < MAX_COMMANDS) {
        commands[command_count].name = (char *)name;
        commands[command_count].func = func;
        command_count++;
    }
}

static int cmd_help() {
    printk(COLOR_WHITE, "Available commands:\n");

    for (int i = 0; i < command_count; i++) {
        printk(COLOR_GREEN, "%s\n", commands[i].name);
    }

    return 0;
}

static int cmd_echo(int argc, char **argv) {
    char buffer[256] = {0};

    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            strcat(buffer, " ");
        }
        strcat(buffer, argv[i]);
    }

    printk(COLOR_WHITE, "%s\n", buffer);
    return 0;
}

static int cmd_clear() {
    screen_clear();
    return 0;
}

static int cmd_ls() {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int result = vfs_list_files(buffer, sizeof(buffer));

    if (result >= 0) {
        char *file = buffer;
        while (*file) {
            printk(COLOR_CYAN, file);

            file += strlen(file) + 1;

            if (file >= buffer + sizeof(buffer)) {
                break;
            }
        }
    } else {
        printk(COLOR_RED, "Failed to list files\n");
    }

    return 0;
}

static int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        printk(COLOR_YELLOW, "Usage: cat <filename>\n");
        return -1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    if (vfs_read(argv[1], buffer, sizeof(buffer)) >= 0) {
        printk(COLOR_WHITE, "%s\n", buffer);
    } else {
        printk(COLOR_RED, "Failed to read file: %s", argv[1]);
        return -1;
    }

    return 0;
}

static int cmd_mem() {
    char totalbuffer[64];
    char usedbuffer[64];
    char freebuffer[64];

    printk(COLOR_WHITE, "Memory Information:\n");

    size_t total_kb = get_total_memory() / 1024;
    size_t used_kb = get_used_memory() / 1024;
    size_t free_kb = get_free_memory() / 1024;

    size_t total_gb = total_kb / (1024 * 1024);
    size_t used_gb = used_kb / (1024 * 1024);
    size_t free_gb = free_kb / (1024 * 1024);

    size_t total_mb_remainder = (total_kb % (1024 * 1024)) * 100 / (1024 * 1024);
    size_t used_mb_remainder = (used_kb % (1024 * 1024)) * 100 / (1024 * 1024);
    size_t free_mb_remainder = (free_kb % (1024 * 1024)) * 100 / (1024 * 1024);

    /* Here's when I finally mastered my own build string function */
    build_string(totalbuffer, sizeof(totalbuffer), "Total: ", "%d.%02d", total_gb, total_mb_remainder, "GB");
    build_string(usedbuffer, sizeof(usedbuffer), "Used: ", "%d.%02d", used_gb, used_mb_remainder, "GB");
    build_string(freebuffer, sizeof(freebuffer), "Free: ", "%d.%02d", free_gb, free_mb_remainder, "GB");

    printk(COLOR_WHITE, "%s\n", totalbuffer);
    printk(COLOR_WHITE, "%s\n", usedbuffer);
    printk(COLOR_WHITE, "%s\n", freebuffer);

    return 0;
}

static int cmd_reboot() {
    printk(COLOR_RED, "Rebooting system...\n");

    /* We don't have a method of rebooting because we don't have acpi yet,
    * this is a temporary solution. We jump to the reset vector, which should
    * cause a triple fault. We'll have to remove this when we have interrupts,
    * since it causes an exception.
    */
    asm volatile("jmp 0xfffffff0");

    /* Should never reach here */
    panic("Arikoto failed to reboot!\n");
    
    /*
    * This should not execute because we panicked, but
    * I'm including it because you know how compilers be.
    */
    return -1;
}
