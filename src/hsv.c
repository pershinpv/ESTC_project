#include "hsv.h"
#include "macro.h"

void hsv_to_rgb(hsv_t const *const hsv, rgb_t *const rgb)
{
    uint16_t Vmin = hsv->v * (100 - hsv->s) / 100;
    uint16_t a = (hsv->v - Vmin) * (hsv->h % 60) / 60;
    uint16_t Vinc = Vmin + a;
    uint16_t Vdec = hsv->v - a;

    switch(hsv->h / 60 % 6)
    {
    case 0:
        rgb->r = hsv->v;
        rgb->g = Vinc;
        rgb->b = Vmin;
        break;
    case 1:
        rgb->r = Vdec;
        rgb->g = hsv->v;
        rgb->b = Vmin;
        break;
    case 2:
        rgb->r = Vmin;
        rgb->g = hsv->v;
        rgb->b = Vinc;
        break;
    case 3:
        rgb->r = Vmin;
        rgb->g = Vdec;
        rgb->b = hsv->v;
        break;
    case 4:
        rgb->r = Vinc;
        rgb->g = Vmin;
        rgb->b = hsv->v;
        break;
    case 5:
        rgb->r = hsv->v;
        rgb->g = Vmin;
        rgb->b = Vdec;
        break;
    }
}

bool hsv_values_validate(hsv_t const *const hsv)
{
    if(hsv->h > HSV_MAX_H || hsv->s > HSV_MAX_S || hsv->v > HSV_MAX_V)
        return false;
    else
        return true;
}

void hsv_values_init(hsv_t *const hsv)
{
    hsv->h = HSV_INIT_H;
    hsv->s = HSV_INIT_S;
    hsv->v = HSV_INIT_V;
}

void hsv_validate_or_reset(hsv_t *const hsv)
{
    if(hsv->h > HSV_MAX_H || hsv->s > HSV_MAX_S || hsv->v > HSV_MAX_V)
    {
        hsv->h = HSV_INIT_H;
        hsv->s = HSV_INIT_S;
        hsv->v = HSV_INIT_V;
    }
}

void rgb_to_hsv(hsv_t *const hsv, rgb_t const *const rgb)
{
    uint8_t rgb_min, rgb_max;

    rgb_min = MIN3(rgb->r, rgb->g, rgb->b);
    rgb_max = MAX3(rgb->r, rgb->g, rgb->b);

    hsv->v = rgb_max;
    if (rgb_max == 0)
    {
        hsv->h = 0;
        hsv->s = 0;
        return;
    }

    hsv->s = ((uint16_t)(rgb_max - rgb_min) * 100) / rgb_max;
    if (hsv->s == 0)
    {
        hsv->h = 0;
        return;
    }

    if (rgb_max == rgb->r)
    {
        if(rgb->g >= rgb->b)
            hsv->h = 60 * ((uint16_t)(rgb->g - rgb->b)) / (rgb_max - rgb_min);
        else
            hsv->h = 60 * ((uint16_t)(rgb_max - rgb_min) * 6 + rgb->g - rgb->b) / (rgb_max - rgb_min);
    }
    else if (rgb_max == rgb->g)
        hsv->h = 60 * ((uint16_t)(rgb_max - rgb_min) * 2 + rgb->b - rgb->r) / (rgb_max - rgb_min);
    else
        hsv->h = 60 * ((uint16_t)(rgb_max - rgb_min) * 4 + rgb->r - rgb->g) / (rgb_max - rgb_min);
}