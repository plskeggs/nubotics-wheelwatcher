/******************************************************************************/
/* WW-01 GNU C for Atmel ATMEGA16                                             */
/* Simple Position and Speed Monitor Program                                  */
/*                                                                            */
/* Copyright 2004, Noetic Design, Inc.                                        */
/*                                                                            */
/* This demonstation program ramps the speed of the two servos back and       */
/* forth, and while doing this, displays the current position and velocity as */
/* measured using the WheelWatchers on the robot.                             */
/******************************************************************************/
/******************************************************************************/
/* TARGET NOTE:                                                               */
/*                                                                            */
/* This example is for use with an ARC 1.1 board from barello.net.  This      */
/* uses the ATMEGA16 processor with a 16MHz resonator.  The following         */
/* fuses are required; use AVR Studio or GNU avrdude to reprogram:            */
/* CKOPT = 1, CKSEL3 = CKSEL2 = CKSEL1 = 1, CKSEL0 = 1, SUT1 = 0, SUT0 = 0    */
/******************************************************************************/
/******************************************************************************/
/* The following note is just about all that remains of the original demo.c   */
/* that came with WinAVR.  We are retaining it as required, though little     */
/* remains of the original code.                                              */
/* ---------------------------------------                                    */
/* "THE BEER-WARE LICENSE" (Revision 42):                                     */
/* <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you*/
/* can do whatever you want with this stuff. If we meet some day, and you     */
/* think this stuff is worth it, you can buy me a beer in return. Joerg Wunsch*/
/* ---------------------------------------                                    */
/******************************************************************************/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>


// timer variables
volatile uint16_t timer_ticks;                             // in multiples of 128us; rolls over every 10 seconds
volatile uint32_t seconds;
volatile int8_t seconds_fraction;

// encoder variables
short enc_fwdir_R;                                         // most recent value read from the right WW-01's DIR line
short enc_fwdir_L;

// position variables
volatile uint32_t enc_pos_R;
volatile uint32_t enc_pos_L;

// velocity control variables
volatile uint16_t out_R;
volatile uint16_t out_L;
volatile uint8_t direction_R;
volatile uint8_t direction_L;

// velocity measurement variables
volatile uint16_t enc_period_L_prev;
volatile uint16_t enc_period_R_prev;
volatile uint16_t enc_period_L;
volatile uint16_t enc_period_R;
volatile int16_t enc_speed_R;
volatile int16_t enc_speed_L;

// program state flags
volatile uint8_t do_print;
uint8_t reset_source;

// constants
#define SYSCLK 16000000UL
#define MAX_SERVO 500
#define MIN_SERVO 250
#define MID_SERVO 375
#define RSERVO PC5                                         // right servo control line on ARC 1.1 board
#define LSERVO PC2                                         // left servo control line on ARC 1.1 board
#define RDIR PC0                                           // right WW-01 DIR signal
#define LDIR PC1                                           // left WW-01 DIR signal
#define SIG_RCLK SIG_INTERRUPT0
#define SIG_LCLK SIG_INTERRUPT1
#define TRUE 1
#define FALSE 0

enum
{
   UP, DOWN, STEADY 
};

// calculate the speed conversion factors
#define Dwh 2.75                                           // wheel diameter
#define PI 3.14159
#define Cwh (Dwh * PI)                                     // wheel circumference
#define TSCALE 60                                          // seconds per minute
#define INVTICK 7812                                       // this is 1 / 128 us, to avoid using floating point math
#define NCLKS 128                                          // number of encoder clocks per wheel rotation
#define Kips ((int16_t)(((int32_t)Cwh * INVTICK) / NCLKS)) // inches per second (IPS) = Kips / PER = 527 / PER
#define Krpm ((int16_t)(((int32_t)TSCALE * INVTICK) / NCLKS)) // revolutions per minute (RPM) = Krpm / PER = 3662 / PER

// function prototypes
extern uint16_t get_timer_ticks();
extern int uart_putchar(char c);


