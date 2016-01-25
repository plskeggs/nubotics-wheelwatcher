/*********************************************************************************
* WW-11 (and WW-12) Wheelwatcher Firmware for Atmel ATTiny85                     *
*                                                                                *
* File: ww11,c                                                                   *
* This contains main() as well as the key interrupt handlers and encoder         *
* processing.                                                                    *
*                                                                                *
* Copyright 2009-2012, Noetic Design, Inc.                                       *
*                                                                                *
* This file is part of the WW-11 firmware.                                       *
*                                                                                *
* NOTE: the ATTiny85 fuses should be set to: EX=0xFE, HI=0x55, LO=0xE2.          *
*                                                                                *
* This firmware works together with USI_I2C and USI_UART code provided by        *
* Atmel.  Atmel did not include any copyright notices or license information     *
* with these examples.  This firmware also includes my modifications to          *
* Megaload (a bootloader) by sbissonnette@MicroSyl.com.  We never completed      *
* implementation of this nor did we complete I2C slave mode; we found I2C slave  *
* and UART reception to both be unreliable.  UART transmission is usable.        *
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/eeprom.h>
//#include <avr/boot.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>
#include "USI_UART/USI_UART_config.h"
#include "USI_I2C/USI_TWI_Slave.h"
#include "ww_types.h"
#include "ww_settings.h"
#include "ww11.h"

// timer variables
volatile uint16_t enc_changed_time;
volatile uint16_t timer_ticks;                              // in multiples of 128us; rolls over every 10 seconds
volatile uint32_t cseconds;                                 // centi-seconds
volatile uint8_t cseconds_since_tick;
volatile BOOL cseconds_flag;

// encoder variables
volatile BOOL enc_dir;
volatile BOOL enc_cha;
volatile BOOL enc_chb;

// position variables
volatile int32_t enc_pos;
volatile BOOL enc_activity;

// velocity measurement variables
volatile uint16_t enc_samp[4];
volatile uint8_t samp_num;
volatile uint16_t tmr_prev;
volatile uint16_t enc_period;
volatile int16_t enc_speed;

// acceleration measurement variables
volatile int16_t enc_accel;

// program state flags
volatile uint8_t prev_b;
volatile uint8_t current_b;
volatile uint8_t changed_b;
volatile BOOL do_print;
uint8_t reset_source;
BOOL led_1;
BOOL led_2;
BOOL led_phase;
uint8_t prev_leds = 0xff;
BOOL hex_comms = TRUE;
unsigned char TWI_slaveAddress = WW_DEV_TYPE;

// name and version
#define NAME_STRING "NWw03\n"

// clock rate
#define SYSCLK 8000000UL

// sign/mag pins
#define DIR_OUT PB2 // J1-2
#define CLK_OUT PB1 // J1-3

// quadrature pins
#define CHA_OUT PB1 // J1-3
#define CHB_OUT PB2 // J1-2

// serial pins
#define RX_IN  PB0  // J1-4
#define TX_OUT PB1  // J1-3

// I2C pins
#define SDA_IO PB0  // J1-4
#define SCL_IN PB2  // J1-2

// SPI pins
#define MOSI_IN  PB0 // J1-4
#define MISO_OUT PB1 // J1-3
#define SCK_IN   PB2 // J1-2

// encoder input pins
#define CHA PB3
#define CHB PB4

// cm_mode pins; if all high, quadrature mode
#define MODE_SIGN_MAG   PB0 // J1-4: if low, sign mag cm_mode
#define MODE_I2C        PB1 // J1-3: if low, I2C cm_mode
#define MODE_SERIAL     PB2 // J1-2: if low, serial cm_mode

// LED
#define LED PB5 // drive low to turn D2 (green, led_2) on; drive high to turn D1 (yellow, led_1) high

// logic
#define TRUE 1
#define FALSE 0

enum
{
    UP, DOWN, STEADY 
};

MODES cm_mode;

#define INVTICK 7812     // this is 1 / 128 us, to avoid using floating point math

// Sample TWI transmission commands
#define TWI_CMD_MASTER_WRITE 0x10
#define TWI_CMD_MASTER_READ  0x20
#define TWI_CMD_MASTER_WRITE_ALOT 0x30

// function prototypes
extern uint16_t get_timer_ticks();
//extern int uart_putchar(char c);
extern void control_leds(void);
void output_string(char *string);
void process_command(void);
void output_byte(BYTE value);
void put_dword_modifier(DWORD data);
void put_word_modifier(WORD data);
//extern void BOOTLOADER_SECTION bootloader(void);

#ifndef _WW_CMD_H_
// control bits -- manages internal state of command system
typedef union _sm_ctrl_union
{
	struct _sm_ctrl {
// command processing
		volatile BYTE process_cmd : 1;                           // set true so main loop will process a command
      BYTE terminated : 1;
		BYTE status_request : 1;
		BYTE hex_comms : 1;
		volatile BYTE transmitting : 1;
		BYTE push_position : 1;
		BYTE push_velocity : 1;
		BYTE unused : 1;
	}
	flags;
	BYTE data;
} SM_CTRL;

#define SC_PROCESS_CMD 0x01 	                               // -- program state -- access via bytes[0]
#define SC_TERMINATED  0x02                                  // command was properly terminated
#define SC_STATUS_REQUEST 0x04
#define SC_HEX_COMMS 0x08
#define SC_TRANSMITTING 0x10
#define SC_PUSH_POSITION 0x20
#define SC_PUSH_VELOCITY 0x40
SM_CTRL ctrl;
#endif

/******************************************************************************/
/*  Setup the Hardware                                                        */
/******************************************************************************/
void setup()
{
// uint16_t baud;
    int i;

    /* as early as possible, grab the current reason for being reset and then clear it */
    reset_source = MCUSR;
    MCUSR = 0xf;                                          // clear reset source

    /* set up port B */
    DDRB = 0; // -- don't do this -- it turns on the LED: _BV(DDB5);
    PORTB = _BV(PB4) | _BV(PB3) | _BV(PB2) | _BV(PB1) | _BV(PB0); // turn on pull ups on inputs
    prev_b = PINB & (_BV(CHA) | _BV(CHB) | _BV(PB0));

    /* set up timer 1 */
    TCCR1 = _BV(CS11) | _BV(CS10); // clock by PCK/4, overflow at 0xff, interrupt when wrap to 0
    GTCCR = 0;
    TIMSK |= _BV(TOIE1); // enable overflow interrupt (8MHz/4/256 = 7812Hz)

    /* enable interrupt on change for CHA and CHB */
    GIMSK = _BV(PCIE); // pin change interrupt enable
    PCMSK = _BV(PCINT3) | _BV(PCINT4);

    _delay_ms(100); // give the user a chance to set the mode signals before sampling them
    /*
 // mode pins; if all high, quadrature mode
 #define MODE_SIGN_MAG   PB0 // J1-4: if low, sign mag mode
 #define MODE_I2C        PB1 // J1-3: if low, I2C mode
 #define MODE_SERIAL     PB2 // J1-2: if low, serial mode
    */
    switch (PINB & 0x07)
    {
        case 7:  // quadrature
            cm_mode = QUADRATURE;
            break;
        case 6: // sign mag
            cm_mode = SIGN_MAG;
            break;
        case 5: // I2C, idle
        case 4: // I2C with SDA low
        case 1: // I2C with SCL low
        case 0: // I2C with SDA and SCL low
            cm_mode = I2C;
            break;
        case 3: // serial, idle
        case 2: // serial with TX from micro active
            cm_mode = SERIAL;
            break;
        default:
            break;
    }

    init_eeprom(FALSE);

    switch (cm_mode)
    {
        case QUADRATURE:
            PORTB &= ~(_BV(CHA_OUT) | _BV(CHB_OUT));
            DDRB |= _BV(CHA_OUT) | _BV(CHB_OUT);
            led_1 = 1;	// yellow on, green off
            led_2 = 0;
            break;
        case SIGN_MAG:
            PORTB &= ~(_BV(DIR_OUT) | _BV(CLK_OUT));
            DDRB |= _BV(DIR_OUT) | _BV(CLK_OUT);
            led_1 = 0;	// green on, yellow off
            led_2 = 1;
            break;
        case SERIAL:
            /* enable serial port */
            hex_comms = TRUE;
            USI_UART_Flush_Buffers();
            USI_UART_Initialise_Receiver();                                   // Initialisation for USI_UART receiver
            fdevopen(USI_UART_PutChar, USI_UART_GetChar);                     // associate uart output function with stdio
            led_1 = 1;	// both on
            led_2 = 1;
            break;
        case I2C:
            /* enable I2C */
            hex_comms = FALSE;
            USI_TWI_Slave_Initialise(TWI_slaveAddress);
            led_1 = 0;
            led_2 = 0;
            break;
        case SPI:
            /* enable SPI */
            break;
    }
    /* enable interrupts */
    for (i = 0; i < 100; i++)
    {
        cseconds_flag = TRUE;
        control_leds();
        _delay_ms(10); // need to do a loop for I2C leds to show up...
    }
    sei();
#ifdef _WW_CMD_H_
    init_cmd();
#endif
    if (cm_mode == SERIAL)
    {
        ctrl.flags.push_position = TRUE;
        ctrl.flags.push_velocity = TRUE;
        output_string(NAME_STRING);                                 // name of device (Wc = Wheel Commander) and version (41d)
    }
}



