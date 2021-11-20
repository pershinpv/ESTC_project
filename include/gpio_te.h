#ifndef GPIO_TE_H
#define GPIO_TE_H

#include "macro.h"
#include "gpio.h"
#include "nrfx_gpiote.h"
#include "nrfx_systick.h"

#define DOUBLE_CLICK_MAX_TIME 1000  //MAX time for double click, ms
#define BOUNCE_PROTECTION_TIME 20  //Anti-bounce guard time interval, ms

typedef struct button_state_s
{
    bool is_double_click;
    bool is_button_clicks_reset;
    uint32_t number_of_state_change;
    uint32_t interval_100ms_counter;
    nrfx_systick_state_t time_of_100ms_timer;
    nrfx_systick_state_t bounce_protection_timer;
} button_state_t;

void gpiote_pin_in_config(uint32_t pin_number, void (*button_click_handler));
void is_big_time_out(uint32_t timeout, nrfx_systick_state_t *timer, uint32_t *counter, bool *is_time_out);
void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger);

#endif