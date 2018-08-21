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
#include "usb.h"
#include <stdio.h>
#include <string.h>
#include "bootldr_usb_cmds.h"
#include "flashprog.h"

const int BULK_OUT_EP = 0x01;
const int BULK_IN_EP = 0x81;

static char usb_write_buf[64];
static char usb_read_buf[64];

static void flash_page_program(usb_dev_handle *hdev, unsigned char *page_buf, int npage)
{
    int i;
    usb_write_buf[0] = CMD_FLASH_WRITE_INIT;
    usb_write_buf[1] = npage;
    usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, 2, 5000);
    usb_bulk_read(hdev, BULK_IN_EP, usb_read_buf, 1, 5000);
    for (i = 0; i < NUM_FLASH_BLOCKS; i++)
    {
        memcpy(usb_write_buf, &page_buf[i*USB_EP_SIZE], USB_EP_SIZE);
        usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, USB_EP_SIZE, 5000);
        usb_bulk_read(hdev, BULK_IN_EP, usb_read_buf, 1, 5000);
    }
}

static void flash_program(usb_dev_handle *hdev, unsigned char *hex_buf, int startpage, int npages)
{
    int i;
    unsigned char page_buf[FLASH_PAGE_SIZE];

    for (i = startpage; i < (startpage + npages); i++)
    {
        memcpy(page_buf, &hex_buf[i * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);
        flash_page_program(hdev, page_buf, i);
    }
}

static int flash_page_verify(usb_dev_handle *hdev, unsigned char *page_buf, int npage)
{
    int i, nblock, n, fail_addr;
    char fail_byte;
    for (i = 0; i < NUM_FLASH_BLOCKS; i++)
    {
        nblock = npage * NUM_FLASH_BLOCKS + i;

        // Select upper/lower flash half:
        usb_write_buf[0] = CMD_FLASH_SELECT_HALF;
        usb_write_buf[1] = (char)(nblock >> 8);
        usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, 2, 5000);
        usb_bulk_read(hdev, BULK_IN_EP, usb_read_buf, 1, 5000);
        
        usb_write_buf[0] = CMD_FLASH_READ;
        usb_write_buf[1] = (char)nblock;
        usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, 2, 5000);
        usb_bulk_read(hdev, BULK_IN_EP, usb_read_buf, USB_EP_SIZE, 5000);
        for (n = 0; n < USB_EP_SIZE; n++)
        {
            if ((unsigned char)usb_read_buf[n] != page_buf[i * USB_EP_SIZE + n])
            {
                fail_addr = npage * FLASH_PAGE_SIZE + i * USB_EP_SIZE + n;
                fail_byte = usb_read_buf[n];
                fprintf(stderr, "ERROR: The Flash contents does not match the file contents\nAddress = 0x%04X, Expected 0x%02X, got 0x%02X\n", fail_addr, (unsigned)page_buf[i * USB_EP_SIZE + n], (unsigned)fail_byte);
                return 0;
            }
        }
    }
    return 1;
}


static int flash_verify(usb_dev_handle *hdev, unsigned char *hex_buf, int startpage, int npages)
{
    int i;
    unsigned char page_buf[FLASH_PAGE_SIZE];

    for (i = startpage; i < (startpage + npages); i++)
    {
        memcpy(page_buf, &hex_buf[i * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);
        if (!flash_page_verify(hdev, page_buf, i))
            return 0;
    }
    return 1;
}

int flash_prog(usb_dev_handle *hdev, unsigned low_addr, unsigned high_addr,unsigned flash_size,  unsigned char *hex_buf)
{
    unsigned num_flash_pages = flash_size/FLASH_PAGE_SIZE;
    fprintf(stdout, "Programming flash pages 1-%d...\n", num_flash_pages - 5);
    //
    // First program and verify the flash pages above page 0 and below the bootloader
    // (last four pages of the flash):
    flash_program(hdev, hex_buf, 1, num_flash_pages - 5);
    fprintf(stdout, "Verifying flash pages 1-%d...\n", num_flash_pages - 5);
    if (flash_verify(hdev, hex_buf, 1, num_flash_pages - 5) == 0)
    {
        return 0;
    }
    //
    // Then program page 0 and the pages containing the bootloader:
    fprintf(stdout, "Programming flash page 0...\n", num_flash_pages - 5);
    flash_program(hdev, hex_buf, 0, 1);
    if (high_addr > (num_flash_pages - 4)*FLASH_PAGE_SIZE)
    {
        // Only program pages containing bootloader if user program uses these pages
        fprintf(stdout, "Programming flash pages %d-%d...\n", num_flash_pages - 4, num_flash_pages - 1);
        flash_program(hdev, hex_buf, num_flash_pages - 4, 4);
    }
    fprintf(stdout, "Verifying flash page 0...\n", num_flash_pages - 5);
    if (flash_verify(hdev, hex_buf, 0, 1) == 0)
        return 0;
    if (high_addr > (num_flash_pages - 4)*FLASH_PAGE_SIZE)
    {
        fprintf(stdout, "Verifying flash pages %d-%d...\n", num_flash_pages - 4, num_flash_pages - 1);
        if(flash_verify(hdev, hex_buf, num_flash_pages - 4, 4) == 0)
        {
            return 0;
        }
    }
    return 1;
}

void reset_bootl(usb_dev_handle *hdev)
{
    fprintf(stdout, "Resetting bootloader...\n");
    // get bootloader firmware version
    usb_write_buf[0] = CMD_FIRMWARE_VERSION;
    usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, 1, 5000);
    usb_bulk_read(hdev, BULK_IN_EP, usb_read_buf, 2, 5000);
    if (usb_read_buf[0]<=0x12)
    {
        fprintf(stderr, "Warning:Bootloader version is %d(<=%d),does not support auto reset!\n", usb_read_buf[0],0x12);
        return;
    }
    // reset bootloader
    usb_write_buf[0] = CMD_RESET;
    usb_bulk_write(hdev, BULK_OUT_EP, usb_write_buf, 1, 5000);
    return;
}