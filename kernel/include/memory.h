#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Initialize the physical memory manager */
void init_pmm(void);

/* Allocate a single page (PAGE_SIZE bytes) of physical memory */
void *allocate_page(void);

/* Free a previously allocated page of physical memory */
void free_page(void *page);

/* Get total size of manageable physical memory in bytes */
size_t get_total_memory(void);

/* Get used physical memory size in bytes */
size_t get_used_memory(void);

/* Get free physical memory size in bytes */
size_t get_free_memory(void);

/* Check if a specific physical page is free (used internally by VMM) */
bool is_page_free(uintptr_t paddr);

/* Helper to align values up */
#ifndef ALIGN_UP
#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))
#endif
