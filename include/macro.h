#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MILITOMIKRO(value) ((value)*1000)

uint32_t digit_capacity(uint32_t numeric);
void deviceID_calc(uint32_t device_ID[], uint32_t leds_numbers, uint32_t board_id);

#endif