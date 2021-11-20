#ifndef PWM_H
#define PWM_H

#include "nrfx_systick.h"
#include "gpio.h"
#include "macro.h"

#define PWM_TIME 1000 // PWM period, us. 1000 us is equal to 1 kHz.Min 100 us (equal to 10 kHz).

#define MIN_PWM_PERCNT 0
#define MAX_PWM_PERCNT 100
#define ONE_PWM_PERCNT(value) ((value) / MAX_PWM_PERCNT)

typedef struct pwm_state_s
{
    int pwm_state;         // Current value of pwm in percent
    int slew_direction;         // PWM slew direction. 1 - UP, (-1) - DOWN
    uint32_t wait_time;         // Time to next state change in PWM
    nrfx_systick_state_t time1; // Current PWM time left
    nrfx_systick_state_t time2; // Timestamp for current PWM percent calc
    nrfx_systick_state_t time3; // Timestamp for left time calc
} pwm_state_t;

void led_pwm(blink_state_t *bstate, pwm_state_t *pstate);
void led_flash_state(uint32_t deviceID[], uint32_t flash_time_ms, blink_state_t *bstate, pwm_state_t *pstate);

#endif