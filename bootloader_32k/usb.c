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
 * Minimalistic USB code for the bootloader.
 *
 */
#include <Nordic\reg24lu1.h>
#include <intrins.h>
#include <stdbool.h>

#include "config.h"
#include "usb.h"

// Place all code and constants in this file in the segment "BOOTLOADER":
#pragma userclass (code = BOOTLOADER)
#pragma userclass (const = BOOTLOADER)

/** Leaves the minimum of the two arguments */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// USB map:
xdata volatile uint8_t out1buf[USB_EP1_SIZE]        _at_ 0xC640;
xdata volatile uint8_t in1buf[USB_EP1_SIZE]         _at_ 0xC680;
xdata volatile uint8_t out0buf[MAX_PACKET_SIZE_EP0] _at_ 0xC6C0;
xdata volatile uint8_t in0buf[MAX_PACKET_SIZE_EP0]  _at_ 0xC700;
xdata volatile uint8_t bout1addr                    _at_ 0xC781;
xdata volatile uint8_t bout2addr                    _at_ 0xC782;
xdata volatile uint8_t bout3addr                    _at_ 0xC783;
xdata volatile uint8_t bout4addr                    _at_ 0xC784;
xdata volatile uint8_t bout5addr                    _at_ 0xC785;
xdata volatile uint8_t binstaddr                    _at_ 0xC788;
xdata volatile uint8_t bin1addr                     _at_ 0xC789;
xdata volatile uint8_t bin2addr                     _at_ 0xC78A;
xdata volatile uint8_t bin3addr                     _at_ 0xC78B;
xdata volatile uint8_t bin4addr                     _at_ 0xC78C;
xdata volatile uint8_t bin5addr                     _at_ 0xC78D;
xdata volatile uint8_t ivec                         _at_ 0xC7A8;
xdata volatile uint8_t in_irq                       _at_ 0xC7A9;
xdata volatile uint8_t out_irq                      _at_ 0xC7AA;
xdata volatile uint8_t usbirq                       _at_ 0xC7AB;
xdata volatile uint8_t in_ien                       _at_ 0xC7AC;
xdata volatile uint8_t out_ien                      _at_ 0xC7AD;
xdata volatile uint8_t usbien                       _at_ 0xC7AE;
xdata volatile uint8_t ep0cs                        _at_ 0xC7B4;
xdata volatile uint8_t in0bc                        _at_ 0xC7B5;
xdata volatile uint8_t in1cs                        _at_ 0xC7B6;
xdata volatile uint8_t in1bc                        _at_ 0xC7B7;
xdata volatile uint8_t out0bc                       _at_ 0xC7C5;
xdata volatile uint8_t out1cs                       _at_ 0xC7C6;
xdata volatile uint8_t out1bc                       _at_ 0xC7C7;
xdata volatile uint8_t usbcs                        _at_ 0xC7D6;
xdata volatile uint8_t inbulkval                    _at_ 0xC7DE;
xdata volatile uint8_t outbulkval                   _at_ 0xC7DF;
xdata volatile uint8_t inisoval                     _at_ 0xC7E0;
xdata volatile uint8_t outisoval                    _at_ 0xC7E1;
xdata volatile uint8_t setupbuf[8]                  _at_ 0xC7E8;

static uint8_t usb_bm_state;
static uint8_t usb_current_config;
static uint8_t usb_current_alt_interface;
static usb_state_t usb_state;

static uint8_t code * packetizer_data_ptr;
static uint8_t packetizer_data_size;
static uint8_t packetizer_pkt_size;
static uint8_t bmRequestType;

bool packet_received;

static void packetizer_isr_ep0_in();
static void usb_process_get_status();
static void usb_process_get_descriptor();

static void delay_ms(uint16_t ms)
{
    uint16_t i, j;
    
    for(i = 0; i < ms; i++ )
    {
        for( j = 0; j < 1403; j++)
        {
            _nop_();
        }
    }
}

