#ifndef GPIO_TE_H
#define GPIO_TE_H

#include "macro.h"
#include "gpio.h"
#include "nrfx_gpiote.h"
#include "nrfx_systick.h"
#include "nrfx_rtc.h"

#define DOUBLE_CLICK_MAX_TIME 1000  //MAX time for double click, ms
#define LONG_PRESS_MIN_TIME 3000  //MIN time for long press, ms
#define BOUNCE_PROTECTION_TIME 50  //Anti-bounce guard time interval, ms

#define RTC_PRESCALER 4095  // RTC one clock is (RTC_PRESCALER + 1) / 32768     4096/32768=0.125sec
#define MILISEC_FOR_RTC(milisec) ((milisec) * 32768 / (RTC_PRESCALER + 1) / 1000)
#define DBL_CLICK_RTC_COMP 0
#define LONG_PRESS_RTC_COMP 1

typedef struct button_state_s
{
    bool is_double_click;
    bool is_long_press;
    bool is_dbl_click_timeout;
    uint32_t number_of_state_change;
    uint32_t double_click_counter;
    nrfx_systick_state_t bounce_protection_timer;
} button_state_t;

void gpiote_pin_in_config(uint32_t pin_number, void (*button_click_handler));
void is_big_time_out(uint32_t timeout, nrfx_systick_state_t *timer, uint32_t *counter, bool *is_time_out);
void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger);

#endif