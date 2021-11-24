#include "gpio_te.h"

extern button_state_t button_state;

void gpiote_pin_in_config(uint32_t pin_number, void (*btc_handler))
{
    nrfx_gpiote_pin_t pin_gpiote_number = pin_number;
    nrfx_gpiote_in_config_t pin_gpiote_in_config =
        {
            .sense = NRF_GPIOTE_POLARITY_TOGGLE,
            .pull = NRF_GPIO_PIN_PULLUP,
            .is_watcher = false,
            .hi_accuracy = true,
            .skip_gpio_setup = true,
        };

    if (!nrfx_gpiote_is_init())
        nrfx_gpiote_init();

    nrfx_gpiote_in_init(pin_gpiote_number, &pin_gpiote_in_config, btc_handler);
    nrfx_gpiote_in_event_enable(pin_gpiote_number, true);
}

void is_big_time_out(uint32_t timeout, nrfx_systick_state_t *timer, uint32_t *counter, bool *is_time_out)
{
    if (nrfx_systick_test(timer, MILITOMIKRO(100)))
    {
        nrfx_systick_get(timer);

        if (*counter < timeout / 100)
        {
            ++(*counter);
            *is_time_out = false;
        }
        else
            *is_time_out = true;
    }
}

extern nrfx_rtc_t rtc;
void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger)
{
    if (!button_state.is_dbl_click_timeout && !nrfx_systick_test(&button_state.bounce_protection_timer, MILITOMIKRO(BOUNCE_PROTECTION_TIME)))
        return;

    nrfx_systick_get(&button_state.bounce_protection_timer);

    button_state.is_long_press = false;

    if (button_state.is_dbl_click_timeout)
    {
        if (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE)
        {
            button_state.is_dbl_click_timeout = false;
            button_state.number_of_state_change = 1;
            nrfx_rtc_counter_clear(&rtc);
            nrfx_rtc_cc_set(&rtc, DBL_CLICK_RTC_COMP, MILISEC_FOR_RTC(DOUBLE_CLICK_MAX_TIME), true);
            nrfx_rtc_cc_set(&rtc, LONG_PRESS_RTC_COMP, MILISEC_FOR_RTC(LONG_PRESS_MIN_TIME), true);
        }
        else
        {
            button_state.number_of_state_change = 0;
        }
        return;
    }

    if (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE)
    {
        nrfx_rtc_cc_disable(&rtc, LONG_PRESS_RTC_COMP);
        nrfx_rtc_cc_set(&rtc, LONG_PRESS_RTC_COMP, nrfx_rtc_counter_get(&rtc) + MILISEC_FOR_RTC(LONG_PRESS_MIN_TIME), true);
    }

    button_state.number_of_state_change++;

    if(button_state.number_of_state_change == 4)
    {
        button_state.number_of_state_change = 0;
        button_state.is_double_click = true;
        button_state.is_dbl_click_timeout = true;
        button_state.double_click_counter = (button_state.double_click_counter + 1) % 4;
    }
}