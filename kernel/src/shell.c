#include <shell.h>
#include <keyboard.h>
#include <print.h>
#include <memory.h>
#include <vfs.h>
#include <kernel.h>
#include <serial.h>
#include <pit.h>
#include <request.h>

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
static int cmd_divzero();
static int cmd_uptime();
static int cmd_raytrace();
static int cmd_vfstest();
static int cmd_changecolor(int argc, char **argv);

void shell_init() {
    shell_register_command("help", cmd_help);
    shell_register_command("echo", cmd_echo);
    shell_register_command("clear", cmd_clear);
    shell_register_command("ls", cmd_ls);
    shell_register_command("cat", cmd_cat);
    shell_register_command("mem", cmd_mem);
    shell_register_command("reboot", cmd_reboot);
    shell_register_command("dividebyzero", cmd_divzero);
    shell_register_command("uptime", cmd_uptime);
    shell_register_command("raytrace", cmd_raytrace);
    shell_register_command("vfstest", cmd_vfstest);
    shell_register_command("changecolor", cmd_changecolor);
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

static int cmd_changecolor(int argc, char **argv) {
    if (argc < 2) {
        printk(COLOR_WHITE, "Usage: changecolor <hexcolor>\n");
        return -1;
    }
    unsigned int color = parse_hex(argv[1]);
    screen_color(color);
    printk(COLOR_WHITE, "Set shell color to 0x%x\n", color);
    return 0;
}

static int cmd_ls(int argc, char **argv) {
    const char *path_to_list = "/";
    if (argc > 1) {
        path_to_list = argv[1];
    }

    char list_buffer[2048];
    memset(list_buffer, 0, sizeof(list_buffer));

    int result = vfs_list_files(path_to_list, list_buffer, sizeof(list_buffer));

    if (result == VFS_SUCCESS) {
        if (list_buffer[0] == '\0') {
             printk(COLOR_YELLOW, "Directory '%s' is empty.\n", path_to_list);
             return 0;
        }
        char *file = list_buffer;
        while (*file) {
            printk(COLOR_CYAN, "%s\n", file);

            file += strlen(file) + 1;

            if (file >= list_buffer + sizeof(list_buffer)) {
                 printk(COLOR_RED, "Error: list buffer overflow during processing.\n");
                 break;
            }
        }
    } else {
        printk(COLOR_RED, "Failed to list files in '%s' (Error code: %d)\n", path_to_list, result);
        return result;
    }

    return 0;
}

static int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        printk(COLOR_YELLOW, "Usage: cat <filename>\n");
        return -1;
    }

    char read_buffer[1025];
    int bytes_read;
    bytes_read = vfs_read(argv[1], read_buffer, sizeof(read_buffer) - 1, 0);

    if (bytes_read >= 0) {
        read_buffer[bytes_read] = '\0';
        printk(COLOR_WHITE, "%s", read_buffer);
        if (bytes_read == sizeof(read_buffer) - 1) {
             printk(COLOR_YELLOW, "\n[Warning: File might be larger than buffer]\n");
        } else {
             if (bytes_read > 0 && read_buffer[bytes_read - 1] != '\n') {
                 printk(COLOR_WHITE, "\n");
             }
        }
    } else {
        printk(COLOR_RED, "Failed to read file '%s' (Error code: %d)\n", argv[1], bytes_read);
        return bytes_read;
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

    /* Send reboot command to PS/2 controller */
    outb(0x64, 0xFE);

    /* Wait for reboot...*/
    for (volatile int i = 0; i < 10000000; i++) {
    }

    /* Okay, now this is bad */
    panic("PANIC: Arikoto failed to reboot, hold down the power button.\n");

    /*
    * This should not execute because we panicked, but
    * I'm including it because you know how compilers be.
    */
    return -1;
}

/* This command is supposed to test THE ALL NEW DIVIDE BY ZERO INTERRUPT HANDLER */
static int cmd_divzero() {
    int a = 1;
    int b = 0;
    int __attribute__((unused))result = a / b;

    return -1;
}