/******************************************************************************/
/*  Setup the Hardware                                                        */
/******************************************************************************/
void setup()
{
   uint16_t baud;

   /* as early as possible, grab the current reason for being reset and then clear it */
   reset_source = MCUCSR;
   MCUCSR = 0x1f;                                          // clear reset source

   /* tmr1 is PWM with TOP set by ICR1, OC1A and OC1B output waveforms in fast PWM mode */
   TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV (WGM11);

   /* tmr1 running on fosc/64 = 250,000 Hz clock */
   TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11) | _BV (CS10);

   /* set PWM frequency to 50Hz */
   ICR1 = 5000;                                            // 5000 * 4us = 20ms

   /* set duty cycle such that 1.5 ms pulses are generated; 1 ms = 250, 1.5ms = 375, 2ms = 500 */
   out_R = 500;
   out_L = 250;                                            // 375;
   OCR1A = out_R;
   OCR1B = out_L;
   direction_R = DOWN;
   direction_L = UP;

   /* enable OC1A and OC1B as output */
   DDRD = _BV (PD5) | _BV(PD4);
   DDRC = _BV (PC5) | _BV(PC2);                            // mirror on PC5 and PC2 for ARC board

   /* enable interrupts on timer 1 overflow and both output compares */
   timer_enable_int (_BV (TOIE1) | _BV(OCIE1A) | _BV(OCIE1B));

   /* enable external interrupts 0 and 1 for falling edge triggering */
   MCUCR = _BV(ISC11) | _BV(ISC01);
   GICR = _BV(INT1) | _BV(INT0);

   /* enable serial port */
   UCSRB = _BV(TXEN);                                      /* tx enable */
   baud = (SYSCLK / (16 * 38400UL)) - 1;                   /* 38400 Bd */
   UBRRH = (unsigned char)(baud >> 8);
   UBRRL = (unsigned char)baud;
   UCSRC = (1<<URSEL)|(3<<UCSZ0);

   /* enable interrupts */
   sei ();

   /* set up serial printing */
   fdevopen(uart_putchar, NULL, 0);                        // associate uart output function with stdio
}

/******************************************************************************/
/* RS232 Character Sender                                                     */
/*                                                                            */
/* Send character c down the UART Tx, wait until tx holding register          */
/* is empty.                                                                  */
/******************************************************************************/
int
uart_putchar(char c)
{
   if (c == '\n')
      uart_putchar('\r');
   loop_until_bit_is_set(UCSRA, UDRE);
   UDR = c;
   return 0;
}

/******************************************************************************/
/* Print Reset Source                                                         */
/*                                                                            */
/* This dumps strings over the serial port to indicate why we recently        */
/* reset.                                                                     */
/******************************************************************************/
void
dump_reset()
{
   if (reset_source & _BV(JTRF))
      printf("JTAG Reset\n");
   if (reset_source & _BV(WDRF))
      printf("Watchdog Reset\n");
   if (reset_source & _BV(BORF))
      printf("Brownout Reset\n");
   if (reset_source & _BV(EXTRF))
      printf("External Reset\n");
   if (reset_source & _BV(PORF))
      printf("Power-on Reset\n");
}



/******************************************************************************/
/* RCLK / INTERRUPT0 ISR                                                      */
/*                                                                            */
/* This interrupt service routine is called on each pulse of the              */
/* right WW-01 CLK line.  We grab the current time stamp, adjust              */
/* the current position based on the RDIR pin, then calculate a               */
/* new encoder clock period using the new time stamp.  This is done           */
/* by subtracting the previous time stamp from the current to get             */
/* the current period, then using that and the last period value              */
/* to calculate a new one using a running average algorithm.                  */
/******************************************************************************/
SIGNAL (SIG_RCLK)
{
   uint16_t tmr =    get_timer_ticks();
   if (PINC & _BV(RDIR))
   {
      enc_fwdir_R = TRUE;
      enc_pos_R++;
   }
   else
   {
      enc_fwdir_R = FALSE;
      enc_pos_R--;
   }
   // this calculates a running average to filter alignment noise
   enc_period_R = (enc_period_R * 3 + (tmr - enc_period_R_prev)) >> 2;
   enc_period_R_prev = tmr;
}

/******************************************************************************/
/* LCLK / INTERRUPT1 ISR                                                      */
/*                                                                            */
/* This interrupt service routine is called on each pulse of the              */
/* left  WW-01 CLK line.  We grab the current time stamp, adjust              */
/* the current position based on the LDIR pin, then calculate a               */
/* new encoder clock period using the new time stamp.  This is done           */
/* by subtracting the previous time stamp from the current to get             */
/* the current period, then using that and the last period value              */
/* to calculate a new one using a running average algorithm.                  */
/******************************************************************************/
SIGNAL (SIG_LCLK)
{
   uint16_t tmr =    get_timer_ticks();
   if (PINC & _BV(LDIR))
   {
      enc_fwdir_L = FALSE;
      enc_pos_L--;
   }
   else
   {
      enc_fwdir_L = TRUE;
      enc_pos_L++;
   }
   // this calculates a running average to filter alignment noise
   enc_period_L = (enc_period_L * 3 + (tmr - enc_period_L_prev)) >> 2;
   enc_period_L_prev = tmr;
}

/******************************************************************************/
/* OUTPUT COMPARE 1A ISR                                                      */
/*                                                                            */
/* This interrupt occurs each time the TCNT1 value matches OCR1A.  We use this*/
/* routine to lower the RSERVO pin on the ARC board.  This is needed because  */
/* the ARC board connects the right servo control signal to PC5 instead of the*/
/* OC1A / PD5 signal which is automatically output by the hardware.           */
/******************************************************************************/
SIGNAL (SIG_OUTPUT_COMPARE1A)
{
   PORTC &= ~_BV(RSERVO);
}