void usb_init(void)
{
    // Setup state information
    usb_state = DEFAULT;
    usb_bm_state = 0;

    // Setconfig configuration information
    usb_current_config = 0;
    usb_current_alt_interface = 0;
    
    // Disconnect from USB-bus since we are in this routine from a power on and not a soft reset:

    usbcs |= 0x08;
    delay_ms(50);
    usbcs &= ~0x08;

    usbien = 0x1d;
    in_ien = 0x01;
    in_irq = 0x1f;
    out_ien = 0x01;
    out_irq = 0x1f;

    // Setup the USB RAM with some OK default values:
    bout1addr = MAX_PACKET_SIZE_EP0/2;
    bout2addr = MAX_PACKET_SIZE_EP0/2 + USB_EP1_SIZE/2;
    bout3addr = MAX_PACKET_SIZE_EP0/2 + 2*USB_EP1_SIZE/2;
    bout4addr = MAX_PACKET_SIZE_EP0/2 + 3*USB_EP1_SIZE/2;
    bout5addr = MAX_PACKET_SIZE_EP0/2 + 4*USB_EP1_SIZE/2;
    binstaddr = 0xc0;
    bin1addr = MAX_PACKET_SIZE_EP0/2;
    bin2addr = MAX_PACKET_SIZE_EP0/2 + USB_EP1_SIZE/2;
    bin3addr = MAX_PACKET_SIZE_EP0/2 + 2*USB_EP1_SIZE/2;
    bin4addr = MAX_PACKET_SIZE_EP0/2 + 3*USB_EP1_SIZE/2;
    bin5addr = MAX_PACKET_SIZE_EP0/2 + 4*USB_EP1_SIZE/2;

    // Set all endpoints to not valid (except EP0IN and EP0OUT)
    inbulkval = 0x01;
    outbulkval = 0x01;
    inisoval = 0x00;
    outisoval = 0x00;

    in_ien |= 0x02;; 
    inbulkval |= 0x02;
    out_ien |= 0x02;
    outbulkval |= 0x02;
    out1bc = 0xff;
}

static void packetizer_isr_ep0_in()
{
    uint8_t code* data_ptr; 
    uint8_t size, i;
    // We are getting a ep0in interupt when the host send ACK and do not have any more data to send
    if(packetizer_data_size == 0)
    {
        in0bc = 0;
        USB_EP0_HSNAK();
        return;
    }

    size = MIN(packetizer_data_size, packetizer_pkt_size);

    // Copy data to the USB-controller buffer
    data_ptr = packetizer_data_ptr;
    for(i = 0; i < size;i++)
    {
        in0buf[i] = *data_ptr++;
    }

    // Tell the USB-controller how many bytes to send
    // If a IN is received from host after this the USB-controller will send the data
    in0bc = size;

    // Update the packetizer data
    packetizer_data_ptr += size;
    packetizer_data_size -= size;
}

static void usb_process_get_status()
{
    in0buf[0] = in0buf[1] = 0x00;
    if((usb_state == ADDRESSED) && (setupbuf[4] == 0x00))
    {
        in0bc = 0x02;
    }
    else if(usb_state == CONFIGURED)
    {
        switch(bmRequestType)
        {
            case 0x80: // Device
                if((usb_bm_state & USB_BM_STATE_ALLOW_REMOTE_WAKEUP ) == USB_BM_STATE_ALLOW_REMOTE_WAKEUP)
                {
                    in0buf[0] = 0x02;
                }
                in0bc = 0x02;
                break;

            case 0x81: // Interface
                in0bc = 0x02;
                break;

            case 0x82: // Endpoint
                if((setupbuf[4] & 0x80) == 0x80) // IN endpoints
                    in0buf[0] = in1cs;
                else
                    in0buf[0] = out1cs;
                in0bc = 0x02;
                break;
            default:
                USB_EP0_STALL();
                break;
        }
    }
    else
    {
        // We should not be in this state
        USB_EP0_STALL();
    }
}

static void usb_process_get_descriptor()
{ 
    packetizer_pkt_size = MAX_PACKET_SIZE_EP0;
    // Switch on descriptor type
    switch(setupbuf[3])
    {
        case USB_DESC_DEVICE:
            packetizer_data_ptr = (uint8_t*)&g_usb_dev_desc;
            packetizer_data_size = MIN(setupbuf[6], sizeof(usb_dev_desc_t));
            packetizer_isr_ep0_in();
            break;

        case USB_DESC_CONFIGURATION:
            // For now we just support one configuration. The asked configuration is stored in LSB(wValue).
            packetizer_data_ptr = (uint8_t*)&g_usb_conf_desc;
            packetizer_data_size = MIN(setupbuf[6], sizeof(usb_conf_desc_bootloader_t));
            packetizer_isr_ep0_in();
            break;

        case USB_DESC_STRING:
            // For now we just support english as string descriptor language.
            if(setupbuf[2] == 0x00)
            {
                packetizer_data_ptr = string_zero;
                packetizer_data_size = MIN(setupbuf[6], sizeof(string_zero));
                packetizer_isr_ep0_in();
            }
            else
            {
                if((setupbuf[2] - 1) < USB_STRING_DESC_COUNT)
                {
                    if (setupbuf[2] == 1)
                        packetizer_data_ptr = g_usb_string_desc_1;
                    else
                        packetizer_data_ptr = g_usb_string_desc_2;;
                    packetizer_data_size = MIN(setupbuf[6], packetizer_data_ptr[0]);
                    packetizer_isr_ep0_in();
                }
                else
                {
                    USB_EP0_STALL();
                }
            }
            break;
        case USB_DESC_INTERFACE:
        case USB_DESC_ENDPOINT:
        case USB_DESC_DEVICE_QUAL:
        case USB_DESC_OTHER_SPEED_CONF:
        case USB_DESC_INTERFACE_POWER:
            USB_EP0_STALL();
            break;
        default:
            USB_EP0_HSNAK();
            break;
    }
}

