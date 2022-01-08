#ifndef CLI_CMD_H_H
#define CLI_CMD_H_H

#include <stdlib.h>
#include "app_usbd_cdc_acm.h"
#include "log.h"
#include "color.h"
#include "nvmc.h"
#include "estc_error.h"

#define CLI_HSV_MAX_H   HSV_MAX_H
#define CLI_HSV_MAX_S   100
#define CLI_HSV_MAX_V   100

#define CLI_COMMANDS_NUMBER 7

#ifdef NVMC_H
#define CLI_COLOR_NAME_MAX_LEN (SIZE_OF_COLOR_NAME_MAX_BYTES - 1)
#define CLI_SAVED_COLORS_MAX_NUM SAVED_COLORS_MAX_NUM
#else
#define CLI_COLOR_NAME_MAX_LEN 23
#define CLI_SAVED_COLORS_MAX_NUM 10
#endif

#define CLI_COMMAND_MAX_LEN (CLI_COLOR_NAME_MAX_LEN + 27)   //add_rgb_color <color_name> <r> <g> <b>\0

typedef estc_ret_code_t (*cmd_handler_t)(char const * cmd_name, char const *usb_msg, uint8_t usb_msg_len);

typedef struct
{
    const char* cmd_name;
    cmd_handler_t cmd_handler;
} cli_cmd_cdescriptor_t;

void cli_cmd_command_reset (void);
void cli_cmd_handler(char const *usb_msg, uint8_t usb_msg_len);

//-------------old
/*
typedef enum
{
    CLI_RES_OK = 0,
    CLI_RES_NOT_FOUND = 1,
} cli_cmd_result_t;

typedef enum
{
    CLI_NO_COMMAND = 0,
    CLI_SET_RGB = 1,
    CLI_SET_HSV = 2,
    CLI_HELP = 3,
    SAVE_RGB_COLOR = 4,
    CLI_COMMANDS_QTY,
} cli_command_types_t;

typedef struct cli_command_s
{
    cli_command_types_t type;
    uint16_t param_1;
    uint8_t param_2;
    uint8_t param_3;
    uint8_t lenght;
} cli_command_t;

typedef struct
{
    rgb_t rgb_args;
    char cli_color_name[CLI_COLOR_NAME_MAX_LEN];
} cli_save_rgb_color_args_t;

typedef union cli_cmd_args_u
{
    rgb_t rgb;
    hsv_t hsv;
    cli_save_rgb_color_args_t save_rgb_color;
} cli_cmd_args_t;

typedef struct
{
    cli_cmd_args_t cli_cmd_args;
    cli_command_types_t type;
    uint8_t lenght;
} cli_cmd_command_t;*/
# endif