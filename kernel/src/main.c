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
#include <vmm.h>

/* Kernel main function */
void kmain(void) {
    /* Start PIT */
    pit_init(100);

    /* Start Framebuffer */
    init_framebuffer();

    /* Print Version and Logo and Tagline */
    printk(COLOR_RED, "[Arikoto 0.0.2]\n");
    print_logo_and_tagline();

    /* Start PIC */
    pic_remap(PIC1_VECTOR_OFFSET, PIC2_VECTOR_OFFSET);

    /* Start GDT */
    init_gdt();

    /* Start IDT */
    init_idt();

    /* Start PMM */
    init_pmm();

    /* Start VMM */
    init_vmm();

    /* Start Keyboard */
    init_keyboard();

    /* Start shell */
    shell_init();
    shell_run();

    /* Halt system for now */
    hcf();
}
