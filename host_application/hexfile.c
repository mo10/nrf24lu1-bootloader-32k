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
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "hexfile.h"

static int read_hex_line(char *line, unsigned char *buf, unsigned bufSize, unsigned *lowAddr, unsigned *highAddr)
{
    unsigned long addr;
    unsigned nbytes, tmp, csum, i;

    while (isspace(*line))
        line++;
    if (sscanf(line, ":%02X%04lX%02X", &nbytes, &addr, &tmp) != 3)
        return ERR_FMT;
    line = &line[9];
    csum = (nbytes & 0xff) + (addr & 0xff) + ((addr >> 8) & 0xff) + (tmp & 0xff);
    switch(tmp)
    {
        case 0: // Data record
            for (i=0;i<nbytes;i++)
            {
                sscanf(line, "%02X", &tmp);
                if (addr >= bufSize)
                {
                    return ERR_ADDR;
                }
                if (addr > *highAddr)
                    *highAddr = addr;
                if (addr < *lowAddr)
                    *lowAddr = addr;
                buf[addr] = (unsigned char)tmp;
                csum += tmp;
                line = &line[2];
                addr++;
            }
            break;

        case 2: // Extended segment address record not supported at this time:
            return ERR_FMT;

        case 3: // Ignore record type 3 (program start address?)
            for (i=0;i<nbytes;i++)
            {
                sscanf(line, "%02X", &tmp);
                csum += tmp;
                line = &line[2];
            }
            break;

        case 1: // End record
            default:
            break;
    }
    sscanf(line, "%02X", &tmp);
    csum = (~csum+1) & 0xff;
    if (csum != tmp)
        return ERR_CRC;
    return NO_ERR;
}

int read_hex_file(FILE *fp, int crc, unsigned char *buf, unsigned buf_size, unsigned *low_addr, unsigned *high_addr)
{
    int res, lcount, err;
    char line[1000];
    lcount = 0;
    err = 0;
    while (!feof(fp) && !ferror(fp))
    {
        fgets(line, 1000, fp);
        lcount++;
        if ((res = read_hex_line(line, buf, buf_size, low_addr, high_addr)) != NO_ERR)
        {
            switch(res)
            {
                case ERR_CRC:
                    if (crc == 1)
                    {
                        fprintf(stderr, "ERROR: Checksum error on line %d\n", lcount);
                        err = res;;
                    }
                    break;

                case ERR_ADDR:
                    fprintf(stderr, "ERROR: Hex file contents does not fit into flash\n");
                    return res;

                case ERR_FMT:
                    fprintf(stderr, "ERROR: Invalid Intel hex format on line %d\n", lcount);
                    return res;
            }
        }
    }
    return err;
}