/******************************************************************************/
/*                                                                            */
/******************************************************************************/
BOOTLOADER_SECTION 
SIGNAL(SIG_PIN_CHANGE)
{
    current_b = PINB & (_BV(CHA) | _BV(CHB) | _BV(PB0));
    changed_b = current_b ^ prev_b;
    prev_b = current_b;
    if ((changed_b & _BV(PB0)) && 
        !(current_b & _BV(PB0)) &&
        (cm_mode == SERIAL))
    {
        USI_UART_change_handler();
    }
    changed_b &= ~_BV(PB0); // force low, so sample_encoder() can assume the only bits ever set in changed_b are CHA and CHB
    if (changed_b & (_BV(CHA) | _BV(CHB)))
        enc_changed_time = timer_ticks;
}


void sample_encoder(void)
{
    uint8_t temp_b;
    uint8_t ch_b;
    uint8_t cu_b;
    uint16_t enct;

    if (!changed_b)
        return;
    cli();
    ch_b = changed_b;
    changed_b = 0;
    cu_b = current_b;
    sei();
    enct = enc_changed_time;
    
    // double change -- don't count, since we don't know which way it's moving now
    if (ch_b == (_BV(CHA) | _BV(CHB)))
        return;

    if (ch_b & (_BV(CHA) | _BV(CHB)))
    {
        if (cu_b & _BV(CHA))
            enc_cha = TRUE;
        else
            enc_cha = FALSE;
        if (cu_b & _BV(CHB))
            enc_chb = TRUE;
        else
            enc_chb = FALSE;

        if (ch_b & _BV(CHA))
        {
            if (enc_cha) // CHA went high
            {
                if (enc_chb)
                    enc_dir = TRUE;
                else
                    enc_dir = FALSE;
            } else // CHA went low
            {
                if (enc_chb)
                    enc_dir = FALSE;
                else
                    enc_dir = TRUE;
            }
        } else
        {
            if (enc_chb) // CHB went high
            {
                if (enc_cha)
                    enc_dir = FALSE;
                else
                    enc_dir = TRUE;
            } else // CHB went low
            {
                if (enc_cha)
                    enc_dir = TRUE;
                else
                    enc_dir = FALSE;
            }
        }

        if (enc_dir)
            enc_pos++;
        else
            enc_pos--;

        enc_activity = TRUE;
        cseconds_since_tick = 0;
        led_1 = enc_dir;
        led_2 = ((enc_pos & 0x04) != 0) ? 1 : 0;

        switch (cm_mode)
        {
            case SIGN_MAG:
                temp_b = PORTB;
                if (enc_dir)
                    temp_b |= _BV(DIR_OUT);
                else
                    temp_b &= ~_BV(DIR_OUT);
                if (enc_pos & 0x01)
                    temp_b |= _BV(CLK_OUT);
                else
                    temp_b &= ~_BV(CLK_OUT);
                PORTB = temp_b;
                break;
            case QUADRATURE:
                temp_b = PORTB;
                if (enc_cha)
                    temp_b |= _BV(CHA_OUT);
                else
                    temp_b &= ~_BV(CHA_OUT);
                if (enc_chb)
                    temp_b |= _BV(CHB_OUT);
                else
                    temp_b &= ~_BV(CHB_OUT);
                PORTB = temp_b;
                break;
            default:
                break;
        }

        // this calculates a running average to filter alignment noise
        enc_samp[samp_num] = enct - tmr_prev;
        tmr_prev = enct;
        samp_num = (samp_num + 1) & 0x03;
        enc_period = (enc_samp[0] + enc_samp[1] + enc_samp[2] + enc_samp[3]) >> 2;
    }
}

