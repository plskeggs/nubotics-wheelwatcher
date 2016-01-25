/*********************************************************************************
* WheelWatcher WW-01, WW-02, WW-11, WW-12 Test Program                           *
*                                                                                *
* Copyright 2004-2010 Noetic Design, Inc.                                        *
*                                                                                *
* This version is for the rev 2 of the Lil'PICcy board.                          *
*                                                                                *
* This program is free software: you can redistribute it and/or modify           *
* it under the terms of the GNU General Public License as published by           *
* the Free Software Foundation, either version 3 of the License, or              *
* (at your option) any later version.                                            *
*                                                                                *
* This program is distributed in the hope that it will be useful,                *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                 *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                  *
* GNU General Public License for more details.                                   *
*                                                                                *
* You should have received a copy of the GNU General Public License              *
* along with this program, in the file COPYING.                                  *
* If not, see <http://www.gnu.org/licenses/>.                                    *
*********************************************************************************/


#define REV1BOARD 1
//#define REV2BOARD_SMALL 1
//#define REV2BOARD 1

#ifdef REV1BOARD
#include <16F873.h>
#endif

#ifdef REV2BOARD_SMALL
#include <16f873.h>
#endif

#ifdef REV2BOARD
#include <16F876.h>
#endif

#device ADC=8
#use delay(clock=20000000)
#use rs232(baud=38400,parity=n,bits=8,xmit=PIN_C6,rcv=PIN_C7)
//#use I2C(master, sda=PIN_C4, scl=PIN_C3, FORCE_HW)
#fuses HS,NOWDT,PUT,NOLVP,NOBROWNOUT,NODEBUG,NOPROTECT

#define PIN_PWM_R    PIN_C2 // tied to B1; = CCP1
#define PIN_PWM_L    PIN_C1 // tied to B2; = CCP2

#ifdef REV1BOARD

#define RED_LED PIN_A0  // output
#define AMBER_LED PIN_A1 // output

#define SW_1 PIN_B4 // input
#define SW_2 PIN_B5 // input

#define PROX_L PIN_B2 // U5/D5
#define PROX_R PIN_B3 // U4/D4

#define DIR_1 PIN_C3 // output
#define DIR_2 PIN_C4 // output; C1 and C2 are also

#define TRIS_A_VAL 0xfc // 1 = input; A0 and A1 are outputs
#define TRIS_B_VAL 0xff
#define TRIS_C_VAL 0xe1 // C1 = PWM2, C2 = PWM1, C3 = DIR1, C4 = DIR2, all outputs


#define PIN_EXP1     PIN_A4
#define PIN_EXP2     PIN_A2
#define PIN_EXP3     PIN_A5
#define PIN_EXP4     PIN_A3
#define PIN_EXP5     PIN_C0
#define PIN_EXP6     PIN_B1
#define PIN_EXP7     PIN_B0
#define PIN_EXP8     PIN_C5

// Vcc - red   -> exp 9
// Gnd - black -> exp 10

#else

#define RED_LED PIN_A2  // output
#define AMBER_LED PIN_A5 // output

#define SW_1 PIN_B3 // input
#define SW_2 PIN_B2 // input

#define PROX_L PIN_C5 // U5/D5
#define PROX_R PIN_B1 // U4/D4

#define DIR_1 PIN_C3 // output
#define DIR_2 PIN_C4 // output; C1 and C2 are also

#define TRIS_A_VAL   0xDB // A2, A5 outputs; rest inputs; 1 = input
#define TRIS_B_VAL   0xFF // all inputs
#define TRIS_C_VAL   0xE1 // C1,C2,C3,C4 outputs; rest inputs

#define PIN_EXP1     PIN_B4
#define PIN_EXP2     PIN_B5
#define PIN_EXP3     PIN_A1
#define PIN_EXP4     PIN_A0
#define PIN_EXP5     PIN_A4
#define PIN_EXP6     PIN_A3
#define PIN_EXP7     PIN_C0
#define PIN_EXP8     PIN_B0
// Vcc - red   -> exp 9
// Gnd - black -> exp 10

#endif

#define ENC_R_A      PIN_EXP8   // yellow -> exp 8; encoder A signal for Right servo
#define ENC_R_A_BIT 0
#define ENC_R_B      PIN_EXP2   // blue   -> exp 2; encoder B signal for Right servo
#define ENC_R_B_BIT 5
#define ENC_DIR        PIN_EXP1   // orange -> exp 1; direction
#define ENC_DIR_BIT 4
#define ENC_CLK      PIN_EXP5   // violet -> exp 5; clock
#define ENC_CLK_BIT 4

#define WW02_BB_MODE PROX_R // PIN_EXP7   // exp 7; high = normal, low = just quadrature inputs
#define UNUSED_PIN   PIN_EXP6   // exp 6

#define VCC_CHECK PIN_EXP4       // white -> Exp 4; Vcc test
#define GND_CHECK PIN_EXP3       // brown -> Exp 3; Gnd test

#use fast_io(A)
#use fast_io(B)
#use fast_io(C)