static int cmd_uptime() {
    uint64_t ticks = pit_get_ticks();
    uint64_t ms = pit_get_elapsed_ms();

    uint64_t seconds = ms / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;

    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    printk(COLOR_WHITE, "System uptime:\n");

    char buffer[128];

    if (days > 0) {
        build_string(buffer, sizeof(buffer),
            "", "%d", days,
            days == 1 ? " day, " : " days, ",
            NULL);
        printk(COLOR_WHITE, "%s", buffer);
    }

    build_string(buffer, sizeof(buffer), "", "%d", hours, ":", NULL);
    printk(COLOR_WHITE, "%s", buffer);

    if (minutes < 10) {
        build_string(buffer, sizeof(buffer), "0", "%d", minutes, ":", NULL);
    } else {
        build_string(buffer, sizeof(buffer), "", "%d", minutes, ":", NULL);
    }
    printk(COLOR_WHITE, "%s", buffer);

    if (seconds < 10) {
        build_string(buffer, sizeof(buffer), "0", "%d", seconds, NULL);
    } else {
        build_string(buffer, sizeof(buffer), "", "%d", seconds, NULL);
    }
    printk(COLOR_WHITE, "%s\n", buffer);

    build_string(buffer, sizeof(buffer), "Total milliseconds: ", "%d", ms, NULL);
    printk(COLOR_CYAN, "%s\n", buffer);

    build_string(buffer, sizeof(buffer), "Total PIT ticks: ", "%d", ticks, NULL);
    printk(COLOR_CYAN, "%s\n", buffer);

    return 0;
}

int cmd_minimal_uptime() {
    uint64_t ms = pit_get_elapsed_ms();

    uint64_t seconds = ms / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;

    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    printk(COLOR_WHITE, "System uptime:\n");

    char buffer[128];

    if (days > 0) {
        build_string(buffer, sizeof(buffer),
            "", "%d", days,
            days == 1 ? " day, " : " days, ",
            NULL);
        printk(COLOR_WHITE, "%s", buffer);
    }

    build_string(buffer, sizeof(buffer), "", "%d", hours, ":", NULL);
    printk(COLOR_WHITE, "%s", buffer);

    if (minutes < 10) {
        build_string(buffer, sizeof(buffer), "0", "%d", minutes, ":", NULL);
    } else {
        build_string(buffer, sizeof(buffer), "", "%d", minutes, ":", NULL);
    }
    printk(COLOR_WHITE, "%s", buffer);

    if (seconds < 10) {
        build_string(buffer, sizeof(buffer), "0", "%d", seconds, NULL);
    } else {
        build_string(buffer, sizeof(buffer), "", "%d", seconds, NULL);
    }
    printk(COLOR_WHITE, "%s\n", buffer);

    return 0;
}

unsigned char *frame_buffer;
int offset = 0;
int X = 1024;
int Y = 768;
int BPP = 32;
int A = 1;

int J = 0;
int K = -10;
int L = -7;
int M = 1296;
int N = 36;
int O = 255;
int P = 9;
int _ = 1 << 15;
int E, S, C, D, p, Z, W, Q, T, U, u, v, w, R, G, B;

void F(int b);
void I(int x, int Y, int X);
void H(int x);
void q(int c, int x, int y, int z, int k, int l, int m, int a, int b);
void o(int c, int x, int y, int z, int k, int l, int m, int a);
void n(int e, int f, int g, int h, int i, int j, int d, int a, int b, int V);
void t(int x, int y, int a, int b);
void r(int x, int y);

static int cmd_raytrace() {
    struct limine_framebuffer *framebuffer = framebuffer_info.response->framebuffers[0];

    X = framebuffer->width;
	Y = framebuffer->height;

	volatile uint32_t *fb_ptr = framebuffer->address;
	frame_buffer = (unsigned char *)fb_ptr;
	int x, y;

	offset = 0;

	printk(COLOR_YELLOW, "Starting raytracer (rendering %d x %d)...\n", X, Y);

	for (y=0; y<Y; y++)
	   for (x=0; x<X; x++)
			r(x, y);

    printk(COLOR_GREEN, "Raytracer finished.\n");
    return 0;
}

