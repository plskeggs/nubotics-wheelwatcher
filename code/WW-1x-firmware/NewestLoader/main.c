//*****************************************************************************
//* BootLoader 7.31
//*
//* Devices supported at this time and report Ok, from users
//* ATMega8
//* ATMega16
//* ATMega32
//* ATMega64
//* ATMega128
//* ATMega162
//* ATMega169
//* ATMega8515
//* ATMega8535
//* ATMega88    
//* ATMega1280  
//* ATMega2560  
//* ATMCAN128   
//* ATMega164/324/644
//* ATMega324  
//* ATMega324P 
//* ATMega2561
//* ATMega164
//* ATMCAN32
//*
//* Version 7.1 Dec 2007
//* 
//* V:7.0 had many modification since the last version,
//*         -Code shrink
//*         -Can fit in a 256k bootloader size
//*         -Add a lot of new MCU
//*         -Syncronise message with MegaLoad .NET V:7.0
//*
//* V:7.1 support device larger than 128k
//* V:7.2 Add define's for M164
//* V:7.3 ?
//* V:7.31 PLS: added Tinyx5 support
//*
//* Everything is AS IS without any warranty of any kind.
//*
//* Note:
//* -----
//* I can't write bootloader of all the MCU it's not my primary job and I don't
//* make $$$ with that
//*
//* If you make new #define please let me know I will update the bootloader
//* file it will help other AVR users
//*
//* sbissonnette@MicroSyl.com
//*****************************************************************************

#ifndef __IMAGECRAFT__
// update Makefile MCU to specify correct target device
   #include <stdio.h>
   #include <stdlib.h>
   #include <inttypes.h>
   #include <avr/io.h>
   #include <avr/boot.h>
   #include <avr/interrupt.h>
   #define INTEGRATED 1 // define this so that we link to bootloader() rather than main(), and thus skip the duplicate startup code
   #include "../USI_UART/USI_UART_config.h"
   #define WDR()
#else
   #define BOOTLOADER_SECTION
#endif

//*****************************************************************************
//*****************************************************************************
// IF YOU MAKE NEW DEFINE THAT IS WORKING PLEASE LET ME KNOW TO UPDATE MEGALOAD
// This software is free, so I can't pass all my time writting new bootloader
// for new MCU. I'm shure that you can help me and help ALL MEGALOAD USERS
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//
// To setup the bootloader for your project you must
// remove the comment below to fit with your hardware
// recompile it using ICCAVR setup for bootloader
//
// Flash, EEprom, Lockbit Programming take a bootloader of 512 word 
//
// if you chose the SMALL256 you will only be able to program the flash without
// any communication and flash verification.  You will need a bootloader size
// of 256 word
//
//*****************************************************************************
// MCU selection
//
// *************************************
// *->Do the same thing in assembly.s<-*
// *************************************
//
//*****************************************************************************

//#define MEGATYPE  Mega8         
//#define MEGATYPE Mega16        
//#define MEGATYPE Mega64        
//#define MEGATYPE Mega128       
//#define MEGATYPE Mega32        
//#define MEGATYPE Mega162       
//#define MEGATYPE Mega169       
//#define MEGATYPE Mega8515      
//#define MEGATYPE Mega8535      
//#define MEGATYPE Mega163       
//#define MEGATYPE Mega323       
//#define MEGATYPE Mega48        
//#define MEGATYPE Mega88        
//#define MEGATYPE Mega168       
//#define MEGATYPE Mega165       
//#define MEGATYPE Mega3250      
//#define MEGATYPE Mega6450      
//#define MEGATYPE Mega3290      
//#define MEGATYPE Mega6490      
//#define MEGATYPE Mega406       
//#define MEGATYPE Mega640       
//#define MEGATYPE Mega1280      
//#define MEGATYPE Mega2560      
//#define MEGATYPE MCAN128       
//#define MEGATYPE Mega164          
//#define MEGATYPE Mega328          
//#define MEGATYPE Mega324          
//#define MEGATYPE Mega325          
//#define MEGATYPE Mega644          
//#define MEGATYPE Mega645          
//#define MEGATYPE Mega1281            
//#define MEGATYPE Mega2561            
//#define MEGATYPE Mega404          
//#define MEGATYPE MUSB1286            
//#define MEGATYPE MUSB1287            
//#define MEGATYPE MUSB162             
//#define MEGATYPE MUSB646             
//#define MEGATYPE MUSB647             
//#define MEGATYPE MUSB82              
//#define MEGATYPE MCAN32              
//#define MEGATYPE MCAN64        
//#define MEGATYPE Mega329
//#define MEGATYPE Mega649    
//#define MEGATYPE Mega256    
#define MEGATYPE Tiny85

