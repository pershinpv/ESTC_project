#include <stdint.h>
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nvmc.h"

static void estc_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

NRF_FSTORAGE_DEF(nrf_fstorage_t estc_fstorage) =
{
    .evt_handler = estc_fstorage_evt_handler,
    .start_addr = APP_DATA_START_ADDR,
    .end_addr   = BOOTLOADER_START_ADDR - 1,
};

static uint32_t write_buffer;

static void estc_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("Event received: wrote %d bytes at address 0x%x: %d %d %d %d.",
                         p_evt->len, p_evt->addr,
                         *((uint8_t *)p_evt->p_src + 0), *((uint8_t *)p_evt->p_src + 1),
                         *((uint8_t *)p_evt->p_src + 2), *((uint8_t *)p_evt->p_src + 3));
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

void estc_fstorage_init(void)
{
    int ret = nrf_fstorage_init(&estc_fstorage, &nrf_fstorage_sd, NULL);
    APP_ERROR_CHECK(ret);

    nvmc_flash_init();
}

nrfx_err_t nrfx_nvmc_page_erase(uint32_t addr)
{
    nrfx_err_t ret_code;
    ret_code = nrf_fstorage_erase(&estc_fstorage, addr, 1, NULL);

    if (ret_code != NRFX_SUCCESS)
        NRF_LOG_INFO("nrf_fstorage_erase ERROR. Code %d.", ret_code);

    return NRFX_SUCCESS;
}

void nrfx_nvmc_word_write(uint32_t addr, uint32_t value)
{
    ret_code_t ret_code;
    write_buffer = value;

    ret_code = nrf_fstorage_write(&estc_fstorage, addr, &write_buffer, sizeof(value), NULL);

    if (ret_code != NRF_SUCCESS)
        NRF_LOG_INFO("nrf_fstorage_write ERROR. Code %d.", ret_code);
}

void nrfx_nvmc_byte_write(uint32_t addr, uint8_t value)
{
    ret_code_t ret_code;
    write_buffer = value;
    ret_code = nrf_fstorage_write(&estc_fstorage, addr, &write_buffer, sizeof(value), NULL);

    if (ret_code != NRF_SUCCESS)
        NRF_LOG_INFO("nrf_fstorage_write ERROR. Code %d.", ret_code);
}