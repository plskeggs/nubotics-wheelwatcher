/******************************************************************************/
/* WW-01 GNU C for Atmel ATMEGA16                                             */
/* Simple Odometry Example Program, v1.0 8/3/2004                             */
/*                                                                            */
/* Copyright 2004, Noetic Design, Inc.                                        */
/*                                                                            */
/* This demonstation program shows off the features                           */
/* of the WheelWatcher.                                                       */
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

#define DEBUG_POS 1
#define DEBUG_SNS 1

// timer variables
volatile uint16_t timer_ticks;                             // in multiples of 128us; rolls over every 10 seconds
volatile uint32_t seconds;
volatile int16_t run_counter;
volatile int8_t seconds_fraction;

// encoder variables
uint8_t enc_fwdir_R;                                       // most recent value read from the right WW-01's DIR line
uint8_t enc_fwdir_L;
uint8_t received_R;                                        // this is set each time we get a new encoder clock (and thus an updated period)
uint8_t received_L;

// position variables
volatile uint32_t enc_pos_R;
volatile uint32_t enc_pos_L;

// velocity measurement variables
volatile int16_t enc_period_L_prev;
volatile int16_t enc_period_R_prev;
volatile int16_t enc_period_L;
volatile int16_t enc_period_R;
volatile int16_t enc_speed_R;
volatile int16_t enc_speed_L;

// velocity control PI variables
int16_t err_R, err_L;                                      // total error
int16_t i_err_R, i_err_L;                                  // integral of error
int16_t out_R, out_L;                                      // value to output to the motor
uint8_t dir_R, dir_L;                                      // direction to drive the motor

// velocity control variables
int16_t req_speed_R;                                       // desired or commanded velocity, in tenths of an inch per second
int16_t req_speed_L;

int16_t req_err_R;                                         // velocity feedforward value
int16_t req_err_L;

// position control variables
int16_t req_pos_R;                                         // desired or commanded position, in encoder ticks
int16_t req_pos_L;
int16_t req_pos_err_R;                                     // current position error
int16_t req_pos_err_L;
int16_t req_pos_max_vel;                                   // desired maximum velocity while moving to the new position
int16_t req_pos_cur_max;                                   // current max value
int16_t req_pos_vel_incs;                                  // amount to add to velocity each increment (used for initial acceleration) 
int16_t req_pos_inc_down_count;                            // when we are done accelerating to the desired maximum speed
int16_t pos_i_err_R, pos_i_err_L;                          // integrator of position error (used to overcome frictional errors)

// program state flags
volatile uint8_t do_print;
uint8_t reset_source;
uint8_t update_servo;                                      // it is time to calculate the PI control loop
uint8_t update_pwm;                                        // it is time to update the motors
uint8_t pos_reached;

// constants
#define TRUE 1
#define FALSE 0
#define MAX_SPEED 63                                       // maximum velocity in RPMs (depends on the servo brand and other factors)
#define INTEG_LIMIT (MAX_SPEED/3)                          // maximum error value that we allow the velocity integral term to grow to to prevent windup
#define POS_INTEG_LIMIT 45                                 // maximum velocity value that we allow the position integral term to grow to to prevent windup
#define ACCEL_INCREMENTS 1                                 // how fast to advance the maximum allowed velocity when doing position control
#define SYSCLK 16000000UL
#define MAX_SERVO 500
#define MIN_SERVO 250
#define MID_SERVO 375

// hardware constants
#define RSERVO PC5                                         // right servo control line on ARC 1.1 board
#define LSERVO PC2                                         // left servo control line on ARC 1.1 board
#define RDIR PC0                                           // right WW-01 DIR signal
#define LDIR PC1                                           // left WW-01 DIR signal
#define SIG_RCLK SIG_INTERRUPT0                            // right WW-01 CLK signal connects to INT0 / PD2
#define SIG_LCLK SIG_INTERRUPT1                            // left WW-01 CLK signal connects to INT1 / PD3

enum
{
   UP, DOWN, STEADY 
};

