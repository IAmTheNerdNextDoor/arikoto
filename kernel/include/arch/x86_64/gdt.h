#pragma once

#include <stdint.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define TSS_SEL   0x18
#define USER_CS   0x28
#define USER_DS   0x30

#define KERNEL_STACK_SIZE 8192
#define USER_STACK_SIZE 4096

extern uint8_t kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
extern uint8_t user_stack[USER_STACK_SIZE] __attribute__((aligned(16)));

void init_gdt_tss(void);
