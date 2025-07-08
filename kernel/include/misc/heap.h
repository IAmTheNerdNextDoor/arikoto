#pragma once

#include <stddef.h>

void init_heap(void);

void *kmalloc(size_t size);

void kfree(void *ptr);

void *kcalloc(size_t num, size_t size);

void *krealloc(void *ptr, size_t size);
