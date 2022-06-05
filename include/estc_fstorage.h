#ifndef ESTC_FSTORAGE_H
#define NVMC_H

void estc_fstorage_init(void);
nrfx_err_t nrfx_nvmc_page_erase(uint32_t addr);
void nrfx_nvmc_word_write(uint32_t addr, uint32_t value);
void nrfx_nvmc_byte_write(uint32_t addr, uint8_t value);

#endif