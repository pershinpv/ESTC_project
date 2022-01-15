#include <stdint.h>
#include "log.h"
#include "nrfx_nvmc.h"
#include "nvmc.h"
#include "cli_cmd.h"
#include "cli_usb.h"

static uint32_t find_first_free_pos_addr_between(uint32_t start_addr, uint32_t stop_addr );
static uint32_t get_active_page_start_addr(uint32_t first_page_address);
static bool check_for_new_rgb_vals(rgb_t const *const rgb_vals, uint8_t const *const address);
static uint32_t nvmc_copy_good_vals_to_new_page(uint32_t const old_page_addr, uint32_t const new_page_addr, uint32_t const last_value);
static uint32_t find_color_name_addr(uint32_t const page_start_addr, char const color_name[]);
static bool is_free_memory_for_colors (uint32_t const page_start_addr);
static void nvmc_write_color_name(uint32_t address, char const color_name[]);
static void nvmc_prepair_string_for_flash(char *const dest_string, char const *const src_string, size_t const buf_len);

uint32_t nvmc_flash_init()
{
    NRF_LOG_INFO("Start flash init");

    uint32_t curr_addr = APP_DATA_START_ADDR;
    uint32_t active_page_addr;
    size_t init_counter = 0;

    for (size_t curr_page = 0; curr_page < NUM_OF_WORK_PAGES; ++curr_page)
    {
        bool is_page_clear = true;

        if (*(uint32_t *)(APP_DATA_START_ADDR + SHIFT_ADDR_ATTRIBUTE_INIT + PAGE_SIZE_BYTES * curr_page) != ATTRIBUTE_INIT)
        {
            while (is_page_clear && curr_addr < APP_DATA_START_ADDR + PAGE_SIZE_BYTES * (curr_page + 1))
            {
                if (*(uint32_t *)(curr_addr + SHIFT_ADDR_ATTRIBUTE_INIT) != CLEAR_WORD)
                {
                    nrfx_nvmc_page_erase(APP_DATA_START_ADDR + PAGE_SIZE_BYTES * curr_page);
                    is_page_clear = false;
                }
                curr_addr += WORD_SIZE;
            }
        }
        else
        {
            ++init_counter;
            active_page_addr = APP_DATA_START_ADDR + PAGE_SIZE_BYTES * curr_page;
        }
    }

    if (init_counter > 1)
    {
        for (size_t curr_page = 0; curr_page < NUM_OF_WORK_PAGES; ++curr_page)
        {
            if (*(uint32_t *)(APP_DATA_START_ADDR + SHIFT_ADDR_ATTRIBUTE_INIT + PAGE_SIZE_BYTES * curr_page) == ATTRIBUTE_INIT)
                nrfx_nvmc_page_erase(APP_DATA_START_ADDR + PAGE_SIZE_BYTES * curr_page);
        }
    }

    if (init_counter == 0)
    {
        nrfx_nvmc_word_write(APP_DATA_START_ADDR + SHIFT_ADDR_ATTRIBUTE_INIT, ATTRIBUTE_INIT);
        active_page_addr = APP_DATA_START_ADDR;
    }
    return active_page_addr;
}

/**
 * @brief Function for writing RGB values (including CRC) to flash as one continius 32-bit word.

 * @param rgb_vals      rgb_t is a struct: r, g, b, CRC.
 */

void nvmc_write_rgb_actual_values(rgb_t const *const rgb_vals)
{
    uint32_t rgb_word = rgb_vals->r + (rgb_vals->g  << 8) + (rgb_vals->b << 16) + (rgb_vals->crc  << 24);
    uint32_t active_page_start_addr = 0;
    uint32_t current_write_address = 0;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_write_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR,
                                                             active_page_start_addr + PAGE_SIZE_BYTES);

     if (current_write_address == 0)
    {
        if (check_for_new_rgb_vals(rgb_vals, (uint8_t *) (active_page_start_addr + PAGE_SIZE_BYTES - WORD_SIZE)))
        {
            uint32_t new_page_start_address = (active_page_start_addr == APP_DATA_START_ADDR) ?
                                              (APP_DATA_START_ADDR + PAGE_SIZE_BYTES) : APP_DATA_START_ADDR;

            nvmc_copy_good_vals_to_new_page(active_page_start_addr, new_page_start_address, rgb_word);
            nrfx_nvmc_page_erase(active_page_start_addr);
        }
    }
    else
    {
        if (check_for_new_rgb_vals(rgb_vals, (uint8_t *) (current_write_address - WORD_SIZE)))
        {
            nrfx_nvmc_word_write(current_write_address, rgb_word);
        }
    }
}

