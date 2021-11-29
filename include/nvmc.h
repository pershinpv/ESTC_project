#ifndef NVMC_H
#define NVMC_H

#include <stdint.h>
#include "nrfx_nvmc.h"
#include "hsv.h"
#include "log.h"

#define BOOTLOADER_START_ADDR (0x000E0000UL)
#define APP_DATA_PAGES_QTY 3UL
#define PAGE_SIZE_BYTES 4096UL
#define APP_DATA_START_ADDR (BOOTLOADER_START_ADDR - APP_DATA_PAGES_QTY * PAGE_SIZE_BYTES)
#define RGB_INIT_VALS_PAGE_ADDR (APP_DATA_START_ADDR + (APP_DATA_PAGES_QTY - 1) * PAGE_SIZE_BYTES)
#define HSV_INIT_VALS_PAGE_ADDR (APP_DATA_START_ADDR + (APP_DATA_PAGES_QTY - 1) * PAGE_SIZE_BYTES)

uint32_t find_first_free_pos_addr(uint32_t page_start_addr);
bool check_for_new_hsv_vals(hsv_t *hsv_vals, uint8_t * last_address);
void write_hsv_actual_values(hsv_t *hsv_vals);
void read_hsv_actual_values(hsv_t *hsv_vals);
void read_hsv_values_for_log(hsv_t *hsv_vals);

#endif