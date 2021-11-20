#include "pwm.h"

pwm_state_t pwm_state =
{
    .pwm_state = 0,
    .slew_direction = 1,
    .wait_time = 0,
    .time1.time = 0,
    .time2.time = 0,
    .time3.time = 0,
};

void led_pwm(blink_state_t *bstate, pwm_state_t *pstate)
{
    if (nrfx_systick_test(&(pstate->time1), pstate->wait_time))
    {
        if (pstate->wait_time == PWM_TIME)
        {
            nrfx_systick_get(&(pstate->time1));
            pstate->wait_time = PWM_TIME * pstate->pwm_state / MAX_PWM_PERCNT;
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
    if (nrfx_systick_test(&(pstate->time3), MILITOMIKRO(1)))
    {
        nrfx_systick_get(&(pstate->time3));
        bstate->left_time_ms--;
    }

    if (bstate->left_time_ms == 0)
    {
        if (bstate->id_number < deviceID[bstate->led_number] * 2)
        {
            bstate->id_number++;
            bstate->left_time_ms = flash_time_ms / 2;
            pstate->slew_direction = (bstate->id_number % 2 != 0) ? 1 : -1;
        }
        else
        {
            bstate->id_number = 0;
            bstate->led_number = (bstate->led_number + 1) % LEDS_NUMBER;
        }
    }

    if (nrfx_systick_test(&(pstate->time2), ONE_PWM_PERCNT(MILITOMIKRO(flash_time_ms) / 2)))
    {
        nrfx_systick_get(&(pstate->time2));
        pstate->pwm_state += pstate->slew_direction;
        pstate->pwm_state = CLAMP(pstate->pwm_state, MIN_PWM_PERCNT, MAX_PWM_PERCNT);

    }
}