/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble.h"
#include "sdk_errors.h"

// TODO: 1. Generate random BLE UUID (Version 4 UUID) and define it in the following format:
#define ESTC_BASE_UUID { 0x6C, 0x65, 0x76, 0x61, 0x50, 0x6E, /* - */ 0x69, 0x68, /* - */ 0x73, 0x72, /* - */ 0x65, 0x50, /* - */ 0x00, 0x00, 0x00, 0x00 } // UUID: 0000xxxx-5065-7273-6869-6E506176656C

// TODO: 2. Pick a random service 16-bit UUID and define it:
#define ESTC_SERVICE_UUID 0x5601

 // TODO: 3. Pick a characteristic UUID and define it:

#define ESTC_CHAR_Color_RGB_UUID 0xABCD
#define ESTC_CHAR_Color_HSV_UUID 0xABCE

#define ESTC_SRV1_CHAR_Quantity  3

typedef struct ble_estc_characteristic_s
{
    ble_gatt_char_props_t c_props;
    ble_gatts_char_handles_t c_handle;
    uint16_t c_uuid;
    uint16_t c_len;
    uint8_t *c_default_value;
    char *c_name;
    bool is_notification_en;
    bool is_indication_en;
} ble_estc_characteristic_t;

typedef struct ble_estc_service_s
{
    uint16_t service_handle;
    uint16_t connection_handle;
    uint8_t char_qty;

    // TODO: 6.3. Add handles for characterstic (type: ble_gatts_char_handles_t)
    ble_estc_characteristic_t characteristic[ESTC_SRV1_CHAR_Quantity];
} ble_estc_service_t;



ret_code_t estc_ble_service_init(ble_estc_service_t *service);

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);

void estc_ble_characteristic_value_notify(ble_estc_service_t const *service);
void estc_ble_characteristic_value_indicate(ble_estc_service_t const *service);
void estc_ble_characteristic_value_update(ble_estc_service_t const *service);
void estc_ble_characteristic_value_incoming_update(uint8_t char_num, uint8_t* data);

#endif /* ESTC_SERVICE_H__ */