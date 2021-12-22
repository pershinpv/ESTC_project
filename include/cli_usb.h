#ifndef CLI_USB_H
#define CLI_USB_H

#include <stdbool.h>
#include <stdint.h>
#include "cli_cmd.h"

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4

#define READ_SIZE 1
#define CLI_USB_CMD_MAX_LEN CLI_COMMAND_MAX_LEN

#define KEY_DELETE 127      //DELETE key code. It's come from backspace key.
#define KEY_ESCAPE 27

void cli_usb_cdc_acm_init(void usb_msg_handler(char const * usb_msg, uint8_t usb_msg_len));
void cli_usb_send_message(char const msg[], size_t msg_lenght, bool is_line_feed);

#endif