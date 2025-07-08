#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/spinlock.h>
#include <arch/x86_64/request.h>

/* Page size definition */
#define PAGE_SIZE 4096

/* Page Table Entry Flags */
#define PTE_PRESENT     (1ull << 0)
#define PTE_WRITABLE    (1ull << 1)
#define PTE_USER        (1ull << 2)
#define PTE_PWT         (1ull << 3)
#define PTE_PCD         (1ull << 4)
#define PTE_ACCESSED    (1ull << 5)
#define PTE_DIRTY       (1ull << 6)
#define PTE_PAT         (1ull << 7)
#define PTE_GLOBAL      (1ull << 8)
#define PTE_NX          (1ull << 63)

/* Page Table utility macros */
#define PTE_ADDR_MASK   0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE) & PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

typedef struct {
    uint64_t *top_level;
    spinlock_t lock;
} pagemap_t;

extern pagemap_t *kernel_pagemap;

#define VMM_HIGHER_HALF (hhdm_request.response->offset)

void init_vmm(void);

void vmm_switch_to(pagemap_t *pagemap);

bool vmm_map_page(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);

bool vmm_unmap_page(pagemap_t *pagemap, uintptr_t virt_addr);

uintptr_t vmm_virt_to_phys(pagemap_t *pagemap, uintptr_t virt_addr);