//*****************************************************************************
// MCU Frequency    
//*****************************************************************************
//#define XTAL        16000000
#define XTAL        8000000

//*****************************************************************************
// Bootload on UART x
//*****************************************************************************
//#define UART        0
//#define UART       1
//#define UART       2
//#define UART       3
#define UART         4 // used for firmware-assisted UART on Tinyx5 devices


//*****************************************************************************
// BaudRate
//*****************************************************************************
//#define BAUDRATE     57600

//*****************************************************************************
// EEprom programming
// enable EEprom programing via bootloader
//*****************************************************************************
//#define EEPROM

//*****************************************************************************
// LockBit programming
// enable LOCKBIT programing via bootloader
//*****************************************************************************
//#define LOCKBIT

//*****************************************************************************
// Small 256 Bootloader without eeprom programming, lockbit programming
// and no data verification
//*****************************************************************************
#define SMALL256

//*****************************************************************************
// RS485
// if you use RS485 half duplex for bootloader
// make the appropriate change for RX/TX transceiver switch
//*****************************************************************************
//#define RS485DDR  DDRB
//#define RS485PORT PORTB
//#define RS485TXE  0x08

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                 DO NOT CHANGE ANYTHING BELOW THIS LINE 
//               IF YOU DON'T REALLY KNOW WHAT YOU ARE DOING
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef __IMAGECRAFT__
#include <macros.h>
#endif

#define Mega8           'A'
#define Mega16          'B'
#define Mega64          'C'
#define Mega128         'D'
#define Mega32          'E'
#define Mega162         'F'
#define Mega169         'G'
#define Mega8515        'H'
//#define Mega8535        'I'
#define Mega163         'J'
#define Mega323         'K'
#define Mega48          'L'
#define Mega88          'M'
#define Mega168         'N'

#define Mega165         0x80
#define Mega3250        0x81
#define Mega6450        0x82
#define Mega3290        0x83
#define Mega6490        0x84
#define Mega406         0x85
#define Mega640         0x86
#define Mega1280        0x87
#define Mega2560        0x88
#define MCAN128         0x89
#define Mega164         0x8a
#define Mega328         0x8b
#define Mega324         0x8c
#define Mega325         0x8d
#define Mega644         0x8e
#define Mega645         0x8f
#define Mega1281        0x90
#define Mega2561        0x91
#define Mega404         0x92
#define MUSB1286        0x93
#define MUSB1287        0x94
#define MUSB162         0x95
#define MUSB646         0x96
#define MUSB647         0x97
#define MUSB82          0x98
#define MCAN32          0x9a
#define MCAN64          0x9b
#define Mega329         0x9c
#define Mega649         0x9d
#define Mega256         0x9e

#define Tiny85          'I' // replaces Mega8535

#define Flash1k         'g'
#define Flash2k         'h'
#define Flash4k         'i'
#define Flash8k         'l'
#define Flash16k        'm'
#define Flash32k        'n'
#define Flash64k        'o'
#define Flash128k       'p'
#define Flash256k       'q'
#define Flash40k        'r'

#define EEprom64        '.'
#define EEprom128       '/'
#define EEprom256       '0'
#define EEprom512       '1'
#define EEprom1024      '2'
#define EEprom2048      '3'
#define EEprom4096      '4'

