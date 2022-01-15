#include <string.h>
#include <ctype.h>
#include "cli_cmd.h"
#include "cli_usb.h"

//usb_msg is a message from USB. Message formated is a zero-terminated string.
static estc_ret_code_t handler_cmd_help(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_rgb(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_hsv(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_add_rgb_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_del_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_apply_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);
static estc_ret_code_t handler_cmd_list_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len);

static estc_ret_code_t cli_cmd_calc_param_values(char const command[], uint16_t params[], size_t param_position, size_t const params_qty);
static size_t cli_cmd_calc_word(char const command[], char word[], size_t param_position);
static bool is_only_spaces(char const *const string, uint8_t start_pos);

static cli_cmd_cdescriptor_t cli_commands[] =
{
    {"help", handler_cmd_help},
    {"rgb", handler_cmd_rgb},
    {"hsv", handler_cmd_hsv},
    {"add_rgb_color", handler_cmd_add_rgb_color},
    {"del_color", handler_cmd_del_color},
    {"apply_color", handler_cmd_apply_color},
    {"list_color", handler_cmd_list_color},
};

static void cli_cmd_error_handler(estc_ret_code_t ret_code)
{
    static char *error_msg;
    switch (ret_code)
    {
    case ESTC_ERROR_NOT_FOUND:
        error_msg = "Unknown command. For help type 'help'";
        break;
    case ESTC_ERROR_INVALID_PARAM:
        error_msg = "Invalid parameters set. For help type 'help'";
        break;
    case ESTC_ERROR_INVALID_PARAM_DATA:
        error_msg = "Invalid parameters values. For help type 'help'";
        break;
    case ESTC_ERROR_NO_MEM:
        error_msg = "No Memory for operation";
        break;
    case ESTC_ERROR_SAME_DATA_NAME:
        error_msg = "The same color name exist";
        break;
    case ESTC_ERROR_SAME_DATA:
        error_msg = "The same data exist";
        break;
    case ESTC_ERROR_NAME_NOT_FOUND:
        error_msg = "Color not found.";
        break;
    default:
        error_msg = "Unknown error. For help type 'help'";
        break;
    }

    cli_usb_send_message(error_msg, strlen(error_msg), true);
}

void cli_cmd_handler(char const *usb_msg, uint8_t usb_msg_len)
{
    estc_ret_code_t ret_code = ESTC_ERROR_NOT_FOUND;

    for(size_t i = 0; i < ARRAY_SIZE(cli_commands); ++i)
    {
        const cli_cmd_cdescriptor_t *cli_cmd = &cli_commands[i];

        if(strncmp(usb_msg, cli_cmd->cmd_name, strlen(cli_cmd->cmd_name)) == 0)
        {
            ret_code = cli_cmd->cmd_handler(cli_cmd->cmd_name, usb_msg, usb_msg_len);
        }
    }

    if (ret_code != ESTC_SUCCESS)
        cli_cmd_error_handler(ret_code);
    else
        nvmc_read_rgb_actual_values_for_log();
}

static estc_ret_code_t handler_cmd_help(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 0;
    uint16_t params[param_num];

    estc_ret_code_t ret_code = cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num);
    static char *help_msg[] =
    {
        [0] = "\nSupport commands:",
        [1] = "1. 'help' to see this list",
        [2] = "2. rgb <r> <g> <b>. r, g, b: (0...255)",
        [3] = "3. hsv <h> <s> <v>. h: (0...360); s, v: (0...100)",
        [4] = "4. add_rgb_color <color_name> <r> <g> <b>. r, g, b: (0...255); color_name MAX length: 22",
        [5] = "5. del_color <color_name>",
        [6] = "6. apply_color <color_name>",
        [7] = "7. 'list_color' to view saved colors list",
    };

    if (ret_code == ESTC_SUCCESS)
    {
        for (size_t i = 0; i <= CLI_COMMANDS_NUMBER; ++i)
            cli_usb_send_message(help_msg[i], strlen(help_msg[i]) + 1, true);
    }

    return ret_code;
}

