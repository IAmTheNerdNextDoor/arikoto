#include <limine.h>
#include <request.h>
#include <kernel.h>

#define PAGE_SIZE 4096
#define BITMAP_SIZE (1024 * 1024)

static uint8_t memory_bitmap[BITMAP_SIZE];

static size_t total_pages = 0;

static size_t used_pages = 0;

/* Helper macros for bitmap manipulation */
#define BITMAP_INDEX(addr) ((addr) / PAGE_SIZE / 8)
#define BITMAP_OFFSET(addr) (((addr) / PAGE_SIZE) % 8)

static inline void set_bit(size_t index) {
    memory_bitmap[index / 8] |= (1 << (index % 8));
}

static inline void clear_bit(size_t index) {
    memory_bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline bool is_bit_set(size_t index) {
    return memory_bitmap[index / 8] & (1 << (index % 8));
}

/* Initialize the physical memory manager */
void init_pmm() {
    if (memorymap_info.response == NULL) {
        panic("PANIC: Could not acquire a response from the memorymap!");
    }

    size_t entry_count = memorymap_info.response->entry_count;
    struct limine_memmap_entry **entries = memorymap_info.response->entries;

    /* Zero out the bitmap */
    for (size_t i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0;
    }

    /* Mark all usable memory in the bitmap */
    for (size_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uintptr_t start = entry->base;
            uintptr_t end = start + entry->length;

            for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
                size_t index = BITMAP_INDEX(addr);
                if (index >= BITMAP_SIZE * 8) {
                    continue; /* Out of bitmap range */
                }

                clear_bit(index); /* Mark as free */
                total_pages++;
            }
        }
    }

    /* Mark kernel and module memory as used */
    for (size_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        if (entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES) {
            uintptr_t start = entry->base;
            uintptr_t end = start + entry->length;

            for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
                size_t index = BITMAP_INDEX(addr);
                if (index >= BITMAP_SIZE * 8) {
                    continue; /* Out of bitmap range */
                }

                set_bit(index); /* Mark as used */
                used_pages++;
            }
        }
    }
}

void *allocate_page() {
    for (size_t i = 0; i < total_pages; i++) {
        if (!is_bit_set(i)) {
            set_bit(i);
            used_pages++;
            /* Align to page boundary and validate address */
            void *addr = (void *)(i * PAGE_SIZE);
            if ((uintptr_t)addr % PAGE_SIZE != 0) {
                return NULL;
            }
            return addr;
        }
    }
    return NULL;
}

void free_page(void *page) {
    uintptr_t addr = (uintptr_t)page;
    size_t index = BITMAP_INDEX(addr);

    if (index >= BITMAP_SIZE * 8 || !is_bit_set(index)) {
        return; /* Invalid page or already free */
    }

    clear_bit(index);
    used_pages--;
}

size_t get_total_memory() {
    return total_pages * PAGE_SIZE;
}

size_t get_used_memory() {
    return used_pages * PAGE_SIZE;
}

size_t get_free_memory() {
    return get_total_memory() - get_used_memory();
}
