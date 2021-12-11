#include <string.h>
#include "cdc_acm_ctrl.h"

#define READ_SIZE 1
#define COMMAND_MAX_LEN 16  //RGB 255 255 255\0

static char m_rx_buffer[READ_SIZE];
static char user_command[COMMAND_MAX_LEN];
static ret_code_t ret = 0;

static cli_command_t cli_command_to_do =
{
    .comand_type = CLI_NO_COMMAND,
    .param_1 = 0,
    .param_2 = 0,
    .param_3 = 0,
    .comand_len = 0,
};

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

void cli_cdc_acm_connect_ep(void)
{
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
}

cli_command_t cli_command_to_do_get (void)
{
    return cli_command_to_do;
}

void cli_send_result_message()
{
    cli_send_message("Color set to ", sizeof("Color set to "), false);
    cli_send_message(user_command, sizeof(user_command), true);
}

void cli_command_reset (void)
{
    cli_command_to_do.comand_type = CLI_NO_COMMAND;
    cli_command_to_do.param_1 = 0;
    cli_command_to_do.param_2 = 0;
    cli_command_to_do.param_3 = 0;
    cli_command_to_do.comand_len = 0;

    for(size_t i = 0; i < COMMAND_MAX_LEN; ++i)
        user_command[i] = 0;
}

void cli_send_message(char const msg[], size_t msg_lenght, bool is_line_feed)
{
    uint8_t lenght = 0;
    if (is_line_feed)
        lenght = msg_lenght + 2;
    else
        lenght = msg_lenght;

    char send_msg[lenght];

    for(int i = 0; i < lenght; ++i )
        send_msg[i] = msg[i];

    if (is_line_feed)
    {
        send_msg[msg_lenght] = '\r';
        send_msg[msg_lenght + 1] = '\n';
    }

    do
    {
        ret = app_usbd_cdc_acm_write(&usb_cdc_acm, send_msg, msg_lenght + 2);
    } while (ret == NRF_ERROR_BUSY);
    NRF_LOG_INFO("Send message. Err code: %d", ret);
}

static bool calc_hsv_rgb_values(char const command[], uint8_t const comand_lenght, size_t char_position, size_t const params_qty)
{
    uint8_t param_counter = 0;
    uint16_t params[params_qty];

    while (command[char_position] == ' ' && param_counter < params_qty)
    {
        ++char_position;
        params[param_counter] = 0;

        while (command[char_position] >= '0' && command[char_position] <= '9')
        {
            params[param_counter] = params[param_counter] * 10 + command[char_position] - '0';
            ++char_position;
        }
        ++param_counter;
    }

    if (param_counter != params_qty || char_position != comand_lenght)
        return false;

    cli_command_to_do.param_1 = params[0];
    cli_command_to_do.param_2 = params[1];
    cli_command_to_do.param_3 = params[2];
    cli_command_to_do.comand_len = comand_lenght;

    return true;
}

static void calc_cli_command(char const *command, uint8_t comand_lenght)
{
    if (strncmp(command, "RGB", 3) == 0 && command[comand_lenght] == '\0')
    {
        if (calc_hsv_rgb_values(command, comand_lenght, 3, 3))
        {
            if (cli_command_to_do.param_1 <= CLI_RGB_MAX_VAL && cli_command_to_do.param_2 <= CLI_RGB_MAX_VAL && cli_command_to_do.param_3 <= CLI_RGB_MAX_VAL)
                cli_command_to_do.comand_type = CLI_SET_RGB;
            else
                cli_command_reset();
        }
        NRF_LOG_INFO("detect RGB command %d %d %d %d", cli_command_to_do.comand_type, cli_command_to_do.param_1, cli_command_to_do.param_2, cli_command_to_do.param_3);
        return;
    }

    if (strncmp(command, "HSV", 3) == 0 && command[comand_lenght] == '\0')
    {
        if (calc_hsv_rgb_values(command, comand_lenght, 3, 3))
        {
            if (cli_command_to_do.param_1 < HSV_MAX_H && cli_command_to_do.param_2 <= HSV_MAX_S && cli_command_to_do.param_3 <= HSV_MAX_V)
                cli_command_to_do.comand_type = CLI_SET_HSV;
            else
                cli_command_reset();
        }
        NRF_LOG_INFO("detect HSV command %d %d %d %d", cli_command_to_do.comand_type, cli_command_to_do.param_1, cli_command_to_do.param_2, cli_command_to_do.param_3);
        return;
    }

    if (strncmp(command, "help", sizeof("help")) == 0)
    {
        cli_command_reset();
        cli_command_to_do.comand_type = CLI_HELP;
        NRF_LOG_INFO("detect 'help' command %d %d %d %d", cli_command_to_do.comand_type, cli_command_to_do.param_1, cli_command_to_do.param_2, cli_command_to_do.param_3);
        return;
    }

    NRF_LOG_INFO("detect wrong command");
    cli_command_reset();
}

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
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
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        static uint8_t command_symbol_position = 0;
        static bool is_need_to_chek_command = false;
        do
        {
            if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 2);
                //NRF_LOG_INFO("TX rn %d", ret);
                user_command[command_symbol_position] = '\0';
                is_need_to_chek_command = true;
            }
            else if (m_rx_buffer[0] == KEY_DELETE || m_rx_buffer[0] == '\b')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\b \b", 3);
                if (command_symbol_position > 0)
                    --command_symbol_position;
                NRF_LOG_INFO("RX backspace %d %d", ret, command_symbol_position);
            }
            else
            {
                if (command_symbol_position < COMMAND_MAX_LEN - 1)
                {
                    ret = app_usbd_cdc_acm_write(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
                    user_command[command_symbol_position] = m_rx_buffer[0];
                    ++command_symbol_position;
                }
            }

            ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);

        } while (ret == NRF_SUCCESS);

        if (is_need_to_chek_command)
        {
            calc_cli_command(user_command, command_symbol_position);
            is_need_to_chek_command = false;
            command_symbol_position = 0;
            if (cli_command_to_do.comand_type == CLI_NO_COMMAND)
            {
                char error_msg[] = "Unknown command. For help type 'help'";
                cli_send_message(error_msg, sizeof(error_msg), true);
            }
        }

        break;
    }
    default:
        break;
    }
}