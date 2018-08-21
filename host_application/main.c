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
#include <getopt.h>
#include "hexfile.h"
#include "flashprog.h"

const unsigned short VID_NORDIC     = 0x1915;
const unsigned short PID_LU1BOOT    = 0x0101;

static unsigned char hex_buf[MAX_FLASH_SIZE];

usb_dev_handle *find_and_open_usb(unsigned short vid, unsigned short pid)
{
    struct usb_bus *bus;
    struct usb_device *dev;
    usb_dev_handle *hdev;

    usb_find_busses();      // Find all USB busses on this system
    usb_find_devices();     // Find all USB devices

    for(bus = usb_busses; bus; bus = bus->next) 
    {
        for(dev = bus->devices; dev; dev = dev->next) 
        {
            if(dev->descriptor.idVendor == vid && dev->descriptor.idProduct == pid)
            {
                // Bootlader found. Open a connection to it:
                hdev = usb_open(dev);
                if(usb_set_configuration(hdev, 1) < 0)
                {
                    usb_close(hdev);
                    continue;
                }
                if(usb_claim_interface(hdev, 0) < 0)
                {
                    usb_close(hdev);
                    continue;
                }
                return hdev;
            }
        }
    }
    return 0;
}

void print_usage(void)
{
    fprintf(stderr, "bootlu1p Modified by Mo10 v0.1\n");
    fprintf(stderr, "usage: bootlu1p [options] <hex-file>\n");
    fprintf(stderr, "       options:\n");
    fprintf(stderr, "       -r Reset after programming\n");
    fprintf(stderr, "       -f 16 Flash size is 16K Bytes\n");
    fprintf(stderr, "       -f 32 Flash size is 32K Bytes\n");
}

int main(int argc, char* argv[])
{   
    char c;
    unsigned flash_size, i, low_addr = 0, high_addr = 0, auto_reset = 0;
    FILE *fp;
    usb_dev_handle *hdev;

    while((c = getopt(argc, argv, "rf:")) != EOF)
    {
        switch(c)
        {
        case 'f':
            i = (unsigned)atoi(optarg);
            if ((i != 16) && (i != 32))
            {
                print_usage();
                exit(EXIT_FAILURE);
            }
            flash_size = i * 1024;
            break;
        case 'r':
            auto_reset = 1;
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }
    if ((argc - optind) != 1)
    {
        print_usage();
        exit(EXIT_FAILURE);
    }

    if ((fp = fopen(argv[optind], "rt")) == 0)
    {
        fprintf(stderr, "ERROR: Can't open input file <%s>\n", argv[optind]);
        return 1;
    }    
    usb_init();             // Initialize libusb
    hdev = find_and_open_usb(VID_NORDIC, PID_LU1BOOT);
    if (hdev == 0)
    {
        fprintf(stderr, "ERROR: nRF24LU1P Bootloader not found\n");
        exit(EXIT_FAILURE);
    }
    for(i=0;i<flash_size;i++)
        hex_buf[i] = 0xff;
    if (read_hex_file(fp, 1, hex_buf, flash_size, &low_addr, &high_addr) != NO_ERR)
    {
        exit(EXIT_FAILURE);
    }
    if (!flash_prog(hdev, low_addr, high_addr, flash_size, hex_buf))
    {
        fprintf(stderr, "ERROR: There was an error programming the flash\n");
        exit(EXIT_FAILURE);
    }
    if (auto_reset)
    {
    	reset_bootl(hdev);
    }
    usb_close(hdev);
    exit(EXIT_SUCCESS);
}
