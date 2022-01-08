#include "gpio.h"
#include "color.h"
#include "macro.h"
#include "nvmc.h"

static hsv_t color_hsv;
static rgb_t color_rgb;

void color_hsv_to_rgb(hsv_t const *const hsv, rgb_t *const rgb)
{
    uint16_t Vmin = hsv->v * (HSV_MAX_S - hsv->s) / HSV_MAX_S;
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
    //rgb->r = (uint16_t)rgb->r * RGB_MAX_VAL / 100;
    //rgb->g = (uint16_t)rgb->g * RGB_MAX_VAL / 100;
    //rgb->b = (uint16_t)rgb->b * RGB_MAX_VAL / 100;
    rgb->crc = color_crc_calc_rgb(rgb);
}

static void rgb_to_hsv(hsv_t *const hsv, rgb_t const *const rgb)
{
    uint8_t rgb_min, rgb_max;

    uint8_t r = (uint16_t)rgb->r;// * 100 / RGB_MAX_VAL;
    uint8_t g = (uint16_t)rgb->g;// * 100 / RGB_MAX_VAL;
    uint8_t b = (uint16_t)rgb->b;// * 100 / RGB_MAX_VAL;

    rgb_min = MIN3(r, g, b);
    rgb_max = MAX3(r, g, b);

    hsv->v = rgb_max;
    if (rgb_max == 0)
    {
        hsv->h = 0;
        hsv->s = 0;
        return;
    }

    hsv->s = ((uint16_t)(rgb_max - rgb_min) * HSV_MAX_S) / rgb_max;
    if (hsv->s == 0)
    {
        hsv->h = 0;
        return;
    }

    if (rgb_max == r)
    {
        if(g >= b)
            hsv->h = 60 * ((uint16_t)(g - b)) / (rgb_max - rgb_min);
        else
            hsv->h = (60 * ((uint32_t)(rgb_max - rgb_min) * 6 + g - b) / (rgb_max - rgb_min)) % HSV_MAX_H;
    }
    else if (rgb_max == g)
        hsv->h = 60 * ((uint16_t)(rgb_max - rgb_min) * 2 + b - r) / (rgb_max - rgb_min);
    else
        hsv->h = 60 * ((uint32_t)(rgb_max - rgb_min) * 4 + r - g) / (rgb_max - rgb_min);
}

uint8_t color_crc_calc_rgb(rgb_t const *const rgb_vals)
{
    return (rgb_vals->r + rgb_vals->g + rgb_vals->b);
}

bool color_rgb_values_validate(rgb_t const *const rgb_vals)
{
    if(color_crc_calc_rgb(rgb_vals) == rgb_vals->crc)
        return true;
    else
        return false;
}

static void color_rgb_values_init(rgb_t *const rgb_vals)
{
    rgb_vals->r = RGB_MAX_VAL;
    rgb_vals->g = 0;
    rgb_vals->b = 0;
    rgb_vals->crc = color_crc_calc_rgb(rgb_vals);
}

void color_value_set_rgb(rgb_t *rgb_vals)
{
    color_rgb.r = rgb_vals->r;
    color_rgb.g = rgb_vals->g;
    color_rgb.b = rgb_vals->b;
    color_rgb.crc = rgb_vals->crc;
    rgb_to_hsv(&color_hsv, &color_rgb);
}

void color_value_set_hsv(hsv_t *hsv_vals)
{
    color_hsv.h = hsv_vals->h;
    color_hsv.s = hsv_vals->s;
    color_hsv.v = hsv_vals->v;
    color_hsv_to_rgb(&color_hsv, &color_rgb);
}

void color_restore_rgb_last_state()
{
    nvmc_read_rgb_actual_values(&color_rgb);
    if (!color_rgb_values_validate(&color_rgb))
        color_rgb_values_init(&color_rgb);
    rgb_to_hsv(&color_hsv, &color_rgb);
}

static nrfx_pwm_t * ctrl_led_pwm_ptr;
static nrfx_pwm_t * rgb_led_pwm_ptr;

//----------PWM control led config begin-------------------------
static nrf_pwm_values_common_t ctrl_led_seq_vals_slow[] = {0x8000, 0x8000, 0x8000, 0, 0, 0};    // One time step is 250 ms.
static nrf_pwm_values_common_t ctrl_led_seq_vals_fast[] = {0x8000, 0};
static nrf_pwm_values_common_t ctrl_led_seq_vals_on[] = {0x8000};
static nrf_pwm_values_common_t ctrl_led_seq_vals_off[] = {0};

static nrf_pwm_sequence_t ctrl_led_seq =
{
    .values.p_common = ctrl_led_seq_vals_off,
    .length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off),
    .repeats = 0,
    .end_delay = 0
};

