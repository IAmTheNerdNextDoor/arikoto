#pragma once

#include <limine.h>

/* Limine requests */
extern volatile struct limine_framebuffer_request framebuffer_info;
extern volatile struct limine_memmap_request memorymap_info;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_executable_address_request kernel_address_request;
extern volatile struct limine_module_request module_request;
