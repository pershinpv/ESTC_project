#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "log.h"
#include "nrfx_systick.h"
#include "nrfx_gpiote.h"

#define DEVICE_ID 2222 //nRF dongle ID 6596
#define LED_TIME 1500   //LED switch on / swich off time, ms
#define PWM_TIME 1000 // PWM period, us. 1000 us is equal to 1 kHz.Min 100 us (equal to 10 kHz).

#define DOUBLE_CLICK_MAX_TIME 1000  //MAX time for double click, ms
#define BOUNCE_PROTECTION_TIME 20  //Anti-bounce guard time interval, ms

#define MILITOMIKRO(time_ms) ((time_ms)*1000)
#define MIN_PWM_PERCNT 0
#define MAX_PWM_PERCNT 100
#define ONE_PWM_PERCNT(value) ((value) / MAX_PWM_PERCNT)
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

uint32_t digit_capacity(uint32_t numeric);
void deviceID_define(void);

uint32_t deviceID[LEDS_NUMBER];

typedef struct pwm_state_s
{
    int pwm_state;         // Current value of pwm in percent
    int slew_direction;         // PWM slew direction. 1 - UP, (-1) - DOWN
    uint32_t wait_time;         // Time to next state change in PWM
    nrfx_systick_state_t time1; // Current PWM time left
    nrfx_systick_state_t time2; // Timestamp for current PWM percent calc
    nrfx_systick_state_t time3; // Timestamp for left time calc
} pwm_state_t;

typedef struct button_state_s
{
    bool is_double_click;
    bool is_button_clicks_reset;
    uint32_t number_of_state_change;
    uint32_t interval_100ms_counter;
    nrfx_systick_state_t time_of_100ms_timer;
    nrfx_systick_state_t bounce_protection_timer;
} button_state_t;

pwm_state_t pwm_state =
{
    .pwm_state = 0,
    .slew_direction = 1,
    .wait_time = 0,
    .time1.time = 0,
    .time2.time = 0,
    .time3.time = 0,
};

button_state_t button_state =
{
    .is_double_click = false,
    .is_button_clicks_reset = false,
    .number_of_state_change = 0,
    .interval_100ms_counter = 0,
    .time_of_100ms_timer.time = 0,
    .bounce_protection_timer.time = 0,
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

void btn_click_handler(uint32_t button_pin, nrf_gpiote_polarity_t DNU_2)
{
    if (!button_state.is_button_clicks_reset && !nrfx_systick_test(&button_state.bounce_protection_timer, MILITOMIKRO(BOUNCE_PROTECTION_TIME)))
        return;

    nrfx_systick_get(&button_state.bounce_protection_timer);

    if (button_state.is_button_clicks_reset)
    {
        button_state.number_of_state_change = (nrf_gpio_pin_read(button_pin) == BUTTON_PUSH_STATE ) ? 1 : 0;
        button_state.interval_100ms_counter = 0;
        button_state.is_button_clicks_reset = false;
        return;
    }

    button_state.number_of_state_change++;

    if(button_state.number_of_state_change == 4)
    {
        button_state.number_of_state_change = 0;
        button_state.interval_100ms_counter = 0;
        button_state.is_double_click = true;
        button_state.is_button_clicks_reset = false;
    }
}

void button_clicks_reset_timer(void)
{
    if (nrfx_systick_test(&button_state.time_of_100ms_timer, MILITOMIKRO(100)))
    {
        nrfx_systick_get(&button_state.time_of_100ms_timer);
        if (button_state.interval_100ms_counter < DOUBLE_CLICK_MAX_TIME / 100)
            button_state.interval_100ms_counter++;
        else
            button_state.is_button_clicks_reset = true;
    }
}

void gpiote_pin_in_config(uint32_t pin_number)
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

    nrfx_gpiote_in_init(pin_gpiote_number, &pin_gpiote_in_config, btn_click_handler);
    nrfx_gpiote_in_event_enable(pin_gpiote_number, true);
}


int main(void)
{
    logs_init();
    led_pins_init();
    button_pins_init();
    deviceID_define();
    nrfx_systick_init();

    gpiote_pin_in_config(button_pins[0]);

    blink_state_t blink_state = {0, 0, 0};
    bool is_go_flash = 0;

    while (true)
    {
        if (button_state.is_double_click)
        {
            nrfx_systick_get(&(pwm_state.time2));
            pwm_state.time3  = pwm_state.time2;
            is_go_flash = !is_go_flash;
            button_state.is_double_click = 0;
            //NRF_LOG_INFO("Button state is changed. Current state is %d", pwm_state.time2.time);
        }

        if (is_go_flash)
            led_flash_state(deviceID, LED_TIME, &blink_state, &pwm_state);

        led_pwm(&blink_state, &pwm_state);
        button_clicks_reset_timer();

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