/*********************************************************************************
* WW-11 (and WW-12) Wheelwatcher Firmware for Atmel ATTiny85                     *
*                                                                                *
* File: ww_settings.h                                                            *
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

#ifndef _WW_SETTINGS_H_
#define _WW_SETTINGS_H_

#include "ww_types.h"


// constants in EEPROM
typedef struct drive_constants
{
   BYTE unused;
   BYTE written;
   BYTE mode;
   BYTE baud;
   BYTE i2c_addr;
} DRIVE_CONSTANTS;

extern DRIVE_CONSTANTS dcon;

#define KVALID 0x7A

//            00         01      02      03     04        05 
enum ADDRS {A_skip, A_Written, A_Mode, A_Baud, A_I2CAddr, A_End};


extern void init_eeprom(SBYTE reset_values);
extern void store_cur_mode(void);
extern void store_cur_baud(void);
extern void store_i2c_address(void);

#endif
