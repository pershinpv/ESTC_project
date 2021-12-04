#include "nvmc.h"

//find_first_free_pos_addr return 0 if the page is full
uint32_t find_first_free_pos_addr(uint32_t page_start_addr)
{
    uint8_t * current_address = 0;
    uint8_t * start_addr = (uint8_t *) page_start_addr;
    do
    {
        if (*start_addr == 0xFF && *(start_addr + 1) == 0xFF && *(start_addr + 2) == 0xFF && *(start_addr + 3) == 0xFF)
            current_address = start_addr;
        start_addr += 4;
    } while (current_address == 0 && (uint32_t) start_addr < page_start_addr + PAGE_SIZE_BYTES);
    return (uint32_t) current_address;
}

bool check_for_new_hsv_vals(hsv_t const *const hsv_vals, uint8_t const *const last_address)
{
    uint16_t hsv_h = *last_address + (*(last_address + 1) << 8);

    return !(hsv_vals->h == hsv_h && hsv_vals->s == *(last_address + 2) && hsv_vals->v == *(last_address + 3));
}

uint32_t get_active_page_start_addr(uint32_t first_page_address)
{
    if (*(uint32_t *)(first_page_address + PAGE_SIZE_BYTES) == 0xFFFFFFFF)
        return first_page_address;
    else
        return first_page_address + PAGE_SIZE_BYTES;
}

void nvmc_write_hsv_actual_values(hsv_t const *const hsv_vals)
{
    uint32_t hsv_word = hsv_vals->h + (hsv_vals->s << 16) + (hsv_vals->v << 24);
    uint32_t active_page_start_addr;
    uint32_t current_write_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_write_address = find_first_free_pos_addr(active_page_start_addr);

    if (current_write_address == 0)
    {
        if (check_for_new_hsv_vals(hsv_vals, (uint8_t *) active_page_start_addr + PAGE_SIZE_BYTES - 4))
        {
            uint32_t new_page_start_address = (active_page_start_addr == APP_DATA_START_ADDR) ? (active_page_start_addr + PAGE_SIZE_BYTES) : active_page_start_addr;
            nrfx_nvmc_word_write(new_page_start_address, hsv_word);
            nrfx_nvmc_page_erase(active_page_start_addr);
        }
    }
    else
    {
        if (check_for_new_hsv_vals(hsv_vals, (uint8_t *) current_write_address - 4))
        {
            nrfx_nvmc_word_write(current_write_address, hsv_word);
        }
    }
}

void nvmc_read_hsv_actual_values(hsv_t *const hsv_vals)
{
    uint32_t active_page_start_addr;
    uint32_t current_read_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_read_address = find_first_free_pos_addr(active_page_start_addr);

    if (current_read_address == 0)
        current_read_address = active_page_start_addr + PAGE_SIZE_BYTES - 4;
    else
        current_read_address -= 4;

    hsv_vals->h = *(uint16_t *) current_read_address;
    hsv_vals->s = *(uint8_t *) (current_read_address + 2);
    hsv_vals->v = *(uint8_t *) (current_read_address + 3);
}

void nvmc_read_hsv_values_for_log()
{
    uint32_t active_page_start_addr;
    uint32_t current_read_address;

    active_page_start_addr = get_active_page_start_addr(APP_DATA_START_ADDR);
    current_read_address = find_first_free_pos_addr(active_page_start_addr);

    if (current_read_address == 0)
        current_read_address = active_page_start_addr + PAGE_SIZE_BYTES - 4;
    else
        current_read_address -= 4;

    for (uint32_t addr = active_page_start_addr; addr <= current_read_address; addr += 4)
    {
        NRF_LOG_INFO("At address 0x%X h=%d s=%d v=%d", addr, *(uint16_t *)addr, *(uint8_t *) (addr + 2), *(uint8_t *) (addr + 3));
    }
    NRF_LOG_INFO("-----Reading %d bytes (%d records) finished-----", current_read_address - active_page_start_addr + 4, (current_read_address - active_page_start_addr + 4) / 4);
}

uint8_t hsv_crc_calc(hsv_t const *const hsv_vals)
{
    return ((hsv_vals->h & 0xFF) + (hsv_vals->h >> 8) + hsv_vals->s + hsv_vals->v);
}