/******************************************************************************/
/* OUTPUT COMPARE 1B ISR                                                      */
/*                                                                            */
/* This interrupt occurs each time the TCNT1 value matches OCR1B.  We use this*/
/* routine to lower the LSERVO pin on the ARC board.  This is needed because  */
/* the ARC board connects the left  servo control signal to PC2 instead of the*/
/* OC1B / PD4 signal which is automatically output by the hardware.           */
/******************************************************************************/
SIGNAL (SIG_OUTPUT_COMPARE1B)
{
   PORTC &= ~_BV(LSERVO);
}

/******************************************************************************/
/* TIMER 1 OVERFLOW ISR                                                       */
/*                                                                            */
/* This interrupt routine is called at the end of every 20ms servo control    */
/* pulse period, which is 5000 ticks of TCNT1 with a prescale value of 64.    */
/* Here is where the RSERVO (PC5) and LSERVO (PC2) lines are manually dropped,*/
/* a new control pulse value is calculated and written to the corresponding   */
/* OCR1 register, and we also do some timekeeping work for use in measuring   */
/* the encoder clock period.                                                  */
/******************************************************************************/
SIGNAL (SIG_OVERFLOW1)
{
   static int8_t pcount = 0;

   PORTC |= _BV(RSERVO) | _BV(LSERVO);
   switch (direction_R)
   {
   case UP:
      if (++out_R >= MAX_SERVO)
         direction_R = DOWN;
      break;

   case DOWN:
      if (--out_R <= MIN_SERVO)
         direction_R = UP;
      break;
   }
   switch (direction_L)
   {
   case UP:
      if (++out_L >= MAX_SERVO)
         direction_L = DOWN;
      break;

   case DOWN:
      if (--out_L <= MIN_SERVO)
         direction_L = UP;
      break;
   }

   OCR1A = out_R;
   OCR1B = out_L;

   if (seconds_fraction++ == 50)
   {
      seconds_fraction = 0;
      seconds++;
   }
   if (pcount++ == 5)
   {
      pcount = 0;
      do_print = 1;
   }
   timer_ticks += 156;                                     // this is the max count of 5000 / 32; unit of time is 128us
}


/******************************************************************************/
/* Get Timer Ticks                                                            */
/*                                                                            */
/* This routine uses the overflow value calculated during the                 */
/* overflow ISR as well as the current TCNT1 value to calculate               */
/* a new time stamp for use in measuring an encoder period.                   */
/* One timer_tick unit is equivalent to 128us, and so it over-                */
/* flows every 10 seconds.  This allows us to measure the                     */
/* speed of a running servo very accurately, and measure it                   */
/* over the range of a maximum of 3600 RPM(!) and a minimum                   */
/* of 0.05 RPM.                                                               */
/******************************************************************************/
uint16_t
get_timer_ticks()
{
   return timer_ticks + (TCNT1 >> 5);                      // add current count to overflow
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
   enc_speed_R = (int16_t)(Krpm / enc_period_R);
   if ((PINC & _BV(RDIR)) == 0)
      enc_speed_R = -enc_speed_R;

   enc_speed_L = (int16_t)(Krpm / enc_period_L);
   if (PINC & _BV(LDIR))
      enc_speed_L = -enc_speed_L;
}


/******************************************************************************/
/* Print Encoder Speeds                                                       */
/******************************************************************************/
void print_enc_speeds()
{
#ifdef DEBUG_SNS
   int fwdv;
   fwdv = (int)enc_fwdir_R;
   printf("EncSpdR %d; RFwd %d; ", enc_speed_R, fwdv);
   fwdv = (int)enc_fwdir_L;
   printf("EncSpdL %d; LFwd %d\r\n", enc_speed_L, fwdv);
#endif
}


/******************************************************************************/
/* Main Program Entry Point                                                   */
/*                                                                            */
/******************************************************************************/
int
main (void)
{
   setup();
   printf("\nAtmel ATMEGA16 / ARC 1.1 WW-01 Monitor Example\n");
   dump_reset();
   /* loop forever, the interrupts are doing the rest */

   for (;;)
   {
      if (do_print)                                        // only print this 10 times a second
      {
         do_print = 0;
         calc_enc_speeds();
         printf("%03ld.%02d ldist %ld, rdist %ld, lspd %d, rspd %d, out_L %d, out_R %d, ", 
            seconds, seconds_fraction * 2, enc_pos_L, enc_pos_R, enc_speed_L, enc_speed_R, out_L, out_R);
         printf("ticks %u, lper %d, rper %d\n", get_timer_ticks(), enc_period_L, enc_period_R);
      }
   }
   return (0);
}
