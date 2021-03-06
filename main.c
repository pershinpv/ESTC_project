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
#include "estc_ble.h"
#include "estc_service.h"
#include "estc_fstorage.h"

#include "nrfx_pwm.h"
#include "nrf_drv_pwm.h"
//#include "nrfx_nvmc.h"
#include "estc_fstorage.h"

#include "nrfx_rtc.h"

#define DEVICE_ID 6596      //nRF dongle ID 6596
#define LED_TIME 1000       //LED switch on / swich off time, ms
#define BUTTON_RTC_INSTANCE 2
#define CTRL_LED_PWM_INSTANCE 0
#define RGB_LED_PWM_INSTANCE 1

#define CLEAR_WORK_PAGES 0  //Clear work pages at start if "1"

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(BUTTON_RTC_INSTANCE);

static nrfx_pwm_t ctrl_led_pwm = NRFX_PWM_INSTANCE(CTRL_LED_PWM_INSTANCE);
static nrfx_pwm_t rgb_led_pwm = NRFX_PWM_INSTANCE(RGB_LED_PWM_INSTANCE);

int main(void)
{
#if CLEAR_WORK_PAGES
    nrfx_nvmc_page_erase(APP_DATA_START_ADDR);
    nrfx_nvmc_page_erase(APP_DATA_START_ADDR + PAGE_SIZE_BYTES);
#endif

    uint32_t deviceID[LEDS_NUMBER];

    logs_init();
    led_pins_init();
    button_pins_init();
    nrfx_systick_init();
    estc_fstorage_init();
    //nvmc_flash_init();
    rtc_button_timer_init(&rtc);
    deviceID_calc(deviceID, LEDS_NUMBER, DEVICE_ID);

    gpiote_pin_in_config(button_pins[0], btn_click_handler);

    color_restore_rgb_last_state();
    color_ctrl_led_pwm_init(&ctrl_led_pwm);
    color_rgb_led_pwm_init(&rgb_led_pwm);

    timers_init();
    //power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();

    estc_ble_timers_init();
    advertising_start();

#if ESTC_USB_CLI_ENABLED
    cli_usb_cdc_acm_init(cli_cmd_handler);
#endif

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
        //idle_state_handle();
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}