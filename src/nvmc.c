#include <stdint.h>
#include "log.h"
#include "nrfx_nvmc.h"
#include "nvmc.h"

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

static void nvmc_copy_good_vals_to_new_page(uint32_t const old_page_addr, uint32_t const new_page_addr, uint32_t const last_value)
{
    uint32_t old_color_name_addr = old_page_addr + SHIFT_ADDR_COLOR_NAMES;
    uint32_t old_color_vals_addr = old_page_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
    uint32_t new_color_name_addr = new_page_addr + SHIFT_ADDR_COLOR_NAMES;
    uint32_t new_color_vals_addr = new_page_addr + SHIFT_ADDR_COLOR_NAMES_VALS;
    size_t colors_counter = 0;

    while(*(uint32_t*)old_color_vals_addr != 0xFFFFFF && colors_counter < SIZE_OF_COLOR_NAME_BUFFER)
    {
        if (*(uint32_t*)(old_color_name_addr + 1) >> 24 != DELETE_ATTRIBUTE)
        {
            nrfx_nvmc_word_write(new_color_name_addr, *(uint32_t*)old_color_name_addr);
            nrfx_nvmc_word_write(new_color_name_addr + 1, *(uint32_t*)(old_color_name_addr + 4));
            new_color_name_addr += SIZE_OF_COLOR_NAME_MAX_BYTES;

            nrfx_nvmc_word_write(new_color_vals_addr, *(uint32_t*)old_color_vals_addr);
            new_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        }

        old_color_name_addr += SIZE_OF_COLOR_NAME_MAX_BYTES;
        old_color_vals_addr += SIZE_OF_COLOR_RGB_VALUE_BYTES;
        ++colors_counter;
    }
    nrfx_nvmc_word_write(new_page_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, last_value);
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
    uint32_t current_read_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_read_address = find_first_free_pos_addr_between(active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR, \
                                                            active_page_start_addr + PAGE_SIZE_BYTES);

    if (current_read_address == 0)      //0 if the page is full
        current_read_address = active_page_start_addr + PAGE_SIZE_BYTES - sizeof(uint32_t);
    else
        current_read_address -= sizeof(uint32_t);

    for (uint32_t addr = active_page_start_addr + SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR; addr <= current_read_address; addr += sizeof(uint32_t))
    {
        NRF_LOG_INFO("At address 0x%X r=%d g=%d b=%d crc=%d", addr, \
        *(uint8_t *)addr, *(uint8_t *) (addr + 1), *(uint8_t *) (addr + 2), *(uint8_t *) (addr + 3));
        //nrf_delay_ms(10);
    }
    NRF_LOG_INFO("-----Reading %d bytes (%d records) finished-----", \
     current_read_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + 4, \
    (current_read_address - active_page_start_addr - SHIFT_ADDR_LAST_RGB_VAL_LST_ADDR + 4) / 4);
}