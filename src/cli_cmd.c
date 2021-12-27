#include <string.h>
#include "cli_cmd.h"
#include "cli_usb.h"

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

static size_t cli_cmd_calc_word(char const command[], char word[], size_t param_position)
{
    uint8_t char_position = 0;

    if (command[param_position] == ' ')
        do
        {
            ++param_position;
        } while (command[param_position] == ' ');
    else
        return param_position = 0;

    if((command[param_position] >='0' && command[param_position] <='9') || command[param_position] =='_')
        return param_position = 0;

    while ((command[param_position] >='A' && command[param_position] <='Z') || \
           (command[param_position] >='a' && command[param_position] <='z') || \
           (command[param_position] >='0' && command[param_position] <='9') || \
           command[param_position] =='_')
    {
        if (char_position < CLI_COLOR_NAME_MAX_LEN - 1)
        {
            word[char_position] = command[param_position];
        }
        else
        {
            return param_position = 0;
        }
        ++char_position;
        ++param_position;
    }

    word[char_position++] = '\0';

    while (char_position < SIZE_OF_COLOR_NAME_MAX_BYTES)
    {
        word[char_position] = 0xFF;
        ++char_position;
    }
    return param_position;
}

static cli_cmd_result_t handler_cmd_rgb(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num))
    {
        if (params[0] <= RGB_MAX_VAL && params[1] <= RGB_MAX_VAL && params[2] <= RGB_MAX_VAL)
        {
            rgb.r = (uint8_t)params[0];
            rgb.g = (uint8_t)params[1];
            rgb.b = (uint8_t)params[2];
            rgb.crc = color_crc_calc_rgb(&rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = sprintf(cmd_msg, "Color set to %s %u %u %u", cmd_name, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
            result = CLI_RES_OK;
        }
        else
        {
            result = CLI_RES_NOT_FOUND;
        }
    }
    return result;
}

static cli_cmd_result_t handler_cmd_hsv(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    hsv_t hsv;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num))
    {
        if (params[0] <= HSV_MAX_H && params[1] <= HSV_MAX_S && params[2] <= HSV_MAX_V)
        {
            hsv.h = (uint16_t)params[0];
            hsv.s = (uint8_t)params[1];
            hsv.v = (uint8_t)params[2];
            color_hsv_to_rgb(&hsv, &rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = sprintf(cmd_msg, "Color set to %s %u %u %u", cmd_name, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
            result = CLI_RES_OK;
        }
        else
        {
            result = CLI_RES_NOT_FOUND;
        }
    }
    return result;
}

static cli_cmd_result_t handler_cmd_add_rgb_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    rgb_t rgb;
    const size_t param_num = 3;
    uint16_t params[param_num];
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    uint8_t param_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));

    if (param_pos > 0)
    {
        if (cli_cmd_calc_param_values(usb_msg, params, param_pos, param_num))
        {
            if (params[0] <= RGB_MAX_VAL && params[1] <= RGB_MAX_VAL && params[2] <= RGB_MAX_VAL)
            {
                rgb.r = (uint8_t)params[0];
                rgb.g = (uint8_t)params[1];
                rgb.b = (uint8_t)params[2];
                rgb.crc = color_crc_calc_rgb(&rgb);

                if(nvmc_add_rgb_color(&rgb, color_name))
                {
                    size_t cmd_msg_len = sizeof("Add color RGB ") + usb_msg_len - 1;
                    char cmd_msg[cmd_msg_len];
                    color_value_set_rgb(&rgb);
                    cmd_msg_len = sprintf(cmd_msg, "Add color '%s' RGB %u %u %u", color_name, params[0], params[1], params[2]);
                    cli_usb_send_message(cmd_msg, cmd_msg_len, true);
                    nvmc_write_rgb_actual_values(&rgb);
                }
                else
                {
                    char msg[] = "Duplicated color name. Current color doesn't added.";
                    cli_usb_send_message(msg, sizeof(msg), true);
                }

                result = CLI_RES_OK;
            }
            else
            {
                result = CLI_RES_NOT_FOUND;
            }
        }
    }
    return result;
}

static cli_cmd_result_t handler_cmd_del_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    uint8_t cmd_color_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));

    if (cmd_color_pos == 0)
        return CLI_RES_NOT_FOUND;

    while (usb_msg[cmd_color_pos] != '\0')
    {
        if (usb_msg[cmd_color_pos] != ' ')
            return CLI_RES_NOT_FOUND;
        ++cmd_color_pos;
    }

    if (nvmc_del_rgb_color(color_name))
    {
        size_t cmd_msg_len = sizeof("Color '' deleted") + usb_msg_len - 1;
        char cmd_msg[cmd_msg_len];
        cmd_msg_len = sprintf(cmd_msg, "Color '%s' deleted", color_name);
        cli_usb_send_message(cmd_msg, cmd_msg_len, true);
    }
    else
    {
        char msg[] = "Color not found.";
        cli_usb_send_message(msg, sizeof(msg), true);
    }

    return CLI_RES_OK;
}

