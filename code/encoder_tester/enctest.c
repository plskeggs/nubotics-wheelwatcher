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


#include "enctest.h"
#include "stdlib.h"


// timer2 variables
long timer2_overflow;                                          // incremented when timer2_ticks overflows
long timer2_ticks;                                             // incremented once per 410us; wraps every 26 seconds
long timer2_seconds_counter;                                   // counts up to 2441 to increment seconds_elapsed
volatile long seconds_elapsed;

// encoder variables
int new_b;
int prev_b;
int change_b;

short change_test;

short quad_mode;
short ww02_mode; // just for WW-02
short smart_mode; // for WW-11 or WW-12
short swap_chs_mode; // for WW-02, ChA/ChB are inverted compared to other WW-xx for same direction of codewheel rotation

short enc_chA_R;
short enc_chB_R;
short enc_DIR_R;
short enc_CLK_R;

short enc_fwdir_R_raw;
short enc_fwdir_R_clk;
signed int16 enc_dist_R_raw;                                   // in encoder ticks, about 0.36" per tick
signed int16 enc_dist_R_clk;                                   // as decoded by clk line

long prev_t2_R_hi_raw;                                         // value of timer 2 overflow at previous tick
long prev_t2_R_raw;
long enc_period_R_hi_raw;                                      // time between changes to enc R ch. A, in multiples of timer 2 overflows (every 410us)
long enc_period_R_raw;                                         // time between changes to enc R ch. A, in multiples of timer 2 overflows (every 410us)
signed int16 enc_speed_R_raw;                                   // current signed rotation rate, in units of 0.1 inches per second

long prev_t2_R_hi_clk;                                         // value of timer 2 overflow at previous tick
long prev_t2_R_clk;
long enc_period_R_hi_clk;                                      // time between changes to enc R ch. A, in multiples of timer 2 overflows (every 410us)
long enc_period_R_clk;                                         // time between changes to enc R ch. A, in multiples of timer 2 overflows (every 410us)
long enc_period_R_clk_array[4];
int array_index;
signed int16 enc_speed_R_clk;                                   // current signed rotation rate, in units of 0.1 inches per second

// velocity control variables
signed int16 req_speed_R;

#define LEFT_MOTOR  TRUE
#define RIGHT_MOTOR FALSE
#define FWD TRUE
#define REV FALSE



/********************************************************/
void init_pwm()
{
   setup_ccp1(CCP_PWM);
   setup_ccp2(CCP_OFF);
   set_pwm1_duty(0);
   set_pwm2_duty(0);

   output_high(DIR_1);
   output_high(DIR_2);
}


void set_motor_speed_dir(short left, int speed, short fwdd)
{
   if (left)
   {
      if (fwdd)
         output_high(DIR_2);
      else
         output_low(DIR_2);
      set_pwm2_duty(speed);
   }
   else
   {
      if (fwdd)
         output_high(DIR_1);
      else
         output_low(DIR_1);
      set_pwm1_duty(speed);
   }
}

/********************************************************/
void read_sensors()
{
   enc_chA_R = input(ENC_R_A);
   enc_chB_R = input(ENC_R_B);
   enc_DIR_R = input(ENC_DIR);
   enc_CLK_R = input(ENC_CLK);
}

/********************************************************/
void print_sensors()
{
   printf("\rRA=%01ld RB=%01ld DIR=%01ld CLK=%01ld",
      (long)enc_chA_R, (long)enc_chB_R, (long)enc_DIR_R, (long)enc_CLK_R);
}

