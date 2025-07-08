#pragma once

#define SHELL_BUFFER_SIZE 1024

void shell_init(void);

void shell_run(void);

void shell_task();

int shell_execute(const char *command);

void shell_register_command(const char *name, int (*func)(int argc, char **argv));

char *shell_readline(const char *prompt);

int cmd_minimal_uptime();