void control_leds(void)
{
    uint8_t cur_leds;

    if (!cseconds_flag)
        return;
    cseconds_flag = FALSE;
    cur_leds = led_1 | (led_2 << 1);
    if ((cur_leds == prev_leds) && (cur_leds != 3))
        return;

    prev_leds = cur_leds;
    led_phase = !led_phase;
    if (led_1 && !led_2)
    {
        PORTB |= _BV(LED); // high = led1 (D1, yellow) on
        DDRB |= _BV(LED);
    }
    if (led_2 && !led_1)
    {
        PORTB &= ~_BV(LED); // low = led2 (D2, green)) on
        DDRB |= _BV(LED);
    }
    if (led_1 && led_2)
    {
        if (led_phase)
        {
            PORTB |= _BV(LED); // high = led1 (D1, yellow) on
            DDRB |= _BV(LED);
        } else
        {
            PORTB &= ~_BV(LED); // low = led2 (D2, green) on
            DDRB |= _BV(LED);
        }
    }
    if (!led_1 && !led_2)
    {
        PORTB &= ~_BV(LED); // pullup off
        DDRB &= ~_BV(LED); // float it == both off
    }
}

/******************************************************************************/
/* TIMER 1 OVERFLOW ISR                                                       */
/*                                                                            */
/******************************************************************************/
BOOTLOADER_SECTION 
SIGNAL(SIG_OVERFLOW1)
{
    static int8_t cseconds_fraction = 78;

    timer_ticks++;
    cseconds_fraction--;
    if (!cseconds_fraction)
    {
        cseconds_fraction = 78;
        cseconds++;
        cseconds_since_tick++;
        if (cseconds_since_tick > 100)
            enc_period = 0;
        cseconds_flag = TRUE;
    }
}


