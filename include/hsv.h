#ifndef HSV_H
#define HSV_H

#include <stdint.h>
#include "log.h"

#define HSV_MAX_H   360
#define HSV_MAX_S   100
#define HSV_MAX_V   100
#define RGB_MAX_VAL 100

#define HSV_INIT_H   0
#define HSV_INIT_S   100
#define HSV_INIT_V   100

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct
{
    uint16_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

void hsv_to_rgb(hsv_t const *const hsv, rgb_t *const rgb);
bool hsv_values_validate(hsv_t const *const hsv);
void hsv_values_init(hsv_t *const hsv);
void hsv_validate_or_reset(hsv_t *const hsv);
void rgb_to_hsv(hsv_t *const hsv, rgb_t const *const rgb);

#endif