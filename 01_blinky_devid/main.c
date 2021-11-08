#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"
#include "nrf_gpio.h"

#define DEVICE_ID 6596 //nRF dongle ID 6596
#define LED_TIME 500   //LED switch on / swich off time, ms

/*Define PCA10059 LED pins*/
#define LED1G   NRF_GPIO_PIN_MAP(0,6)    //Port (0,6) is equal to pin_number 6
#define LED2R   NRF_GPIO_PIN_MAP(0,8)    //Port (0,8) is equal to pin_number 8
#define LED2G   NRF_GPIO_PIN_MAP(1,9)    //Port (1,9) is equal to pin_number 41
#define LED2B   NRF_GPIO_PIN_MAP(0,12)   //Port (0,12) is equal to pin_number 12

#define LED_PIN_NUMBERS {LED1G, LED2R, LED2G, LED2B}
#define LED_ON_PORT_STATE 0             //GPIO port state for LED ON

/*Define PCA10059 Buttons pins*/
#define BUTTON1    NRF_GPIO_PIN_MAP(1,6)    //Port (1,9) is equal to pin_number 38

#define BUTTON_PIN_NUMBERS { BUTTON1 }
#define BUTTON_PUSH_STATE 0                //GPIO port state for PUSH BUTTON

uint32_t led_pins[] = LED_PIN_NUMBERS;
uint32_t button_pins[] = BUTTON_PIN_NUMBERS;

struct blink_state
{
    uint32_t id_number;
    uint32_t led_number;
    uint32_t left_time_ms;
};

uint32_t digit_capacity(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void led_pins_init()
{
    for (int i = 0; i < LEDS_NUMBER; i++)       //sizeof(*led_pins) is equal to LEDS_NUMBER ???
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

void led_flash_on_button(uint32_t deviceID[], uint32_t flash_time_ms, struct blink_state *state)
{
    if (!state->left_time_ms)
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

            if (state->led_number < LEDS_NUMBER)
                state->led_number++;
            else
                state->led_number = 0;
        }
    }

    nrfx_coredep_delay_us(1000);
    (state->left_time_ms)--;
}

int main(void)
{
    struct blink_state blink_state_s = {0, 0, 0};
    uint32_t button_last_state = BUTTON_PUSH_STATE ^ 1UL;

    uint32_t digits = digit_capacity(DEVICE_ID);
    uint32_t dev_id = DEVICE_ID;

    uint32_t deviceID[LEDS_NUMBER];

    for (uint32_t i = 0; i < LEDS_NUMBER; i++)
    {
        deviceID[i] = dev_id / digits;
        dev_id %= digits;
        if (digits/10)
            digits /= 10;
    }

    /* Configure led pins. */
    led_pins_init();

    /* Configure button pins. */
    button_pins_init();

    while (true)
    {
        if (nrf_gpio_pin_read(button_pins[0]) != button_last_state)
            button_last_state = get_button_state(button_pins[0]);

        if (button_last_state == BUTTON_PUSH_STATE)
            led_flash_on_button(deviceID, LED_TIME, &blink_state_s);
    }
}