/******************************************************************************/
/* USI OVERFLOW ISR                                                           */
/*                                                                            */
/******************************************************************************/
BOOTLOADER_SECTION 
SIGNAL(SIG_USI_OVERFLOW)
{
    if (cm_mode == SERIAL)
    {
        USI_UART_OVERFLOW_ISR();
    } 
    else if (cm_mode == I2C)
    {
        USI_TWI_OVERFLOW_ISR();
    }
}

/******************************************************************************/
/* Get Timer Ticks                                                            */
/*                                                                            */
/******************************************************************************/
uint16_t
get_timer_ticks()
{
    return timer_ticks;
}

/******************************************************************************/
/* Calc Encoder Speeds                                                        */
/*                                                                            */
/* speed in RPM = (1 / NCLKS) * TSCALE / (TICK * PER),  where NCLKS = 128     */
/* (32 stripe disk) per rotation, TSCALE = 60 seconds per minute, TICK =128us */
/* per timer tick, and PER is the measured period in timer ticks;             */
/* so RPMs = 3662 / PER.                                                      */
/******************************************************************************/
void calc_enc_speeds()
{
    if (enc_period)
        enc_speed = (int16_t)(((uint32_t)INVTICK) / ((uint32_t)enc_period));
    else
    {
        if (enc_speed)
            enc_activity = TRUE;    // output indication that we've reached zero speed
        enc_speed = 0;
    }
    if (!enc_dir)
        enc_speed = -enc_speed;
}



#ifndef _WW_CMD_H_

