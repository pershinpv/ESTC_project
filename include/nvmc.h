#ifndef NVMC_H
#define NVMC_H

#include "color.h"

#define CLEAR_WORD                      (0xFFFFFFFF)
#define BOOTLOADER_START_ADDR           (0x000E0000UL)
#define APP_DATA_PAGES_QTY              3UL
#define PAGE_SIZE_BYTES                 4096UL
#define APP_DATA_START_ADDR (BOOTLOADER_START_ADDR - APP_DATA_PAGES_QTY * PAGE_SIZE_BYTES)
#define SIZE_OF_COLOR_NAME_MAX_BYTES    8UL     //Maximum size of color name: 7 bytes for name and 1 byte for attribute "deleted"
#define SIZE_OF_COLOR_RGB_VALUE_BYTES   4UL     //Size of RGB color value: R + G + B + CRC
#define SIZE_OF_COLOR_NAME_BUFFER       20UL    //Size of buffer for color names
#define DELETE_ATTRIBUTE                ('d')   // The value of attribute for deleted color

//Start address shifting of the saved color names relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_COLOR_NAMES          0UL

//Start address shifting of the RGB values for saved color names relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_COLOR_NAMES_VALS     (SHIFT_ADDR_COLOR_NAMES + SIZE_OF_COLOR_NAME_MAX_BYTES * SIZE_OF_COLOR_NAME_BUFFER)

//Start address shifting of an address of first position of last saved RGB values relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR (SHIFT_ADDR_COLOR_NAMES_VALS + SIZE_OF_COLOR_RGB_VALUE_BYTES * SIZE_OF_COLOR_NAME_BUFFER)

void nvmc_read_rgb_actual_values(rgb_t *const rgb_vals);
void nvmc_write_rgb_actual_values(rgb_t const *const rgb_vals);
void nvmc_read_rgb_actual_values_for_log();
#endif