// constants used to calculate the speed conversion factors
#define Dwh 2.75                                           // wheel diameter
#define PI 3.14159
#define Cwh (Dwh * PI)                                     // wheel circumference
#define TSCALE 60                                          // seconds per minute
#define INVTICK 7812                                       // this is 1 / 128 us, to avoid using floating point math
#define NCLKS 128                                          // number of encoder clocks per wheel rotation
#define Kips ((Cwh * INVTICK) / NCLKS)                     // inches per second (IPS) = Kips / PER = 527 / PER
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
   out_R = 375;
   out_L = 375;                                            // 375;
   OCR1A = out_R;
   OCR1B = out_L;

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
   received_R = TRUE;
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
   received_L = TRUE;
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
   static int8_t srv = 0;

   PORTC |= _BV(RSERVO) | _BV(LSERVO);
   if (update_pwm)
   {
      update_pwm = 0;
      if (dir_R != 0)
         OCR1A = (375 - out_R);
      else
         OCR1A = (out_R + 375);
      if (dir_L != 0)
         OCR1B = (out_L + 375);
      else
         OCR1B = (375 - out_L);
   }
   update_servo = 1;                                       // 50 Hz control loop rate
   if (srv == 1)
   {
      srv = 0;
      run_counter++;
   }
   else
      srv = 1;

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
   if (received_R)
   {
      enc_speed_R = (int16_t)(Krpm / enc_period_R);
      if ((PINC & _BV(RDIR)) == 0)
         enc_speed_R = (int16_t)-enc_speed_R;
   }

   if (received_L)
   {
      enc_speed_L = (int16_t)(Krpm / enc_period_L);
      if ((PINC & _BV(LDIR)) != 0)
         enc_speed_L = (int16_t)-enc_speed_L;
   }
}


/******************************************************************************/
/* Print Encoder Speeds                                                       */
/******************************************************************************/
void print_enc_speeds()
{
#ifdef DEBUG_SNS
   int fwdv;
   fwdv = (int)enc_fwdir_R;
   printf("EncSpdR %d; RFwd %d; EncPerR %u; ", enc_speed_R, fwdv, enc_period_R);
   fwdv = (int)enc_fwdir_L;
   printf("EncSpdL %d; LFwd %d; EncPerL %u\r\n", enc_speed_L, fwdv, enc_period_L);
#endif
}

/******************************************************************************/
/* Sat16                                                                      */
/* Limit value to be between positive and negative limits.                    */
/******************************************************************************/
int16_t sat16(int16_t value, int16_t limit)
{
   if (value > limit)
      value = limit;
   else if (value < -limit)
      value = -limit;
   return value;
}

/******************************************************************************/
/* Sat32                                                                      */
/* Limit value to be between positive and negative limits.                    */
/******************************************************************************/
int32_t sat32(int32_t value, int32_t limit)
{
   if (value > limit)
      value = limit;
   else if (value < -limit)
      value = -limit;
   return value;
}


/******************************************************************************/
/* Set Velocity                                                               */
/*                                                                            */
/* Set velocity in terms of RPMs.                                             */
/******************************************************************************/
void set_velocity(int16_t speed_R, int16_t speed_L)
{
   speed_R = sat16(speed_R, MAX_SPEED);
   if (((req_speed_R >= 0) && (speed_R < 0)) || ((req_speed_R < 0) && (speed_R >= 0)))
      i_err_R = 0;                                         // reset integral on direction change
   req_speed_R = speed_R;

   speed_L = sat16(speed_L, MAX_SPEED);
   if (((req_speed_L >= 0) && (speed_L < 0)) || ((req_speed_L < 0) && (speed_L >= 0)))
      i_err_L = 0;                                         // reset integral on direction change
   req_speed_L = speed_L;
}


/******************************************************************************/
/* Print Control                                                              */
/*                                                                            */
/******************************************************************************/
void print_control()
{
#ifdef DEBUG_SNS
   printf("errR %d; ierrR %d; reqR %d; outR %d; dirR %d\r\n", err_R, i_err_R, req_err_R, out_R, dir_R);
   printf("errL %d; ierrL %d; reqL %d; outL %d; dirL %d\r\n", err_L, i_err_L, req_err_L, out_L, dir_L);
#endif
}

/******************************************************************************/
/* Calculate Velocity Integral Term                                           */
/*                                                                            */
/* This adds 1/6th of the current error to the integral, but saturates        */
/* it to a limit of INTEG_LIMIT.  This prevents the integral term from growing*/
/* so large that it takes a long time to reduce it once the desired velocity  */
/* is reached.                                                                */
/******************************************************************************/
int16_t calc_vel_integ(int16_t integ, int16_t err)
{
   return sat16(integ + (err)/6, INTEG_LIMIT);
}