estc_ret_code_t nvmc_rgb_color_add(rgb_t const *const rgb_vals, char const color_name[])
{
    uint32_t rgb_word = rgb_vals->r + (rgb_vals->g  << 8) + (rgb_vals->b << 16) + (rgb_vals->crc  << 24);
    uint32_t active_page_start_addr = 0;
    uint32_t current_color_vals_address = 0;
    uint32_t current_color_name_address = 0;
    char write_buffer[SIZE_OF_COLOR_NAME_MAX_BYTES];

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_color_vals_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS,
                                                                  active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR);

    nvmc_prepair_string_for_flash(write_buffer, color_name, SIZE_OF_COLOR_NAME_MAX_BYTES);

    if (find_color_name_addr(active_page_start_addr, write_buffer) > 0)
        return ESTC_ERROR_SAME_DATA_NAME;

    if (!is_free_memory_for_colors(active_page_start_addr))
        return ESTC_ERROR_NO_MEM;

    if (current_color_vals_address == 0)
    {
        uint32_t new_page_start_address = (active_page_start_addr == APP_DATA_START_ADDR) ?
                                          (APP_DATA_START_ADDR + PAGE_SIZE_BYTES) : APP_DATA_START_ADDR;

        current_color_vals_address = nvmc_copy_good_vals_to_new_page(active_page_start_addr, new_page_start_address, rgb_word);
        current_color_name_address = SHIFT_ADDR_COLOR_NAMES + new_page_start_address +
        ((current_color_vals_address - new_page_start_address - SHIFT_ADDR_COLOR_NAMES_VALS) / 4) * SIZE_OF_COLOR_NAME_MAX_BYTES;

        nvmc_write_color_name(current_color_name_address, write_buffer);
        nrfx_nvmc_word_write(current_color_vals_address, rgb_word);
        nrfx_nvmc_page_erase(active_page_start_addr);
    }
    else
    {
        current_color_name_address = SHIFT_ADDR_COLOR_NAMES + active_page_start_addr +
        ((current_color_vals_address - active_page_start_addr - SHIFT_ADDR_COLOR_NAMES_VALS) / 4) * SIZE_OF_COLOR_NAME_MAX_BYTES;

        nvmc_write_color_name(current_color_name_address, write_buffer);
        nrfx_nvmc_word_write(current_color_vals_address, rgb_word);
    }

    return ESTC_SUCCESS;
}

estc_ret_code_t nvmc_rgb_color_del(char const color_name[])
{
    uint32_t color_addr = 0;
    char color_name_flash[SIZE_OF_COLOR_NAME_MAX_BYTES];

    nvmc_prepair_string_for_flash(color_name_flash, color_name, SIZE_OF_COLOR_NAME_MAX_BYTES);
    color_addr = find_color_name_addr(get_active_page_start_addr(APP_DATA_START_ADDR), color_name_flash);

    if (color_addr == 0)
    {
        return ESTC_ERROR_NAME_NOT_FOUND;
    }
    else
    {
        nrfx_nvmc_byte_write(color_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1, ATTRIBUTE_DELETE);
        return ESTC_SUCCESS;
    }
}

