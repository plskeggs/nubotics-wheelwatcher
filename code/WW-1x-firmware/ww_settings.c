/*********************************************************************************
* WW-11 (and WW-12) Wheelwatcher Firmware for Atmel ATTiny85                     *
*                                                                                *
* File: ww_settings.c                                                            *
* This contains code that manages the storing of settings in EEPROM.             *
*                                                                                *
* Copyright 2009-2012, Noetic Design, Inc.                                       *
*                                                                                *
* This file is part of the WW-11 firmware.                                       *
*                                                                                *
* The WW-11 firmware is free software: you can redistribute it and/or modify     *
* it under the terms of the GNU General Public License as published by           *
* the Free Software Foundation, either version 3 of the License, or              *
* (at your option) any later version.                                            *
*                                                                                *
* The WW-11 firmware is distributed in the hope that it will be useful,          *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                 *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                  *
* GNU General Public License for more details.                                   *
*                                                                                *
* You should have received a copy of the GNU General Public License              *
* along with the WW-11 firmware, in the file COPYING.                            *
* If not, see <http://www.gnu.org/licenses/>.                                    *
*********************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "ww_types.h"
#include "ww11.h"
#include "ww_settings.h"
#include "USI_UART/USI_UART_config.h"
#include "USI_I2C/USI_TWI_Slave.h"

// user-modifiable constants that affect performance
DRIVE_CONSTANTS dcon;

const DRIVE_CONSTANTS default_dcon /*__attribute__ ((section (".eeprom"))) */ =
{
  0xff, KVALID, DEFAULT_MODE, DEFAULT_BAUD, WW_DEV_TYPE 
};

const DRIVE_CONSTANTS stored_dcon __attribute__ ((section (".eeprom"))) =
{
  0xff, KVALID, DEFAULT_MODE, DEFAULT_BAUD, WW_DEV_TYPE 
};



/******************************************************************************/
/* Init EEPROM                                                                */
/*                                                                            */
/* If eeprom contains valid data, read constants from it; otherwise, write    */
/* them for the first time.                                                   */
/******************************************************************************/
void init_eeprom(SBYTE reset_values)
{
   SBYTE val;
   BYTE * adr;
   BYTE * src;
   BYTE * dst;
   int i;

   adr = (BYTE *)A_Written;
   val = eeprom_read_byte(adr);
   if ((val != KVALID) || reset_values)             // have we upgraded to new firmware which requires an updated set of constants, or are we being forced?
   {
      adr = (BYTE *)0;                              // reset drive constants to defaults
      src = (BYTE *)&default_dcon;                  // sizeof(DRIVE_CONSTANTS);
      for (i = 0; i < sizeof(DRIVE_CONSTANTS); i++)
         eeprom_write_byte(adr++, *(src++));        // eeprom_read_byte(src++));
      reset_values = 0;
   }
   adr = (BYTE *)0;
   dst = (BYTE *)&dcon;
   for (i = 0; i < sizeof(DRIVE_CONSTANTS); i++)        // return current PID loop constants
      *(dst++) = eeprom_read_byte(adr++);
   dcon.baud = DEFAULT_BAUD;
   dcon.mode = cm_mode;   // can't change mode or baud rate
   //set_cur_baud(dcon.baud);
   TWI_slaveAddress = dcon.i2c_addr;
}


/******************************************************************************/
/* STORE CUR BAUD                                                             */
/*                                                                            */
/******************************************************************************/
void store_cur_baud(void)
{
   SBYTE val;
   BYTE * adr;
   adr = (BYTE *)A_Baud;
   val = DEFAULT_BAUD;
   if (eeprom_read_byte(adr) != val)
      eeprom_write_byte(adr, val);
}


/******************************************************************************/
/* STORE I2C ADDRESS                                                          */
/*                                                                            */
/******************************************************************************/
void store_i2c_address(void)
{
   SBYTE val;
   BYTE * adr;
   adr = (BYTE *)A_I2CAddr;
   val = TWI_slaveAddress;
   eeprom_write_byte(adr, val);
}



