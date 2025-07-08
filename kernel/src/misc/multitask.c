#include <misc/multitask.h>
#include <misc/heap.h>
#include <misc/kernel.h>
#include <misc/string.h>
#include <arch/x86_64/pit.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/spinlock.h>

static task_t* task_list_head = NULL;
static task_t* task_list_tail = NULL;
task_t* current_task = NULL;
task_t* next_task = NULL;
static uint32_t next_task_id = 1;
static spinlock_t scheduler_lock = SPINLOCK_INIT;

static task_t* ready_queue_head = NULL;
static task_t* ready_queue_tail = NULL;
static task_t* blocked_queue_head = NULL;
static task_t* sleeping_queue_head = NULL;

#define DEFAULT_TIME_SLICE 10

static task_t kernel_task;

static void task_add_to_ready_queue(task_t* task) {
    task->state = TASK_STATE_READY;
    task->next = NULL;

    if (ready_queue_tail == NULL) {
        ready_queue_head = task;
        ready_queue_tail = task;
    } else {
        ready_queue_tail->next = task;
        ready_queue_tail = task;
    }
}

static task_t* task_remove_from_ready_queue(void) {
    if (ready_queue_head == NULL) {
        return NULL;
    }

    task_t* task = ready_queue_head;
    ready_queue_head = task->next;

    if (ready_queue_head == NULL) {
        ready_queue_tail = NULL;
    }

    task->next = NULL;
    return task;
}

void init_multitasking(void) {
    memset(&kernel_task, 0, sizeof(task_t));
    kernel_task.id = 0;
    strncpy(kernel_task.name, "kernel_main", 31);
    kernel_task.kernel_stack = NULL;
    kernel_task.state = TASK_STATE_ACTIVE;
    kernel_task.priority = 0;
    kernel_task.time_slice = DEFAULT_TIME_SLICE;
    kernel_task.ticks_remaining = DEFAULT_TIME_SLICE;

    kernel_task.next = NULL;
    task_list_head = &kernel_task;
    task_list_tail = &kernel_task;

    current_task = &kernel_task;
}

task_t* task_create(void (*entry_point)(void*), void* arg, const char* name, uint8_t priority) {
    spinlock_acquire(&scheduler_lock);

    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) {
        spinlock_release(&scheduler_lock);
        return NULL;
    }
    memset(task, 0, sizeof(task_t));

    task->kernel_stack = kmalloc(65536);
    if (!task->kernel_stack) {
        kfree(task);
        spinlock_release(&scheduler_lock);
        return NULL;
    }
    task->kernel_stack_top = (void*)((uint64_t)task->kernel_stack + 65536);

    task->id = next_task_id++;
    strncpy(task->name, name, 31);
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->time_slice = DEFAULT_TIME_SLICE;
    task->ticks_remaining = DEFAULT_TIME_SLICE;

    uint64_t* stack_ptr = (uint64_t*)task->kernel_stack_top;

    *(--stack_ptr) = (uint64_t)entry_point;

    *(--stack_ptr) = 0x202;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = (uint64_t)arg;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;

    task->registers.rsp = (uint64_t)stack_ptr;

    task->next = NULL;
    if (task_list_tail) {
        task_list_tail->next = task;
        task_list_tail = task;
    } else {
        task_list_head = task;
        task_list_tail = task;
    }

    task_add_to_ready_queue(task);

    spinlock_release(&scheduler_lock);

    return task;
}

task_t* task_get_current(void) {
    return current_task;
}

void schedule(void) {
    spinlock_acquire(&scheduler_lock);

    task_t* sleeping = sleeping_queue_head;
    task_t* prev_sleeping = NULL;

    while (sleeping) {
        if (pit_get_elapsed_ms() >= sleeping->wake_up_time) {
            task_t* task_to_wake = sleeping;

            if (prev_sleeping) {
                prev_sleeping->next = sleeping->next;
            } else {
                sleeping_queue_head = sleeping->next;
            }

            sleeping = sleeping->next;

            task_to_wake->next = NULL;
            task_add_to_ready_queue(task_to_wake);
        } else {
            prev_sleeping = sleeping;
            sleeping = sleeping->next;
        }
    }

    if (ready_queue_head == NULL) {
        if (current_task && current_task->state == TASK_STATE_ACTIVE) {
            current_task->ticks_remaining = current_task->time_slice;
        }

        spinlock_release(&scheduler_lock);
        return;
    }

    next_task = task_remove_from_ready_queue();

    if (current_task && current_task->state == TASK_STATE_ACTIVE) {
        current_task->state = TASK_STATE_READY;
        task_add_to_ready_queue(current_task);
    }

    next_task->state = TASK_STATE_ACTIVE;
    next_task->ticks_remaining = next_task->time_slice;

    if (next_task == current_task) {
        spinlock_release(&scheduler_lock);
        return;
    }

    task_t* prev_task = current_task;
    current_task = next_task;

    uint64_t* old_sp = &prev_task->registers.rsp;
    uint64_t new_sp = current_task->registers.rsp;

    spinlock_release(&scheduler_lock);

    task_switch_asm(old_sp, new_sp);
}

void task_sleep(uint32_t milliseconds) {
    if (milliseconds == 0) {
        return;
    }

    spinlock_acquire(&scheduler_lock);

    current_task->wake_up_time = pit_get_elapsed_ms() + milliseconds;
    current_task->state = TASK_STATE_SLEEPING;

    task_t** pp = &sleeping_queue_head;
    while (*pp && (*pp)->wake_up_time <= current_task->wake_up_time) {
        pp = &(*pp)->next;
    }

    current_task->next = *pp;
    *pp = current_task;

    spinlock_release(&scheduler_lock);

    schedule();
}

void task_block(void) {
    spinlock_acquire(&scheduler_lock);

    current_task->state = TASK_STATE_BLOCKED;

    current_task->next = blocked_queue_head;
    blocked_queue_head = current_task;

    spinlock_release(&scheduler_lock);

    schedule();
}

void task_unblock(task_t* task) {
    if (!task || task->state != TASK_STATE_BLOCKED) {
        return;
    }

    spinlock_acquire(&scheduler_lock);

    task_t** pp = &blocked_queue_head;
    while (*pp && *pp != task) {
        pp = &(*pp)->next;
    }

    if (*pp) {
        *pp = task->next;

        task_add_to_ready_queue(task);
    }

    spinlock_release(&scheduler_lock);
}

void task_exit(void) {
    spinlock_acquire(&scheduler_lock);

    current_task->state = TASK_STATE_ZOMBIE;

    if (current_task->kernel_stack) {
        kfree(current_task->kernel_stack);
        current_task->kernel_stack = NULL;
    }

    spinlock_release(&scheduler_lock);

    schedule();

    hcf();
}

void task_timer_tick(void) {
    if (!current_task) {
        return;
    }

    if (current_task->ticks_remaining > 0) {
        current_task->ticks_remaining--;

        if (current_task->ticks_remaining == 0) {
            schedule();
        }
    }
}
