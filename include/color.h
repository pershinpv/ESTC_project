#ifndef HSV_H
#define HSV_H

#include <stdint.h>
#include "nrfx_pwm.h"
#include "nrf_drv_pwm.h"
#include "gpio_te.h"
#include "log.h"

#define HSV_MAX_H   360
#define HSV_MAX_S   255
#define HSV_MAX_V   255
#define RGB_MAX_VAL 255

#define HSV_INIT_H   0
#define HSV_INIT_S   255
#define HSV_INIT_V   255

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t crc;
} rgb_t;

typedef struct
{
    uint16_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

uint8_t color_crc_calc_rgb(rgb_t const *const rgb_vals);
void color_hsv_to_rgb(hsv_t const *const hsv, rgb_t *const rgb);
void color_ctrl_led_pwm_init(nrfx_pwm_t * led_pwm_inst_ptr);
void color_rgb_led_pwm_init(nrfx_pwm_t * led_pwm_inst_ptr);
void color_set_work_mode(hsv_change_mode_t work_mode);
void color_restore_rgb_last_state();
void color_value_set_rgb(rgb_t *rgb_vals);
void color_value_set_hsv(hsv_t *hsv_vals);
bool color_rgb_values_validate(rgb_t const *const rgb_vals);

#endif