static cli_cmd_result_t handler_cmd_apply_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    rgb_t rgb;
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    uint32_t color_addr = 0;
    uint8_t cmd_color_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));

    if (cmd_color_pos == 0)
        return CLI_RES_NOT_FOUND;

    while (usb_msg[cmd_color_pos] != '\0')
    {
        if (usb_msg[cmd_color_pos] != ' ')
            return CLI_RES_NOT_FOUND;
        ++cmd_color_pos;
    }

    color_addr = nvmc_find_rgb_color(color_name);

    if (color_addr > 0)
    {
        rgb.r = *(uint8_t *)(color_addr + 0);
        rgb.g = *(uint8_t *)(color_addr + 1);
        rgb.b = *(uint8_t *)(color_addr + 2);
        rgb.crc = *(uint8_t *)(color_addr + 3);

        if(color_rgb_values_validate(&rgb))
        {
            size_t cmd_msg_len = sizeof("Color '' applyed") + usb_msg_len - 1;
            char cmd_msg[cmd_msg_len];

            color_value_set_rgb(&rgb);
            nvmc_write_rgb_actual_values(&rgb);
            cmd_msg_len = sprintf(cmd_msg, "Color '%s' applyed", color_name);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
        }
        else
        {
            size_t cmd_msg_len = sizeof("Color '' CRC check faild. Color doesn't set.") + usb_msg_len - 1;
            char cmd_msg[cmd_msg_len];

            cmd_msg_len = sprintf(cmd_msg, "Color '%s' CRC check faild. Color doesn't set.", color_name);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
        }
    }
    else
    {
        char msg[] = "Color not found.";
        cli_usb_send_message(msg, sizeof(msg), true);
    }

    return CLI_RES_OK;
}

static cli_cmd_result_t handler_cmd_help(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 0;
    uint16_t params[param_num];

    char *help_msg[CLI_COMMANDS_NUMBER + 1];
    cli_cmd_result_t result = CLI_RES_NOT_FOUND;

    if (cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num))
    {
        help_msg[0] = "\nSupport commands:";
        help_msg[1] = "1. 'help' to see this list";
        help_msg[2] = "2. rgb <r> <g> <b>. r, g, b: (0...255)";
        help_msg[3] = "3. hsv <h> <s> <v>. h: (0...360); s, v: (0...100)";
        help_msg[4] = "4. add_rgb_color <color_name> <r> <g> <b>. r, g, b: (0...255); color_name MAX length: 23";
        help_msg[5] = "5. del_color <color_name>";
        help_msg[6] = "6. apply_color <color_name>";

        for (size_t i = 0; i <= CLI_COMMANDS_NUMBER; ++i)
            cli_usb_send_message(help_msg[i], strlen(help_msg[i]) + 1, true);

        result = CLI_RES_OK;
    }
    return result;
}

static cli_cmd_cdescriptor_t cli_commands[] =
{
    {"help", handler_cmd_help},
    {"rgb", handler_cmd_rgb},
    {"hsv", handler_cmd_hsv},
    {"add_rgb_color", handler_cmd_add_rgb_color},
    {"del_color", handler_cmd_del_color},
    {"apply_color", handler_cmd_apply_color},
};

void cli_cmd_handler(char const *usb_msg, uint8_t usb_msg_len)
{
    cli_cmd_result_t ret = CLI_RES_NOT_FOUND;

    for(size_t i = 0; i < ARRAY_SIZE(cli_commands); ++i)
    {
        const cli_cmd_cdescriptor_t *cli_cmd = &cli_commands[i];

        if(strncmp(usb_msg, cli_cmd->cmd_name, strlen(cli_cmd->cmd_name)) == 0)
        {
            ret = cli_cmd->cmd_handler(cli_cmd->cmd_name, usb_msg, usb_msg_len);
        }
    }

    if (ret == CLI_RES_NOT_FOUND)
    {
        char error_msg[] = "Unknown command. For help type 'help'.";
        cli_usb_send_message(error_msg, sizeof(error_msg), true);
    }
    else
    {
        nvmc_read_rgb_actual_values_for_log();
    }
}