#define Boot128         'a'
#define Boot256         'b'
#define Boot512         'c'
#define Boot1024        'd'
#define Boot2048        'e'
#define Boot4096        'f'

#define Page32          'Q'
#define Page64          'R'
#define Page128         'S'
#define Page256         'T'
#define Page512         'V'

#if !(defined MEGATYPE) && !(defined MCU)
   #error "Processor Type is Undefined"
#endif

#ifdef EEPROM
   #define  BootSize       Boot1024
#endif

#ifndef EEPROM
   #define  BootSize       Boot512
#endif

#if (MEGATYPE == Mega8)
#ifdef __IMAGECRAFT__
   #include "iom8v.h"
#endif
   #define  DeviceID       Mega8
   #define  FlashSize      Flash8k
   #define  PageSize       Page64
   #define  EEpromSize     EEprom512
   #define  PageByte       64
   #define  NSHIFTPAGE     6
   #define  INTVECREG      GICR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega16)
#ifdef __IMAGECRAFT__
   #include "iom16v.h"
#endif
   #define  DeviceID       Mega16
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      GICR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega64)
#ifdef __IMAGECRAFT__
   #include "iom64v.h"
#endif
   #define  DeviceID       Mega64
   #define  FlashSize      Flash64k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom2048
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif
#endif

#if (MEGATYPE == Mega128)
#ifdef __IMAGECRAFT__
   #include "iom128v.h"
#endif
   #define  DeviceID       Mega128
   #define  FlashSize      Flash128k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG
   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif
#endif

#if (MEGATYPE == Mega32)
#ifdef __IMAGECRAFT__
   #include "iom32v.h"
#endif
   #define  DeviceID       Mega32
   #define  FlashSize      Flash32k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom1024
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      GICR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega162)
#ifdef __IMAGECRAFT__
   #include "iom162v.h"
#endif
   #define  DeviceID       Mega162
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      GICR
   #if (UART == 0)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTB
      #define PULLUPPIN       0x04
   #endif
#endif

#if (MEGATYPE == Mega169)
#ifdef __IMAGECRAFT__
   #include "iom169v.h"
#endif
   #define  DeviceID       Mega169
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTE
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega8515)
#ifdef __IMAGECRAFT__
   #include "iom8515v.h"
#endif
   #define  DeviceID       Mega8515
   #define  FlashSize      Flash8k
   #define  PageSize       Page64
   #define  EEpromSize     EEprom512
   #define  PageByte       64
   #define  NSHIFTPAGE     6
   #define  INTVECREG      GICR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Tiny85)
#ifdef __IMAGECRAFT__
#endif
   #define  DeviceID       Tiny85
   #define  FlashSize      Flash8k
   #define  PageSize       Page64
   #define  EEpromSize     EEprom512
   #define  PageByte       64
   #define  NSHIFTPAGE     6
   #define  PULLUPPORT     PORTB
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega163)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega323)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega48)
#ifdef __IMAGECRAFT__
   #include "iom48v.h"
#endif
   #define  DeviceID       Mega48
   #define  FlashSize      Flash4k
   #define  PageSize       Page64
   #define  EEpromSize     EEprom256
   #define  PageByte       64
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega88)
#ifdef __IMAGECRAFT__
   #include "iom88v.h"
#endif
   #define  DeviceID       Mega88
   #define  FlashSize      Flash8k
   #define  PageSize       Page64
   #define  EEpromSize     EEprom512
   #define  PageByte       64
   #define  NSHIFTPAGE     6
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega168)
#ifdef __IMAGECRAFT__
   #include "iom168v.h"
#endif
   #define  DeviceID       Mega168
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega165)
#ifdef __IMAGECRAFT__
   #include "iom165v.h"
#endif
   #define  DeviceID       Mega165
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTE
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega3250)
#ifdef __IMAGECRAFT__
   #include "iom325v.h"
#endif
   #define  DeviceID       Mega3250
   #define  FlashSize      Flash32k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom1024
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTE
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega6450)
#ifdef __IMAGECRAFT__
   #include "iom645v.h"
