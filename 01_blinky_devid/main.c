/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#define DEVICE_ID 6596 //nRF dongle ID
#define LED_TIME 400   //LED switch on / swich off time, ms


/*Define PCA10059 LED pins*/
#define LED1G   NRF_GPIO_PIN_MAP(0,6)    //Port (0,6) is equal to pin_number 6
#define LED2R   NRF_GPIO_PIN_MAP(0,8)    //Port (0,8) is equal to pin_number 8
#define LED2G   NRF_GPIO_PIN_MAP(1,9)    //Port (1,9) is equal to pin_number 41
#define LED2B   NRF_GPIO_PIN_MAP(0,12)   //Port (0,12) is equal to pin_number 12

#define LED_PIN_NUMBERS {LED1G, LED2R, LED2G, LED2B}

#define LED_ON_PORT_STATE 0             //GPIO port state for LED ON

uint32_t dig_cap(uint32_t numeric)
{
    uint32_t capacity = 1;
    while (numeric /= 10)
        capacity *= 10;
    return capacity;
}

void led_pins_init(uint32_t led_pins[])
{
    for (int i = 0; i < LEDS_NUMBER; i++)
    {
        nrf_gpio_cfg_output(led_pins[i]);
        nrf_gpio_pin_write(led_pins[i], LED_ON_PORT_STATE ? 0 : 1);
    }
}

void led_flash(uint32_t led_idx, uint32_t flash_time_ms)
{
    nrf_gpio_pin_toggle(led_idx);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
    nrf_gpio_pin_toggle(led_idx);
    nrfx_coredep_delay_us(flash_time_ms * 1000);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t digits = dig_cap(DEVICE_ID); //DEVICE_ID digit capacity
    uint32_t dev_id = DEVICE_ID;
    uint32_t digtmp = digits;

    uint32_t led_pins[LEDS_NUMBER] = LED_PIN_NUMBERS;

    /* Configure led pins. */
    led_pins_init(led_pins);

    /* Toggle LEDs as DEVICE_ID */
    while (true)
    {
        dev_id = DEVICE_ID;
        digtmp = digits;

        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            for (int j = 0; j < dev_id / digtmp; j++)
            {
                led_flash(led_pins[i], LED_TIME);
            }
            dev_id %= digtmp;
            digtmp /= 10;
        }
    }
}

/**
 *@}
 **/
