#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/memory.h>

#define TASK_STATE_INVALID    0
#define TASK_STATE_ACTIVE     1
#define TASK_STATE_READY      2
#define TASK_STATE_BLOCKED    3
#define TASK_STATE_SLEEPING   4
#define TASK_STATE_ZOMBIE     5

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} task_registers_t;

typedef struct task {
    uint32_t id;
    char name[32];

    void* kernel_stack;
    void* kernel_stack_top;

    task_registers_t registers;

    uint8_t state;
    uint8_t priority;
    uint64_t time_slice;
    uint64_t ticks_remaining;
    uint64_t wake_up_time;

    struct task* next;
} task_t;

void init_multitasking(void);

task_t* task_create(void (*entry_point)(void*), void* arg, const char* name, uint8_t priority);

void schedule(void);

void task_exit(void);

void task_sleep(uint32_t milliseconds);

void task_block(void);

void task_unblock(task_t* task);

task_t* task_get_current(void);

void task_switch(void);

void task_timer_tick(void);

extern void task_switch_asm(uint64_t* old_sp, uint64_t new_sp);
