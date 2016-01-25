/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File          : main.c
* Compiler      : IAR EWAAVR 2.28a
* Created       : 18.07.2002 by JLL
* Modified      : 11-07-2003 by LTA
*
* Support mail  : avr@atmel.com
*
* AppNote       : AVR307 - Half duplex UART using the USI Interface
*
* Description   : Example showing how you could use the USI_UART drivers
*
*
****************************************************************************/

//#include <ioavr.h>  // Note there is a bug in this file that includes iotiny22.h instead of iotiny26.h
//#include <iotiny85.h>
//#include <inavr.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>

#include "USI_UART_config.h"

#define LED PB5

unsigned char *myString = (unsigned char *)"\n\rYou just sent me: ";

int main( void )
{
    unsigned int i;
    unsigned char *p;
    
    USI_UART_Flush_Buffers();
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver
    //__enable_interrupt();                                                   // Enable global interrupts
    sei();
    
    for (i = 0; i < 100; i++)
    {
        PORTB |= _BV(LED); // high = led2 on
        DDRB |= _BV(LED);
        _delay_ms(10);
        PORTB &= ~_BV(LED); // low = led1 on
        DDRB |= _BV(LED);
        _delay_ms(10);
    }
        
    USI_UART_Transmit_Byte('W');
    USI_UART_Transmit_Byte('W');
    USI_UART_Transmit_Byte('-');
    USI_UART_Transmit_Byte('1');
    USI_UART_Transmit_Byte('1');
    USI_UART_Transmit_Byte('\r');
    USI_UART_Transmit_Byte('\n');
    //MCUCR = (1<<SE)|(0<<SM1)|(0<<SM0);                                      // Enable Sleepmode: Idle
    
    for( ; ; )                                                                // Run forever
    {
        if( USI_UART_Data_In_Receive_Buffer() )
        {
            p = myString;
            while (*p)
                USI_UART_Transmit_Byte(*(p++));
            USI_UART_Transmit_Byte(USI_UART_Receive_Byte());                  // Echo the received character      
        }    
        //__sleep();                                                          // Sleep when waiting for next event
    }    
}
