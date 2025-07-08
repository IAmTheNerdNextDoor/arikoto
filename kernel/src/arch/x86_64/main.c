#include <misc/kernel.h>
#include <misc/print.h>
#include <arch/x86_64/memory.h>
#include <vfs/vfs.h>
#include <misc/keyboard.h>
#include <misc/shell.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/pit.h>
#include <arch/x86_64/vmm.h>
#include <misc/heap.h>
#include <misc/multitask.h>
#include <misc/serial.h>

/* Kernel main function */
void kmain(void) {
    /* Start PIT */
    pit_init(10000);

    /* Start Serial */
    init_serial();

    /* Start Framebuffer */
    init_framebuffer();

    /* Print Version and Logo and Tagline */
    printk(COLOR_RED, "[Arikoto 0.0.3]\n");
    print_logo_and_tagline();

    /* Start PIC */
    pic_remap(PIC1_VECTOR_OFFSET, PIC2_VECTOR_OFFSET);

    /* Start GDT and TSS */
    init_gdt_tss();

    /* Start IDT */
    init_idt();

    /* Start PMM */
    init_pmm();

    /* Start VMM */
    init_vmm();

    /* Start Heap */
    init_heap();

    /* Start Keyboard */
    init_keyboard();

    /* Start Initramfs */
    if (module_request.response && module_request.response->module_count > 0) {
        struct limine_file *mod = module_request.response->modules[0];
        printk(COLOR_YELLOW, "Mounting initramfs module: %s (size: %llu)\n", mod->path, mod->size);
        int res = vfs_mount_initramfs("/", mod->address, mod->size);
        if (res == !VFS_SUCCESS) {
            printk(COLOR_RED, "Failed to mount initramfs: %d\n", res);
        }
    } else {
        printk(COLOR_YELLOW, "No initramfs module found.\n");
    }

    /* Start Multitasking */
    init_multitasking();

    task_create(shell_task, NULL, "shell", 1);

    /* Halt system for now */
    hcf();
}
