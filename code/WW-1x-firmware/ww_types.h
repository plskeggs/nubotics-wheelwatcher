/*********************************************************************************
* WW-11 (and WW-12) Wheelwatcher Firmware for Atmel ATTiny85                     *
*                                                                                *
* File: ww_types.h                                                               *
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

#ifndef _WW_TYPES_H_
#define _WW_TYPES_H_

#define TRUE 1
#define FALSE 0

#ifndef BOOL
typedef uint8_t BOOL;
#endif
#ifndef BYTE
typedef uint8_t BYTE;
#endif
#ifndef SBYTE
typedef int8_t SBYTE;
#endif
#ifndef WORD
typedef uint16_t WORD;
#endif
#ifndef SWORD
typedef int16_t SWORD;
#endif
#ifndef DWORD
typedef uint32_t DWORD;
#endif
#ifndef SDWORD
typedef int32_t SDWORD;
#endif

#define SETBIT(x, y) (x) |= (y)
#define CLRBIT(x, y) (x) &= ~(y)
#define TSTBIT(x, y) ((BYTE)(((x) & (y)) != 0))
#define SETBITNUM(x, y) (x) |= _BV((y))
#define CLRBITNUM(x, y) (x) &= ~_BV(y))
#define TSTBITNUM(x, y) ((BYTE)(((x) & _BV((y))) != 0))

#define SETBIT8(x, y) (x) |= (BYTE)(y)
#define CLRBIT8(x, y) (x) &= (BYTE)~(y)
#define TSTBIT8(x, y) ((BYTE)(((BYTE)(x) & (BYTE)(y)) != (BYTE)0))
#define SETBITNUM8(x, y) (x) |= (BYTE)_BV((y))
#define CLRBITNUM8(x, y) (x) &= (BYTE)~_BV(y))
#define TSTBITNUM8(x, y) ((BYTE)(((BYTE)(x) & (BYTE)_BV((y))) != (BYTE)0))

#define MAKE8(v, o) (((BYTE *)&(v))[(o)])
#define MAKE16(h, l) ((((SWORD)(h) & 0xff)<<8)+((l) & 0xff))
#define MAKE32(a, b, c, d) ((((SDWORD)(a) & 0xff) << 24) + (((SDWORD)(b) & 0xff) << 16) + (((SDWORD)(c) & 0xff) << 8) + ((SDWORD)(d) & 0xff))
#define MAKE32FROM16(a, b) ((((SDWORD)(a) & 0xffff) << 16) + ((SDWORD)(b) & 0xffff))

#define min(x,y) ({ \
	(x) < (y) ? (x) : (y); })

#define max(x,y) ({ \
	(x) > (y) ? (x) : (y); })

#define TOSW(x) ((SWORD)((BYTE)(x)))
#define TOSDW(x) ((SDWORD)((BYTE)(x)))
#define TOW(x) ((WORD)((BYTE)(x)))
#define TODW(x) ((DWORD)((BYTE)(x)))

#endif