void F(int b) {
	E = "1111886:6:??AAFFHHMMOO55557799@@>>>BBBGGIIKK" [b] - 64;
	C = "C@=::C@@==@=:C@=:C@=:C531/513/5131/31/531/53" [b] - 64;
	S = b < 22 ? 9 : 0;
	D = 2;
}

void I(int x, int Y, int X) {
	Y ? (X ^= Y, X * X > x ? (X ^= Y) : 0, I(x, Y / 2, X)) : (E = X);
}

void H(int x) {
	I(x, _, 0);
}

void q(int c, int x, int y, int z, int k, int l, int m, int a, int b) {
	F(c);
	x -= E * M;
	y -= S * M;
	z -= C * M;
	b = x * x / M + y * y / M + z * z / M - D * D * M;
	a = -x * k / M - y * l / M - z * m / M;
	p = ((b = a * a / M - b) >= 0 ? (I(b * M, _, 0), b = E, a + (a > b ? -b : b)) : -1);
}

void o(int c, int x, int y, int z, int k, int l, int m, int a) {
	Z = !c ? -1 : Z;
	c < 44 ? (q(c, x, y, z, k, l, m, 0, 0), (p > 0 && c != a && (p < W || Z < 0)) ? (W = p, Z = c) : 0, o(c + 1, x, y, z, k, l, m, a)) : 0;
}

void n(int e, int f, int g, int h, int i, int j, int d, int a, int b, int V) {
	o(0, e, f, g, h, i, j, a);
	d > 0 && Z >= 0 ? (e += h * W / M, f += i * W / M, g += j * W / M, F(Z), u = e - E * M, v = f - S * M, w = g - C * M, b = (-2 * u - 2 * v + w) / 3, H(u * u + v * v + w * w), b /= D, b *= b, b *= 200, b /= (M * M), V = Z, E != 0 ? (u = -u * M / E, v = -v * M / E, w = -w * M / E) : 0, E = (h * u + i * v + j * w) / M, h -= u * E / (M / 2), i -= v * E / (M / 2), j -= w * E / (M / 2), n(e, f, g, h, i, j, d - 1, Z, 0, 0), Q /= 2, T /= 2, U /= 2, V = V < 22 ? 7 : (V < 30 ? 1 : (V < 38 ? 2 : (V < 44 ? 4 : (V == 44 ? 6 : 3)))), Q += V & 1 ? b : 0, T += V & 2 ? b : 0, U += V & 4 ? b : 0) : (d == P ? (g += 2, j = g > 0 ? g / 8 : g / 20) : 0, j > 0 ? (U = j * j / M, Q = 255 - 250 * U / M, T = 255 - 150 * U / M, U = 255 - 100 * U / M) : (U = j * j / M, U < M / 5 ? (Q = 255 - 210 * U / M, T = 255 - 435 * U / M, U = 255 - 720 * U / M) : (U -= M / 5, Q = 213 - 110 * U / M, T = 168 - 113 * U / M, U = 111 - 85 * U / M)), d != P ? (Q /= 2, T /= 2, U /= 2) : 0);
	Q = Q < 0 ? 0 : Q > O ? O : Q;
	T = T < 0 ? 0 : T > O ? O : T;
	U = U < 0 ? 0 : U > O ? O : U;
}

void t(int x, int y, int a, int b) {
	n(M * J + M * 40 * (A * x + a) / X / A - M * 20, M * K, M * L - M * 30 * (A * y + b) / Y / A + M * 15, 0, M, 0, P, -1, 0, 0);
	R += Q;
	G += T;
	B += U;
	++a < A ? t(x, y, a, b) : (++b < A ? t(x, y, 0, b) : 0);
}

void r(int x, int y) {
	R = G = B = 0;
	t(x, y, 0, 0);
	frame_buffer[offset++] = B / A / A;
	frame_buffer[offset++] = G / A / A;
	frame_buffer[offset++] = R / A / A;
	offset++;
}

static int cmd_vfstest() {
    screen_clear();
    vfs_test();

    return 0;
}
