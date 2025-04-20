#pragma once

#define PIT_CHANNEL0_DATA      0x40
#define PIT_CHANNEL1_DATA      0x41
#define PIT_CHANNEL2_DATA      0x42
#define PIT_COMMAND            0x43

#define PIT_BASE_FREQUENCY     1193182
#define PIT_CMD_CHANNEL0       0x00
#define PIT_CMD_ACCESS_BOTH    0x30
#define PIT_CMD_MODE3          0x06
#define PIT_CMD_BINARY         0x00

void pit_init(uint32_t frequency);
void pit_set_frequency(uint32_t frequency);
uint64_t pit_get_ticks(void);
uint64_t pit_get_elapsed_ms(void);
void pit_irq_handler(void);

void pit_sleep_ms(uint32_t ms);

static __attribute__((unused)) uint32_t pit_frequency = 0;
