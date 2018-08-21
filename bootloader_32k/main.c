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
 * $LastChangedRevision: 235 $
 */

/** @file
 * This file copies the bootloader code to XDATA RAM and starts the bootlader command parser
 *
 */
#include <srom.h>
#include <stdint.h>

#include "bootloader.h"
#include "config.h"

#if __C51__ < 810 && !defined(_lint)
#error "This project requires Keil C51 v8.10 or higher"
#endif

//
// For a description of the Special ROM (SROM) functionality in Keil please
// search for SROM on the Keil web site and read the two articles
// "C51: IN-SYSTEM FLASH PROGRAMMING (PART 1)" and "C51: IN-SYSTEM FLASH
// PROGRAMMING (PART 2)"
//
// These macros will place the code/const from the BOOTLOADER segment in
// the hex file/flash at the location given by SROM (see linker settings):
SROM_MC (CODE_BOOTLOADER)
SROM_MC (CONST_BOOTLOADER)

void main(void)
{
    uint16_t i;

    //
    // copy bootloader functions from FLASH to RAM:
    uint8_t code *psrc = (uint8_t code*)SROM_MC_SRC(CODE_BOOTLOADER);
    uint8_t xdata *pdest = (uint8_t xdata*)SROM_MC_TRG(CODE_BOOTLOADER);
    for(i=0;i<SROM_MC_LEN(CODE_BOOTLOADER);i++)
    {
        *pdest++ = *psrc++;
    }
    //
    // Copy bootloader constants from FLASH to RAM:
    psrc = (uint8_t code*)SROM_MC_SRC(CONST_BOOTLOADER);
    pdest = (uint8_t xdata*)SROM_MC_TRG(CONST_BOOTLOADER);
    for(i=0;i<SROM_MC_LEN(CONST_BOOTLOADER);i++)
    {
        *pdest++ = *psrc++;
    }
    bootloader(); // Will never return
}
