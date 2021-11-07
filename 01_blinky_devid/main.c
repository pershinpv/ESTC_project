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

uint32_t digit_capacity(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void led_pins_init(uint32_t led_pins[])
{
    for (int i = 0; i < LEDS_NUMBER; i++)       //sizeof(*led_pins) is equal to LEDS_NUMBER ???
    {
        nrf_gpio_cfg_output(led_pins[i]);
        nrf_gpio_pin_write(led_pins[i], LED_ON_PORT_STATE ? 0 : 1);
    }
}

void button_pin_init(uint32_t button_pin[])
{
    for (int i = 0; i < BUTTONS_NUMBER; i++)
        nrf_gpio_cfg_input(button_pin[i], NRF_GPIO_PIN_PULLUP);
}

uint32_t get_button_state(uint32_t button_pin)
{
    uint32_t state;
    do
    {
        state = nrf_gpio_pin_read(button_pin);
        nrfx_coredep_delay_us(1000);
    } while (state != nrf_gpio_pin_read(button_pin));
    return state;
}

void led_flash(uint32_t led_pin, uint32_t flash_time_ms)
{
    nrf_gpio_pin_toggle(led_pin);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
    nrf_gpio_pin_toggle(led_pin);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
}

uint32_t led_flash_on_button(uint32_t led_pin, uint32_t flash_time_ms, uint32_t button_pin)
{
    uint32_t time_left;
    uint32_t last_state = nrf_gpio_pin_read(button_pin);
    uint32_t timeout = 1 << 27; // It is about 35 sec

    for (int i = 0; i < 2; i++)
    {
        nrf_gpio_pin_toggle(led_pin);
        time_left = flash_time_ms;

        while (time_left && timeout)
        {
            if (last_state != nrf_gpio_pin_read(button_pin))
                last_state = get_button_state(button_pin);

            if (last_state == BUTTON_PUSH_STATE)
            {
                nrfx_coredep_delay_us(1000);
                time_left--;
            }
            if (!--timeout)
                    time_left = 0;
        }
    }
    return timeout;
}

int main(void)
{
    uint32_t digits = digit_capacity(DEVICE_ID);
    uint32_t dev_id = DEVICE_ID;
    uint32_t digtmp = digits;

    uint32_t led_pins[] = LED_PIN_NUMBERS;
    uint32_t button_pins[] = BUTTON_PIN_NUMBERS;

    /* Configure led pins. */
    led_pins_init(led_pins);

    /* Configure button pins. */
    button_pin_init(button_pins);

    /* Toggle LEDs as DEVICE_ID */
    while (true)
    {
        dev_id = DEVICE_ID;
        digtmp = digits;

        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            for (int j = 0; j < dev_id / digtmp; )
            {
                if (get_button_state(button_pins[0]) == BUTTON_PUSH_STATE)
                {
                    //led_flash(led_pins[i], LED_TIME);     //Base version of dependency from button
                    if (led_flash_on_button(led_pins[i], LED_TIME, button_pins[0]) > 0)
                        j++;
                    else
                        i = j = 0;
                }
            }
            dev_id %= digtmp;
            digtmp /= 10;
        }
    }
}