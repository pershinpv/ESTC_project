#include <string.h>
#include "log.h"
#include "cli_usb.h"

static char m_rx_buffer[READ_SIZE];
static char cli_usb_msg_buffer[CLI_USB_CMD_MAX_LEN];

static void (*cli_usb_msg_handler)(char const * usb_msg, uint8_t usb_msg_len);

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

void cli_usb_cdc_acm_init(void usb_msg_handler(char const * usb_msg, uint8_t usb_msg_len))
{
    ret_code_t ret;
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    cli_usb_msg_handler = usb_msg_handler;
    APP_ERROR_CHECK(ret);
}

void cli_usb_send_message(char const msg[], size_t msg_lenght, bool is_line_feed)
{
    ret_code_t ret = 0;

    do
    {
            ret = app_usbd_cdc_acm_write(&usb_cdc_acm, msg, msg_lenght);
    } while (ret == NRF_ERROR_BUSY);

    if (is_line_feed)
    {
        do
        {
            ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 3);
        } while (ret == NRF_ERROR_BUSY);
    }

    //NRF_LOG_INFO("Send message. Err code: %d", ret);
}

static void clear_string(char *msg, uint8_t lenght)
{
    for(size_t i = 0; i < lenght; ++i)
        msg[i] = 0;
}

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
    ret_code_t ret;
    switch (event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    {
        ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
        NRF_LOG_INFO("Port open %d", m_rx_buffer[0]);
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
    {
        NRF_LOG_INFO("Port close");
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    {
        //NRF_LOG_INFO("TX done %d", m_rx_buffer[0]);
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        static uint8_t command_symbol_position = 0;
        do
        {
            if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 3);
                cli_usb_msg_buffer[command_symbol_position] = '\0';
                ++command_symbol_position;
                //NRF_LOG_INFO("RX rn %d", ret);

                cli_usb_msg_handler(cli_usb_msg_buffer, command_symbol_position);
                clear_string(cli_usb_msg_buffer, command_symbol_position);
                command_symbol_position = 0;
            }
            else if (m_rx_buffer[0] == KEY_ESCAPE)
            {
                size_t rx_bufer_size = app_usbd_cdc_acm_bytes_stored(&usb_cdc_acm);

                //NRF_LOG_INFO("RX escape %d", m_rx_buffer[0]);

                if (rx_bufer_size > 0)
                {
                    uint8_t rx_big_bufer[rx_bufer_size - 1];
                    ret = app_usbd_cdc_acm_read(&usb_cdc_acm, rx_big_bufer, rx_bufer_size);
                    //NRF_LOG_INFO("RX escape sec %d %d %d", rx_bufer_size, rx_big_bufer[0], rx_big_bufer[1]);
                }
            }
            else if (m_rx_buffer[0] == KEY_DELETE || m_rx_buffer[0] == '\b')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\b \b", 3);
                if (command_symbol_position > 0)
                    --command_symbol_position;
                //NRF_LOG_INFO("RX backspace %d %d", ret, command_symbol_position);
            }
            else
            {
                if (command_symbol_position < CLI_USB_CMD_MAX_LEN - 1)
                {
                    ret = app_usbd_cdc_acm_write(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
                    cli_usb_msg_buffer[command_symbol_position] = m_rx_buffer[0];
                    ++command_symbol_position;
                }
            }

            ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);

        } while (ret == NRF_SUCCESS);
        break;
    }
    default:
        break;
    }
}