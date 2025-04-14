#include <limine.h>
#include <request.h>
#include <kernel.h>
#include <print.h>
#include <string.h>
#include <memory.h>
#include <vmm.h>

extern uint8_t _text_start[], _text_end[];
extern uint8_t _rodata_start[], _rodata_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

#define PMM_BITMAP_SIZE (1024 * 1024)
static uint8_t memory_bitmap[PMM_BITMAP_SIZE];

static size_t highest_page = 0;
static size_t used_pages = 0;
static size_t free_pages = 0;
static size_t total_ram_pages = 0;

#define BITMAP_GET(index) (memory_bitmap[(index) / 8] & (1 << ((index) % 8)))
#define BITMAP_SET(index) (memory_bitmap[(index) / 8] |= (1 << ((index) % 8)))
#define BITMAP_CLEAR(index) (memory_bitmap[(index) / 8] &= ~(1 << ((index) % 8)))

/* Get the total size of manageable physical memory */
size_t get_total_memory() {
    return total_ram_pages * PAGE_SIZE;
}

/* Get the size of used physical memory */
size_t get_used_memory() {
    return used_pages * PAGE_SIZE;
}

/* Get the size of free physical memory */
size_t get_free_memory() {
    return free_pages * PAGE_SIZE;
}

/* Initialize the physical memory manager */
void init_pmm() {
    if (memorymap_info.response == NULL) {
        panic("PANIC: Could not acquire memory map response\n");
    }
     if (kernel_address_request.response == NULL) {
        panic("PANIC: Could not acquire kernel address response\n");
    }

    size_t entry_count = memorymap_info.response->entry_count;
    struct limine_memmap_entry **entries = memorymap_info.response->entries;

    uintptr_t highest_addr = 0;
    for (size_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];
        uintptr_t top = entry->base + entry->length;
        if (top > highest_addr) {
            highest_addr = top;
        }
    }
    highest_page = highest_addr / PAGE_SIZE;

    size_t max_bitmap_pages = PMM_BITMAP_SIZE * 8;
    if (highest_page >= max_bitmap_pages) {
        printk(COLOR_YELLOW, "Warning: PMM bitmap capacity exceeded. High memory (>%d GiB) may be unavailable.\n",
               (max_bitmap_pages * PAGE_SIZE) / (1024*1024*1024));
        highest_page = max_bitmap_pages - 1;
    }

    memset(memory_bitmap, 0xFF, PMM_BITMAP_SIZE);

    total_ram_pages = 0;
    for (size_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            uintptr_t base = entry->base;
            uintptr_t top = base + entry->length;

            size_t first_page = (base + PAGE_SIZE - 1) / PAGE_SIZE;
            size_t last_page = top / PAGE_SIZE;

            for (size_t page_idx = first_page; page_idx < last_page; ++page_idx) {
                if (page_idx <= highest_page) {
                    if (BITMAP_GET(page_idx)) {
                        BITMAP_CLEAR(page_idx);
                        total_ram_pages++;
                    }
                }
            }
        }
    }
    free_pages = total_ram_pages;

    struct limine_executable_address_response *kaddr = kernel_address_request.response;
    uintptr_t k_phys_start = kaddr->physical_base;
    uintptr_t k_virt_start = (uintptr_t)_text_start;
    uintptr_t k_virt_end = (uintptr_t)_bss_end;
    uintptr_t k_size = k_virt_end - k_virt_start;
    uintptr_t k_phys_end = k_phys_start + k_size;

    size_t k_first_page = k_phys_start / PAGE_SIZE;
    size_t k_last_page = (k_phys_end + PAGE_SIZE - 1) / PAGE_SIZE;

    for (size_t page_idx = k_first_page; page_idx < k_last_page; ++page_idx) {
        if (page_idx <= highest_page) {
            if (!BITMAP_GET(page_idx)) {
                free_pages--;
            }
            BITMAP_SET(page_idx);
        }
    }

    size_t pages_below_1mb = 0x100000 / PAGE_SIZE;
    for (size_t page_idx = 0; page_idx < pages_below_1mb; ++page_idx) {
         if (page_idx <= highest_page) {
             if (!BITMAP_GET(page_idx)) {
                 free_pages--;
             }
             BITMAP_SET(page_idx);
         }
    }

    used_pages = (total_ram_pages > free_pages) ? (total_ram_pages - free_pages) : 0;
}

void *allocate_page() {
    for (size_t i = 0; i <= highest_page; ++i) {
        if (!BITMAP_GET(i)) {
            BITMAP_SET(i);
            used_pages++;
            free_pages--;
            void *addr = (void *)(i * PAGE_SIZE);
            return addr;
        }
    }
    printk(COLOR_RED, "PMM: Out of memory!\n");
    return NULL;
}

void free_page(void *page) {
    uintptr_t addr = (uintptr_t)page;
    if (addr % PAGE_SIZE != 0) {
        printk(COLOR_YELLOW, "Warning: PMM free_page called with non-page-aligned address 0x%x\n", addr);
        return;
    }

    size_t index = addr / PAGE_SIZE;

    if (index > highest_page) {
         printk(COLOR_YELLOW, "Warning: PMM free_page called with address 0x%x outside managed range\n", addr);
        return;
    }

    if (!BITMAP_GET(index)) {
         printk(COLOR_YELLOW, "Warning: PMM free_page called on already free page 0x%x\n", addr);
        return;
    }
    BITMAP_CLEAR(index);
    used_pages--;
    free_pages++;
}
