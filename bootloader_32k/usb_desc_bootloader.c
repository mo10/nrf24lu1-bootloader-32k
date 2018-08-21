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
 * USB descriptor for Bootloader
 *
 */
#include "config.h"

// Place all code and constants in this file in the segment "BOOTLOADER":
#pragma userclass (code = BOOTLOADER)
#pragma userclass (const = BOOTLOADER)

/** Swaps the upper byte with the lower byte in a 16 bit variable */
#define SWAP(x) ((((x)&0xFF)<<8)|(((x)>>8)&0xFF))

#include "usb_desc_bootloader.h"


code usb_dev_desc_t g_usb_dev_desc = {
  sizeof(usb_dev_desc_t), 
  USB_DESC_DEVICE, 
  SWAP(0x0200),       // bcdUSB
  0xff,               // bDeviceClass
  0xff,               // bDeviceSubclass
  0xff,               // bDeviceProtocol
  MAX_PACKET_SIZE_EP0,// bMaxPAcketSize0
  SWAP(6421),         // idVendor - 6421 (0x1915) : Nordic Semiconductor ASA
  SWAP(0x0101),       // idProduct
  SWAP(0x0001),       // bcdDevice - Device Release Number (BCD)
  0x01,               // iManufacturer
  0x02,               // iProduct
  0x00,               // iSerialNumber
  0x01                // bNumConfigurations
};

code usb_conf_desc_bootloader_t g_usb_conf_desc = 
{
    {
        sizeof(usb_conf_desc_t),
        USB_DESC_CONFIGURATION,
        SWAP(sizeof(usb_conf_desc_bootloader_t)),
        1,            // bNumInterfaces
        1,            // bConfigurationValue
        2,            // iConfiguration
        0xE0,         // bmAttributes - D6: self-powered, bus powered: 0xA0
        50,           // bMaxPower
    },
    /* Interface Descriptor 0 */ 
    {
        sizeof(usb_if_desc_t),
        USB_DESC_INTERFACE,
        0,            // bInterfaceNumber
        0,            // bAlternateSetting
        2,            // bNumEndpoints
        0xff,         // bInterfaceClass
        0x00,         // bInterfaceSubClass  
        0xff,         // bInterfaceProtocol 
        0x00,         // iInterface
    },
     /* Endpoint Descriptor EP1IN */
     {
        sizeof(usb_ep_desc_t),
        USB_DESC_ENDPOINT,
        0x81,                   // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK, // bmAttributes
        SWAP(USB_EP1_SIZE),     // wMaxPacketSize
        0x06                    // bInterval
     },
     /* Endpoint Descriptor EP1OUT */
     {
        sizeof(usb_ep_desc_t),
        USB_DESC_ENDPOINT,
        0x01,                   // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK, // bmAttributes
        SWAP(USB_EP1_SIZE),     // wMaxPacketSize
        0x06                    // bInterval
    },
};

#define USB_STRING_IDX_1_DESC "Nordic Semiconductor"

code uint8_t g_usb_string_desc_1[] = 
{
    sizeof(USB_STRING_IDX_1_DESC) * 2, 0x03,
    'N', 00,
    'o', 00,
    'r', 00,
    'd', 00,
    'i', 00,
    'c', 00,
    ' ', 00,
    'S', 00,
    'e', 00,
    'm', 00,
    'i', 00,
    'c', 00,
    'o', 00,
    'n', 00,
    'd', 00,
    'u', 00,
    'c', 00,
    't', 00,
    'o', 00,
    'r', 00 
};

#define USB_STRING_IDX_2_DESC "nRF24LU1P-F32 BOOT LDR"

code uint8_t g_usb_string_desc_2[] = 
{
    sizeof(USB_STRING_IDX_2_DESC) * 2, 0x03,
    'n', 00,
    'R', 00,
    'F', 00,
    '2', 00,
    '4', 00,
    'L', 00,
    'U', 00,
    '1', 00,
    'P', 00,
    '-', 00,
    'F', 00,
    '3', 00,
    '2', 00,
    ' ', 00,
    'B', 00,
    'O', 00,
    'O', 00,
    'T', 00,
    ' ', 00,
    'L', 00,
    'D', 00,
    'R', 00
};

// This is for setting language American English (String descriptor 0 is an array of supported languages):
code uint8_t string_zero[] = {0x04, 0x03, 0x09, 0x04} ;
