/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA.Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *              
 * $LastChangedRevision: 133 $
 */

/** @file
 * This file contain parsing of USB commands
 *
 */
#include <Nordic\reg24lu1.h>
#include <intrins.h>

#include "usb.h"
#include "bootloader.h"
#include "version.h"
#include "usb_cmds.h"
#include "flash.h"
#include "config.h"

// Place all code and constants in this file in the segment "BOOTLOADER":
#pragma userclass (code = BOOTLOADER)
#pragma userclass (const = BOOTLOADER)

extern bool packet_received;

extern xdata volatile uint8_t in1buf[];
extern xdata volatile uint8_t out1buf[];
extern xdata volatile uint8_t in1bc;
extern xdata volatile uint8_t usbcs;

static xdata uint8_t rdismb _at_ 0x0023;                // Readback Disable byte in InfoPage

static bool page_write;
static uint16_t nblock;                                 // Holds the number of the current USB_EP1_SIZE bytes block
static uint8_t nblocks;                                 // Holds number of the blocks programmed

static bool idata used_flash_pages[NUM_FLASH_PAGES];    // Holds which flash pages to erase


void parse_commands(void)
{
    uint8_t count = 0, tmp;

    if(page_write)
    {
        // Multiply nblock with 64 to get block start address in flash:
        flash_bytes_write(nblock << 6, out1buf, USB_EP1_SIZE);
        nblock++;
        nblocks++;
        in1buf[0] = 0;
        count = 1;
        if (nblocks == (FLASH_PAGE_SIZE/USB_EP1_SIZE))
        {
            page_write = false;
        }
    }
    else
    {
        switch(out1buf[0])
        {
            case CMD_FIRMWARE_VERSION:
                in1buf[0] = FW_VER_MAJOR;
                in1buf[1] = FW_VER_MINOR;
                count = 2;
                break;

            case CMD_FLASH_ERASE_PAGE:
                flash_page_erase(out1buf[1]);
                used_flash_pages[out1buf[1]] = false;
                in1buf[0] = 0;
                count = 1;
                break;

            case CMD_FLASH_WRITE_INIT:                  // Eight 64 bytes bulk packets <- PC follow after this command
                if (used_flash_pages[out1buf[1]])
                {
                    flash_page_erase(out1buf[1]);
                }
                used_flash_pages[out1buf[1]] = 1;
                nblock = (uint16_t)out1buf[1] << 3;     // Multiply page number by 8 to get block number
                nblocks = 0;
                page_write = true;
                in1buf[0] = 0;
                count = 1;
                break;

            case CMD_FLASH_READ:
                // Read one USB_EP1_SIZE bytes block from the address given
                // by out1buf[1] << 6 and MS bit set by CMD_FLASH_SELECT_HALF
                // below:
                nblock = (nblock & 0xff00) | (uint16_t)out1buf[1];
                if (RDIS)
                {
                    // RDISMB is set. Will return 0x00 for pages that are in use and 0xff
                    // for unused pages.
                    if (used_flash_pages[nblock >> 3])
                        tmp = 0x00;
                    else
                        tmp = 0xff;
                    for(count=0;count<USB_EP1_SIZE;count++)
                        in1buf[count] = tmp;
                } 
                else
                    flash_bytes_read((uint16_t)nblock<<6, in1buf, USB_EP1_SIZE);
                count = USB_EP1_SIZE;
                break;

            case CMD_FLASH_SET_PROTECTED:
                count = 1;
                INFEN = 1;
                if (rdismb != 0xff)
                {
                    in1buf[0] = 1;
                }
                else
                {
                    flash_byte_write((uint16_t)&rdismb, 0x00);
                    in1buf[0] = 0;
                }
                INFEN = 0;
                break;

            case CMD_FLASH_SELECT_HALF:
                // When outbuf[1] = 0 program the lower half of the 32K bytes flash
                // and when outbuf[1] = 1 program the upper part:
                if (out1buf[1] == 1)
                    nblock = (nblock & 0x00ff) | 0x0100;
                else
                    nblock &= 0x00ff;
                in1buf[0] = 0;
                count = 1;
                break;
            case CMD_RESET:
                EA = 0;
                usbcs |= 0x08;
                // Reset MCU by activating watchdog
                REGXH = 0;
                REGXL = 1;
                REGXC = 0x08;
            default:
                break;
        }
    }
    if (count > 0)
        in1bc = count;
}

static void get_used_flash_pages(void)
{    
    uint8_t xdata *pb;
    uint8_t i;
    uint16_t j;
    //
    // Read through the whole flash to find out which flash
    // pages that are in use. Store the result in the NUM_FLASH_PAGES
    // sized array used_flash_pages[]:
    for(i=0;i<NUM_FLASH_PAGES;i++)
    {
        used_flash_pages[i] = false;        
        for(j=0,pb = (uint8_t xdata *)(FLASH_PAGE_SIZE * (uint16_t)i);j<FLASH_PAGE_SIZE;j++, pb++)
        {
            if(*pb != 0xff)
            {
                used_flash_pages[i] = true;
                break;
            }
        }
    }
}

void bootloader(void)
{
    EA = 0;
    get_used_flash_pages();
    usb_init();
    CKCON = 0x02;       // See nRF24LU1p AX PAN
    nblock = 0;
    packet_received = page_write = false;
    //
    // Enter an infinite loop waiting checking the USB interrupt flag and
    // call the interrupt handler, usb_irq, when the flag is set. The interrupt
    // handler will set the variable packet_received to true when a packet is
    // received.     
    for(;;)
    {
        if (USBF)
        {
            USBF = 0;
            usb_irq();
            if(packet_received)
            {
                parse_commands();
                packet_received = false;
            }
        }
    }
}
