#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "macro.h"
#include "log.h"
#include "gpio.h"
#include "gpio_te.h"
#include "pwm.h"
#include "color.h"
#include "nvmc.h"
#include "cli_usb.h"
#include "cli_cmd.h"

#include "nrfx_pwm.h"
#include "nrf_drv_pwm.h"

#include "nrfx_rtc.h"

#define DEVICE_ID 2222      //nRF dongle ID 6596
#define LED_TIME 1000       //LED switch on / swich off time, ms
#define BUTTON_RTC_INSTANCE 0
#define CTRL_LED_PWM_INSTANCE 0
#define RGB_LED_PWM_INSTANCE 1

#define CLEAR_WORK_PAGES 0  //Clear work pages at start if TRUE

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(BUTTON_RTC_INSTANCE);

static nrfx_pwm_t ctrl_led_pwm = NRFX_PWM_INSTANCE(CTRL_LED_PWM_INSTANCE);
static nrfx_pwm_t rgb_led_pwm = NRFX_PWM_INSTANCE(RGB_LED_PWM_INSTANCE);

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

#if CLEAR_WORK_PAGES
    nrfx_nvmc_page_erase(APP_DATA_START_ADDR);
    nrfx_nvmc_page_erase(APP_DATA_START_ADDR + PAGE_SIZE_BYTES);
#endif

    color_restore_rgb_last_state();
    color_ctrl_led_pwm_init(&ctrl_led_pwm);
    color_rgb_led_pwm_init(&rgb_led_pwm);

    cli_usb_cdc_acm_init(cli_cmd_handler);

    while (true)
    {
        while (app_usbd_event_queue_process())
        {
            //NRF_LOG_INFO("Nothing to do");
        }

        if (gpiote_is_new_dbl_click())
        {
            //NRF_LOG_INFO("sizeof %d %d", sizeof("Wrong command") / sizeof(char), sizeof(char));
            color_set_work_mode(btn_double_click_counter_get_state());
        }

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}