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

/* HHDM request */
__attribute__((used, section(".limine_requests")))
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

/* Kernel Address request */
__attribute__((used, section(".limine_requests")))
volatile struct limine_executable_address_request kernel_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0
};

/* Module request */
__attribute__((used, section(".limine_requests")))
volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};