/******************************************************************************/
/* INT_RB                                                                     */
/*                                                                            */
/* This handles changes to B4-B7.                                             */
/*                                                                            */
/* Outputs: enc_period_R - period in terms of ticks of timer2 (410us)  per    */
/*          A channel edge                                                    */
/*          enc_fwdir_R - boolean, true if wheel rotating forward             */
/*          enc_dist_R - distance R wheel has travelled (signed) in terms of  */
/*          A channel edges (26 per rotation, wheel is 2.625" diameter ->     */
/*          0.317")                                                           */
/******************************************************************************/
#int_rb
change_isr() // connects to DIR and CHB
{
   change_test = 1;
   new_b = input_b();
   change_b = prev_b ^ new_b;
   prev_b = new_b;
   if ((change_b & ENC_R_B_BIT) && (new_b & ENC_R_B_BIT)) // for symmetry, only sample on rising edge
   {
      if (!(new_b & ENC_R_B_BIT))
         enc_fwdir_R_raw = TRUE;
      else
         enc_fwdir_R_raw = FALSE;
      if (swap_chs_mode)
         enc_fwdir_R_raw = !enc_fwdir_R_raw;
      if (enc_fwdir_R_raw)
         enc_dist_R_raw += 4;
      else
         enc_dist_R_raw -= 4;
      enc_period_R_raw = timer2_ticks - prev_t2_R_raw;
      prev_t2_R_raw = timer2_ticks;
   }
}

void add_period(long period)
{
   enc_period_R_clk_array[array_index++] = period;
   if (array_index >= 4)
      array_index = 0;
}

#int_timer0
timer0_isr() // connects to CLK line
{
   set_timer0(255); // max count so first tick gives us an interrupt
   enc_fwdir_R_clk = !input(ENC_DIR);
   if (swap_chs_mode)
      enc_fwdir_R_clk = !enc_fwdir_R_clk;
   if (smart_mode) // CLK line toggles on each change of ChA/ChB
   {
      if (enc_fwdir_R_clk)
         enc_dist_R_clk += 2;
      else
         enc_dist_R_clk -= 2;
   }
   else  // CLK line pulses on each change of ChA/ChB
   {
      if (enc_fwdir_R_clk)
         enc_dist_R_clk++;
      else
         enc_dist_R_clk--;
   }
   enc_period_R_clk = timer2_ticks - prev_t2_R_clk;
   if (!quad_mode)
      add_period(enc_period_R_clk);
   prev_t2_R_clk = timer2_ticks;
}

#int_ext
b0_isr() // connects to CHA
{
   enc_fwdir_R_raw = input(ENC_R_B);   // we are sampling direction on rising edge of B
   if (swap_chs_mode)
      enc_fwdir_R_raw = !enc_fwdir_R_raw;
   if (enc_fwdir_R_raw)
      enc_dist_R_raw += 4;
   else
      enc_dist_R_raw -= 4;
   enc_period_R_raw = timer2_ticks - prev_t2_R_raw;
   if (quad_mode)
      add_period(enc_period_R_raw);
   prev_t2_R_raw = timer2_ticks;
}

//---------------------------------------------------
// Check Duty Cycle
//---------------------------------------------------
short check_duty_cycle()
{
   long average = 0;
   long minimum = 65535;
   long maximum = 0;
   int i;

   for (i = 0; i < 4; i++)
   {
      average += enc_period_R_clk_array[i];
      if (enc_period_R_clk_array[i] < minimum)
         minimum = enc_period_R_clk_array[i];
      if (enc_period_R_clk_array[i] > maximum)
         maximum = enc_period_R_clk_array[i];
   }
   average >>= 2; // divide by 4

   printf("Minimum Period = %ld, Average Period = %ld, Maximum Period = %ld\r\n", minimum, average, maximum);

   if (minimum < (average / 3))
   {
      printf("ERROR: minimum is less than average / 3.\r\n");
      return 0;
   }
   else if (maximum > (average * 2))
   {
      printf("ERROR: minimum is greater than average * 2.\r\n");
      return 0;
   }
   else
      return 1;
}


//---------------------------------------------------
// Calc Encoder Speeds
//---------------------------------------------------
void calc_enc_speeds()
{
   if (enc_period_R_hi_raw) // overflows every 26 seconds, so just count this as close enough to zero; avoid full 32 bit math
      enc_speed_R_raw = 0;
   else if (enc_period_R_raw == 0)
      enc_speed_R_raw = 0;
   else
   {
      enc_speed_R_raw = 15464 / enc_period_R_raw;  // converts from 410us ticks per edge to multiples of 0.1 inches per second
      if (enc_fwdir_R_raw == 0)
         enc_speed_R_raw = -enc_speed_R_raw;
   }

   if (enc_period_R_hi_clk)
      enc_speed_R_clk = 0;
   else if (enc_period_R_clk == 0)
      enc_speed_R_clk = 0;
   else
   {
      enc_speed_R_clk = 15464 / enc_period_R_clk;
      if (enc_fwdir_R_clk == 0)
         enc_speed_R_clk = -enc_speed_R_clk;
   }
}

