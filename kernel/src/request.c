#include <request.h>

/* Framebuffer request */
__attribute__((used, section(".limine_requests")))
volatile struct limine_framebuffer_request framebuffer_info = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

/* Memory map request */
__attribute__((used, section(".limine_requests")))
volatile struct limine_memmap_request memorymap_info = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};
