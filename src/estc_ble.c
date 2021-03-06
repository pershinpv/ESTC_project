#include "estc_ble.h"
#include "estc_service.h"

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {.uuid = BLE_UUID_DEVICE_INFORMATION_SERVICE, .type = BLE_UUID_TYPE_BLE},
    // TODO: 5. Add ESTC service UUID to the table
    {.uuid = ESTC_SERVICE_UUID, .type = BLE_UUID_TYPE_BLE}
};

ble_estc_service_t m_estc_service = {0}; /**< ESTC example BLE service */

APP_TIMER_DEF(timer_Upd_id);
APP_TIMER_DEF(timer_Ntf_id);
APP_TIMER_DEF(timer_Idn_id);

static uint8_t  notification_request_counter = 0;
static uint8_t  indication_request_counter = 0;

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
static void nrf_qwr_error_handler(uint32_t nrf_error);
static void conn_params_error_handler(uint32_t nrf_error);
static void timer_handler_Ntf(void * p_context);
static void timer_handler_Idn(void * p_context);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static void estc_ble_on_write_evt(ble_evt_t const * p_ble_evt);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_WATCH_SPORTS_WATCH);
	APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
    NRF_LOG_INFO("QWR error: %d", nrf_error);
}

/**@brief Function for initializing services that will be used by the application.
 */
void services_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    err_code = estc_ble_service_init(&m_estc_service);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
    ret_code_t err_code;

    //err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    //APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    //err_code = bsp_btn_ble_sleep_mode_prepare();
    //APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

static void timer_handler_Upd(void * p_context)
{
    estc_ble_characteristic_value_update(&m_estc_service);
}

static void timer_handler_Ntf(void * p_context)
{
    estc_ble_characteristic_value_notify(&m_estc_service);
}

static void timer_handler_Idn(void * p_context)
{
    estc_ble_characteristic_value_indicate(&m_estc_service);
}
/**@brief Function for starting timers.
 */
void estc_ble_timers_init(void)
{
        //YOUR_JOB: Start your timers. below is an example of how to start a timer.
        app_timer_create(&timer_Upd_id, APP_TIMER_MODE_REPEATED, timer_handler_Upd);
        app_timer_create(&timer_Ntf_id, APP_TIMER_MODE_REPEATED, timer_handler_Ntf);
        app_timer_create(&timer_Idn_id, APP_TIMER_MODE_REPEATED, timer_handler_Idn);
}

static void estc_ble_on_write_evt(ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    for (size_t i = 0; i < m_estc_service.char_qty; ++i)
    {
        if ((p_evt_write->handle == m_estc_service.characteristic[i].c_handle.value_handle) && (p_evt_write->len > 0))
        {
            if (m_estc_service.characteristic[i].c_props.write == 1 && p_evt_write->len == m_estc_service.characteristic[i].c_len)
                estc_ble_characteristic_value_incoming_update(i, (uint8_t*) p_evt_write->data);
            NRF_LOG_INFO("Characteristic value update request. Data length is %d bytes.", p_evt_write->len);
            return;
        }

        if ((p_evt_write->handle == m_estc_service.characteristic[i].c_handle.cccd_handle) && (p_evt_write->len == 2))
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data) != m_estc_service.characteristic[i].is_notification_en)
            {
                m_estc_service.characteristic[i].is_notification_en ^= true;
                notification_request_counter += m_estc_service.characteristic[i].is_notification_en ? 1 : -1;

                if (notification_request_counter == 0)
                {
                    app_timer_stop(timer_Ntf_id);
                    NRF_LOG_INFO("Notification timer stopped");
                }
                else if (notification_request_counter == 1 && m_estc_service.characteristic[i].is_notification_en)
                {
                    app_timer_start(timer_Ntf_id, APP_TIMER_TICKS(NOTIFICATION_UPDATE_TIME), NULL);
                    NRF_LOG_INFO("Notification timer started");
                }
                return;
            }

            if (ble_srv_is_indication_enabled(p_evt_write->data) != m_estc_service.characteristic[i].is_indication_en)
            {
                m_estc_service.characteristic[i].is_indication_en ^= true;
                indication_request_counter += m_estc_service.characteristic[i].is_indication_en ? 1 : -1;

                if (indication_request_counter == 0)
                {
                    app_timer_stop(timer_Idn_id);
                    NRF_LOG_INFO("Indication timer stopped");
                }
                else if (indication_request_counter == 1 && m_estc_service.characteristic[i].is_indication_en)
                {
                    app_timer_start(timer_Idn_id, APP_TIMER_TICKS(INDICATION_UPDATE_TIME), NULL);
                    NRF_LOG_INFO("Indication timer started");
                }
                return;
            }
        }
    }
}

