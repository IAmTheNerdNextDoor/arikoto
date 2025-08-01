#include <misc/heap.h>
#include <arch/x86_64/memory.h>
#include <arch/x86_64/vmm.h>
#include <misc/print.h>
#include <arch/x86_64/spinlock.h>
#include <misc/kernel.h>
#include <misc/string.h>

typedef struct heap_free_block {
    size_t size;
    struct heap_free_block *next;
} heap_free_block_t;

#define MIN_ALLOC_SIZE sizeof(heap_free_block_t)
#define HEAP_ALIGNMENT 16

#define ALIGN_UP_HEAP(size) (((size) + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1))

static void *heap_start = NULL;
static size_t heap_size = 0;
static heap_free_block_t *free_list_head = NULL;
static spinlock_t heap_lock = SPINLOCK_INIT;

#define INITIAL_HEAP_PAGES 1024

void init_heap(void) {
    spinlock_acquire(&heap_lock);

    if (heap_start != NULL) {
        spinlock_release(&heap_lock);
        return;
    }

    void *heap_phys_base = allocate_page();
    if (heap_phys_base == NULL) {
        panic("Failed to allocate initial heap page");
    }

    heap_start = (void *)((uintptr_t)heap_phys_base + VMM_HIGHER_HALF);
    heap_size = PAGE_SIZE;

    for (size_t i = 1; i < INITIAL_HEAP_PAGES; ++i) {
        void *next_page_phys = allocate_page();
        if (next_page_phys == NULL) {
            printk(COLOR_YELLOW, "Warning: HEAP: Could only allocate %zu/%d pages\n", i, INITIAL_HEAP_PAGES);
            break;
        }
        heap_size += PAGE_SIZE;
    }

    free_list_head = (heap_free_block_t *)heap_start;
    free_list_head->size = heap_size;
    free_list_head->next = NULL;

    spinlock_release(&heap_lock);

    printk(COLOR_GREEN, "Heap installed\n");
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t total_size = ALIGN_UP_HEAP(size + sizeof(size_t));
    if (total_size < MIN_ALLOC_SIZE) {
        total_size = MIN_ALLOC_SIZE;
    }

    spinlock_acquire(&heap_lock);

    heap_free_block_t *prev = NULL;
    heap_free_block_t *curr = free_list_head;

    while (curr != NULL) {
        if (curr->size >= total_size) {
            if (curr->size - total_size >= MIN_ALLOC_SIZE) {
                heap_free_block_t *new_block = (heap_free_block_t *)((uintptr_t)curr + total_size);
                new_block->size = curr->size - total_size;
                new_block->next = curr->next;

                curr->size = total_size;

                if (prev == NULL) {
                    free_list_head = new_block;
                } else {
                    prev->next = new_block;
                }
            } else {
                if (prev == NULL) {
                    free_list_head = curr->next;
                } else {
                    prev->next = curr->next;
                }
            }

            size_t *size_ptr = (size_t *)curr;
            *size_ptr = curr->size;

            void *user_ptr = (void *)((uintptr_t)curr + sizeof(size_t));

            spinlock_release(&heap_lock);
            return user_ptr;
        }
        prev = curr;
        curr = curr->next;
    }

    spinlock_release(&heap_lock);
    printk(COLOR_RED, "kmalloc: Out of heap memory!\n");
    return NULL;
}

void kfree(void *ptr) {
    if (ptr == NULL) return;

    size_t *size_ptr = (size_t *)((uintptr_t)ptr - sizeof(size_t));
    void *block_start = (void *)size_ptr;
    size_t block_size = *size_ptr;

    if ((uintptr_t)block_start < (uintptr_t)heap_start ||
        (uintptr_t)block_start + block_size > (uintptr_t)heap_start + heap_size ||
        block_size < MIN_ALLOC_SIZE ||
        ((uintptr_t)block_start % HEAP_ALIGNMENT) != 0)
    {
        panic("Invalid pointer or heap corruption detected");
        return;
    }

    spinlock_acquire(&heap_lock);

    heap_free_block_t *prev = NULL;
    heap_free_block_t *curr = free_list_head;

    while (curr != NULL && (uintptr_t)curr < (uintptr_t)block_start) {
        prev = curr;
        curr = curr->next;
    }

    heap_free_block_t *freed_block = (heap_free_block_t *)block_start;
    freed_block->size = block_size;

    if (prev == NULL) {
        freed_block->next = free_list_head;
        free_list_head = freed_block;
    } else {
        freed_block->next = prev->next;
        prev->next = freed_block;
    }

    if (freed_block->next != NULL &&
        (uintptr_t)freed_block + freed_block->size == (uintptr_t)freed_block->next)
    {
        freed_block->size += freed_block->next->size;
        freed_block->next = freed_block->next->next;
    }

    if (prev != NULL &&
        (uintptr_t)prev + prev->size == (uintptr_t)freed_block)
    {
        prev->size += freed_block->size;
        prev->next = freed_block->next;
    }

    spinlock_release(&heap_lock);
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *krealloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return kmalloc(size);
    }
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    size_t old_size = *((size_t *)((uintptr_t)ptr - sizeof(size_t))) - sizeof(size_t);
    if (size <= old_size) {
        return ptr;
    }

    void *new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
        kfree(ptr);
    }
    return new_ptr;
}