void print_enc_speeds()
{
   int fwdv;
   fwdv = (int)enc_fwdir_R_raw;
   printf("RAW EncPer %04lX%04lX; EncSpd %ld 10ths/s; EncDst %ld 10ths, Fwd %d\r\n", 
      enc_period_R_hi_raw, enc_period_R_raw, enc_speed_R_raw, enc_dist_R_raw, fwdv);
   fwdv = (int)enc_fwdir_R_clk;
   printf("CLK EncPer %04lX%04lX; EncSpd %ld 10ths/s; EncDst %ld 10ths, Fwd %d\r\n", 
      enc_period_R_hi_clk, enc_period_R_clk, enc_speed_R_clk, enc_dist_R_clk, fwdv);
}

//---------------------------------------------------
// Timer 2 Interrupt Handler
//---------------------------------------------------
#int_timer2
timer2_isr()
{
   timer2_ticks++;
   timer2_seconds_counter++;
   if (timer2_seconds_counter == 244) // 2441 is 1 sec; 244 is 0.10 sec
   {
      timer2_seconds_counter = 0;
      seconds_elapsed++;                                              // count timer ticks, one per second
   }
}

//---------------------------------------------------
// Control Velocity
//---------------------------------------------------
void set_velocity(signed long speed_R)
{
   if (speed_R > 60)
      speed_R = 60;
   if (speed_R < -60)
      speed_R = -60;
   req_speed_R = speed_R;
}


signed long err_R;
signed long out_R;
short dir_R;

void print_control()
{
   int tmp;
   tmp = dir_R;
   printf("req speed R %ld; enc speed R raw %ld; enc speed R clk %ld; ", req_speed_R, enc_speed_R_raw, enc_speed_R_clk);
   printf("err R %ld; out R %ld; dir R %d\r\n", err_R, out_R, tmp);
}

//---------------------------------------------------
// Control Velocity
//---------------------------------------------------
void control_velocity()
{
   calc_enc_speeds();
   print_enc_speeds();
}

void remainder_of_control_velocity()
{
   err_R = req_speed_R - enc_speed_R_clk;

   err_R *= 4;
   err_R /= 3;

// Futaba S3003 servo rotates at 43RPM max, with a MarkIII tire = 5.91 inches / sec = 59.1 tenths per sec; a command of 255
// to the set_servo_speed_dir function is equivalent to this; so Kp = ~4.3 = 17/4

   out_R = ((req_speed_R + err_R) * 17) / 4;

   if (out_R >= 0)
      dir_R = TRUE;
   else
   {
      dir_R = FALSE;
      out_R = -out_R;
   }
   if (out_R > 255)
      out_R = 255;

   print_control();

   set_motor_speed_dir(FALSE, (int)out_R, dir_R);
}



//---------------------------------------------------
// Setup the Hardware
//---------------------------------------------------
void setup()
{
/* zeroed by #zero_ram
   array_index = 0;
   timer2_seconds_counter = 0;
   timer2_ticks = 0;
   seconds_elapsed = 0;
   prev_t2_R_raw = 0;
   prev_t2_R_clk = 0;
   prev_b = 0;
   enc_dist_R_raw = 0;
   enc_dist_R_clk = 0;
   enc_period_R_hi_raw = 0;
   enc_period_R_hi_clk = 0;
*/
   set_tris_a(TRIS_A_VAL);
   set_tris_b(TRIS_B_VAL);
   set_tris_c(TRIS_C_VAL);

   port_b_pullups(TRUE);
   setup_adc(ADC_CLOCK_DIV_32);
   setup_adc_ports(RA0_RA1_RA3_ANALOG);
   setup_spi(FALSE);
   setup_counters(RTCC_EXT_L_TO_H, WDT_2304MS);
   set_timer0(255);                                // max count so interrupts on first edge
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_2);
   set_timer1(0);
   setup_timer_2(T2_DIV_BY_4,255,1);

   set_tris_a(TRIS_A_VAL);
   set_tris_b(TRIS_B_VAL);
   set_tris_c(TRIS_C_VAL);

   // set_adc_channel(0);
   // enable_interrupts(INT_ADC);
   // ext_int_edge(L_TO_H);
   enable_interrupts(INT_EXT);
   enable_interrupts(INT_RTCC);
   enable_interrupts(INT_TIMER2);
