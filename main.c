#include <stdbool.h>
#include <stdint.h>

#include "macro.h"
#include "log.h"
#include "gpio.h"
#include "gpio_te.h"
#include "pwm.h"
#include "hsv.h"
#include "nvmc.h"

#include "nrfx_pwm.h"
#include "nrf_drv_pwm.h"

#include "nrfx_rtc.h"

#define DEVICE_ID 2222      //nRF dongle ID 6596
#define LED_TIME 1000       //LED switch on / swich off time, ms
#define BUTTON_RTC_INSTANCE 0

static hsv_t hsv;
static rgb_t rgb;

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(BUTTON_RTC_INSTANCE);

static nrfx_pwm_t ctrl_led_pwm = NRFX_PWM_INSTANCE(0);
static nrfx_pwm_t rgb_led_pwm = NRFX_PWM_INSTANCE(1);

//----------PWM0 config begin-------------------------
static nrf_pwm_values_common_t ctrl_led_seq_vals_slow[] = {0x8000, 0x8000, 0x8000, 0, 0, 0};    // One time step is 250 ms.
static nrf_pwm_values_common_t ctrl_led_seq_vals_fast[] = {0x8000, 0};
static nrf_pwm_values_common_t ctrl_led_seq_vals_on[] = {0x8000};
static nrf_pwm_values_common_t ctrl_led_seq_vals_off[] = {0};

nrf_pwm_sequence_t ctrl_led_seq =
{
    .values.p_common = ctrl_led_seq_vals_off,
    .length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off),
    .repeats = 0,
    .end_delay = 0
};

nrfx_pwm_config_t pwm_config_ctrl =
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
//----------PWM0 config end-------------------------

//----------PWM1 config begin-------------------------

uint16_t const rgb_led_pwm_top_val = 20000;
nrf_pwm_values_individual_t rgb_led_seq_vals;
nrf_pwm_sequence_t const    rgb_led_seq =
{
    .values.p_individual = &rgb_led_seq_vals,
    .length              = NRF_PWM_VALUES_LENGTH(rgb_led_seq_vals),
    .repeats             = 0,
    .end_delay           = 0
};

nrfx_pwm_config_t const pwm_config_rgb =
    {
        .output_pins =
            {
                NRF_DRV_PWM_PIN_NOT_USED,
                LED2R,
                LED2G,
                LED2B,
            },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock = NRF_PWM_CLK_1MHz,
        .count_mode = NRF_PWM_MODE_UP,
        .top_value = rgb_led_pwm_top_val,
        .load_mode = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode = NRF_PWM_STEP_AUTO
    };
//----------PWM1 config end-------------------------

static void rgb_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
    static uint16_t *p_channels = (uint16_t *)&rgb_led_seq_vals;
    static int h_step = 1;
    static int s_step = 1;
    static int v_step = 1;

    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        if (btn_is_long_press_get_state() && btn_double_click_counter_get_state() != HSV_CHANGE_NO)
        {
            hsv_to_rgb(&hsv, &rgb);
            //NRF_LOG_INFO("hsv %d %d %d", hsv.h, hsv.s, hsv.v);
            //NRF_LOG_INFO("rgb %d %d %d", rgb.r, rgb.g, rgb.b);
            //NRF_LOG_INFO("------------");

            if (btn_double_click_counter_get_state() == HSV_CHANGE_H)
                hsv.h = (hsv.h + h_step) % HSV_MAX_H;
            else if (btn_double_click_counter_get_state() == HSV_CHANGE_S)
                hsv.s = (hsv.s + calc_step(hsv.s, 0, HSV_MAX_S, &s_step)) % (HSV_MAX_V + ABS(s_step));
            else if (btn_double_click_counter_get_state() == HSV_CHANGE_V)
                hsv.v = (hsv.v + calc_step(hsv.v, 0, HSV_MAX_V, &v_step)) % (HSV_MAX_V + ABS(v_step));
        }
        p_channels[1] = rgb_led_pwm_top_val / RGB_MAX_VAL * rgb.r;
        p_channels[2] = rgb_led_pwm_top_val / RGB_MAX_VAL * rgb.g;
        p_channels[3] = rgb_led_pwm_top_val / RGB_MAX_VAL * rgb.b;
    }
}

int main(void)
{
    uint32_t deviceID[LEDS_NUMBER];

    logs_init();
    led_pins_init();
    button_pins_init();
    nrfx_systick_init();
    rtc_button_timer_init(&rtc);
    deviceID_calc(deviceID, LEDS_NUMBER, DEVICE_ID);

    gpiote_pin_in_config(button_pins[0], btn_click_handler);

    read_hsv_actual_values(&hsv);
    hsv_validate_or_reset(&hsv);
    hsv_to_rgb(&hsv, &rgb);

    nrfx_pwm_init(&ctrl_led_pwm, &pwm_config_ctrl, NULL);
    nrfx_pwm_init(&rgb_led_pwm, &pwm_config_rgb, rgb_pwm_handler);

    nrfx_pwm_simple_playback(&ctrl_led_pwm, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
    nrfx_pwm_simple_playback(&rgb_led_pwm, &rgb_led_seq, 1, NRFX_PWM_FLAG_LOOP);

    while (true)
    {
        if (btn_is_dbl_click_get_state())
        {
            btn_is_dbl_click_reset();
            //NRF_LOG_INFO("sizeof %d %d", sizeof(rgb_t), sizeof(hsv_t));

            switch(btn_double_click_counter_get_state())
            {
            case HSV_CHANGE_NO:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_off;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off);
                write_hsv_actual_values(&hsv);
                read_hsv_values_for_log(&hsv);
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

            nrfx_pwm_simple_playback(&ctrl_led_pwm, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
        }

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}