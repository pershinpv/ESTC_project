#include <stdbool.h>
#include <stdint.h>

#include "macro.h"
#include "log.h"
#include "gpio.h"
#include "gpio_te.h"
#include "pwm.h"
#include "hsv.h"

#include "nrfx_pwm.h"
#include "nrf_drv_pwm.h"

#include "nrfx_rtc.h"

#define DEVICE_ID 2222      //nRF dongle ID 6596
#define LED_TIME 1000       //LED switch on / swich off time, ms

HSV_t HSV =
{
    .h = 0,
    .s = 100,
    .v = 100
};

RGB_t RGB =
{
    .r = 100,
    .g = 0,
    .b = 0
};

button_state_t button_state =
{
    .is_double_click = false,
    .is_long_press = false,
    .is_dbl_click_timeout = true,
    .number_of_state_change = 0,
    .double_click_counter = 0,
    .bounce_protection_timer.time = 0,
};

nrfx_pwm_t ctrl_led_pwm = NRFX_PWM_INSTANCE(0);
nrfx_pwm_t rgb_led_pwm = NRFX_PWM_INSTANCE(1);

nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(0);

//----------PWM0 config begin-------------------------
nrf_pwm_values_common_t ctrl_led_seq_vals_slow[] = {0x8000, 0x8000, 0x8000, 0, 0, 0};    // One time step is 250 ms.
nrf_pwm_values_common_t ctrl_led_seq_vals_fast[] = {0x8000, 0};
nrf_pwm_values_common_t ctrl_led_seq_vals_on[] = {0x8000};
nrf_pwm_values_common_t ctrl_led_seq_vals_off[] = {0};

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
    uint16_t *p_channels = (uint16_t *)&rgb_led_seq_vals;

    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        if (button_state.is_long_press && button_state.double_click_counter > 0)
        {
            hsv_to_rgb(&HSV, &RGB);
            NRF_LOG_INFO("HSV %d %d %d", HSV.h, HSV.s, HSV.v);
            NRF_LOG_INFO("RGB %d %d %d", RGB.r, RGB.g, RGB.b);
            NRF_LOG_INFO("------------");

            if (button_state.double_click_counter == 1)
                HSV.h = (HSV.h + 1) % 361;
            else if (button_state.double_click_counter == 2)
                HSV.s = (HSV.s + 1) % 101;
            else if (button_state.double_click_counter == 3)
                HSV.v = (HSV.v + 1) % 101;
        }
    }
    p_channels[1] = rgb_led_pwm_top_val / 100 * RGB.r;
    p_channels[2] = rgb_led_pwm_top_val / 100 * RGB.g;
    p_channels[3] = rgb_led_pwm_top_val / 100 * RGB.b;
}

void rtc_handler(nrfx_rtc_int_type_t int_type)
{
    if (int_type == DBL_CLICK_RTC_COMP)
    {
        button_state.is_dbl_click_timeout = true;
    }
    else if (int_type == LONG_PRESS_RTC_COMP)
    {
        if (nrf_gpio_pin_read(button_pins[0]) == BUTTON_PUSH_STATE && !button_state.is_double_click)
        {
            button_state.is_long_press = true;
        }
    }
}

void rtc_config(void)
{
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = RTC_PRESCALER;
    nrfx_rtc_init(&rtc, &config, rtc_handler);
    nrfx_rtc_enable(&rtc);
}

int main(void)
{
    uint32_t deviceID[LEDS_NUMBER];

    logs_init();
    led_pins_init();
    button_pins_init();
    nrfx_systick_init();
    rtc_config();
    deviceID_calc(deviceID, LEDS_NUMBER, DEVICE_ID);

    gpiote_pin_in_config(button_pins[0], btn_click_handler);

    nrfx_pwm_init(&ctrl_led_pwm, &pwm_config_ctrl, NULL);
    nrfx_pwm_init(&rgb_led_pwm, &pwm_config_rgb, rgb_pwm_handler);

    nrfx_pwm_simple_playback(&ctrl_led_pwm, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
    nrfx_pwm_simple_playback(&rgb_led_pwm, &rgb_led_seq, 1, NRFX_PWM_FLAG_LOOP);

    while (true)
    {
        if (button_state.is_double_click)
        {
            button_state.is_double_click = 0;

            switch(button_state.double_click_counter)
            {
            case 0:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_off;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off);
                break;

            case 1:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_slow;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_slow);
                break;

            case 2:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_fast;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_fast);
                break;

            case 3:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_on;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_on);
                break;
            }

            nrfx_pwm_simple_playback(&ctrl_led_pwm, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
            //NRF_LOG_INFO("Button state is changed. Current state is %d", pwm_state.time2.time);
        }

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}