/******************************************************************************/
/* Swap                                                                       */
/*                                                                            */
/* Reverse nibbles in a byte.                                                 */
/******************************************************************************/
BYTE swap(BYTE value)
{
    return((value & 0x0f) << 4) + ((value & 0xf0) >> 4);
}

void flush_buffers(void)
{
    if (cm_mode == SERIAL)
        USI_UART_Flush_Buffers();
    else if (cm_mode == I2C)
        Flush_TWI_Buffers();
}

BYTE input_byte(void)
{
    if (cm_mode == SERIAL)
        return USI_UART_Receive_Byte();
    else if (cm_mode == I2C)
        return USI_TWI_Receive_Byte();
    else
        return 0;
}

void output_byte(BYTE value)
{
    if (cm_mode == SERIAL)
        USI_UART_Transmit_Byte(value);
    else if (cm_mode == I2C)
        USI_TWI_Transmit_Byte(value);
}

/******************************************************************************/
/* Output String                                                              */
/******************************************************************************/
void output_string(char *string)
{
   while (*string)
      output_byte(*(string++));
}

/******************************************************************************/
/* Put Byte Modifier                                                          */
/******************************************************************************/
void put_byte_modifier(BYTE data)
{
    if (hex_comms)
    {
        int temp;

        temp = data;
        temp = swap(temp);                                      // output high nibble first
        temp &= 0x0f;
        if (temp > 9)                                           // convert to ASCII hex digit
            temp += 'A' - 10;
        else
            temp += '0';
        output_byte(temp);

        temp = data;
        temp &= 0x0f;                                           // output low nibble
        if (temp > 9)
            temp += 'A' - 10;
        else
            temp += '0';
        output_byte(temp);
    } else
        output_byte(data);
}

/******************************************************************************/
/* Put Word Modifier                                                          */
/******************************************************************************/
void put_word_modifier(WORD data)
{
    if (hex_comms)
    {
        put_byte_modifier(MAKE8(data, 1));                      // MSB first; data >> 8);
        put_byte_modifier(MAKE8(data, 0));                      // data & 0x0ff);
    } 
    else
    {
        put_byte_modifier(MAKE8(data, 0));                      // LSB first; data >> 8);
        put_byte_modifier(MAKE8(data, 1));                      // data & 0x0ff);
    }
}

/******************************************************************************/
/* Put DWORD Modifier                                                         */
/******************************************************************************/
void put_dword_modifier(DWORD data)
{
    if (hex_comms)
    {
        put_word_modifier(MAKE16(MAKE8(data, 3), MAKE8(data, 2)));
        put_word_modifier(MAKE16(MAKE8(data, 1), MAKE8(data, 0)));
    } 
    else
    {
        put_byte_modifier(MAKE8(data, 0));                      // LSB first
        put_byte_modifier(MAKE8(data, 1));
        put_byte_modifier(MAKE8(data, 2));
        put_byte_modifier(MAKE8(data, 3));
    }
}

