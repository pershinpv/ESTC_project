#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "log.h"
#include "nrfx_systick.h"

#define DEVICE_ID 2222 //nRF dongle ID 6596
#define LED_TIME 1500   //LED switch on / swich off time, ms
#define PWM_TIME 1000 // PWM period, us. 1000 us is equal to 1 kHz.Min 100 us (equal to 10 kHz).

uint32_t digit_capacity(uint32_t numeric);
void deviceID_define(void);

uint32_t deviceID[LEDS_NUMBER];

typedef struct pwm_state_s
{
    uint32_t pwm_state;         // Current value of pwm in percent
    int slew_direction;         // PWM slew direction. 1 - UP, (-1) - DOWN
    uint32_t wait_time;         // Time to next state change in PWM
    nrfx_systick_state_t time1; // Current PWM time left
    nrfx_systick_state_t time2; // Timestamp for current PWM percent calc
    nrfx_systick_state_t time3; // Timestamp for left time calc
} pwm_state_t;

void led_pwm(blink_state_t *bstate, pwm_state_t *pstate)
{
    if (nrfx_systick_test(&(pstate->time1), pstate->wait_time))
    {
        if (pstate->wait_time == PWM_TIME)
        {
            nrfx_systick_get(&(pstate->time1));
            pstate->wait_time = pstate->pwm_state * PWM_TIME / 100;
            if (pstate->pwm_state > 10)
                nrf_gpio_pin_write(led_pins[bstate->led_number], LED_ON_PORT_STATE);
        }
        else
        {
            nrf_gpio_pin_write(led_pins[bstate->led_number], !LED_ON_PORT_STATE);
            pstate->wait_time = PWM_TIME;
        }
    }
}

void led_flash_state(uint32_t deviceID[], uint32_t flash_time_ms, blink_state_t *bstate, pwm_state_t *pstate)
{
    if (bstate->left_time_ms == 0)
    {
        if (bstate->id_number < deviceID[bstate->led_number] * 2)
        {
            bstate->id_number++;
            bstate->left_time_ms = flash_time_ms / 2;

            if(bstate->id_number % 2 != 0)
                pstate->slew_direction = 1;
            else
                pstate->slew_direction = -1;
        }
        else
        {
            bstate->id_number = 0;
            bstate->left_time_ms = 0;

            if (bstate->led_number < LEDS_NUMBER - 1)
                bstate->led_number++;
            else
                bstate->led_number = 0;
        }
    }

    if (nrfx_systick_test(&(pstate->time2), flash_time_ms * 1000 / 100 / 2))
    {
        nrfx_systick_get(&(pstate->time2));

        if (pstate->pwm_state > 0 && pstate->pwm_state < 100)
            pstate->pwm_state += pstate->slew_direction;

        if (pstate->pwm_state == 0 && pstate->slew_direction > 0)
            pstate->pwm_state++;

        if (pstate->pwm_state == 100 && pstate->slew_direction < 0)
            pstate->pwm_state--;
    }

    if (nrfx_systick_test(&(pstate->time3), 1000))
    {
        nrfx_systick_get(&(pstate->time3));
        bstate->left_time_ms--;
    }
}

int main(void)
{
    logs_init();
    led_pins_init();
    button_pins_init();
    deviceID_define();
    nrfx_systick_init();

    blink_state_t blink_state = {0, 0, 0};
    uint32_t button_last_state = BUTTON_PUSH_STATE ^ 1UL;

    pwm_state_t pwm_state = {0, 1, 0};
    nrfx_systick_get(&(pwm_state.time1));
    nrfx_systick_get(&(pwm_state.time2));
    nrfx_systick_get(&(pwm_state.time3));

    while (true)
    {
        if (nrf_gpio_pin_read(button_pins[0]) != button_last_state)
        {
            //button_last_state = get_button_state(button_pins[0]);
            button_last_state = nrf_gpio_pin_read(button_pins[0]);
            nrfx_systick_get(&(pwm_state.time2));
            nrfx_systick_get(&(pwm_state.time3));
            NRF_LOG_INFO("Button state is changed. Current state is %d", pwm_state.time2.time);
        }

        if (button_last_state == BUTTON_PUSH_STATE)
            led_flash_state(deviceID, LED_TIME, &blink_state, &pwm_state);

        led_pwm(&blink_state, &pwm_state);

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