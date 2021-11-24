#ifndef HSV_H
#define HSV_H

#include <stdint.h>

typedef struct
{
    uint32_t r;
    uint32_t g;
    uint32_t b;
} RGB_t;

typedef struct
{
    uint32_t h;
    uint32_t s;
    uint32_t v;
} HSV_t;

void hsv_to_rgb(HSV_t *hsv, RGB_t *rgb);

#endif