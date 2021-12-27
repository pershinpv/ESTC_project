#include <stdint.h>
#include "log.h"
#include "nrfx_nvmc.h"
#include "nvmc.h"
#include "cli_cmd.h"

//find_first_free_pos_addr_between return 0 if free position not found
static uint32_t find_first_free_pos_addr_between(uint32_t start_addr, uint32_t stop_addr )
{
    uint32_t current_address = 0;
    do
    {
        if (*(uint32_t *) start_addr == CLEAR_WORD)
            current_address = start_addr;
        start_addr += sizeof(uint32_t);
    } while (current_address == 0 && start_addr < stop_addr);
    return current_address;
}

static uint32_t get_active_page_start_addr(uint32_t first_page_address)
{
    if (*(uint32_t *)(first_page_address + PAGE_SIZE_BYTES) == CLEAR_WORD && \
        *(uint32_t *)(first_page_address + PAGE_SIZE_BYTES + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR) == CLEAR_WORD)
        return first_page_address;
    else
        return first_page_address + PAGE_SIZE_BYTES;
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

    while(*(uint32_t*)old_color_vals_addr != 0xFFFFFF && colors_counter < SIZE_OF_COLOR_NAME_BUFFER)
    {
        if (*(uint8_t*)(old_color_name_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) != DELETE_ATTRIBUTE)
        {
            for (size_t cur_byte = 0; cur_byte < SIZE_OF_COLOR_NAME_MAX_BYTES - 1; cur_byte += sizeof(uint32_t))
            {
                nrfx_nvmc_word_write(new_color_name_addr, *(uint32_t*)(old_color_name_addr + cur_byte));
                new_color_name_addr += sizeof(uint32_t);
            }

            nrfx_nvmc_word_write(new_color_vals_addr, *(uint32_t*)old_color_vals_addr);
            new_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        }

        old_color_name_addr += SIZE_OF_COLOR_NAME_MAX_BYTES;
        old_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        ++colors_counter;
    }
    nrfx_nvmc_word_write(new_page_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, last_value);
    return new_color_vals_addr;
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
    current_write_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, \
                                                             active_page_start_addr + PAGE_SIZE_BYTES);

     if (current_write_address == 0)
    {
        if (check_for_new_rgb_vals(rgb_vals, (uint8_t *) (active_page_start_addr + PAGE_SIZE_BYTES - sizeof(uint32_t))))
        {
            uint32_t new_page_start_address = (active_page_start_addr == APP_DATA_START_ADDR) ? \
                                              (active_page_start_addr + PAGE_SIZE_BYTES) : active_page_start_addr;

            nvmc_copy_good_vals_to_new_page(active_page_start_addr, new_page_start_address, rgb_word);
            nrfx_nvmc_page_erase(active_page_start_addr);
        }
    }
    else
    {
        if (check_for_new_rgb_vals(rgb_vals, (uint8_t *) (current_write_address - sizeof(uint32_t))))
        {
            nrfx_nvmc_word_write(current_write_address, rgb_word);
        }
    }
}

