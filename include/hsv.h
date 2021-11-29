#ifndef HSV_H
#define HSV_H

#include <stdint.h>

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

void hsv_to_rgb(hsv_t *hsv, rgb_t *rgb);
void hsv_validate_or_reset(hsv_t *hsv);

#endif