#include <arch/x86_64/vmm.h>
#include <arch/x86_64/memory.h>
#include <arch/x86_64/request.h>
#include <misc/kernel.h>
#include <misc/string.h>
#include <misc/print.h>
#include <arch/x86_64/spinlock.h>

extern uint8_t _text_start[], _text_end[];
extern uint8_t _rodata_start[], _rodata_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

pagemap_t *kernel_pagemap = NULL;

static bool vmm_is_table_empty(uint64_t *table_virt) {
    for (int i = 0; i < 512; i++) {
        if (table_virt[i] != 0) {
            return false;
        }
    }
    return true;
}

static uint64_t *vmm_get_next_level(uint64_t *current_level_virt, size_t index, bool allocate) {
    uint64_t entry = current_level_virt[index];

    if (entry & PTE_PRESENT) {
        return (uint64_t *)(PTE_GET_ADDR(entry) + VMM_HIGHER_HALF);
    }

    if (!allocate) {
        return NULL;
    }

    void *next_level_phys = allocate_page();
    if (next_level_phys == NULL) {
        printk(COLOR_RED, "VMM: Failed to allocate page for new page table level (index %u)\n", index);
        return NULL;
    }

    uint64_t *next_level_virt = (uint64_t *)((uintptr_t)next_level_phys + VMM_HIGHER_HALF);

    memset(next_level_virt, 0, PAGE_SIZE);

    uint64_t new_entry_flags = PTE_PRESENT | PTE_WRITABLE;
    current_level_virt[index] = (uint64_t)(uintptr_t)next_level_phys | new_entry_flags;

    return next_level_virt;
}

bool vmm_map_page(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
    virt_addr &= ~(PAGE_SIZE - 1);
    phys_addr &= ~(PAGE_SIZE - 1);

    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    spinlock_acquire(&pagemap->lock);

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, true);
    if (pdpt == NULL) {
        spinlock_release(&pagemap->lock);
        printk(COLOR_RED, "VMM Error: Failed to get/create PDPT for virt %p\n", (void*)virt_addr);
        return false;
    }

    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, true);
    if (pd == NULL) {
        spinlock_release(&pagemap->lock);
        printk(COLOR_RED, "VMM Error: Failed to get/create PD for virt %p\n", (void*)virt_addr);
        return false;
    }

    uint64_t *pt = vmm_get_next_level(pd, pd_index, true);
    if (pt == NULL) {
        spinlock_release(&pagemap->lock);
        printk(COLOR_RED, "VMM Error: Failed to get/create PT for virt %p\n", (void*)virt_addr);
        return false;
    }

    pt[pt_index] = phys_addr | flags;

    asm volatile ("invlpg (%0)" :: "r"(virt_addr) : "memory");

    spinlock_release(&pagemap->lock);
    return true;
}

bool vmm_unmap_page(pagemap_t *pagemap, uintptr_t virt_addr) {
    if (virt_addr % PAGE_SIZE != 0) {
         printk(COLOR_YELLOW, "Warning: vmm_unmap_page called with non-aligned virt %p\n", (void*)virt_addr);
         return false;
    }

    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    spinlock_acquire(&pagemap->lock);

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, false);
    if (!pdpt) {
        spinlock_release(&pagemap->lock);
        return true;
    }
    uint64_t pdpt_entry = pml4[pml4_index];

    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, false);
    if (!pd) {
        spinlock_release(&pagemap->lock);
        return true;
    }
    uint64_t pd_entry = pdpt[pdpt_index];

    uint64_t *pt = vmm_get_next_level(pd, pd_index, false);
    if (!pt) {
        spinlock_release(&pagemap->lock);
        return true;
    }
    uint64_t pt_entry = pd[pd_index];

    if (!(pt[pt_index] & PTE_PRESENT)) {
        spinlock_release(&pagemap->lock);
        return true;
    }

    pt[pt_index] = 0;

    asm volatile ("invlpg (%0)" :: "r"(virt_addr) : "memory");

    if (vmm_is_table_empty(pt)) {
        pd[pd_index] = 0;

        uintptr_t pt_phys = PTE_GET_ADDR(pt_entry);
        free_page((void*)pt_phys);

        if (vmm_is_table_empty(pd)) {
            pdpt[pdpt_index] = 0;

            uintptr_t pd_phys = PTE_GET_ADDR(pd_entry);
            free_page((void*)pd_phys);

            if (vmm_is_table_empty(pdpt)) {
                pml4[pml4_index] = 0;
                uintptr_t pdpt_phys = PTE_GET_ADDR(pdpt_entry);
                free_page((void*)pdpt_phys);
            }
        }
    }

    spinlock_release(&pagemap->lock);
    return true;
}

