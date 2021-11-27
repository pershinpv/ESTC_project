#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MILITOMIKRO(value) ((value)*1000)
#define CALC_STEP(value, min_value, max_value) (((value) == (min_value)) || ((value) == (max_value)) ? -1 : 1)
#define ABS(value) (((value) < 0) ? -(value) : (value))

uint32_t digit_capacity(uint32_t numeric);
void deviceID_calc(uint32_t device_ID[], uint32_t leds_numbers, uint32_t board_id);
int calc_step(uint32_t value, uint32_t min_value, uint32_t max_value, int *current_step);

#endif