//return address of RGB values for color_name or 0 if color_name not found
estc_ret_code_t nvmc_rgb_color_get_vals(rgb_t *const rgb_vals, char const color_name[])
{
    uint32_t color_addr = 0;
    char color_name_flash[SIZE_OF_COLOR_NAME_MAX_BYTES];

    nvmc_prepair_string_for_flash(color_name_flash, color_name, SIZE_OF_COLOR_NAME_MAX_BYTES);
    color_addr = find_color_name_addr(get_active_page_start_addr(APP_DATA_START_ADDR), color_name_flash);

    if (color_addr > 0)
    {
        color_addr = (color_addr - get_active_page_start_addr(APP_DATA_START_ADDR) - SHIFT_ADDR_COLOR_NAMES) /
                     SIZE_OF_COLOR_NAME_MAX_BYTES * SIZE_OF_COLOR_RGB_VALUE_BYTES + SHIFT_ADDR_COLOR_NAMES_VALS +
                     get_active_page_start_addr(APP_DATA_START_ADDR);

        rgb_vals->r = *(uint8_t *)(color_addr + 0);
        rgb_vals->g = *(uint8_t *)(color_addr + 1);
        rgb_vals->b = *(uint8_t *)(color_addr + 2);
        rgb_vals->crc = *(uint8_t *)(color_addr + 3);

        return ESTC_SUCCESS;
    }
    else
        return ESTC_ERROR_NAME_NOT_FOUND;
}

estc_ret_code_t nvmc_rgb_color_get_list(rgb_t rgb_vals[], char color_name[][SIZE_OF_COLOR_NAME_MAX_BYTES])
{
    uint32_t active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    uint32_t name_addr;
    size_t name_counter = 0;
    estc_ret_code_t ret_code = ESTC_ERROR_NOT_FOUND;

    for (size_t curr_color_num = 0; curr_color_num < SIZE_OF_COLOR_NAME_BUFFER; ++curr_color_num)
    {
        name_addr = active_page_start_addr + SHIFT_ADDR_COLOR_NAMES + curr_color_num * SIZE_OF_COLOR_NAME_MAX_BYTES;

        if (*(uint8_t *)(name_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) != ATTRIBUTE_DELETE &&
            *(uint32_t *)name_addr != CLEAR_WORD && name_counter < SAVED_COLORS_MAX_NUM)
        {
            for (size_t chars_counter = 0; chars_counter < SIZE_OF_COLOR_NAME_MAX_BYTES; chars_counter += WORD_SIZE)
            {
                *(uint32_t *)(&color_name[name_counter][chars_counter]) = *(uint32_t *)(name_addr + chars_counter);
            }

            nvmc_rgb_color_get_vals(&rgb_vals[name_counter], color_name[name_counter]);
            ++name_counter;
        }
    }

    if (name_counter > 0)
        ret_code = ESTC_SUCCESS;

    while(name_counter < SAVED_COLORS_MAX_NUM)
    {
        *(uint32_t *)(&rgb_vals[name_counter]) = CLEAR_WORD;
        *(uint32_t *)(&color_name[name_counter][0]) = 0UL;
        ++name_counter;
    }

    return ret_code;
}

void nvmc_read_rgb_actual_values(rgb_t *const rgb_vals)
{
    uint32_t active_page_start_addr;
    uint32_t current_read_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_read_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR,
                                                            active_page_start_addr + PAGE_SIZE_BYTES);

    if (current_read_address == 0)      //0 if the page is full
        current_read_address = active_page_start_addr + PAGE_SIZE_BYTES - WORD_SIZE;
    else
        current_read_address -= WORD_SIZE;

    rgb_vals->r = *(uint8_t *) (current_read_address + 0);
    rgb_vals->g = *(uint8_t *) (current_read_address + 1);
    rgb_vals->b = *(uint8_t *) (current_read_address + 2);
    rgb_vals->crc = *(uint8_t *) (current_read_address + 3);
}