uintptr_t vmm_virt_to_phys(pagemap_t *pagemap, uintptr_t virt_addr) {
    size_t pml4_index = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    size_t pd_index   = (virt_addr >> 21) & 0x1FF;
    size_t pt_index   = (virt_addr >> 12) & 0x1FF;

    spinlock_acquire(&pagemap->lock);

    uint64_t *pml4 = pagemap->top_level;
    uint64_t *pdpt = vmm_get_next_level(pml4, pml4_index, false);
    if (pdpt == NULL) {
        spinlock_release(&pagemap->lock);
        return (uintptr_t)-1;
    }

    uint64_t *pd = vmm_get_next_level(pdpt, pdpt_index, false);
    if (pd == NULL) {
        spinlock_release(&pagemap->lock);
        return (uintptr_t)-1;
    }

    uint64_t *pt = vmm_get_next_level(pd, pd_index, false);
    if (pt == NULL) {
        spinlock_release(&pagemap->lock);
        return (uintptr_t)-1;
    }

    uint64_t entry = pt[pt_index];

    spinlock_release(&pagemap->lock);

    if (!(entry & PTE_PRESENT)) {
        return (uintptr_t)-1;
    }

    uintptr_t phys_addr_base = PTE_GET_ADDR(entry);
    uintptr_t offset = virt_addr % PAGE_SIZE;

    return phys_addr_base + offset;
}

void vmm_switch_to(pagemap_t *pagemap) {
    if (!pagemap || !pagemap->top_level) {
        panic("PANIC: VMM: Attempted to switch to an invalid pagemap\n");
        return;
    }

    uintptr_t pml4_phys = (uintptr_t)pagemap->top_level - VMM_HIGHER_HALF;

    asm volatile ("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}

void init_vmm(void) {
    if (hhdm_request.response == NULL) {
        panic("PANIC: VMM: HHDM request response missing\n");
    }
    if (kernel_address_request.response == NULL) {
        panic("PANIC: VMM: Kernel Address request response missing\n");
    }
    if (memorymap_info.response == NULL) {
        panic("PANIC: VMM: Memory Map request response missing\n");
    }

    void *pml4_phys = allocate_page();
    if (pml4_phys == NULL) {
        panic("PANIC: VMM: Failed to allocate kernel PML4 table page\n");
    }
    uint64_t *pml4_virt = (uint64_t *)((uintptr_t)pml4_phys + VMM_HIGHER_HALF);
    memset(pml4_virt, 0, PAGE_SIZE);

    static pagemap_t k_pagemap;
    kernel_pagemap = &k_pagemap;
    kernel_pagemap->top_level = pml4_virt;
    spinlock_init(&kernel_pagemap->lock);

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
        } else if (p_virt >= rodata_start_addr && p_virt < rodata_end_addr) {
            flags |= PTE_NX;
        } else if (p_virt >= data_start_addr && p_virt < data_end_addr) {
             flags |= PTE_WRITABLE | PTE_NX;
        } else {
             flags |= PTE_WRITABLE | PTE_NX;
        }

        if (!vmm_map_page(kernel_pagemap, p_virt, p_phys, flags)) {
            panic("PANIC: VMM: Failed to map kernel page Virt: %p Phys: 0x%x Flags: %p\n",
                  (void*)p_virt, p_phys, (void*)flags);
        }
    }

    struct limine_memmap_response *memmap = memorymap_info.response;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        uintptr_t base = entry->base;
        uintptr_t top = base + entry->length;
        uintptr_t map_base = ALIGN_UP(base, PAGE_SIZE);
        uintptr_t map_top = top & ~(PAGE_SIZE - 1);

        if (map_top <= map_base) continue;

        for (uintptr_t p = map_base; p < map_top; p += PAGE_SIZE) {
            if (!vmm_map_page(kernel_pagemap, p + VMM_HIGHER_HALF, p, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) {
                 panic("PANIC: VMM: Failed to map HHDM page Phys: 0x%x Virt: 0x%x\n", p, p + VMM_HIGHER_HALF);
            }
        }

        const uintptr_t IDENTITY_MAP_LIMIT = 0x100000000;
        if (map_base < IDENTITY_MAP_LIMIT) {
            uintptr_t identity_top = (map_top > IDENTITY_MAP_LIMIT) ? IDENTITY_MAP_LIMIT : map_top;
            for (uintptr_t p = map_base; p < identity_top; p += PAGE_SIZE) {
                if (p == 0) continue;

                if (!vmm_map_page(kernel_pagemap, p, p, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) {
                     panic("PANIC: VMM: Failed to identity map low page Phys: 0x%x\n", p);
                }
            }
        }
    }
    vmm_switch_to(kernel_pagemap);
    printk(COLOR_GREEN, "VMM installed\n");
}
