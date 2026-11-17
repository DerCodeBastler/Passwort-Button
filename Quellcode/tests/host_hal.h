#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define PICO_FLASH_SIZE_BYTES (4u*1024u*1024u)
#define FLASH_SECTOR_SIZE 4096u
#define FLASH_PAGE_SIZE 256u
#define PICO_DEFAULT_LED_PIN 25u
#define KEYBOARD_LED_CAPSLOCK 2u
extern uint8_t persisted[FLASH_SECTOR_SIZE];
#define XIP_BASE ((uintptr_t)persisted - (PICO_FLASH_SIZE_BYTES-FLASH_SECTOR_SIZE))
typedef enum {HID_REPORT_TYPE_INVALID=0,HID_REPORT_TYPE_INPUT=1,HID_REPORT_TYPE_OUTPUT=2,HID_REPORT_TYPE_FEATURE=3} hid_report_type_t;
uint32_t get_absolute_time(void);
uint32_t to_ms_since_boot(uint32_t time);
uint32_t save_and_disable_interrupts(void);
void restore_interrupts(uint32_t state);
void flash_range_erase(uint32_t offset,size_t count);
void flash_range_program(uint32_t offset,const uint8_t *data,size_t count);
void tud_cdc_write_str(const char *text);
void tud_cdc_write_flush(void);
unsigned tud_cdc_available(void);
int tud_cdc_read_char(void);
void tud_cdc_read_flush(void);
bool tud_mounted(void);
bool tud_suspended(void);
bool tud_hid_ready(void);
bool tud_hid_keyboard_report(uint8_t id,uint8_t modifier,const uint8_t keys[6]);
void tud_task(void);
void gpio_put(unsigned pin,bool state);
static bool bootsel_pressed(void);
