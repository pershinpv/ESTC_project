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

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    ble_uuid128_t base_uid = {ESTC_BASE_UUID};
    ble_uuid_t service_uuid =
    {
        .uuid = ESTC_SERVICE_UUID,
        .type = BLE_UUID_TYPE_VENDOR_BEGIN,
    };

    // TODO: 4. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    error_code = sd_ble_uuid_vs_add(&base_uid, &service_uuid.type);
    APP_ERROR_CHECK(error_code);

    // TODO: 5. Add service to the BLE stack using `sd_ble_gatts_service_add`
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle);
    APP_ERROR_CHECK(error_code);

    // NRF_LOG_DEBUG("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    // NRF_LOG_DEBUG("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    // NRF_LOG_DEBUG("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);

    return estc_ble_add_characteristics(service);
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
    ble_uuid128_t base_uid = {ESTC_BASE_UUID};

    ble_uuid_t char_uuid =
    {
        .uuid = ESTC_GATT_CHAR_1_UUID,
        .type = BLE_UUID_TYPE_VENDOR_BEGIN,
    };

    ble_uuid_t char_2_uuid =
    {
        .uuid = ESTC_GATT_CHAR_2_UUID,
        .type = BLE_UUID_TYPE_VENDOR_BEGIN,
    };

    error_code = sd_ble_uuid_vs_add(&base_uid, &char_uuid.type);
    APP_ERROR_CHECK(error_code);

    // TODO: 6.5. Configure Characteristic metadata (enable read and write)
    ble_gatts_char_md_t char_md     = { 0 }, char_2_md = { 0 };
    char_md.char_props.read         = 1;
    char_md.char_props.write        = 1;

    char_2_md.char_props.read         = 1;
    char_2_md.char_props.broadcast    = 1;

    //Add user description
    uint8_t estc_desc[]             = "ESTC GATT char test";
    char_md.p_char_user_desc        = estc_desc;
    char_md.char_user_desc_size     = sizeof(estc_desc);
    char_md.char_user_desc_max_size = sizeof(estc_desc);

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    ble_gatts_attr_t attr_char_value = { 0 }, attr_char_2_value = { 0 };

    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;

    attr_char_2_value.p_uuid    = &char_2_uuid;
    attr_char_2_value.p_attr_md = &attr_md;

    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure
    attr_char_value.init_len    = 1;
    attr_char_value.max_len     = 1;

    attr_char_2_value.init_len  = 1;
    attr_char_2_value.max_len   = 1;

    uint8_t value = 127;
    attr_char_value.p_value     = &value;
    attr_char_2_value.p_value   = &value;

    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`
    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->characteristic_handle);
    APP_ERROR_CHECK(error_code);

    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_2_md, &attr_char_2_value, &service->characteristic_2_handle);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}