#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <request.h>
#include <kernel.h>
#include <print.h>
#include <memory.h>
#include <vfs.h>
#include <scheduler.h>

// Kernel main function.
void kmain(void) {
    // Ensure we have a framebuffer.
    if (framebuffer_info.response == NULL || framebuffer_info.response->framebuffer_count < 1) {
        hcf();
    }

    // Extract framebuffer information
    struct limine_framebuffer *framebuffer = framebuffer_info.response->framebuffers[0];
    uint32_t *fb = framebuffer->address;
    size_t pitch = framebuffer->pitch;
    size_t bpp = framebuffer->bpp;
    size_t max_width = framebuffer->width;
    size_t max_height = framebuffer->height;

    // Start framebuffer
    init_framebuffer(fb, pitch, bpp, max_width, max_height);

    puts("[Arikoto 0.0.2]", COLOR_RED);

    // Start PMM
    init_pmm();

    // Start scheduler
    scheduler_t scheduler;
    scheduler_init(&scheduler, 2); // Set the time quantum to 2 (arbitrary units)

    scheduler_add_process(&scheduler, 1, display_info);
    scheduler_add_process(&scheduler, 2, vfs_test);

    scheduler_run(&scheduler);

    // We're done, just hang...
    hcf();
}