static void estc_ble_on_disconnect(ble_evt_t const * p_ble_evt)
{
    app_timer_stop_all();
    NRF_LOG_INFO("All timer stopped");

    for (size_t i = 0; i < m_estc_service.char_qty; ++i)
    {
        m_estc_service.characteristic[i].is_notification_en = false;
        m_estc_service.characteristic[i].is_indication_en = false;
    }
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    //ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("ADV Event: Start fast advertising");
            break;

        case BLE_ADV_EVT_IDLE:
            NRF_LOG_INFO("ADV Event: idle, no connectable advertising is ongoing");
            //sleep_mode_enter();
            break;

        default:
            break;
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            estc_ble_on_write_evt(p_ble_evt);
            /*NRF_LOG_INFO("On_write write: handle %d type %d len %d data %d",
                          p_ble_evt->evt.gatts_evt.params.write.handle,
                          p_ble_evt->evt.gatts_evt.params.write.op,
                          p_ble_evt->evt.gatts_evt.params.write.len,
                          p_ble_evt->evt.gatts_evt.params.write.data);

            NRF_LOG_INFO("On_write handles: value handle %d user_desc_handle %d sccd_handle %d cccd_handle %d",
                         m_estc_service.characteristic[1].c_handle.value_handle,
                         m_estc_service.characteristic[1].c_handle.user_desc_handle,
                         m_estc_service.characteristic[1].c_handle.sccd_handle,
                         m_estc_service.characteristic[1].c_handle.cccd_handle);*/

            break;

        case BLE_GAP_EVT_DISCONNECTED:
            estc_ble_on_disconnect(p_ble_evt);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            NRF_LOG_INFO("Disconnected (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);
            break;

        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            app_timer_start(timer_Upd_id, APP_TIMER_TICKS(VALUE_UPDATE_TIME), NULL);
            NRF_LOG_INFO("Update value timer start");
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
            NRF_LOG_DEBUG("PHY update request (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout (conn_handle: %d)", p_ble_evt->evt.gattc_evt.conn_handle);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout (conn_handle: %d)", p_ble_evt->evt.gatts_evt.conn_handle);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            //NRF_LOG_DEBUG("Unknown ble_evt %d", p_ble_evt->header.evt_id);
            break;
    }
    m_estc_service.connection_handle = m_conn_handle;
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("nrf_sdh_ble_enable err_code %d", err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_LOG_INFO("ble_stack_init m_ble_observer %d", m_ble_observer.handler);
}

/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_SHORT_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    // TODO: 6. Consider moving the device characteristics to the Scan Response if necessary
    //init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    //init.advdata.uuids_complete.p_uuids  = m_adv_uuids;
    init.advdata.uuids_more_available.uuid_cnt = 1;
    init.advdata.uuids_more_available.p_uuids = m_adv_uuids + 1;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.advdata.short_name_len = strlen(LAST_NAME);

    // TODO: Add more data to the advertisement data
    ble_advdata_manuf_data_t manuf_data;
    uint8_t data[] = "Da";
    manuf_data.data.p_data = data;
    manuf_data.data.size = sizeof(data);
    manuf_data.company_identifier = 0x004C;
    init.advdata.p_tx_power_level = (int8_t *)100500;
    init.advdata.p_manuf_specific_data = &manuf_data;

    // TODO: Add more data to the scan response data
    init.srdata.name_type = BLE_ADVDATA_FULL_NAME;
    ble_advdata_manuf_data_t response_data;
    uint8_t resp_data[] = "rd";
    response_data.data.p_data = resp_data;
    response_data.data.size = sizeof(resp_data);
    response_data.company_identifier = 0x004C;
    init.srdata.p_manuf_specific_data = &response_data;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing power management.
 */
void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
	LOG_BACKEND_USB_PROCESS();
}


/**@brief Function for starting advertising.
 */
void advertising_start(void)
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}