static nrfx_pwm_config_t pwm_config_ctrl =
{
        .output_pins =
            {
                LED1G,
                NRF_DRV_PWM_PIN_NOT_USED,
                NRF_DRV_PWM_PIN_NOT_USED,
                NRF_DRV_PWM_PIN_NOT_USED,
            },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock = NRF_PWM_CLK_125kHz,
        .count_mode = NRF_PWM_MODE_UP,
        .top_value = 31250, // 250ms period
        .load_mode = NRF_PWM_LOAD_COMMON,
        .step_mode = NRF_PWM_STEP_AUTO
};
//----------PWM control led config end-------------------------

//----------PWM RGB led config begin-------------------------

uint16_t const rgb_led_pwm_top_val = 25500;
static nrf_pwm_values_individual_t rgb_led_seq_vals;
static nrf_pwm_sequence_t const    rgb_led_seq =
{
    .values.p_individual = &rgb_led_seq_vals,
    .length              = NRF_PWM_VALUES_LENGTH(rgb_led_seq_vals),
    .repeats             = 0,
    .end_delay           = 0
};

static nrfx_pwm_config_t const pwm_config_rgb =
    {
        .output_pins =
            {
                NRF_DRV_PWM_PIN_NOT_USED,
                LED2R,
                LED2G,
                LED2B,
            },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock = NRF_PWM_CLK_8MHz,
        .count_mode = NRF_PWM_MODE_UP,
        .top_value = rgb_led_pwm_top_val,
        .load_mode = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode = NRF_PWM_STEP_AUTO
    };
//----------PWM RGB led config end-------------------------

static void rgb_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
    static uint16_t *p_channels = (uint16_t *)&rgb_led_seq_vals;
    static int h_step = 1;
    static int s_step = 1;
    static int v_step = 1;
    const uint8_t speed_prescaler = 8;
    static uint8_t prescaler_counter = 0;

    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        if (btn_is_long_press_get_state() && btn_double_click_counter_get_state() != HSV_CHANGE_NO && prescaler_counter == speed_prescaler)
        {
            if (btn_double_click_counter_get_state() == HSV_CHANGE_H)
                color_hsv.h = (color_hsv.h + h_step) % HSV_MAX_H;
            else if (btn_double_click_counter_get_state() == HSV_CHANGE_S)
                color_hsv.s = (color_hsv.s + calc_step(color_hsv.s, 0, HSV_MAX_S, &s_step)) % (HSV_MAX_V + ABS(s_step));
            else if (btn_double_click_counter_get_state() == HSV_CHANGE_V)
                color_hsv.v = (color_hsv.v + calc_step(color_hsv.v, 0, HSV_MAX_V, &v_step)) % (HSV_MAX_V + ABS(v_step));
            color_hsv_to_rgb(&color_hsv, &color_rgb);

            //NRF_LOG_INFO("hsv %d %d %d", color_hsv.h, color_hsv.s, color_hsv.v);
            //NRF_LOG_INFO("rgb %d %d %d", color_rgb.r, color_rgb.g, color_rgb.b);
            //NRF_LOG_INFO("------------");
        }
        p_channels[1] = rgb_led_pwm_top_val / RGB_MAX_VAL * color_rgb.r;
        p_channels[2] = rgb_led_pwm_top_val / RGB_MAX_VAL * color_rgb.g;
        p_channels[3] = rgb_led_pwm_top_val / RGB_MAX_VAL * color_rgb.b;
        prescaler_counter %= speed_prescaler;
        ++prescaler_counter;
    }
}

void color_ctrl_led_pwm_init(nrfx_pwm_t * led_pwm_inst_ptr)
{
    ctrl_led_pwm_ptr = led_pwm_inst_ptr;
    nrfx_pwm_init(ctrl_led_pwm_ptr, &pwm_config_ctrl, NULL);
    nrfx_pwm_simple_playback(ctrl_led_pwm_ptr, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
}

void color_rgb_led_pwm_init(nrfx_pwm_t * led_pwm_inst_ptr)
{
    rgb_led_pwm_ptr = led_pwm_inst_ptr;
    nrfx_pwm_init(rgb_led_pwm_ptr, &pwm_config_rgb, rgb_pwm_handler);
    nrfx_pwm_simple_playback(rgb_led_pwm_ptr, &rgb_led_seq, 1, NRFX_PWM_FLAG_LOOP);
}

void color_set_work_mode(hsv_change_mode_t work_mode)
{
            switch(work_mode)
            {
            case HSV_CHANGE_NO:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_off;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off);
                nvmc_write_rgb_actual_values(&color_rgb);
                nvmc_read_rgb_actual_values_for_log();
                break;

            case HSV_CHANGE_H:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_slow;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_slow);
                break;

            case HSV_CHANGE_S:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_fast;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_fast);
                break;

            case HSV_CHANGE_V:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_on;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_on);
                break;

            default:
                break;
            }

            nrfx_pwm_simple_playback(ctrl_led_pwm_ptr, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
}