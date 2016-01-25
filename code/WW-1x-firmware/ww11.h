/*********************************************************************************
* WW-11 (and WW-12) Wheelwatcher Firmware for Atmel ATTiny85                     *
*                                                                                *
* File: ww11,h                                                                   *
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

#ifndef _WW11_H_
#define _WW11_H_

typedef enum _cm_mode_
{
    SIGN_MAG, QUADRATURE, SERIAL, I2C, SPI
} MODES;

extern MODES cm_mode;

#define DEFAULT_MODE QUADRATURE
#define DEFAULT_BAUD 4 // standard value for 38400
#define WW_DEV_TYPE 0x30
#endif

