#ifndef CDC_ACM_CTRL_H
#define CDC_ACM_CTRL_H

#include <stdlib.h>
#include "app_usbd_cdc_acm.h"
#include "log.h"
#include "hsv.h"

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4

#define CLI_RGB_MAX_VAL 255

typedef enum
{
    CLI_NO_COMMAND = 0,
    CLI_SET_RGB = 1,
    CLI_SET_HSV = 2,
    CLI_HELP = 3,
    CLI_COMMANDS_QTY,
} cli_command_types_t;

typedef struct cli_command_s
{
    cli_command_types_t comand_type;
    uint16_t param_1;
    uint8_t param_2;
    uint8_t param_3;
    uint8_t comand_len;
} cli_command_t;

void cli_cdc_acm_connect_ep(void);
void cli_command_reset (void);
cli_command_t cli_command_to_do_get (void);
void cli_send_message(char const msg[], size_t msg_lenght, bool is_line_feed);
void cli_send_result_message();

# endif