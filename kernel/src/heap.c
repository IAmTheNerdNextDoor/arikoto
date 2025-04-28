#include <heap.h>
#include <memory.h>
#include <vmm.h>
#include <print.h>
#include <spinlock.h>
#include <kernel.h>

typedef struct heap_free_block {
    size_t size;
    struct heap_free_block *next;
} heap_free_block_t;

#define MIN_ALLOC_SIZE sizeof(heap_free_block_t)
#define HEAP_ALIGNMENT 16

#define ALIGN_UP_HEAP(size) ((size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1))

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
        panic("PANIC: HEAP: Failed to allocate initial heap page\n");
    }

    heap_start = (void *)((uintptr_t)heap_phys_base + VMM_HIGHER_HALF);
    heap_size = PAGE_SIZE;

    for (size_t i = 1; i < INITIAL_HEAP_PAGES; ++i) {
        void *next_page_phys = allocate_page();
        if (next_page_phys == NULL) {
             printk(COLOR_YELLOW, "Warning: HEAP: Could not allocate all %u initial heap pages. Only got %u.\n",
                    INITIAL_HEAP_PAGES, i);
             break;
        }
        heap_size += PAGE_SIZE;
    }

    free_list_head = (heap_free_block_t *)heap_start;
    free_list_head->size = heap_size;
    free_list_head->next = NULL;

    spinlock_release(&heap_lock);
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

    heap_free_block_t *current = free_list_head;
    heap_free_block_t *previous = NULL;

    while (current != NULL) {
        if (current->size >= total_size) {

            if (current->size - total_size > MIN_ALLOC_SIZE) {
                heap_free_block_t *new_free_block = (heap_free_block_t *)((uintptr_t)current + total_size);
                new_free_block->size = current->size - total_size;
                new_free_block->next = current->next;

                current->size = total_size;

                if (previous == NULL) {
                    free_list_head = new_free_block;
                } else {
                    previous->next = new_free_block;
                }
            } else {
                if (previous == NULL) {
                    free_list_head = current->next;
                } else {
                    previous->next = current->next;
                }
            }

            size_t *allocated_size_ptr = (size_t *)current;
            *allocated_size_ptr = current->size;

            void *allocated_ptr = (void *)((uintptr_t)current + sizeof(size_t));

            spinlock_release(&heap_lock);
            return allocated_ptr;
        }

        previous = current;
        current = current->next;
    }

    spinlock_release(&heap_lock);
    printk(COLOR_RED, "kmalloc: Out of heap memory!\n");
    return NULL;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    size_t *allocated_size_ptr = (size_t *)((uintptr_t)ptr - sizeof(size_t));
    void *block_start = (void *)allocated_size_ptr;
    size_t block_size = *allocated_size_ptr;

    if ((uintptr_t)block_start < (uintptr_t)heap_start ||
        (uintptr_t)block_start + block_size > (uintptr_t)heap_start + heap_size ||
        block_size < MIN_ALLOC_SIZE || (uintptr_t)block_start % HEAP_ALIGNMENT != 0)
    {
        panic("PANIC: KFREE: Invalid pointer or heap corruption detected at %p (block size %u)\n", ptr, block_size);
        return;
    }

    spinlock_acquire(&heap_lock);

    heap_free_block_t *freed_block = (heap_free_block_t *)block_start;
    freed_block->size = block_size;

    heap_free_block_t *current = free_list_head;
    heap_free_block_t *previous = NULL;

    while (current != NULL && (uintptr_t)current < (uintptr_t)freed_block) {
        previous = current;
        current = current->next;
    }

    if (previous == NULL) {
        freed_block->next = free_list_head;
        free_list_head = freed_block;
    } else {
        freed_block->next = previous->next;
        previous->next = freed_block;
    }

    if (freed_block->next != NULL && (uintptr_t)freed_block + freed_block->size == (uintptr_t)freed_block->next) {
        freed_block->size += freed_block->next->size;
        freed_block->next = freed_block->next->next;
    }

    if (previous != NULL && (uintptr_t)previous + previous->size == (uintptr_t)freed_block) {
        previous->size += freed_block->size;
        previous->next = freed_block->next;
    }

    spinlock_release(&heap_lock);
}


void *kcalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
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