static estc_ret_code_t handler_cmd_rgb(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    estc_ret_code_t ret_code = cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num);

    if (ret_code == ESTC_SUCCESS)
    {
        if (params[0] <= RGB_MAX_VAL && params[1] <= RGB_MAX_VAL && params[2] <= RGB_MAX_VAL)
        {
            rgb.r = (uint8_t)params[0];
            rgb.g = (uint8_t)params[1];
            rgb.b = (uint8_t)params[2];
            rgb.crc = color_crc_calc_rgb(&rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Color set to %s %u %u %u", cmd_name, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
        }
        else
        {
            ret_code = ESTC_ERROR_INVALID_PARAM_DATA;
        }
    }
    return ret_code;
}

static estc_ret_code_t handler_cmd_hsv(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    const size_t param_num = 3;
    size_t cmd_msg_len = sizeof("Color set to ") + usb_msg_len - 1;
    rgb_t rgb;
    hsv_t hsv;
    uint16_t params[param_num];
    char cmd_msg[cmd_msg_len];
    estc_ret_code_t ret_code = cli_cmd_calc_param_values(usb_msg, params, strlen(cmd_name), param_num);

    if (ret_code == ESTC_SUCCESS)
    {
        if (params[0] <= CLI_HSV_MAX_H && params[1] <= CLI_HSV_MAX_S && params[2] <= CLI_HSV_MAX_V)
        {
            hsv.h = (uint16_t)params[0];
            hsv.s = (uint8_t)(params[1] * HSV_MAX_S / CLI_HSV_MAX_S);
            hsv.v = (uint8_t)(params[2] * HSV_MAX_V / CLI_HSV_MAX_V);
            color_hsv_to_rgb(&hsv, &rgb);
            color_value_set_rgb(&rgb);
            cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Color set to %s %u %u %u", cmd_name, params[0], params[1], params[2]);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
            nvmc_write_rgb_actual_values(&rgb);
        }
        else
        {
            ret_code = ESTC_ERROR_INVALID_PARAM_DATA;
        }
    }
    return ret_code;
}

static estc_ret_code_t handler_cmd_add_rgb_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    rgb_t rgb;
    const size_t param_num = 3;
    uint16_t params[param_num];
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    estc_ret_code_t ret_code;

    uint8_t param_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));

    if (param_pos > 0)
    {
        ret_code = cli_cmd_calc_param_values(usb_msg, params, param_pos, param_num);
        if (ret_code == ESTC_SUCCESS)
        {
            if (params[0] <= RGB_MAX_VAL && params[1] <= RGB_MAX_VAL && params[2] <= RGB_MAX_VAL)
            {
                rgb.r = (uint8_t)params[0];
                rgb.g = (uint8_t)params[1];
                rgb.b = (uint8_t)params[2];
                rgb.crc = color_crc_calc_rgb(&rgb);

                ret_code = nvmc_rgb_color_add(&rgb, color_name);

                if(ret_code == ESTC_SUCCESS)
                {
                    size_t cmd_msg_len = sizeof("Add color RGB ") + usb_msg_len - 1;
                    char cmd_msg[cmd_msg_len];
                    color_value_set_rgb(&rgb);
                    cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Add color '%s' RGB %u %u %u", color_name, params[0], params[1], params[2]);
                    cli_usb_send_message(cmd_msg, cmd_msg_len, true);
                    nvmc_write_rgb_actual_values(&rgb);
                }
            }
            else
                ret_code = ESTC_ERROR_INVALID_PARAM_DATA;
        }
        else
            ret_code = ESTC_ERROR_INVALID_PARAM;
    }
    else
        ret_code = ESTC_ERROR_INVALID_PARAM;

    return ret_code;
}

static estc_ret_code_t handler_cmd_del_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    uint8_t cmd_color_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));
    estc_ret_code_t ret_code;

    if (cmd_color_pos == 0)
        return ESTC_ERROR_INVALID_PARAM;

    if (!is_only_spaces(usb_msg, cmd_color_pos))
        return ESTC_ERROR_INVALID_PARAM;

    ret_code = nvmc_rgb_color_del(color_name);
    if (ret_code == ESTC_SUCCESS)
    {
        size_t cmd_msg_len = sizeof("Color '' deleted") + usb_msg_len - 1;
        char cmd_msg[cmd_msg_len];
        cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Color '%s' deleted", color_name);
        cli_usb_send_message(cmd_msg, cmd_msg_len, true);
    }

    return ret_code;
}

