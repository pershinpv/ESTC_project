#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#define DEVICE_ID 6596 //nRF dongle ID
#define LED_TIME 400   //LED switch on / swich off time, ms

/*Define PCA10059 LED pins*/
#define LED1G   NRF_GPIO_PIN_MAP(0,6)    //Port (0,6) is equal to pin_number 6
#define LED2R   NRF_GPIO_PIN_MAP(0,8)    //Port (0,8) is equal to pin_number 8
#define LED2G   NRF_GPIO_PIN_MAP(1,9)    //Port (1,9) is equal to pin_number 41
#define LED2B   NRF_GPIO_PIN_MAP(0,12)   //Port (0,12) is equal to pin_number 12

#define LED_PIN_NUMBERS {LED1G, LED2R, LED2G, LED2B}
#define LED_ON_PORT_STATE 0             //GPIO port state for LED ON

uint32_t dig_cap(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void led_pins_init(uint32_t led_pins[])
{
    for (int i = 0; i < LEDS_NUMBER; i++)
    {
        nrf_gpio_cfg_output(led_pins[i]);
        nrf_gpio_pin_write(led_pins[i], LED_ON_PORT_STATE ? 0 : 1);
    }
}

void led_flash(uint32_t led_idx, uint32_t flash_time_ms)
{
    nrf_gpio_pin_toggle(led_idx);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
    nrf_gpio_pin_toggle(led_idx);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
}

int main(void)
{
    uint32_t digits = dig_cap(DEVICE_ID); //DEVICE_ID digit capacity
    uint32_t dev_id = DEVICE_ID;
    uint32_t digtmp = digits;

    uint32_t led_pins[LEDS_NUMBER] = LED_PIN_NUMBERS;

    /* Configure led pins. */
    led_pins_init(led_pins);

    /* Toggle LEDs as DEVICE_ID */
    while (true)
    {
        dev_id = DEVICE_ID;
        digtmp = digits;

        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            for (int j = 0; j < dev_id / digtmp; j++)
            {
                led_flash(led_pins[i], LED_TIME);
            }
            dev_id %= digtmp;
            digtmp /= 10;
        }
    }
}