void nvmc_read_rgb_actual_values_for_log()
{
    uint32_t active_page_start_addr;
    uint32_t current_last_val_address;
    uint32_t current_color_vals_address;
    size_t number_of_saved_colors = 0;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);

    current_color_vals_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS,
                                                                  active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR);

    if (current_color_vals_address == 0)      //0 if the page is full
        current_color_vals_address = active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR - WORD_SIZE;
    else
        current_color_vals_address -= WORD_SIZE;

    if (*(uint32_t *)(APP_DATA_START_ADDR + SHIFT_ADDR_ATTRIBUTE_INIT) != ATTRIBUTE_INIT &&
        *(uint32_t *)(APP_DATA_START_ADDR + PAGE_SIZE_BYTES + SHIFT_ADDR_ATTRIBUTE_INIT) != ATTRIBUTE_INIT)
            NRF_LOG_INFO("+++++ATTENTION! Pages not inited+++++");

    number_of_saved_colors = (current_color_vals_address - active_page_start_addr - SHIFT_ADDR_COLOR_NAMES_VALS + WORD_SIZE) / WORD_SIZE;

    for (size_t curr_color_num = 0; curr_color_num < number_of_saved_colors; ++curr_color_num)
    {
        uint32_t vals_addr = active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS + curr_color_num * SIZE_OF_COLOR_RGB_VALUE_BYTES;
        uint32_t name_adrr = active_page_start_addr + SHIFT_ADDR_COLOR_NAMES + curr_color_num * SIZE_OF_COLOR_NAME_MAX_BYTES;

        char color_name[SIZE_OF_COLOR_NAME_MAX_BYTES];

        for(size_t char_counter = 0; char_counter < SIZE_OF_COLOR_NAME_MAX_BYTES; ++char_counter)
        {
            if (*(uint8_t*)(name_adrr + char_counter) != 0xFF)
                color_name[char_counter] = *(uint8_t*)(name_adrr + char_counter);
            else
                color_name[char_counter] = 0;
        }

        NRF_LOG_INFO("Name address: 0x%X Color name: %c%c%c%c Delete attribute: %d",
                      name_adrr, color_name[0] % 255, color_name[1] % 255, color_name[2] % 255, color_name[3] % 255,
                                 color_name[SIZE_OF_COLOR_NAME_MAX_BYTES - 1] % 255);

        NRF_LOG_INFO("Vals address: 0x%X r=%d g=%d b=%d crc=%d",
                     vals_addr, *(uint8_t *)vals_addr, *(uint8_t *)(vals_addr + 1), *(uint8_t *)(vals_addr + 2), *(uint8_t *)(vals_addr + 3));
    }

    NRF_LOG_INFO("-----Reading %d colors values (%d bytes) finished-----", number_of_saved_colors, number_of_saved_colors * WORD_SIZE);

    current_last_val_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR,
                                                                active_page_start_addr + PAGE_SIZE_BYTES);

    if (current_last_val_address == 0)      //0 if the page is full
        current_last_val_address = active_page_start_addr + PAGE_SIZE_BYTES - WORD_SIZE;
    else
        current_last_val_address -= WORD_SIZE;

    for (uint32_t addr = active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR; addr <= current_last_val_address; addr += WORD_SIZE)
    {
        NRF_LOG_INFO("At address 0x%X r=%d g=%d b=%d crc=%d", addr,
                     *(uint8_t *)addr, *(uint8_t *) (addr + 1), *(uint8_t *) (addr + 2), *(uint8_t *) (addr + 3));
    }
    NRF_LOG_INFO("-----Reading %d bytes (%d records) finished-----",
                 current_last_val_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + WORD_SIZE,
                 (current_last_val_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + WORD_SIZE) / WORD_SIZE);
}

//find_first_free_pos_addr_between return 0 if free position not found
static uint32_t find_first_free_pos_addr_between(uint32_t start_addr, uint32_t stop_addr )
{
    uint32_t current_address = 0;
    do
    {
        if (*(uint32_t *) start_addr == CLEAR_WORD)
            current_address = start_addr;
        start_addr += WORD_SIZE;
    } while (current_address == 0 && start_addr < stop_addr);
    return current_address;
}

static uint32_t get_active_page_start_addr(uint32_t first_page_address)
{
    for (size_t curr_page = 0; curr_page < NUM_OF_WORK_PAGES; ++curr_page)
    {
        if (*(uint32_t *)(APP_DATA_START_ADDR + SHIFT_ADDR_ATTRIBUTE_INIT + PAGE_SIZE_BYTES * curr_page) == ATTRIBUTE_INIT)
            return APP_DATA_START_ADDR + PAGE_SIZE_BYTES * curr_page;
    }

    NRF_LOG_INFO("get_active_page_start_addr fault. Go to flash init");
    return nvmc_flash_init();
}

static bool check_for_new_rgb_vals(rgb_t const *const rgb_vals, uint8_t const *const address)
{
    return !(rgb_vals->r == *address && rgb_vals->g == *(address + 1) && rgb_vals->b == *(address + 2) && rgb_vals->crc == *(address + 3));
}

