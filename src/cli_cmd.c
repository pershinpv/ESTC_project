#include <string.h>
#include "cli_cmd.h"
#include "cli_usb.h"
#include "nvmc.h"

static bool cli_cmd_calc_param_values(char const command[], uint16_t params[], size_t param_position, size_t const params_qty)
{
    uint8_t param_counter = 0;
    bool is_param = false;

    while (command[param_position] != '\0')
    {
        if(command[param_position] == ' ')
        {
            ++param_position;
            if(param_counter < params_qty)
            {
                params[param_counter] = 0;

                while (command[param_position] >= '0' && command[param_position] <= '9')
                {
                    params[param_counter] = params[param_counter] * 10 + command[param_position] - '0';
                    ++param_position;
                    is_param = true;
                }

                if (is_param)
                {
                    ++param_counter;
                    is_param = false;
                }
            }
        }
        else
        {
            return false;
        }
    }

    if (param_counter < params_qty)
        return false;
    else
        return true;
}

static cli_cmd_result_t handler_cmd_help(char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 0;
    const char cmd_type[] = "help";
    uint16_t params[param_num];

    char *help_msg[CLI_COMMANDS_NUMBER + 1];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, sizeof(cmd_type) - 1, param_num))
    {
        help_msg[0] = "Support commands:";
        help_msg[1] = "1. 'help' to see this list";
        help_msg[2] = "2. RGB <r> <g> <b>. r, g, b: (0...255)";
        help_msg[3] = "3. HSV <h> <s> <v>. h: (0...360); s, v: (0...100)";
        help_msg[4] = "4. No command yet";

        for (size_t i = 0; i < CLI_COMMANDS_NUMBER + 1; ++i)
            cli_usb_send_message(help_msg[i], strlen(help_msg[i]), true);

        result = CLI_RES_OK;
    }
    return result;
}

static cli_cmd_result_t handler_cmd_rgb(char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    const char cmd_type[] = "RGB";
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, sizeof(cmd_type) - 1, param_num))
    {
        if (params[0] <= RGB_MAX_VAL && params[1] <= RGB_MAX_VAL && params[2] <= RGB_MAX_VAL)
        {
            rgb.r = (uint8_t)params[0];
            rgb.g = (uint8_t)params[1];
            rgb.b = (uint8_t)params[2];
            rgb.crc = color_crc_calc_rgb(&rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = sprintf(cmd_msg, "Color set to %s %u %u %u", cmd_type, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
            nvmc_read_rgb_actual_values_for_log();

            result = CLI_RES_OK;
        }
        else
        {
            result = CLI_RES_NOT_FOUND;
        }
    }
    return result;
}

static cli_cmd_result_t handler_cmd_hsv(char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    const char cmd_type[] = "HSV";
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    hsv_t hsv;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, sizeof(cmd_type) - 1, param_num))
    {
        if (params[0] <= HSV_MAX_H && params[1] <= HSV_MAX_S && params[2] <= HSV_MAX_V)
        {
            hsv.h = (uint16_t)params[0];
            hsv.s = (uint8_t)params[1];
            hsv.v = (uint8_t)params[2];
            color_hsv_to_rgb(&hsv, &rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = sprintf(cmd_msg, "Color set to %s %u %u %u", cmd_type, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
            nvmc_read_rgb_actual_values_for_log();

            result = CLI_RES_OK;
        }
        else
        {
            result = CLI_RES_NOT_FOUND;
        }
    }
    return result;
}

static cli_cmd_result_t handler_cmd_save_rgb(char const *usb_msg, uint8_t usb_msg_len)
{
    return CLI_RES_NOT_FOUND;
}

static cli_cmd_cdescriptor_t cli_commands[] =
{
    {"help", handler_cmd_help},
    {"RGB", handler_cmd_rgb},
    {"HSV", handler_cmd_hsv},
    {"save_rgb_color", handler_cmd_save_rgb},
};

void cli_cmd_handler(char const *usb_msg, uint8_t usb_msg_len)
{
    cli_cmd_result_t ret = CLI_RES_NOT_FOUND;

    for(size_t i = 0; i < ARRAY_SIZE(cli_commands); ++i)
    {
        const cli_cmd_cdescriptor_t *cli_cmd = &cli_commands[i];
        if(strncmp(usb_msg, cli_cmd->cmd_name, sizeof(cli_cmd->cmd_name) - 1) == 0)
        {
            ret = cli_cmd->cmd_handler(usb_msg, usb_msg_len);
        }
    }

    if (ret == CLI_RES_NOT_FOUND)
    {
        char error_msg[] = "Unknown command. For help type 'help'.";
        cli_usb_send_message(error_msg, sizeof(error_msg), true);
    }
}