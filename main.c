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
#include "cdc_acm_ctrl.h"

#define DEVICE_ID 2222      //nRF dongle ID 6596
#define LED_TIME 1000       //LED switch on / swich off time, ms
#define BUTTON_RTC_INSTANCE 0

static hsv_t hsv;
static rgb_t rgb;

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(BUTTON_RTC_INSTANCE);

static nrfx_pwm_t ctrl_led_pwm = NRFX_PWM_INSTANCE(0);
static nrfx_pwm_t rgb_led_pwm = NRFX_PWM_INSTANCE(1);
static void process_cli_command(cli_command_t command);

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

uint16_t const rgb_led_pwm_top_val = 25000;
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
            // NRF_LOG_INFO("hsv %d %d %d", hsv.h, hsv.s, hsv.v);
            // NRF_LOG_INFO("rgb %d %d %d", rgb.r, rgb.g, rgb.b);
            // NRF_LOG_INFO("------------");

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
    cli_command_t main_cli_command;

    logs_init();
    led_pins_init();
    button_pins_init();
    nrfx_systick_init();
    rtc_button_timer_init(&rtc);
    deviceID_calc(deviceID, LEDS_NUMBER, DEVICE_ID);

    gpiote_pin_in_config(button_pins[0], btn_click_handler);

    nvmc_read_hsv_actual_values(&hsv);
    if (!hsv_values_validate(&hsv))
        hsv_values_init(&hsv);
    hsv_to_rgb(&hsv, &rgb);

    nrfx_pwm_init(&ctrl_led_pwm, &pwm_config_ctrl, NULL);
    nrfx_pwm_init(&rgb_led_pwm, &pwm_config_rgb, rgb_pwm_handler);

    nrfx_pwm_simple_playback(&ctrl_led_pwm, &ctrl_led_seq, 1, NRFX_PWM_FLAG_LOOP);
    nrfx_pwm_simple_playback(&rgb_led_pwm, &rgb_led_seq, 1, NRFX_PWM_FLAG_LOOP);

    cli_cdc_acm_connect_ep();

    while (true)
    {
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }

        main_cli_command = cli_command_to_do_get();

        if (main_cli_command.comand_type != CLI_NO_COMMAND)
        {
            process_cli_command(main_cli_command);
            cli_command_reset();
            btn_is_dbl_click_set();
            btn_double_click_counter_reset();
        }

        if (btn_is_dbl_click_get_state())
        {
            btn_is_dbl_click_reset();
            //NRF_LOG_INFO("sizeof %d %d", sizeof("Wrong command") / sizeof(char), sizeof(char));

            switch(btn_double_click_counter_get_state())
            {
            case HSV_CHANGE_NO:
                ctrl_led_seq.values.p_common = ctrl_led_seq_vals_off;
                ctrl_led_seq.length = NRF_PWM_VALUES_LENGTH(ctrl_led_seq_vals_off);
                nvmc_write_hsv_actual_values(&hsv);
                nvmc_read_hsv_values_for_log();
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

static void process_cli_command(cli_command_t command)
{
    static char *help_msg[CLI_COMMANDS_QTY];
    static uint8_t help_msg_len;

    switch (command.comand_type)
    {
    case CLI_SET_RGB:
        rgb.r = (uint8_t)(command.param_1 * RGB_MAX_VAL / CLI_RGB_MAX_VAL);
        rgb.g = (uint8_t)(command.param_2 * RGB_MAX_VAL / CLI_RGB_MAX_VAL);
        rgb.b = (uint8_t)(command.param_3 * RGB_MAX_VAL / CLI_RGB_MAX_VAL);
        rgb_to_hsv(&hsv, &rgb);
        cli_send_result_message();
        break;

    case CLI_SET_HSV:
        hsv.h = (uint8_t) command.param_1;
        hsv.s = (uint8_t) command.param_2;
        hsv.v = (uint8_t) command.param_3;
        hsv_to_rgb(&hsv, &rgb);
        cli_send_result_message();
        break;

    case CLI_HELP:

        help_msg[0] = "Support commands:";
        help_msg[1] = "1. RGB <r> <g> <b>. r, g, b: (0...255)";
        help_msg[2] = "2. HSV <h> <s> <v>. h: (0...359); s, v: (0...100)";
        help_msg[3] = "3. 'help' to see this list";

        for (size_t i = 0; i < CLI_COMMANDS_QTY; ++i)
        {
            help_msg_len = 0;
            while (help_msg[i][help_msg_len] != '\0')
                ++help_msg_len;
            cli_send_message(help_msg[i], ++help_msg_len, true);
        }
        break;

    default:
        break;
    }
}