// enable_interrupts(RB_CHANGE);
   enable_interrupts(global);

   init_pwm();

   delay_ms(1000);

}


void clear_counts()
{
   enc_dist_R_raw = 0;
   enc_dist_R_clk = 0;
   enc_speed_R_raw = 0;
   enc_speed_R_clk = 0;
}


//---------------------------------------------------
// Main Program Entry Point
//
//---------------------------------------------------
#zero_ram
void main()
{
   long speed = 0;
   long goal_speed = 100;
   long count = 0;
   short lasta = 0;
   short lastb = 0;
   short printit = 1;
   signed int16 diff_dst;
   signed int16 diff_spd;
   short failed_fwd;
   short failed_bak;
   short failed_vcc;
   short failed_gnd;
   short failed_fwd_per;
   short failed_bak_per;
   short failed;
   short blink;
   int8 voltage;
   signed int16 lower_limit;
   signed int16 upper_limit;

   setup();
   
   // check mode settings
   if (input(SW_1))
      smart_mode = TRUE;
   else
      smart_mode = FALSE;
   
   if (input(SW_2))
      ww02_mode = FALSE;
   else 
      ww02_mode = TRUE;
   
   if (input(WW02_BB_MODE))
      quad_mode = FALSE;
   else
      quad_mode = TRUE;

   if (ww02_mode && !smart_mode)
      swap_chs_mode = TRUE;
   else
      swap_chs_mode = FALSE;

   // print what our current settings are
   if (smart_mode && !ww02_mode)
      printf("WW-11 Tester 1.00\r\n");
   else if (smart_mode && ww02_mode)
      printf("WW-12 Tester 1.00\r\n");
   else if (!smart_mode && !ww02_mode)
      printf("WW-01 Tester 1.03\r\n");
   else if (!smart_mode && ww02_mode)
      printf("WW-02 Tester 1.02\r\n");
   if (!quad_mode)
      printf("-- full mode\r\n");
   else
      printf("-- quadrature mode\r\n");

   output_bit(RED_LED, 0);
   output_bit(AMBER_LED, 0);
   delay_ms(2000);

   for (;;)
   {
      // wait for user to turn on sw1
      failed_fwd = FALSE;
      failed_bak = FALSE;
      failed_vcc = FALSE;
      failed_gnd = FALSE;
      failed_fwd_per = FALSE;
      failed_bak_per = FALSE;
      failed = FALSE;
      blink = FALSE;


//-----------------------------
// do initial tests of cable
//-----------------------------
      output_bit(AMBER_LED, 0); // testing!


      if (!ww02_mode)// servo fixture runs faster
      {
         lower_limit = 200;
         upper_limit = 300;
      }
      else
      {
         lower_limit = 175;
         upper_limit = 250;
      }

      if (!smart_mode && !ww02_mode) // check extra pins on WW-01 only
      {
         set_adc_channel(0);
         delay_us(50);
         voltage = read_adc(); // read extra Vcc pin
         if (voltage < 250)
         {
            printf("FAILED! Bad ");
            failed_vcc = TRUE;
         }
         printf("Vcc level = %ld\r\n", (long) voltage);

         set_adc_channel(1);
         delay_us(50);
         voltage = read_adc(); // read extra Gnd pin
         if (voltage > 5)
         {
            printf("FAILED! Bad ");
            failed_gnd = TRUE;
         }
         printf("Gnd level = %ld\r\n", (long) voltage);
      }

//-----------------------------
// run motor forward
//-----------------------------
      printf("Testing Forward\r\n");
      timer2_ticks = 0;
      seconds_elapsed = 0;
      clear_counts();
      set_motor_speed_dir(FALSE, 254, 1); // start motor forward
      while (seconds_elapsed < 30)
      {
      }

      read_sensors();
      if (printit)
         print_sensors();

      if (quad_mode)
         enc_speed_R_clk = enc_speed_R_raw;
      else if (smart_mode) // we don't get quad signals in smart mode if also in full mode
         enc_speed_R_raw = enc_speed_R_clk;
      diff_spd = abs(enc_speed_R_raw - enc_speed_R_clk);
      failed_fwd_per = !check_duty_cycle();
      calc_enc_speeds();

      set_motor_speed_dir(FALSE, 0, 1); // stop motor
      while (seconds_elapsed < 40)
      {
      }

      if (quad_mode)
         enc_dist_R_clk = enc_dist_R_raw;
      else if (smart_mode)
         enc_dist_R_raw = enc_dist_R_clk;
      diff_dst = abs(enc_dist_R_raw - enc_dist_R_clk);

      print_enc_speeds();
      printf("distance diff = %ld; speed diff = %ld\r\n", diff_dst, diff_spd);

      if ((diff_dst > 4) || (enc_dist_R_clk < lower_limit) || (enc_dist_R_clk > upper_limit))
      {
         failed_fwd = TRUE;
         printf("FAILED!  Dist = %ld, not >= %ld and <= %ld\r\n", enc_dist_R_clk, lower_limit, upper_limit);
      }

//-----------------------------
// run motor backward
//-----------------------------
      printf("Testing Backward\r\n");
      timer2_ticks = 0;
      seconds_elapsed = 0;
      clear_counts();
      set_motor_speed_dir(FALSE, 254, 0); // start motor backward
      while (seconds_elapsed < 30)
      {
      }

      read_sensors();
      if (printit)
         print_sensors();

      if (quad_mode)
         enc_speed_R_clk = enc_speed_R_raw;
      else if (smart_mode)
         enc_speed_R_raw = enc_speed_R_clk;
      diff_spd = abs(enc_speed_R_raw - enc_speed_R_clk);
      failed_bak_per = !check_duty_cycle();
      calc_enc_speeds();

      set_motor_speed_dir(FALSE, 0, 0); // stop motor
      while (seconds_elapsed < 40)
      {
      }

      if (quad_mode)
         enc_dist_R_clk = enc_dist_R_raw;
      else if (smart_mode)
         enc_dist_R_raw = enc_dist_R_clk;
      diff_dst = abs(enc_dist_R_raw - enc_dist_R_clk);

      print_enc_speeds();
      printf("distance diff = %ld; speed diff = %ld\r\n", diff_dst, diff_spd);

      if ((diff_dst > 4) || (enc_dist_R_clk > -lower_limit) || (enc_dist_R_clk < -upper_limit))
      {
         failed_bak = TRUE;
         printf("FAILED!\r\n");
         printf("FAILED!  Dist = %ld, not <= %ld and >= %ld\r\n", enc_dist_R_clk, -lower_limit, -upper_limit);
      }

//-----------------------------
// check for overall pass/fail
//-----------------------------
      if (failed_fwd || failed_bak || failed_gnd || failed_vcc || failed_fwd_per || failed_bak_per)
      {
         output_bit(RED_LED, 0); // on = FAILED
         printf("FAILED!\r\n");
         failed = TRUE;
      }
      else
      {
         output_bit(RED_LED, 1); // off
         printf("PASSED!\r\n");
      }

//-----------------------------
// wait for results acknowledged
//-----------------------------
      output_bit(AMBER_LED, 1); // done testing!
      printf("change isr active: %d\r\n", change_test);
      for (;;)
      {
         read_sensors();
         if (printit)
            print_sensors();
         delay_ms(200);
         if (failed)
            output_bit(RED_LED, blink);
         else
            output_bit(AMBER_LED, blink);
         blink = !blink;
      }
   }
}