/******************************************************************************/
/* Calculate Motor Output                                                     */
/*                                                                            */
/* This converts the total control error to PWM or servo control value.       */
/*                                                                            */
/* Futaba S3003 servo rotates at 43RPM max; GWS S03NF rotates at 63 RPM;      */
/* a command of 127 + the midpoint value of 375 is equivalent to this;        */
/* so Kp = 127 / 63 = ~2; however, at 8.4v instead of 6, need to reduce       */
/******************************************************************************/
int16_t calc_vel_out(int16_t value)
{
   return (value * (int16_t)7)/8;
}

/******************************************************************************/
/* Control Velocity                                                           */
/*                                                                            */
/* This performs the PI (Proportional / Integral) control loop calculations.  */
/* Note that the calculations are not performed if we have not received any   */
/* new information from a given servo's encoders since the last update.       */
/* This helps with stability of the integral term.                            */
/******************************************************************************/
void control_velocity()
{
   update_servo = 0;                                       // wait until next interval to do this again

   calc_enc_speeds();                                      // how fast are we going?

   if (received_R)                                         // skip calculations if we have not received any new information since the last update period
   {
      received_R = FALSE;
      err_R = (req_speed_R - enc_speed_R);                 // calculate proportional term
      i_err_R = calc_vel_integ(i_err_R, err_R);            // calculate integral term, with saturation to prevent windup
      req_err_R = (req_speed_R * 8) / 10;                  // calculate velocity feed forward term
      out_R = calc_vel_out(req_err_R + err_R + i_err_R);   // calcuate motor outputs based on total error term
      if (out_R >= 0)                                      // convert signed values to sign/magnitude for motor drivers
         dir_R = TRUE;
      else
      {
         dir_R = FALSE;
         out_R = -out_R;
      }
      if (out_R > 125)                                     // limit to maximum of 255
         out_R = 125;
   }

   if (received_L)                                         // skip calculations if we have not received any new information since the last update period
   {
      received_L = FALSE;
      err_L = (req_speed_L - enc_speed_L);
      i_err_L = calc_vel_integ(i_err_L, err_L);
      req_err_L = (req_speed_L * 8) / 10;
      out_L = calc_vel_out(req_err_L + err_L + i_err_L);
      if (out_L >= 0)
         dir_L = TRUE;
      else
      {
         dir_L = FALSE;
         out_L = -out_L;
      }
      if (out_L > 125)
         out_L = 125;
   }

   // print_control();
   update_pwm = 1;                                         // tell interrupt handler to update servo; this ensures it happens at a fixed rate
}



/******************************************************************************/
/* Set Position                                                               */
/*                                                                            */
/* This sets the left and right wheel positions.  If delta is 1, then the     */
/* position values are assumed to be relative to the current position, other- */
/* wise they are assumed to be absolute.  max_velocity is the upper limit     */
/* at which the caller wishes the robot to go while reaching that position.   */
/*                                                                            */
/* If the wheels are 8.64" in circumference, this is 0.0675" per              */
/* position increment, or 14.8 ticks per inch.                                */
/* Position is controlled in terms of encoder ticks.                          */
/******************************************************************************/
void set_position(int16_t posR, int16_t posL, int16_t max_velocity, uint8_t delta)
{
   pos_reached = FALSE;
   if (delta)
   {
      req_pos_R += posR;
      req_pos_L += posL;
   }
   else
   {
      req_pos_R = posR;
      req_pos_L = posL;
   }
   set_velocity(max_velocity, max_velocity);               // do this to calculate goal max vel
   received_R = received_L = TRUE;                         // force initial velocity update
   req_pos_max_vel = req_speed_R;

   req_pos_inc_down_count = req_speed_R / ACCEL_INCREMENTS; // calculate the number of steps to accelerate in
   if (req_pos_inc_down_count < 0)                         // bit_test(req_pos_inc_down_count, 15))
      req_pos_inc_down_count = -req_pos_inc_down_count;    // make sure it is a positive value

   if (!req_pos_inc_down_count)                            // not enough speed, so just go in one jump
   {
      req_pos_inc_down_count = 1;
      req_pos_vel_incs = req_speed_R;                      // otherwise, go in one step
   }
   else
   {
      if (req_speed_R < 0)                                 // bit_test(req_speed_R, 15)) // go in multiple steps, of the correct sign
         req_pos_vel_incs = -ACCEL_INCREMENTS;
      else
         req_pos_vel_incs = ACCEL_INCREMENTS;
   }
   req_pos_cur_max = req_pos_vel_incs;
   pos_i_err_R = pos_i_err_L = 0;
}


