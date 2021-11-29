#include "hsv.h"

void hsv_to_rgb(hsv_t *hsv, rgb_t *rgb)
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

void hsv_validate_or_reset(hsv_t *hsv)
{
    if(hsv->h > HSV_MAX_H || hsv->s > HSV_MAX_S || hsv->v > HSV_MAX_V)
    {
        hsv->h = HSV_INIT_H;
        hsv->s = HSV_INIT_S;
        hsv->v = HSV_INIT_V;
    }
}