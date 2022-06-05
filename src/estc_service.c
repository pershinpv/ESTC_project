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
#include "color.h"
#include "nvmc.h"

static rgb_t rgb_vals;
static hsv_t hsv_vals;

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, uint8_t char_num);

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

    color_value_get_rgb(&rgb_vals);
    color_value_get_hsv(&hsv_vals);

    service->char_qty = sizeof(service->characteristic) / sizeof(ble_estc_characteristic_t);

    service->characteristic[0].c_uuid = 0x2901;
    service->characteristic[0].c_default_value = NULL;
    service->characteristic[0].c_name = "ESTC color service";

    service->characteristic[1].c_uuid = ESTC_CHAR_Color_RGB_UUID;
    service->characteristic[1].c_props.read = 1;
    service->characteristic[1].c_props.write = 1;
    service->characteristic[1].c_props.notify = 1;
    service->characteristic[1].c_default_value = (uint8_t *)&rgb_vals;
    service->characteristic[1].c_len = 3;
    service->characteristic[1].c_name = "RGB value";

    service->characteristic[2].c_uuid = ESTC_CHAR_Color_HSV_UUID;
    service->characteristic[2].c_props.read = 1;
    service->characteristic[2].c_props.write = 1;
    service->characteristic[2].c_props.notify = 1;
    service->characteristic[2].c_default_value = (uint8_t *)&hsv_vals;
    service->characteristic[2].c_len = 4;
    service->characteristic[2].c_name = "HSV value";

    for (size_t i = 0; i < service->char_qty; ++i)
    {
        error_code = estc_ble_add_characteristics(service, i);
        APP_ERROR_CHECK(error_code);
    }

    return error_code;
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, uint8_t char_num)
{
    ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
    ble_uuid128_t base_uid = {ESTC_BASE_UUID};

    ble_uuid_t char_uuid =
    {
        .uuid = service->characteristic[char_num].c_uuid,
        .type = BLE_UUID_TYPE_VENDOR_BEGIN,
    };

    error_code = sd_ble_uuid_vs_add(&base_uid, &char_uuid.type);
    APP_ERROR_CHECK(error_code);

    // TODO: 6.5. Configure Characteristic metadata
    ble_gatts_char_md_t char_md     = { 0 };
    char_md.char_props = service->characteristic[char_num].c_props;

    //Add user description
    char_md.p_char_user_desc        = (uint8_t *)service->characteristic[char_num].c_name;
    char_md.char_user_desc_size     = strlen(service->characteristic[char_num].c_name);
    char_md.char_user_desc_max_size = strlen(service->characteristic[char_num].c_name);

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    ble_gatts_attr_t attr_char_value = { 0 };
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;

    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure
    attr_char_value.init_len    = service->characteristic[char_num].c_len;
    attr_char_value.max_len     = service->characteristic[char_num].c_len;
    attr_char_value.p_value     = service->characteristic[char_num].c_default_value;

    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`
    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->characteristic[char_num].c_handle);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}

void estc_ble_characteristic_value_notify(ble_estc_service_t const *service)
{
    if (service->connection_handle == BLE_CONN_HANDLE_INVALID)
        return;

    ret_code_t error_code = NRF_SUCCESS;
    ble_gatts_hvx_params_t params = {0};
    uint16_t p_len;

    UNUSED_PARAMETER(error_code);
    color_value_get_hsv(&hsv_vals);
    color_value_get_rgb(&rgb_vals);

    for (size_t i = 0; i < service->char_qty; ++i)
    {
        if(service->characteristic[i].is_notification_en)
        {
            memset(&params, 0, sizeof(params));
            params.type = BLE_GATT_HVX_NOTIFICATION;
            params.handle = service->characteristic[i].c_handle.value_handle;
            p_len = service->characteristic[i].c_len;
            params.p_len = &p_len;
            params.p_data = service->characteristic[i].c_default_value;

            error_code = sd_ble_gatts_hvx(service->connection_handle, &params);

            if (error_code != NRF_SUCCESS)
            {
                NRF_LOG_INFO("Ntf sd_ble_gatts_hvx err_code %d", error_code);
                APP_ERROR_CHECK(error_code);
            }
        }
    }
}

void estc_ble_characteristic_value_indicate(ble_estc_service_t const *service)
{
    if (service->connection_handle == BLE_CONN_HANDLE_INVALID)
        return;

    ret_code_t error_code = NRF_SUCCESS;
    ble_gatts_hvx_params_t params = {0};
    uint16_t p_len;

    UNUSED_PARAMETER(error_code);
    color_value_get_rgb(&rgb_vals);
    color_value_get_rgb(&rgb_vals);

    for (size_t i = 0; i < service->char_qty; ++i)
    {
        if(service->characteristic[i].is_indication_en)
        {
            memset(&params, 0, sizeof(params));
            params.type = BLE_GATT_HVX_INDICATION;
            params.handle = service->characteristic[i].c_handle.value_handle;
            p_len = service->characteristic[i].c_len;
            params.p_len = &p_len;
            params.p_data = service->characteristic[i].c_default_value;

            error_code = sd_ble_gatts_hvx(service->connection_handle, &params);

            if (error_code != NRF_SUCCESS)
            {
                NRF_LOG_INFO("Idn sd_ble_gatts_hvx err_code %d", error_code);
                if (error_code != NRF_ERROR_BUSY)
                    APP_ERROR_CHECK(error_code);
            }
        }
    }
}

void estc_ble_characteristic_value_update(ble_estc_service_t const *service)
{
    if (service->connection_handle == BLE_CONN_HANDLE_INVALID)
        return;

    ret_code_t error_code = NRF_SUCCESS;
    ble_gatts_value_t gat_value = {0};

    UNUSED_PARAMETER(error_code);
    color_value_get_hsv(&hsv_vals);
    color_value_get_rgb(&rgb_vals);

    for (size_t i = 0; i < service->char_qty; ++i)
    {
        if(service->characteristic[i].c_props.read == 1)
        {
            memset(&gat_value, 0, sizeof(gat_value));
            gat_value.len = service->characteristic[i].c_len;
            gat_value.p_value = service->characteristic[i].c_default_value;
            error_code = sd_ble_gatts_value_set(service->connection_handle, service->characteristic[i].c_handle.value_handle, &gat_value);

            if (error_code != NRF_SUCCESS)
            {
                NRF_LOG_INFO("Upd sd_ble_gatts_value_set err_code %d", error_code);
                APP_ERROR_CHECK(error_code);
            }
        }
    }
}

void estc_ble_characteristic_value_incoming_update(uint8_t char_num, uint8_t* data)
{
    switch (char_num)
    {
        case 1:
            rgb_vals.r = *data;
            rgb_vals.g = *(data + 1);
            rgb_vals.b = *(data + 2);
            rgb_vals.crc = color_crc_calc_rgb(&rgb_vals);
            color_value_set_rgb(&rgb_vals);
            break;

        case 2:
            hsv_vals = *(hsv_t *) data;
            color_value_set_hsv(&hsv_vals);
            break;

        default:
            break;
    }
    nvmc_write_rgb_actual_values(&rgb_vals);
}