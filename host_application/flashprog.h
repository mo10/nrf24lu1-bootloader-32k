/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA. Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef FLASH_PROG_H_
#define FLASH_PROG_H_

void reset_bootl(usb_dev_handle *hdev);
int flash_prog(usb_dev_handle *hdev, unsigned low_addr, unsigned high_addr, unsigned flash_size, unsigned char *hex_buf);

#define USB_EP_SIZE         64
#define FLASH_PAGE_SIZE     512
#define NUM_FLASH_BLOCKS    FLASH_PAGE_SIZE / USB_EP_SIZE
#define MAX_FLASH_SIZE      32*1024

#endif // FLASH_PROG_H_