#endif
   #define  DeviceID       Mega6450
   #define  FlashSize      Flash64k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom2048
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTE
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega3290)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega6490)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega406)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega640)
#ifdef __IMAGECRAFT__
   #include "iom640v.h"
#endif
   #define  DeviceID       Mega640
   #define  FlashSize      Flash64k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif

   #if (UART == 2)
      #define PULLUPPORT      PORTH
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 3)
      #define PULLUPPORT      PORTJ
      #define PULLUPPIN       0x01
   #endif
#endif

#if (MEGATYPE == Mega1280)
#ifdef __IMAGECRAFT__
   #include "iom1280v.h"
#endif
   #define  DeviceID       Mega1280
   #define  FlashSize      Flash128k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR

   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif

   #if (UART == 2)
      #define PULLUPPORT      PORTH
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 3)
      #define PULLUPPORT      PORTJ
      #define PULLUPPIN       0x01
   #endif
#endif

#if (MEGATYPE == Mega2560)
#ifdef __IMAGECRAFT__
   #include "iom256v.h"
#endif
   #define  DeviceID       Mega2560
   #define  FlashSize      Flash256k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG


   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif

   #if (UART == 2)
      #define PULLUPPORT      PORTH
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 3)
      #define PULLUPPORT      PORTJ
      #define PULLUPPIN       0x01
   #endif
#endif

#if (MEGATYPE == MCAN128)
#ifdef __IMAGECRAFT__
   #include "ioCAN128v.h"
#endif
   #define  DeviceID       MCAN128
   #define  FlashSize      Flash128k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG

   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif
#endif

#if (MEGATYPE == Mega164)
#ifdef __IMAGECRAFT__
   #include "iom164pv.h"
#endif
   #define  DeviceID       Mega164
   #define  FlashSize      Flash16k
   #define  PageSize       Page128
   #define  EEpromSize     EEprom512
   #define  PageByte       128
   #define  NSHIFTPAGE     7
   #define  INTVECREG      MCUCR
   #define  PULLUPPORT     PORTD
   #define  PULLUPPIN      0x01
#endif

#if (MEGATYPE == Mega328)
   #error "This MCU had not been define"
#endif

#if(MEGATYPE == Mega324)
#ifdef __IMAGECRAFT__
   #include "iom324v.h"
#endif
   #define  DeviceID                Mega324
   #define  FlashSize            Flash32k
   #define  PageSize             Page128
   #define  EEpromSize           EEprom1024
   #define  PageBye              128
   #define  NSHIFTPAGE           7
   #define  INTVECREG            MCUCR
   #if (UART == 0)
      #define PULLUPPORT         PORTD
      #define PULLUPPIN          0x01
   #endif
#endif

#if (MEGATYPE == Mega325)
   #error "This MCU had not been define"
#endif

#if(MEGATYPE == Mega644)
#ifdef __IMAGECRAFT__
   #include "iom644v.h"
#endif
   #define DeviceID          Mega644
   #define FlashSize         Flash64k
   #define PageSize          Page256
   #define EEpromSize        EEprom2048
   #define PageByte          256
   #define NSHIFTPAGE         8
   #define INTVECREG          MCUCR
   #define PULLUPPORT         PORTD
   #define PULLUPPIN          0x01
#endif

#if (MEGATYPE == Mega645)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega1281)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega2561)
#ifdef __IMAGECRAFT__
   #include "iom2561v.h"
#endif
   #define  DeviceID       Mega2561
   #define  FlashSize      Flash256k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG


   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif

   #if (UART == 2)
      #define PULLUPPORT      PORTH
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 3)
      #define PULLUPPORT      PORTJ
      #define PULLUPPIN       0x01
   #endif
#endif

#if (MEGATYPE == Mega404)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB1286)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB1287)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB162)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB646)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB647)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MUSB82)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == MCAN32)
#ifdef __IMAGECRAFT__
   #include "ioCAN32v.h"
