#include "macro.h"

uint32_t digit_capacity(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void deviceID_calc(uint32_t device_ID[], uint32_t leds_numbers, uint32_t board_id)
{
    uint32_t digits = digit_capacity(board_id);
    uint32_t dev_id = board_id;

    for (uint32_t i = 0; i < leds_numbers; i++)
    {
        device_ID[i] = dev_id / digits;
        dev_id %= digits;
        if (digits/10 > 0)
            digits /= 10;
    }
}