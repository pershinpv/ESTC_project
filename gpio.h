#ifndef GPIO_H
#define GPIO_H

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nordic_common.h"
#include "boards.h"

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

typedef struct blink_state_s
{
    uint32_t id_number;     // Current value of number of led flashes
    uint32_t led_number;    // Current led number
    uint32_t left_time_ms;  // Left time of running flash
} blink_state_t;

uint32_t get_button_state(uint32_t button_pin);
void led_flash(uint32_t led_pin, uint32_t flash_time_ms, uint32_t num);
void led_flash_on_button(uint32_t deviceID[], uint32_t flash_time_ms, blink_state_t *state);

void led_pins_init();
void button_pins_init();

static const uint32_t led_pins[] = LED_PIN_NUMBERS;
static const uint32_t button_pins[] = BUTTON_PIN_NUMBERS;

#endif