void process_command(void)
{
    static BYTE command = 0;
    static BOOL want_ack = FALSE;
    static BOOL want_val = FALSE;
    static BYTE temp_val = 0;
    static BOOL half_val = FALSE;
    BYTE val;

    val = input_byte();

    if (!command)
    {
        command = val;
        switch(command)
        {
            case 'V':
                if (!hex_comms)
                {
                    output_byte('V');
                    put_word_modifier(enc_speed);
                    command = 0;
                }
                else
                {    
                   ctrl.flags.push_velocity = !ctrl.flags.push_velocity;
                   want_ack = TRUE;
                }
                break;
            case 'D':
                if (!hex_comms)
                {
                    output_byte('D');
                    put_dword_modifier(enc_pos);
                    command = 0;
                }
                else
                {
                   ctrl.flags.push_position = !ctrl.flags.push_position;
                   want_ack = TRUE;
                }
                break;
            case 'I':
            case 'E':
                want_val = TRUE;
                half_val = FALSE;
                break;
            case 'T':
                output_byte('T');
                put_dword_modifier(cseconds);
                if (hex_comms)
                    output_byte('\n');
                else
                    command = 0;
                break;
            case 'M': // output mode
                output_byte('M');
                output_byte('0' + cm_mode);
                if (hex_comms)
                    output_byte('\n');
                else
                    command = 0;
                break;
            case '.':
                flush_buffers();
                output_byte('.');
                want_ack = half_val = want_val = FALSE;
                command = 0;
                break;
            case 'N':
                output_string(NAME_STRING);
                if (!hex_comms)
                    command = 0;
                break;
            default:
                command = 0;
                break;
        }
    }
    else if (hex_comms)
    {
        if ((val == '\0') || (val == '\n') || (val == '\r'))
        {
            if (want_ack)
            {
                output_byte('a');
                if (hex_comms)
                    output_byte('\n');
            }
            half_val = want_val = FALSE;
        }
        else if (want_val && (((val >= '0') && (val <= '9')) || ((val >= 'A') && (val <= 'F'))))
        {
            val -= '0';
            if (val > 9)
                val -= 7;
            if (!half_val)
            {
                temp_val = (val << 4) & 0xf0;
                half_val = TRUE;
                return; // leave command set
            }
            else
            {
                temp_val |= (val & 0x0f);
                half_val = FALSE;
                want_val = FALSE;
                if (command == 'E')
                {
                    output_byte('E');
                    temp_val = ((temp_val << 4) & 0xf0) | ((temp_val >> 4) & 0x0f);
                    put_byte_modifier(temp_val);
                }
                else if (command == 'I')
                {
                    TWI_slaveAddress = temp_val;
                    store_i2c_address();
                    USI_TWI_Slave_Initialise(TWI_slaveAddress);
                    output_byte('a');
                }
                if (hex_comms)
                    output_byte('\n');
            }
        }
        else if (want_val && (val == '?') && (command == 'I'))
        {
            output_byte('I');
            put_byte_modifier(TWI_slaveAddress);
            output_byte('\n');
        }
        else
        {
            output_byte('n');
            if (hex_comms)
                output_byte('\n');
        }
        half_val = want_val = want_ack = FALSE;
        command = 0;
    }
    else
    {
        if (want_val)
        {
            if (command == 'E')
            {
                temp_val = ((val << 4) & 0xf0) | ((val >> 4) & 0x0f);
                output_byte('E');
                put_byte_modifier(temp_val);
            }
            else if (command == 'I')
            {
                TWI_slaveAddress = val;
                store_i2c_address();
                output_byte('a');
            }
        }
        else if (want_ack)
            output_byte('a');
        else
            output_byte('n');
        want_val = FALSE;
        want_ack = FALSE;
        command = 0;
    }
}


#endif

/******************************************************************************/
/* Main Program Entry Point                                                   */
/*                                                                            */
/******************************************************************************/
int main(void)
{
    setup();
    /* loop forever, the interrupts are doing the rest */
    for (;;)
    {
        sample_encoder();
        if (cseconds_flag)
        {
            control_leds();
            if (cm_mode == SERIAL)
            {
                sample_encoder();
                control_leds();
                calc_enc_speeds();
                control_leds();
                sample_encoder();
                if (USI_UART_Data_In_Receive_Buffer())
                    process_command();
                control_leds();
                sample_encoder();
                if (enc_activity)
                {
                    enc_activity = FALSE;
                    if (ctrl.flags.push_position)
                    {
                        output_byte('D');
                        sample_encoder();
                        control_leds();
                        put_dword_modifier(enc_pos);
                        sample_encoder();
                        control_leds();
                    }
                    if (ctrl.flags.push_velocity)
                    {
                        if (ctrl.flags.push_position)
                        {    
                            output_byte('\n');
                            sample_encoder();
                            control_leds();
                        }
                        output_byte('V');
                        sample_encoder();
                        control_leds();
                        put_word_modifier(enc_speed);
                        sample_encoder();
                        control_leds();
                    }
                    if (ctrl.flags.push_position || ctrl.flags.push_velocity)
                        output_byte('\n');
                }
            } 
            else if (cm_mode == I2C)
            {
                sample_encoder();
                control_leds();
                calc_enc_speeds();
                sample_encoder();
                control_leds();
                if (USI_TWI_Data_In_Receive_Buffer())
                    process_command();
                control_leds();
                sample_encoder();
            }
        }
    }
    return(0);
}