#endif
   #define  DeviceID       MCAN32
   #define  FlashSize      Flash32k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom1024
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG
#endif

#if (MEGATYPE == MCAN64)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega329)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega649)
   #error "This MCU had not been define"
#endif

#if (MEGATYPE == Mega256)
#ifdef __IMAGECRAFT__
   #include "iom256v.h"
#endif
   #define  DeviceID       Mega256
   #define  FlashSize      Flash256k
   #define  PageSize       Page256
   #define  EEpromSize     EEprom4096
   #define  PageByte       256
   #define  NSHIFTPAGE     8
   #define  INTVECREG      MCUCR
   #define  RAMPZ_FLAG


   #if (UART == 0)
      #define PULLUPPORT      PORTE
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 1)
      #define PULLUPPORT      PORTD
      #define PULLUPPIN       0x04
   #endif

   #if (UART == 2)
      #define PULLUPPORT      PORTH
      #define PULLUPPIN       0x01
   #endif

   #if (UART == 3)
      #define PULLUPPORT      PORTJ
      #define PULLUPPIN       0x01
   #endif
#endif


// Serial Port defenition

#if !(defined MEGATYPE) && !(defined MCU)
   #error "Processor Type is Undefined"
#endif

#if (UART == 0) && !(defined UCSR0A)
   #define  _UCSRA          UCSRA
   #define  _UCSRB            UCSRB
   #define  _UCSRC            UCSRC
   #define  _UBRRL          UBRRL
   #define  _UBRRH          UBRRH
   #define  _UDR            UDR
   #define  _TXC                 TXC
#endif

#if (UART == 0) && (defined UCSR0A)
   #define  _UCSRA          UCSR0A
   #define  _UCSRB            UCSR0B
   #define  _UCSRC            UCSR0C
   #define  _UBRRL          UBRR0L
   #define  _UBRRH          UBRR0H
   #define  _UDR            UDR0
   #define  _TXC                 TXC0 
#endif

#if (UART == 1)
   #define  _UCSRA          UCSR1A
   #define  _UCSRB            UCSR1B
   #define  _UCSRC            UCSR1C
   #define  _UBRRL          UBRR1L
   #define  _UBRRH          UBRR1H
   #define  _UDR            UDR1
   #define  _TXC                 TXC1 
#endif

#if (UART == 2)
   #define  _UCSRA          UCSR2A
   #define  _UCSRB            UCSR2B
   #define  _UCSRC            UCSR2C
   #define  _UBRRL          UBRR2L
   #define  _UBRRH          UBRR2H
   #define  _UDR            UDR2
   #define  _TXC                 TX2 
#endif

#if (UART == 3)
   #define  _UCSRA          UCSR3A
   #define  _UCSRB            UCSR3B
   #define  _UCSRC            UCSR3C
   #define  _UBRRL          UBRR3L
   #define  _UBRRH          UBRR3H
   #define  _UDR            UDR3
   #define  _TXC                 TXC3 
#endif

#if (UART == 4)
// this is for Tinyx5
#endif

#define  FALSE          0
#define  TRUE           1

#ifdef SMALL256
   #undef EEPROM
   #undef LOCKBIT
#endif


/*****************************************************************************/
/*                          I N C L U D E                                    */
/*****************************************************************************/
#include "assembly.h"

/*****************************************************************************/
/*                        P R O T O T Y P E                                  */
/*****************************************************************************/

void BOOTLOADER_SECTION GetPageNumber(void);
char BOOTLOADER_SECTION WriteFlashPage(void);

unsigned char BOOTLOADER_SECTION RxChar(void);
void BOOTLOADER_SECTION TxChar(unsigned char ch);

#ifdef EEPROM
void BOOTLOADER_SECTION EEpromLoad(void);
void BOOTLOADER_SECTION EEPROMwrite(int location, unsigned char byte);
unsigned char BOOTLOADER_SECTION EEPROMread( int location);
void BOOTLOADER_SECTION LockBit(void);
#endif