// is_color_name_exist return 0 if color name is new and return color address if the color name exist
static uint32_t is_color_name_exist(uint32_t const page_start_addr, char const color_name[])
{
    for (size_t color_addr = page_start_addr + SHIFT_ADDR_COLOR_NAMES; color_addr < page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
         color_addr += SIZE_OF_COLOR_NAME_MAX_BYTES)
    {
        bool is_next_color_name = false;
        size_t current_char = 0;

        if (*(uint8_t *)(color_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1) == DELETE_ATTRIBUTE)
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

static void nvmc_write_color_name(uint32_t address, char const color_name[])
{
    uint32_t color_word = 0;

    for (size_t cur_char = 0; cur_char < SIZE_OF_COLOR_NAME_MAX_BYTES; cur_char += sizeof(uint32_t))
    {
        color_word = (color_name[cur_char + 0]) + (color_name[cur_char + 1] << 8) \
                   + (color_name[cur_char + 2] << 16) + (color_name[cur_char + 3] << 24);

        nrfx_nvmc_word_write(address + cur_char, color_word);
    }
}

bool nvmc_add_rgb_color(rgb_t const *const rgb_vals, char const color_name[])
{
    uint32_t rgb_word = rgb_vals->r + (rgb_vals->g  << 8) + (rgb_vals->b << 16) + (rgb_vals->crc  << 24);
    uint32_t active_page_start_addr = 0;
    uint32_t current_color_vals_address = 0;
    uint32_t current_color_name_address = 0;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_color_vals_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS, \
                                                                  active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR);

    if (is_color_name_exist(active_page_start_addr, color_name) > 0)
        return false;

    /*NRF_LOG_INFO("page_addr %d color_name %d %d %d %d %d", active_page_start_addr, color_name[0], color_name[1], color_name[2], \
                                                                                   color_name[3], color_name[4]);
*/
    if (current_color_vals_address == 0)
    {
        uint32_t new_page_start_address = (active_page_start_addr == APP_DATA_START_ADDR) ? \
                                          (active_page_start_addr + PAGE_SIZE_BYTES) : active_page_start_addr;

        current_color_vals_address = nvmc_copy_good_vals_to_new_page(active_page_start_addr, new_page_start_address, rgb_word);
        current_color_name_address = active_page_start_addr + \
        ((current_color_vals_address - active_page_start_addr - SHIFT_ADDR_COLOR_NAMES_VALS) / 4) * SIZE_OF_COLOR_NAME_MAX_BYTES;

        nvmc_write_color_name(current_color_name_address, color_name);
        nrfx_nvmc_word_write(current_color_vals_address, rgb_word);
        nrfx_nvmc_page_erase(active_page_start_addr);
    }
    else
    {
        current_color_name_address = active_page_start_addr + \
        ((current_color_vals_address - active_page_start_addr - SHIFT_ADDR_COLOR_NAMES_VALS) / 4) * SIZE_OF_COLOR_NAME_MAX_BYTES;

        nvmc_write_color_name(current_color_name_address, color_name);
        nrfx_nvmc_word_write(current_color_vals_address, rgb_word);
    }

    return true;
}

bool nvmc_del_rgb_color(char const color_name[])
{
    uint32_t color_addr = 0;

    color_addr = is_color_name_exist(get_active_page_start_addr(APP_DATA_START_ADDR), color_name);

    if (color_addr == 0)
    {
        return false;
    }
    else
    {
        nrfx_nvmc_byte_write(color_addr + SIZE_OF_COLOR_NAME_MAX_BYTES - 1, DELETE_ATTRIBUTE);
        return true;
    }
}

//return address of RGB values for color_name or 0 if color_name not found
uint32_t nvmc_find_rgb_color(char const color_name[])
{
    uint32_t color_addr = is_color_name_exist(get_active_page_start_addr(APP_DATA_START_ADDR), color_name);
    if (color_addr > 0)
    {
        return (color_addr - get_active_page_start_addr(APP_DATA_START_ADDR) - SHIFT_ADDR_COLOR_NAMES) / \
                SIZE_OF_COLOR_NAME_MAX_BYTES * SIZE_OF_COLOR_RGB_VALUE_BYTES + SHIFT_ADDR_COLOR_NAMES_VALS + \
                get_active_page_start_addr(APP_DATA_START_ADDR);
    }
    else
        return 0;
}

void nvmc_read_rgb_actual_values(rgb_t *const rgb_vals)
{
    uint32_t active_page_start_addr;
    uint32_t current_read_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_read_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, \
                                                            active_page_start_addr + PAGE_SIZE_BYTES);

    if (current_read_address == 0)      //0 if the page is full
        current_read_address = active_page_start_addr + PAGE_SIZE_BYTES - sizeof(uint32_t);
    else
        current_read_address -= sizeof(uint32_t);

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

    current_color_vals_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS, \
                                                                  active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR);

    if (current_color_vals_address == 0)      //0 if the page is full
        current_color_vals_address = active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR - sizeof(uint32_t);
    else
        current_color_vals_address -= sizeof(uint32_t);

    number_of_saved_colors = (current_color_vals_address - active_page_start_addr - SHIFT_ADDR_COLOR_NAMES_VALS + 4) / 4;

    for (size_t curr_color_num = 0; curr_color_num < number_of_saved_colors; ++curr_color_num)
    {
        uint32_t vals_addr = active_page_start_addr + SHIFT_ADDR_COLOR_NAMES_VALS + curr_color_num * SIZE_OF_COLOR_RGB_VALUE_BYTES;
        uint32_t name_adrr = active_page_start_addr + SHIFT_ADDR_COLOR_NAMES + curr_color_num * SIZE_OF_COLOR_NAME_MAX_BYTES;
        char color_name[SIZE_OF_COLOR_NAME_MAX_BYTES];

        for(size_t char_counter = 0; char_counter < SIZE_OF_COLOR_NAME_MAX_BYTES; ++char_counter)
            color_name[char_counter] = *(uint8_t*)(name_adrr + char_counter);

        NRF_LOG_INFO("Name address: 0x%X Name: %c%c%c%c Delete attribute: %d", \
                      name_adrr, color_name[0] % 255, color_name[1] % 255, color_name[2] % 255, color_name[3] % 255, \
                                 color_name[SIZE_OF_COLOR_NAME_MAX_BYTES - 1] % 255);

        NRF_LOG_INFO("Vals address: 0x%X r=%d g=%d b=%d crc=%d",
                     vals_addr, *(uint8_t *)vals_addr, *(uint8_t *)(vals_addr + 1), *(uint8_t *)(vals_addr + 2), *(uint8_t *)(vals_addr + 3));
    }

    NRF_LOG_INFO("-----Reading %d colors values (%d bytes) finished-----", number_of_saved_colors, number_of_saved_colors * 4);

    current_last_val_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, \
                                                            active_page_start_addr + PAGE_SIZE_BYTES);

    if (current_last_val_address == 0)      //0 if the page is full
        current_last_val_address = active_page_start_addr + PAGE_SIZE_BYTES - sizeof(uint32_t);
    else
        current_last_val_address -= sizeof(uint32_t);

    for (uint32_t addr = active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR; addr <= current_last_val_address; addr += sizeof(uint32_t))
    {
        NRF_LOG_INFO("At address 0x%X r=%d g=%d b=%d crc=%d", addr, \
        *(uint8_t *)addr, *(uint8_t *) (addr + 1), *(uint8_t *) (addr + 2), *(uint8_t *) (addr + 3));
    }
    NRF_LOG_INFO("-----Reading %d bytes (%d records) finished-----", \
     current_last_val_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + 4, \
    (current_last_val_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + 4) / 4);
}