static uint32_t nvmc_copy_good_vals_to_new_page(uint32_t const old_page_addr, uint32_t const new_page_addr, uint32_t const last_value)
{
    uint32_t old_color_name_addr = old_page_addr + SHIFT_ADDR_COLOR_NAMES;
    uint32_t old_color_vals_addr = old_page_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
    uint32_t new_color_name_addr = new_page_addr + SHIFT_ADDR_COLOR_NAMES;
    uint32_t new_color_vals_addr = new_page_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
    size_t colors_counter = 0;

    while(*(uint32_t*)old_color_vals_addr != CLEAR_WORD && colors_counter < SIZE_OF_COLOR_NAME_BUFFER)
    {
        if (*(uint8_t*)(old_color_name_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) != ATTRIBUTE_DELETE)
        {
            for (size_t cur_byte = 0; cur_byte < SIZE_OF_COLOR_NAME_MAX_BYTES - 1; cur_byte += WORD_SIZE)
            {
                nrfx_nvmc_word_write(new_color_name_addr, *(uint32_t*)(old_color_name_addr + cur_byte));
                new_color_name_addr += WORD_SIZE;
            }

            nrfx_nvmc_word_write(new_color_vals_addr, *(uint32_t*)old_color_vals_addr);
            new_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        }

        old_color_name_addr += SIZE_OF_COLOR_NAME_MAX_BYTES;
        old_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        ++colors_counter;
    }
    nrfx_nvmc_word_write(new_page_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, last_value);
    nrfx_nvmc_word_write(new_page_addr + SHIFT_ADDR_ATTRIBUTE_INIT, ATTRIBUTE_INIT);

    return new_color_vals_addr;
}

// find_color_name_addr return 0 if color name is new and return color address if the color name exist
static uint32_t find_color_name_addr(uint32_t const page_start_addr, char const color_name[])
{
    for (size_t color_addr = page_start_addr + SHIFT_ADDR_COLOR_NAMES; color_addr < page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
         color_addr += SIZE_OF_COLOR_NAME_MAX_BYTES)
    {
        bool is_next_color_name = false;
        size_t current_char = 0;

        if (*(uint8_t *)(color_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) == ATTRIBUTE_DELETE)
            is_next_color_name = true;

        while (!is_next_color_name && current_char < SIZE_OF_COLOR_NAME_MAX_BYTES - 1)
        {
            if (color_name[current_char] != *(uint8_t *)(color_addr + current_char))
                is_next_color_name = true;
            ++current_char;
        }
        if (!is_next_color_name && current_char == SIZE_OF_COLOR_NAME_MAX_BYTES - 1)
            return color_addr;
    }
    return 0;
}

static bool is_free_memory_for_colors (uint32_t const page_start_addr)
{
    uint8_t colors_counter = 0;

    for (size_t color_addr = page_start_addr + SHIFT_ADDR_COLOR_NAMES; color_addr < page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
         color_addr += SIZE_OF_COLOR_NAME_MAX_BYTES)
    {
        if (*(uint32_t *)color_addr != CLEAR_WORD && *(uint8_t *)(color_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) != ATTRIBUTE_DELETE)
            ++colors_counter;

        if (colors_counter == SAVED_COLORS_MAX_NUM)
            return false;
    }

    return true;
}

static void nvmc_write_color_name(uint32_t address, char const color_name[])
{
    uint32_t color_word = 0;

    for (size_t cur_char = 0; cur_char < SIZE_OF_COLOR_NAME_MAX_BYTES; cur_char += WORD_SIZE)
    {
        color_word = (color_name[cur_char + 0]) + (color_name[cur_char + 1] << 8)
                   + (color_name[cur_char + 2] << 16) + (color_name[cur_char + 3] << 24);

        nrfx_nvmc_word_write(address + cur_char, color_word);
    }
}

static void nvmc_prepair_string_for_flash(char *const dest_string, char const *const src_string, size_t const buf_len)
{
    size_t str_len = strlen (src_string) + 1;

    strcpy(dest_string, src_string);
    memset(dest_string + str_len, 0xFF, buf_len - str_len);
}