#ifdef __IMAGECRAFT__
void 
#else
int
#endif
BOOTLOADER_SECTION
#ifdef INTEGRATED
bootloader(void);
#else
main(void);
#endif

/*****************************************************************************/
/*                G L O B A L    V A R I A B L E S                           */
/*****************************************************************************/
unsigned char PageBuffer[PageByte];
unsigned int PageAddress;
unsigned int RealPageAddress;

/*****************************************************************************/

void BOOTLOADER_SECTION GetPageNumber(void)
{
   unsigned char PageAddressHigh = RxChar();

   RealPageAddress = (int)((PageAddressHigh << 8) + RxChar());
   PageAddress = RealPageAddress << NSHIFTPAGE;

#ifdef RAMPZ_FLAG
   RAMPZ = PageAddressHigh;
#endif
}

/*****************************************************************************/

char BOOTLOADER_SECTION WriteFlashPage(void)
{
#ifdef SMALL256
   //-------------
   unsigned int i;
   unsigned int TempInt;

   for (i=0;i<PageByte;i+=2)
   {
      TempInt = RxChar();
      TempInt |= (RxChar()<<8);
      fill_temp_buffer(TempInt,i);    //call asm routine.
   }
   write_page(PageAddress,0x03);     //Perform page ERASE
   write_page(PageAddress,0x05);     //Perform page write
   enableRWW();    
   i = RxChar();
   return 1;

#else //--------------

   unsigned int i;
   unsigned int TempInt;
   unsigned char FlashCheckSum = 0;
   unsigned char CheckSum = 0;
   unsigned char Left;
   unsigned char Right;

   for (i=0;i<PageByte;i+=2)
   {
      Right = RxChar();
      Left = RxChar();
      TempInt = Right + (Left<<8);
      CheckSum += (Right + Left);
      fill_temp_buffer(TempInt,i);      //call asm routine.
   }

   if (CheckSum != RxChar()) return 0;

   write_page(PageAddress,0x03);     //Perform page ERASE
   write_page(PageAddress,0x05);     //Perform page write
   enableRWW();

   for (i=0;i<PageByte;i+=2)
   {
      TempInt = read_program_memory(PageAddress + i,0x00);
      FlashCheckSum += (TempInt & 0x00ff) + ((TempInt & 0xff00) >> 8);
   }
   if (CheckSum != FlashCheckSum) return 0;

   return 1;

#endif
}

/*****************************************************************************/
/* EEprom Programing Code                                                    */
/*****************************************************************************/
#ifdef EEPROM
void BOOTLOADER_SECTION EEpromLoad()
{
   unsigned char ByteAddressHigh;
   unsigned char ByteAddressLow;
   unsigned int ByteAddress;
   unsigned char Data;
   unsigned char LocalCheckSum;
   unsigned char CheckSum;

   TxChar(')');
   TxChar('!');
   while (1)
   {
      WDR(); 
      LocalCheckSum = 0;

      ByteAddressHigh = RxChar();
      LocalCheckSum += ByteAddressHigh;

      ByteAddressLow = RxChar();
      LocalCheckSum += ByteAddressLow;

      ByteAddress = (ByteAddressHigh<<8)+ByteAddressLow;

      if (ByteAddress == 0xffff) return;

      Data = RxChar();
      LocalCheckSum += Data;

      CheckSum = RxChar();

      if (CheckSum == LocalCheckSum)
      {
         EEPROMwrite(ByteAddress, Data);
         if (EEPROMread(ByteAddress) == Data) TxChar('!');
         else TxChar('@');
      }
      else
      {
         TxChar('@');
      }
   }
}
#endif

/*****************************************************************************/

#ifdef EEPROM
void BOOTLOADER_SECTION EEPROMwrite( int location, unsigned char byte)
{
   while (EECR & 0x02) WDR();        // Wait until any earlier write is done
   EEAR = location;
   EEDR = byte;
   EECR |= 0x04;                     // Set MASTER WRITE enable
   EECR |= 0x02;                     // Set WRITE strobe
}
#endif