static estc_ret_code_t handler_cmd_apply_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    rgb_t rgb;
    char color_name[CLI_COLOR_NAME_MAX_LEN];
    uint8_t cmd_color_pos = cli_cmd_calc_word(usb_msg, color_name, strlen(cmd_name));
    estc_ret_code_t ret_code;

    if (cmd_color_pos == 0)
        return ESTC_ERROR_INVALID_PARAM;

    if (!is_only_spaces(usb_msg, cmd_color_pos))
        return ESTC_ERROR_INVALID_PARAM;

    ret_code = nvmc_rgb_color_get_vals(&rgb, color_name);

    if (ret_code == ESTC_SUCCESS)
    {
        if(color_rgb_values_validate(&rgb))
        {
            size_t cmd_msg_len = sizeof("Color '' applyed") + usb_msg_len - 1;
            char cmd_msg[cmd_msg_len];

            color_value_set_rgb(&rgb);
            nvmc_write_rgb_actual_values(&rgb);
            cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Color '%s' applyed", color_name);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
        }
        else
        {
            size_t cmd_msg_len = sizeof("Color '' CRC check faild. Color doesn't set.") + usb_msg_len - 1;
            char cmd_msg[cmd_msg_len];

            cmd_msg_len = snprintf(cmd_msg, cmd_msg_len, "Color '%s' CRC check faild. Color doesn't set.", color_name);
            cli_usb_send_message(cmd_msg, cmd_msg_len, true);
        }
    }

    return ret_code;
}

static estc_ret_code_t handler_cmd_list_color(char const *cmd_name, char const *usb_msg, uint8_t usb_msg_len)
{
    rgb_t rgb[CLI_SAVED_COLORS_MAX_NUM];
    size_t cmd_msg_len = sizeof("Color 12 : RGB 123 123 123 : ") + CLI_COLOR_NAME_MAX_LEN + 1;
    char color_name[CLI_SAVED_COLORS_MAX_NUM][CLI_COLOR_NAME_MAX_LEN + 1];
    char cmd_msg[cmd_msg_len];

    if (!is_only_spaces(usb_msg, strlen(cmd_name)))
        return ESTC_ERROR_INVALID_PARAM;

    if (nvmc_rgb_color_get_list(rgb, color_name) == ESTC_SUCCESS)
    {
        size_t counter = 0;

        while (color_crc_calc_rgb(&rgb[counter]) == rgb[counter].crc && counter < CLI_SAVED_COLORS_MAX_NUM)
        {
            snprintf(cmd_msg, cmd_msg_len, "Color %2d : RGB %3u %3u %3u : %s",
                     counter + 1, rgb[counter].r, rgb[counter].g, rgb[counter].b, color_name[counter]);

            cli_usb_send_message(cmd_msg, strlen(cmd_msg), true);
            ++counter;
        }
    }
    else
    {
        cli_usb_send_message("Color list is empty.", sizeof("Color list is empty."), true);
    }
    return ESTC_SUCCESS;
}

static estc_ret_code_t cli_cmd_calc_param_values(char const command[], uint16_t params[], size_t param_position, size_t const params_qty)
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

                while (isdigit((int)command[param_position]) > 0)
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
            return ESTC_ERROR_INVALID_PARAM;
        }
    }

    if (param_counter < params_qty)
        return ESTC_ERROR_INVALID_PARAM;
    else
        return ESTC_SUCCESS;
}

static bool is_only_spaces(char const *const string, uint8_t start_pos)
{
    while (string[start_pos] != '\0')
    {
        if (string[start_pos] != ' ')
            return false;
        ++start_pos;
    }
    return true;
}

static size_t cli_cmd_calc_word(char const command[], char word[], size_t param_position)
{
    uint8_t char_position = 0;
    word[0] = '\0';

    if (command[param_position] != ' ')
        return 0;

    do
    {
        ++param_position;
    } while (command[param_position] == ' ');

    if(isdigit((int)command[param_position]) > 0)
        return 0;

    while (isalnum((int)command[param_position]) > 0)
    {
        if (char_position == CLI_COLOR_NAME_MAX_LEN - 1)
            return 0;

        word[char_position] = command[param_position];
        ++char_position;
        ++param_position;
    }

    memset(word + char_position, '\0', SIZE_OF_COLOR_NAME_MAX_BYTES - char_position);

    return param_position;
}