#ifndef NVMC_H
#define NVMC_H

#include "color.h"
#include "estc_error.h"

#define CLEAR_WORD                      (0xFFFFFFFF)
#define BOOTLOADER_START_ADDR           (0x000E0000)
#define APP_DATA_PAGES_QTY              3UL
#define PAGE_SIZE_BYTES                 4096UL
#define APP_DATA_START_ADDR (BOOTLOADER_START_ADDR - APP_DATA_PAGES_QTY * PAGE_SIZE_BYTES)
#define WORD_SIZE                       4UL
#define NUM_OF_WORK_PAGES               2UL
#define SAVED_COLORS_MAX_NUM            10UL    //Maximum number of saved colors for add_rgb_color command
#define SIZE_OF_COLOR_NAME_MAX_BYTES    24UL    //Maximum size of color name: 23 bytes for name and 1 byte for attribute "deleted"
#define SIZE_OF_COLOR_RGB_VALUE_BYTES   4UL                                             //Size of RGB color value: R + G + B + CRC
#define SIZE_OF_COLOR_NAME_BUFFER       (SAVED_COLORS_MAX_NUM + 10UL)                   //Size of buffer for color names
#define ATTRIBUTE_DELETE                ('d')                                           //The value of attribute for deleted color
#define ATTRIBUTE_INIT                  ('I' + ('n' << 8) + ('i' << 16) + ('t' << 24))  //The value of attribute for inited pages. One WORD.

//Start address shifting of the saved color names relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_ATTRIBUTE_INIT       0UL

//Start address shifting of the saved color names relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_COLOR_NAMES          (SHIFT_ADDR_ATTRIBUTE_INIT + WORD_SIZE)

//Start address shifting of the RGB values for saved color names relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_COLOR_NAMES_VALS     (SHIFT_ADDR_COLOR_NAMES + SIZE_OF_COLOR_NAME_MAX_BYTES * SIZE_OF_COLOR_NAME_BUFFER)

//Start address shifting of an address of first position of last saved RGB values relatively to APP_DATA_START_ADDR
#define SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR (SHIFT_ADDR_COLOR_NAMES_VALS + SIZE_OF_COLOR_RGB_VALUE_BYTES * SIZE_OF_COLOR_NAME_BUFFER)

uint32_t nvmc_flash_init();
void nvmc_read_rgb_actual_values(rgb_t *const rgb_vals);
void nvmc_write_rgb_actual_values(rgb_t const *const rgb_vals);
void nvmc_read_rgb_actual_values_for_log();
estc_ret_code_t nvmc_rgb_color_add(rgb_t const *const rgb_vals, char const color_name[]);
estc_ret_code_t nvmc_rgb_color_del(char const color_name[]);
estc_ret_code_t nvmc_rgb_color_get_vals(rgb_t*const rgb_vals, char const color_name[]);
estc_ret_code_t nvmc_rgb_color_get_list(rgb_t rgb_vals[], char color_name[][SIZE_OF_COLOR_NAME_MAX_BYTES]);

#endif