/******************************************************************************/
/* Print Position Control Values                                              */
/*                                                                            */
/******************************************************************************/
void print_pos_control()
{
#ifdef DEBUG_POS
   uint8_t tmp;
   printf("  req enc edir err ier spd pwm dir\r\n");
   tmp = enc_fwdir_R;
   printf("R %d, %ld, %d, %d, ",
      req_pos_R, enc_pos_R, tmp, req_pos_err_R);
   tmp = dir_R;
   printf("%d, %d, %d, %d\r\n",
      pos_i_err_R, req_speed_R, out_R, tmp);

   tmp = enc_fwdir_L;
   printf("L %d, %ld, %d, %d, ",
      req_pos_L, enc_pos_L, tmp, req_pos_err_L);
   tmp = dir_L;
   printf("%d, %d, %d, %d\r\n",
      pos_i_err_L, req_speed_L, out_L, tmp);
#endif
}


/******************************************************************************/
/* Calculate Velocity Based on Position Error                                 */
/*                                                                            */
/* The constant '125' is empirically found for a given system.  In effect,    */
/* it sets how quickly the robot decelerates as it gets close to the desired  */
/* position.                                                                  */
/* Since req_pos_cur_max (the current allowed maximum velocity) is in tenths  */
/* per second, and value is the error term in units of position in encoder    */
/* ticks, the constant 150 is in units of encoder ticks too.  Thus, the       */
/* value returned is velocity in tenths of an inch per second.                */
/******************************************************************************/
int16_t calc_pos_speed(int16_t value)
{
   return (int16_t)(((int32_t)req_pos_cur_max * value) / 125);
}

/******************************************************************************/
/* Calculate Position Integral Term                                           */
/*                                                                            */
/* This adds 3/2   of the current error to the integral, but saturates        */
/* it to a limit of POS_INTEG_LIMIT.  This prevents the integral term from    */
/* growing so large that it takes a long time to reduce it once the desired   */
/* velocity   is reached.                                                     */
/******************************************************************************/
int16_t calc_pos_integ(int16_t integ, int16_t err)
{
   return sat16(integ + (err * 3) / 2, POS_INTEG_LIMIT);   // saturate the integrator to prevent windup / overshoot / instability
}


/******************************************************************************/
/* Control Position                                                           */
/*                                                                            */
/* This routine calculates the position PI control loop.  It then uses        */
/* the resulting velocity values to set the inner velocity control loop.      */
/******************************************************************************/
void control_position()
{
   int16_t spdR;
   int16_t spdL;

   req_pos_err_R = req_pos_R - enc_pos_R;                  // calculate current error in position
   pos_i_err_R = calc_pos_integ(pos_i_err_R, req_pos_err_R); // integrate the error to ensure we reach the destination despite various frictional loads
   spdR = calc_pos_speed(req_pos_err_R + pos_i_err_R);     // convert the total position error term to desired velocity
   if ((spdR != req_speed_R) || req_pos_inc_down_count)
      received_R = TRUE;                                   // force velocity loop update, since the command to it has now changed

   req_pos_err_L = req_pos_L - enc_pos_L;
   pos_i_err_L = calc_pos_integ(pos_i_err_L, req_pos_err_L); // integrate the error to ensure we reach the destination despite various frictional loads
   spdL = calc_pos_speed(req_pos_err_L + pos_i_err_L);
   if ((spdL != req_speed_L) || req_pos_inc_down_count)
      received_L = TRUE;                                   // force velocity loop update, since the command to it has now changed

   if (!pos_reached)                                       // check to see if we are close enough
   {
      if ((abs(req_pos_err_R) < 5) && (abs(req_pos_err_L) < 5)) // allow up to 1 stripe in error (too small, and the system is unstable because we can't stop that accurately)
      {
         if ((abs(spdR) < 20) && (abs(spdL) < 20))         // also make sure we've slowed down enough
            pos_reached = TRUE;
      }
      if (req_pos_inc_down_count)                          // have we finished accelerating?
      {
         req_pos_inc_down_count--;
         req_pos_cur_max += req_pos_vel_incs;              // not yet -- bump up the speed
      }
      else
         req_pos_cur_max = req_pos_max_vel;                // we've reached the speed limit
   }

   set_velocity(spdR, spdL);                               // output our calculated speeds
   control_velocity();                                     // then use them to drive the motors.
}