static void isr_sudav()
{
    bmRequestType = setupbuf[0];
    if((bmRequestType & 0x60 ) == 0x00)
    {
        switch(setupbuf[1])
        {
           case USB_REQ_GET_DESCRIPTOR:
               usb_process_get_descriptor();
               break;

           case USB_REQ_GET_STATUS:
               usb_process_get_status();
               break;

            case USB_REQ_SET_ADDRESS:
               usb_state = ADDRESSED;
               usb_current_config = 0x00;
               break;

            case USB_REQ_GET_CONFIGURATION:
                switch(usb_state)
                {
                    case ADDRESSED:
                        in0buf[0] = 0x00;
                        in0bc = 0x01;
                        break;
                    case CONFIGURED:
                        in0buf[0] = usb_current_config;
                        in0bc = 0x01;
                        break;
                    case ATTACHED:
                    case POWERED:
                    case SUSPENDED:
                    case DEFAULT:
                    default:
                        USB_EP0_STALL();
                        break;
                }
                break;

            case USB_REQ_SET_CONFIGURATION:
                switch(setupbuf[2])
                {
                    case 0x00:
                        usb_state = ADDRESSED;
                        usb_current_config = 0x00;
                        USB_EP0_HSNAK();
                        break;
                    case 0x01:
                        usb_state = CONFIGURED;
                        usb_bm_state |= USB_BM_STATE_CONFIGURED;
                        usb_current_config = 0x01;
                        USB_EP0_HSNAK();
                        break;
                    default:
                        USB_EP0_STALL();
                        break;
                }
               break;

            case USB_REQ_GET_INTERFACE: // GET_INTERFACE
                in0buf[0] = usb_current_alt_interface;
                in0bc = 0x01;
                break;

            case USB_REQ_SET_DESCRIPTOR:
            case USB_REQ_SET_INTERFACE: // SET_INTERFACE
            case USB_REQ_SYNCH_FRAME:   // SYNCH_FRAME
            default:
                USB_EP0_STALL();
                break;
        }
    } 
    // bmRequestType = 0 01 xxxxx : Data transfer direction: Host-to-device, Type: Class
    else if((bmRequestType & 0x60 ) == 0x20)  // Class request
    {
        if(setupbuf[6] != 0 && ((bmRequestType & 0x80) == 0x00))
        {
            // If there is a OUT-transaction associated with the Control-Transfer-Write we call the callback
            // when the OUT-transaction is finished. Note that this function do not handle several out transactions.
            out0bc = 0xff;
        }
        else
        {
            USB_EP0_HSNAK();
        }
    } 
    else  // Unknown request type
    {
        USB_EP0_STALL();
    }
}

void usb_irq(void)
{
    //
    // Split case into an if and a switch to force Keil into not using a library function:
    if (ivec == INT_USBRESET)
    {
        usbirq = 0x10;
        usb_state = DEFAULT;
        usb_current_config = 0;
        usb_current_alt_interface = 0;
        usb_bm_state = 0;
    }
    else
    {
        switch(ivec)
        {
            case INT_SUDAV:
                usbirq = 0x01;
                isr_sudav();
                break;
            case INT_SOF:
                usbirq = 0x02;
                break;
            case INT_SUTOK:
                usbirq = 0x04;
                packetizer_data_ptr = NULL;
                packetizer_data_size = 0;
                packetizer_pkt_size = 0;
                break;
            case INT_SUSPEND:
                usbirq = 0x08;
                break;
            case INT_EP0IN:
                in_irq = 0x01;
                packetizer_isr_ep0_in();
                break;
            case INT_EP0OUT:
                out_irq = 0x01;
                packetizer_data_size = 0;
                //        req.misc_data = out0buf;
                USB_EP0_HSNAK();
                break;
            case INT_EP1IN:
                // Clear interrupt 
                in_irq = 0x02;
                in1cs = 0x02;
                break;
            case INT_EP1OUT:
                // Clear interrupt
                out_irq = 0x02;     
                packet_received = true;
                out1bc = 0xff;
                break;
            default:
                break;
        }
    }
}

