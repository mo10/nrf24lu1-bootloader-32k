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
#ifndef HEXFILE_H_
#define HEXFILE_H_

int read_hex_file(FILE *fp, int crc, unsigned char *buf, unsigned buf_size, unsigned *low_addr, unsigned *high_addr);

#define ERR_CRC  1
#define ERR_ADDR 2
#define ERR_FMT  3
#define NO_ERR   0

#endif  // HEXFILE_H_