/******************************************************************************/
/* Speed Control Test                                                         */
/*                                                                            */
/* This ramps the speed of each wheel down to zero from MAX_SPEED forward, to */
/* MAX_SPEED in reverse.  The robot should go in a straight line.             */
/******************************************************************************/
void speed_control_test()
{
   int16_t spd_R, spd_L;
   int i;

#ifdef DEBUG_SNS
   printf("Speed ramp test\r\n");
#endif
   spd_L = spd_R = MAX_SPEED;
   set_velocity(spd_R, spd_L);
   run_counter = 0;
   for (i = 0; i < ((MAX_SPEED * 2) / 5); )
   {
      if (update_servo)
         control_velocity();
      if (!(run_counter & 0x1f))
      {
#ifdef DEBUG_SNS
         calc_enc_speeds();
         print_enc_speeds();
         print_control();
#endif
      }
      if (run_counter > 60)
      {
         calc_enc_speeds();
         print_enc_speeds();
         print_control();
         run_counter = 0;
         spd_R -= 5;
         spd_L -= 5;
         if (spd_R < -MAX_SPEED)
         {
            spd_R = spd_L = MAX_SPEED;
         }
         set_velocity(spd_R, spd_L);
#ifdef DEBUG_SNS
         printf("--> Requested SpdR = %d; SpdL = %d\r\n", spd_R, spd_L);
#endif
         i++;
      }
   }
   set_velocity(0, 0);
   out_L = out_R = 0;
   update_pwm = 1;
}

/******************************************************************************/
/* Run to Position                                                            */
/*                                                                            */
/* Keep executing the control loops until the desired position is reached.    */
/******************************************************************************/
void run_to_position()
{
   run_counter = 0;
   while (!pos_reached)
   {
      if (update_servo)
         control_position();
      if (run_counter == 4) 
         print_enc_speeds();
      if (run_counter == 8)
         print_control();
      if (run_counter >= 12)
      {
         print_pos_control();
         run_counter = 0;
      }
   }
   set_velocity(0, 0);
   out_L = out_R = 0;
   update_pwm = 1;
}


/******************************************************************************/
/* Wait Seconds                                                               */
/*                                                                            */
/* This uses timer2's interrupt handler to delay for a while.                 */
/******************************************************************************/
void wait_seconds(long int seconds_to_wait)
{
   seconds_to_wait += seconds;
   while (seconds_to_wait > seconds)
   {
   }
}

/******************************************************************************/
/* Position Control Test                                                      */
/*                                                                            */
/* This drives the robot 12" forward, then turns 90 degrees counter clock     */
/* wise, then repeats.                                                        */
/* There are 14.8 ticks per inch, so 12" = 12 * 14.8 = 178 ticks.             */
/* To turn 90 degrees, we need to turn 1/4 of the wheel base forward on       */
/* one wheel and backward on the other.  The wheelbase is 3.5", which means   */
/* the circumference of wheel base is 11". 11/4 * 14.8 = 41 ticks.            */
/******************************************************************************/
void position_control_test()
{
   int i;

#ifdef DEBUG_POS
   printf("Position test\r\n");
#endif

   enc_pos_R = 0;
   enc_pos_L = 0;
   for (;;)
   {
      for (i = 0; i < 16; i++)
      {
#ifdef DEBUG_POS
         printf("Fwd 1 foot\r\n");
#endif
//       output_bit(RED_LED, 1);
         set_position(297, 297, MAX_SPEED, TRUE);          // forward 20"
         run_to_position();
//       output_bit(RED_LED, 0);
         wait_seconds(2);
#ifdef DEBUG_POS
         printf("Turning 90CC\r\n");
#endif
//       output_bit(RED_LED, 1);
         set_position(41, -41, 50, TRUE);                  // rotate counter clockwise 90 degrees
         run_to_position();
//       output_bit(RED_LED, 0);
         wait_seconds(2);
      }
   }
#ifdef DEBUG_POS
   printf("Done.\r\n");
#endif
   set_velocity(0, 0);
   out_R = out_L = 0;
   update_pwm = 1;
}


/******************************************************************************/
/* Main Program Entry Point                                                   */
/*                                                                            */
/******************************************************************************/
int
main (void)
{
   setup();
   printf("\nAtmel ATMEGA16 / ARC 1.1 WW-01 Odometry Example\n");
   dump_reset();
   //printf("speed test\r\n");
   //speed_control_test();
   printf("position_test\r\n");
   position_control_test();

   // stop
   for (;;)
   {
   }
   return (0);
}
