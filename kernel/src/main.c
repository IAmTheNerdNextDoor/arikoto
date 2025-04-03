#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <kernel.h>
#include <print.h>
#include <memory.h>
#include <vfs.h>
#include <keyboard.h>
#include <shell.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <pit.h>

/* Kernel main function */
void kmain(void) {
    /* Start PIT */
    pit_init(100);

    /* Start PIC */
    pic_remap(PIC1_VECTOR_OFFSET, PIC2_VECTOR_OFFSET);

    /* Start GDT */
    init_gdt();

    /* Start IDT */
    init_idt();

    /* Start PMM */
    init_pmm();

    /* Start Framebuffer */
    init_framebuffer();

    /* Print Version and Logo and Tagline */
    printk(COLOR_RED, "[Arikoto 0.0.2]\n");
    print_logo_and_tagline();

    /* Start Keyboard */
    init_keyboard();

    /* Mount VFS and create file via vfs_test */
    vfs_test();

    /* Start shell */
    shell_init();
    shell_run();

    /* Halt system for now */
    hcf();
}
