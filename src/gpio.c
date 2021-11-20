#include "gpio.h"

blink_state_t blink_state =
{
    .id_number = 0,
    .led_number = 0,
    .left_time_ms = 0,
};

void led_pins_init()
{
    for (int i = 0; i < LEDS_NUMBER; i++)
    {
        nrf_gpio_cfg_output(led_pins[i]);
        nrf_gpio_pin_write(led_pins[i], LED_ON_PORT_STATE ? 0 : 1);
    }
}

void button_pins_init()
{
    for (int i = 0; i < BUTTONS_NUMBER; i++)
        nrf_gpio_cfg_input(button_pins[i], NRF_GPIO_PIN_PULLUP);
}

uint32_t get_button_state(uint32_t button_pin)
{
    uint32_t state;
    do
    {
        state = nrf_gpio_pin_read(button_pin);
        nrfx_coredep_delay_us(50*1000);
    } while (state != nrf_gpio_pin_read(button_pin));
    return state;
}

void led_flash(uint32_t led_pin, uint32_t flash_time_ms, uint32_t num)
{
    for (int i = 0; i < num * 2; i++)
    {
        nrf_gpio_pin_toggle(led_pin);
        nrfx_coredep_delay_us(flash_time_ms * 1000);
    }
}

void led_flash_on_button(uint32_t deviceID[], uint32_t flash_time_ms, blink_state_t *state)
{
    if (state->left_time_ms == 0)
    {
        if (state->id_number < deviceID[state->led_number] * 2)
        {
            state->id_number++;
            state->left_time_ms = flash_time_ms;
            nrf_gpio_pin_toggle(led_pins[state->led_number]);
        }
        else
        {
            state->id_number = 0;
            state->left_time_ms = 1;

            if (state->led_number < LEDS_NUMBER - 1)
                state->led_number++;
            else
                state->led_number = 0;
        }
    }

    nrfx_coredep_delay_us(1000);
    (state->left_time_ms)--;
}