/*****************************************************************************/

#ifdef EEPROM
unsigned char BOOTLOADER_SECTION EEPROMread( int location)
{
   while (EECR & 0x02) WDR(); 
   EEAR = location;
   EECR |= 0x01;                     // Set READ strobe
   return(EEDR);                    // Return byte
}
#endif

/*****************************************************************************/
/* LockBit Code                                                              */
/*****************************************************************************/
#ifdef LOCKBIT
void BOOTLOADER_SECTION LockBit(void)
{
   unsigned char Byte;

   TxChar('%');

   Byte = RxChar();

   if (Byte == ~RxChar()) write_lock_bits(~Byte);
}
#endif

/*****************************************************************************/
/* Serial Port Code                                                          */
/*****************************************************************************/

/*****************************************************************************/
#if UART != 4
unsigned char BOOTLOADER_SECTION RxChar(void)
{
   unsigned int TimeOut = 0;

   while (!(_UCSRA & 0x80))
   {
      WDR(); 
      TimeOut += 2;
      TimeOut -= 1;
      if (TimeOut > 65530) break;
   }

   return _UDR;
}

/*****************************************************************************/

void BOOTLOADER_SECTION TxChar(unsigned char ch)
{
   while (!(_UCSRA & 0x20)) WDR();      // wait for empty transmit buffer
#ifndef RS485DDR
   _UDR = ch;                         // write char
#endif

#ifdef RS485DDR
   RS485PORT |= RS485TXE;            // RS485 in TX mode
   _UDR = ch;                        // write char
   while (!(_UCSRA & 0x40)) WDR();    // Wait for char to be cue off
   _UCSRA |= 0x40;                   // Clear flag
   RS485PORT &= ~RS485TXE;           // RS485 in RX mode
#endif   
}

#else
/*****************************************************************************/
// define special support for Tinyx5 devices
unsigned char BOOTLOADER_SECTION RxChar(void)
{
   return(unsigned char)USI_UART_Receive_Byte();
}

void BOOTLOADER_SECTION TxChar(unsigned char ch)
{
   USI_UART_Transmit_Byte(ch);
}

#endif

/*****************************************************************************/

#ifdef __IMAGECRAFT__
void
#else
int
#endif
BOOTLOADER_SECTION 
#ifdef INTEGRATED
bootloader(void)
#else
main(void)
#endif
{
   PULLUPPORT = PULLUPPIN;           // Pull up on RX line

#if UART != 4
   //_UBRRH = ((XTAL / (16 * BAUDRATE)) - 1)>>8;
   _UBRRL = (XTAL / (16 * BAUDRATE)) - 1;      //set baud rate;
   _UCSRB = 0x18;                     // Rx enable Tx Enable
   _UCSRC = 0x86;                     // Asyn,NoParity,1StopBit,8Bit 
#endif

#ifdef RS485DDR
   RS485DDR |= RS485TXE;             // RS485 Tranceiver switch pin as output
   RS485PORT &= ~RS485TXE;           // RS485 in Rx mode
#endif

   RxChar();
   TxChar('>');
   if (RxChar() == '<')
   {
      TxChar(PageSize);
      TxChar(DeviceID);
      TxChar(FlashSize);
      TxChar(BootSize);
      TxChar(EEpromSize);

      RxChar();
      TxChar('!');

      while (1)
      {
         WDR(); 
         GetPageNumber();

         if (RealPageAddress == 0xffff) break;

         if (WriteFlashPage()) TxChar('!');
         else TxChar('@');
      }

#ifdef EEPROM
      EEpromLoad();
#endif
#ifdef LOCKBIT
      LockBit();
#endif      
   }

#ifdef RAMPZ_FLAG
   RAMPZ = 0;
#endif

#ifdef INTVECREG
   MCUCR = (1<<IVCE);
   MCUCR = 0x00;
#endif
   asm("ldi r30, 0");                // Run application code
   asm("ldi r31, 0");                // Run application code
   asm("ijmp");                      // Run application code
   return 0;
}
