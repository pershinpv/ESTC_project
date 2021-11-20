#include "gpio_te.h"

button_state_t button_state =
{
    .is_double_click = false,
    .is_button_clicks_reset = false,
    .number_of_state_change = 0,
    .interval_100ms_counter = 0,
    .time_of_100ms_timer.time = 0,
    .bounce_protection_timer.time = 0,
};

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

void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger)
{
    if (!button_state.is_button_clicks_reset && !nrfx_systick_test(&button_state.bounce_protection_timer, MILITOMIKRO(BOUNCE_PROTECTION_TIME)))
        return;

    nrfx_systick_get(&button_state.bounce_protection_timer);

    if (button_state.is_button_clicks_reset)
    {
        button_state.number_of_state_change = (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE ) ? 1 : 0;
        button_state.interval_100ms_counter = 0;
        button_state.is_button_clicks_reset = false;
        return;
    }

    button_state.number_of_state_change++;

    if(button_state.number_of_state_change == 4)
    {
        button_state.number_of_state_change = 0;
        button_state.interval_100ms_counter = 0;
        button_state.is_double_click = true;
        button_state.is_button_clicks_reset = false;
    }
}