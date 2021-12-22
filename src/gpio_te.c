#include "gpio_te.h"

static nrfx_rtc_t * btn_rtc_ptr;

static button_state_t button_state =
{
    .bounce_protection_timer.time = 0,
    .double_click_counter = HSV_CHANGE_NO,
    .is_double_click = false,
    .is_long_press = false,
    .is_dbl_click_timeout = true,
};

bool gpiote_is_new_dbl_click(void)
{
    bool result = button_state.is_double_click;
    button_state.is_double_click = false;
    return result;
}

bool btn_is_long_press_get_state(void)
{
    return button_state.is_long_press;
}

hsv_change_mode_t btn_double_click_counter_get_state(void)
{
    return button_state.double_click_counter;
}

static void btn_is_long_press_set(void)
{
    button_state.is_long_press = true;
}

static void btn_is_dbl_click_timeout_set(void)
{
    button_state.is_dbl_click_timeout = true;
}

void gpiote_pin_in_config(uint32_t pin_number, void (*btc_handler))
{
    nrfx_gpiote_pin_t pin_gpiote_number = pin_number;
    nrfx_gpiote_in_config_t pin_gpiote_in_config =
        {
            .sense = NRF_GPIOTE_POLARITY_TOGGLE,
            .pull = NRF_GPIO_PIN_PULLUP,
            .is_watcher = false,
            .hi_accuracy = true,
            .skip_gpio_setup = true,
        };

    if (!nrfx_gpiote_is_init())
        nrfx_gpiote_init();

    nrfx_gpiote_in_init(pin_gpiote_number, &pin_gpiote_in_config, btc_handler);
    nrfx_gpiote_in_event_enable(pin_gpiote_number, true);
}

void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t trigger)
{
    static uint8_t button_state_changes_counter = 0;

    if (!nrfx_systick_test(&button_state.bounce_protection_timer, MILITOMIKRO(BOUNCE_PROTECTION_TIME)) && !button_state.is_dbl_click_timeout)
        return;
    nrfx_systick_get(&button_state.bounce_protection_timer);

    button_state.is_long_press = false;

    if (button_state.is_dbl_click_timeout)
    {
        if (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE)
        {
            button_state.is_dbl_click_timeout = false;
            button_state_changes_counter = 1;
            nrfx_rtc_counter_clear(btn_rtc_ptr);
            nrfx_rtc_cc_set(btn_rtc_ptr, DBL_CLICK_RTC_COMP, MILISEC_FOR_RTC(DOUBLE_CLICK_MAX_TIME_MS), true);
            nrfx_rtc_cc_set(btn_rtc_ptr, LONG_PRESS_RTC_COMP, MILISEC_FOR_RTC(LONG_PRESS_MIN_TIME_MS), true);
        }
        else
        {
            button_state_changes_counter = 0;
        }
        return;
    }

    if (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE)
    {
        nrfx_rtc_cc_disable(btn_rtc_ptr, LONG_PRESS_RTC_COMP);
        nrfx_rtc_cc_set(btn_rtc_ptr, LONG_PRESS_RTC_COMP, nrfx_rtc_counter_get(btn_rtc_ptr) + MILISEC_FOR_RTC(LONG_PRESS_MIN_TIME_MS), true);
    }

    button_state_changes_counter++;

    if(button_state_changes_counter == 4)
    {
        button_state_changes_counter = 0;
        button_state.is_double_click = true;
        button_state.is_dbl_click_timeout = true;
        button_state.double_click_counter = (button_state.double_click_counter + 1) % HSV_CHANGE_MODE_QTY;
    }
}

void rtc_handler(nrfx_rtc_int_type_t int_type)
{
    if (int_type == DBL_CLICK_RTC_COMP)
    {
        btn_is_dbl_click_timeout_set();
    }
    else if (int_type == LONG_PRESS_RTC_COMP)
    {
        if (nrf_gpio_pin_read(button_pins[0]) == BUTTON_PUSH_STATE)
        {
            btn_is_long_press_set();
        }
    }
}

void rtc_button_timer_init(nrfx_rtc_t * rtc_ptr)
{
    btn_rtc_ptr = rtc_ptr;
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = RTC_PRESCALER;
    nrfx_rtc_init(btn_rtc_ptr, &config, rtc_handler);
    nrfx_rtc_enable(btn_rtc_ptr);
}
