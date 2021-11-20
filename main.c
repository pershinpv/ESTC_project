#include <stdbool.h>
#include <stdint.h>

#include "macro.h"
#include "log.h"
#include "gpio.h"
#include "gpio_te.h"
#include "pwm.h"

#define DEVICE_ID 2222 //nRF dongle ID 6596
#define LED_TIME 1000   //LED switch on / swich off time, ms

int main(void)
{
    uint32_t deviceID[LEDS_NUMBER];
    extern blink_state_t blink_state;
    extern pwm_state_t pwm_state;
    extern button_state_t button_state;
    bool is_go_flash = false;

    logs_init();
    led_pins_init();
    button_pins_init();
    nrfx_systick_init();
    deviceID_calc(deviceID, LEDS_NUMBER, DEVICE_ID);

    gpiote_pin_in_config(button_pins[0], btn_click_handler);

    while (true)
    {
        if (button_state.is_double_click)
        {
            nrfx_systick_get(&(pwm_state.time2));
            pwm_state.time3  = pwm_state.time2;
            is_go_flash = !is_go_flash;
            button_state.is_double_click = 0;
            //NRF_LOG_INFO("Button state is changed. Current state is %d", pwm_state.time2.time);
        }

        if (is_go_flash)
            led_flash_state(deviceID, LED_TIME, &blink_state, &pwm_state);

        led_pwm(&blink_state, &pwm_state);
        is_big_time_out(DOUBLE_CLICK_MAX_TIME, &button_state.time_of_100ms_timer, &button_state.interval_100ms_counter, &button_state.is_button_clicks_reset);

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}