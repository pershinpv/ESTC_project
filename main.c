#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "log.h"

#define DEVICE_ID 6596 //nRF dongle ID 6596
#define LED_TIME 400   //LED switch on / swich off time, ms

uint32_t digit_capacity(uint32_t numeric);
void deviceID_define(void);

uint32_t deviceID[LEDS_NUMBER];

int main(void)
{
    logs_init();
    led_pins_init();
    button_pins_init();
    deviceID_define();

    blink_state_t blink_state = {0, 0, 0};
    uint32_t button_last_state = BUTTON_PUSH_STATE ^ 1UL;
    uint32_t led_last_position = LEDS_NUMBER;

    while (true)
    {
        if (nrf_gpio_pin_read(button_pins[0]) != button_last_state)
            button_last_state = get_button_state(button_pins[0]);

        if (button_last_state == BUTTON_PUSH_STATE)
        {
            led_flash_on_button(deviceID, LED_TIME, &blink_state);

            if (led_last_position != blink_state.led_number)
            {
                led_last_position = blink_state.led_number;
                NRF_LOG_INFO("Button is pushed. Led number %d", led_last_position + 1);
            }
        }
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

uint32_t digit_capacity(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void deviceID_define(void)
{
    uint32_t digits = digit_capacity(DEVICE_ID);
    uint32_t dev_id = DEVICE_ID;

    for (uint32_t i = 0; i < LEDS_NUMBER; i++)
    {
        deviceID[i] = dev_id / digits;
        dev_id %= digits;
        if (digits/10 > 0)
            digits /= 10;
    }
}