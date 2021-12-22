#ifndef GPIO_TE_H
#define GPIO_TE_H

#include "macro.h"
#include "gpio.h"
#include "log.h"
#include "nrfx_gpiote.h"
#include "nrfx_systick.h"
#include "nrfx_rtc.h"

#define DOUBLE_CLICK_MAX_TIME_MS 1000  //MAX time for double click, ms
#define LONG_PRESS_MIN_TIME_MS 2000  //MIN time for long press, ms
#define BOUNCE_PROTECTION_TIME 50  //Anti-bounce guard time interval, ms

#define RTC_PRESCALER 4095  // RTC one clock is (RTC_PRESCALER + 1) / 32768     4096/32768=0.125sec
#define MILISEC_FOR_RTC(milisec) ((milisec) * 32768 / (RTC_PRESCALER + 1) / 1000)
#define DBL_CLICK_RTC_COMP 0
#define LONG_PRESS_RTC_COMP 1

typedef enum
{
    HSV_CHANGE_NO = 0,
    HSV_CHANGE_H = 1,
    HSV_CHANGE_S = 2,
    HSV_CHANGE_V = 3,
    HSV_CHANGE_MODE_QTY,
} hsv_change_mode_t;

typedef struct button_state_s
{
    nrfx_systick_state_t bounce_protection_timer;
    hsv_change_mode_t double_click_counter;
    bool is_double_click;
    bool is_long_press;
    bool is_dbl_click_timeout;
} button_state_t;

void gpiote_pin_in_config(uint32_t pin_number, void (*button_click_handler));
void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger);

void rtc_button_timer_init(nrfx_rtc_t * rtc_ptr);
void rtc_handler(nrfx_rtc_int_type_t int_type);

bool gpiote_is_new_dbl_click(void);
bool btn_is_long_press_get_state(void);
hsv_change_mode_t btn_double_click_counter_get_state(void);

#endif