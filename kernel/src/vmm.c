#include <vmm.h>
#include <memory.h>
#include <request.h>
#include <kernel.h>
#include <string.h>
#include <print.h>

#ifndef ALIGN_UP
#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))
#endif

extern uint8_t _text_start[], _text_end[];
extern uint8_t _rodata_start[], _rodata_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

pagemap_t *kernel_pagemap = NULL;

static uint64_t *vmm_get_next_level(uint64_t *current_level, size_t index, bool allocate) {
    uint64_t entry = current_level[index];

    if (entry & PTE_PRESENT) {
        return (uint64_t *)(PTE_GET_ADDR(entry) + VMM_HIGHER_HALF);
    }

    if (!allocate) {
        return NULL;
    }

    void *next_level_phys = allocate_page();
    if (next_level_phys == NULL) {
        panic("PANIC: Failed to allocate page for new page table level\n");
        return NULL;
    }

    uint64_t *next_level_virt = (uint64_t *)((uintptr_t)next_level_phys + VMM_HIGHER_HALF);

    memset(next_level_virt, 0, PAGE_SIZE);

    current_level[index] = (uint64_t)(uintptr_t)next_level_phys | PTE_PRESENT | PTE_WRITABLE;

    return next_level_virt;
}

bool vmm_map_page(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, true);
    if (pdpt == NULL) { return false; }
    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, true);
    if (pd == NULL) { return false; }
    uint64_t *pt = vmm_get_next_level(pd, pd_index, true);
    if (pt == NULL) { return false; }

    pt[pt_index] = phys_addr | flags;

    asm volatile ("invlpg (%0)" :: "r"(virt_addr) : "memory");

    return true;
}

bool vmm_unmap_page(pagemap_t *pagemap, uintptr_t virt_addr) {
    if (virt_addr % PAGE_SIZE != 0) {
         return false;
    }

    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, false);
    if (pdpt == NULL) return true;
    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, false);
    if (pd == NULL) return true;
    uint64_t *pt = vmm_get_next_level(pd, pd_index, false);
    if (pt == NULL) return true;

    pt[pt_index] = 0;

    asm volatile ("invlpg (%0)" :: "r"(virt_addr) : "memory");

    return true;
}

uintptr_t vmm_virt_to_phys(pagemap_t *pagemap, uintptr_t virt_addr) {
    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, false);
    if (pdpt == NULL) return (uintptr_t)-1;
    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, false);
    if (pd == NULL) return (uintptr_t)-1;
    uint64_t *pt = vmm_get_next_level(pd, pd_index, false);
    if (pt == NULL) return (uintptr_t)-1;

    uint64_t entry = pt[pt_index];
    if (!(entry & PTE_PRESENT)) {
        return (uintptr_t)-1;
    }

    return PTE_GET_ADDR(entry) + (virt_addr % PAGE_SIZE);
}

void vmm_switch_to(pagemap_t *pagemap) {
    if (!pagemap || !pagemap->top_level) {
        panic("PANIC: Attempted to switch to an invalid pagemap\n");
    }
    uintptr_t pml4_phys = (uintptr_t)pagemap->top_level - VMM_HIGHER_HALF;
    asm volatile ("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}

void init_vmm(void) {
    if (hhdm_request.response == NULL) {
        panic("PANIC: HHDM request response missing\n");
    }
    if (kernel_address_request.response == NULL) {
        panic("PANIC: Kernel Address request response missing\n");
    }
    if (memorymap_info.response == NULL) {
        panic("PANIC: Memory Map request response missing\n");
    }

    void *pml4_phys = allocate_page();
    if (pml4_phys == NULL) {
        panic("PANIC: Failed to allocate kernel PML4 table\n");
    }
    uint64_t *pml4_virt = (uint64_t *)((uintptr_t)pml4_phys + VMM_HIGHER_HALF);
    memset(pml4_virt, 0, PAGE_SIZE);

    static pagemap_t k_pagemap;
    kernel_pagemap = &k_pagemap;
    kernel_pagemap->top_level = pml4_virt;

    struct limine_executable_address_response *kaddr = kernel_address_request.response;
    uintptr_t kernel_phys_base = kaddr->physical_base;
    uintptr_t kernel_virt_base = kaddr->virtual_base;

    uintptr_t text_start_addr    = (uintptr_t)_text_start;
    uintptr_t text_end_addr      = (uintptr_t)_text_end;
    uintptr_t rodata_start_addr  = (uintptr_t)_rodata_start;
    uintptr_t rodata_end_addr    = (uintptr_t)_rodata_end;
    uintptr_t data_start_addr    = (uintptr_t)_data_start;
    uintptr_t data_end_addr      = ALIGN_UP((uintptr_t)_bss_end, PAGE_SIZE);

    uintptr_t kernel_virt_end = data_end_addr;

    for (uintptr_t p_virt = kernel_virt_base; p_virt < kernel_virt_end; p_virt += PAGE_SIZE) {
        uintptr_t p_phys = (p_virt - kernel_virt_base) + kernel_phys_base;
        uint64_t flags = PTE_PRESENT;

        if (p_virt >= text_start_addr && p_virt < text_end_addr) {
             flags |= 0;
        } else if (p_virt >= rodata_start_addr && p_virt < rodata_end_addr) {
            flags |= PTE_NX;
        } else if (p_virt >= data_start_addr && p_virt < data_end_addr) {
             flags |= PTE_WRITABLE | PTE_NX;
        } else {
             flags |= PTE_NX;
        }

        if (!vmm_map_page(kernel_pagemap, p_virt, p_phys, flags)) {
            panic("PANIC: Failed to map kernel page Virt: %p Phys: 0x%x Flags: 0x%llx\n",
                  (void*)p_virt, p_phys, flags);
        }
    }

    struct limine_memmap_response *memmap = memorymap_info.response;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        uintptr_t base = entry->base;
        uintptr_t top = entry->base + entry->length;

        base = base & ~(PAGE_SIZE - 1);
        top = ALIGN_UP(top, PAGE_SIZE);

        for (uintptr_t p = base; p < top; p += PAGE_SIZE) {
            if (!vmm_map_page(kernel_pagemap, p + VMM_HIGHER_HALF, p, PTE_PRESENT | PTE_WRITABLE)) {
                 panic("PANIC: Failed to map HHDM page Phys: 0x%x Virt: 0x%x\n", p, p + VMM_HIGHER_HALF);
            }
        }

        const uintptr_t IDENTITY_MAP_LIMIT = 0x100000000;
        if (base < IDENTITY_MAP_LIMIT) {
            uintptr_t identity_top = top > IDENTITY_MAP_LIMIT ? IDENTITY_MAP_LIMIT : top;
            for (uintptr_t p = base; p < identity_top; p += PAGE_SIZE) {
                if (p == 0) continue;
                if (!vmm_map_page(kernel_pagemap, p, p, PTE_PRESENT | PTE_WRITABLE)) {
                     panic("PANIC: Failed to identity map low page Phys: 0x%x\n", p);
                }
            }
        }
    }
    vmm_switch_to(kernel_pagemap);
}
