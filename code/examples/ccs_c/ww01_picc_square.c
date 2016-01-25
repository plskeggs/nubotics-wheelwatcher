//---------------------------------------------------
// WW-01 CCS C Advanced Drive-a-Square Example Program
//
// Copyright 2004-2009, Noetic Design, Inc.
//
// This demonstation program shows off the features
// of the WheelWatcher.
// It is a pretty complex program.  It can be used with
// either RC servos modified for continuous rotation, or
// with stripped RC servos being driven by an external H bridge
// (which gives you much better control).  This external H bridge
// can be wither directly connected or controlled through
// an I2C parallel port chip, as in the MarkIII sensor board.
// Further, this example measures the current velocity of each
// wheel by measuring the time between each clock pulse, and
// performs a running average of the most recent 4 clock periods
// to filter out alignment errors.  This example also
// performs both closed loop velocity control as well as
// closed loop position control of each wheel independently.
// Velocity control includes velocity feedforward for quicker
// response, and has a saturating integrator to prevent
// integrator windup from causing overshoot.  We also reset
// the integrators when changing directions, which helps
// stability.
// The actual demo uses all of these features to drive in
// a 12" square pattern.
//---------------------------------------------------

//*********************************************************
// CONFIGURE SOURCE CODE FOR VARIOUS HARDWARE OPTIONS
//*********************************************************

// *** uncomment these to turn on various debug print statements
//#define DEBUG_SNS 1
//#define DEBUG_STAT 1

// *** uncomment this to enable rc servo mode
// comment it out to enable directly connected H bridge mode
#define RC_SERVO_MODE 1

#ifdef RC_SERVO_MODE
// *** uncomment to set to produce 1.5ms pulses for calibrating servos
#define ZERO_SERVOS 1
#endif

// *** uncomment this to enable H bridge mode where the direction lines
// are connected to a Philips 8574 I2C parallel port chip
//#define MARKIII_SENSOR_BOARD_HBRIDGE_MODE 1

// *** uncomment this to enable connecting the left WW-01's CLK line to T1CKI
// comment it to connect the CLK line to RB0 / EXT INT
//#define LEFT_CLK_ON_TIMER_1 1

// *** uncomment this to try nested position and velocity
// loops; otherwise, use one or the other, depending on which
// control function is used
//#define NESTED_LOOPS 1

// *** encomment to enable various tests
#define ENCODER_TEST 1  // use this to verify that your encoders are working
//#define MOTOR_TEST 1    // use this to verify that your motors or servos are wired right, and spin the right way
//#define SPEED_TEST 1    // use this to test velocity control
//#define POSITION_TEST 1 // use this to test position control

#ifdef ENCODER_TEST
#ifndef DEBUG_SNS
#define DEBUG_SNS 1  // this must be on for encoder test
#endif
#endif

// configure before including this:
#include "ww01_picc_square.h"


//*********************************************************
// DEFINE VARIABLES
//*********************************************************


ROBOTWHEEL rwheel;
ROBOTWHEEL lwheel;
ROBOTWHEEL *rw;                                            // points to the one for the right
ROBOTWHEEL *lw;                                            // and the left

// rc servo variables
#ifdef RC_SERVO_MODE
short servos_on = FALSE;
#endif
#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
int pcf8574_value;                                         // current value written to I2C parallel port chip
extern void write_pcf8574(int value);
#endif

// timer2 variables
int32 timer2_ticks;                                        // incremented once per 204us; wraps every 10 days
long control_loop_counter;                                 // counts up to 2441 to increment seconds_elapsed
long seconds_elapsed;

// program state flags
short update_servo;                                        // it is time to calculate the PI control loop
short update_pwm;                                          // it is time to update the motors

// function prototypes
void init_motors();
#ifdef RC_SERVO_MODE
void set_servo_position(ROBOTWHEEL *w, long deg);
void set_motor_speed_dir(ROBOTWHEEL *w);
#endif


/******************************************************************************/
/*  Setup the Hardware                                                        */
/******************************************************************************/
void setup()
{
   lw = &lwheel;
   lwheel.high_fwd_dir = TRUE;                             // high direction line when wheel encoder rotates forward
   rw = &rwheel;
   rwheel.pos_fwd_pw = TRUE;                               // greater than 1.5ms is forward direction of rotation

   set_tris_a(TRIS_A_VAL);
   set_tris_b(TRIS_B_VAL);
   set_tris_c(TRIS_C_VAL);
   #if __device__==877
   set_tris_d(TRIS_D_VAL);
   set_tris_e(TRIS_E_VAL);
   #endif

   port_b_pullups(TRUE);
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_CLOCK_DIV_32);
   setup_psp(PSP_DISABLED);

   setup_counters(RTCC_EXT_L_TO_H, WDT_2304MS);            // set up timer 0 input T0CKI for Right CLK input
   set_timer0(255);                                        // max count so interrupts on first edge

#ifdef LEFT_CLK_ON_TIMER_1
   setup_timer_1(T1_EXTERNAL|T1_DIV_BY_1);                 // use these settings when connecting Left CLK to timer 1 external input T1CKI
   set_timer1(65535);
#else
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_2);                 // set up timer 1 to generate proper timing for COMPARE unit RC servo timing
   set_timer1(0);
   ext_int_edge(H_TO_L);                                   // set up RB0 to interrupt on falling edge of Left CLK
#endif

   setup_timer_2(T2_DIV_BY_1,255,4);                       // 4883 Hz interrupt rate (204.8us period); 19531 Hz PWM frequency

   init_motors();
   enable_interrupts(INT_RTCC);                            // Right CLK
#ifdef LEFT_CLK_ON_TIMER_1
   enable_interrupts(INT_TIMER1);                          // Left CLK
#else
   enable_interrupts(INT_EXT);                             // Left CLK
#endif
   enable_interrupts(INT_TIMER2);                          // time keeping
   enable_interrupts(global);

#ifdef DEBUG_SNS
   printf("WW-01 CCS C Square Example Starting!\r\n");
#endif
}



#ifdef RC_SERVO_MODE
/******************************************************************************/
/* Init Motors                                                                */
/*                                                                            */
/* This does all the setup necessary to drive two servos from the two CCP     */
/* units.  The servo control lines do not need to be connected to the CCP     */
/* output lines.                                                              */
/******************************************************************************/
void init_motors()
{
   setup_ccp1(CCP_COMPARE_INT);
   setup_ccp2(CCP_COMPARE_INT);
   enable_interrupts(INT_CCP1);
   enable_interrupts(INT_CCP2);
   set_motor_speed_dir(lw);                                // speed will be zero due to power on memory clearing
   set_motor_speed_dir(rw);
   lwheel.servo_high = 1;
   output_high(SERVO_L);
   CCP_1 = lwheel.servo_position;
   rwheel.servo_high = 1;
   output_high(SERVO_R);
   CCP_2 = rwheel.servo_position;
   servos_on = FALSE;
}


/******************************************************************************/
/* CCP1 ISR: GENERATE SERVO CONTROL PULSES IN BACKGROUND                      */
/*                                                                            */
/* This works by using the 16 bit timer1 as a master clock.  The servo control*/
/* pulse needs to be output once every 20 milliseconds.                       */
/* The high portion of the servo control pulse should be between approximately*/
/* 0.5ms to 2.5ms; this is set by the global variable servo_L_position.       */
/* So, when it is time to set the SERVO_L output line high, we set the value  */
/* to compare CCP1 with timer1 to servo_L_position, and place CCP1 in simple  */
/* COMPARE_INT mode.  When timer1 = servo_L_position, we get an interrupt.    */
/* Since the variable servo_L_high is true, we turn off SERVO_L, set CCP1 to  */
/* compare timer1 against 50000 (to finish the remainder of the 20ms), then   */
/* change CCP1's mode to not only be in compare mode, but also automatically  */
/* reset timer1 to zero.  When that point is reached, we get another interrupt*/
/* at which point it is time again for the high side of the control signal.   */
/* This mechanism results in a very clean, jitter-free waveform.              */
/******************************************************************************/
#int_CCP1
CCP1_isr() {
   if (lwheel.servo_high)                                  // time to switch to the low side
   {
      output_low(SERVO_L);                                 // turn off the servo control line
      CCP_1 = 50000;                                       // interrupt me again when timer1 = 50000
      setup_ccp1(CCP_COMPARE_RESET_TIMER);                 // also, reset timer1 to zero upon interrupt
      lwheel.servo_high = 0;                               // next time, process the high side
   }
   else                                                    // process the high side
   {
      if (servos_on)                                       // don't output anything if servos are turned off.
         output_high(SERVO_L);                             // otherwise, drive the control line high
      CCP_1 = lwheel.servo_position;                       // when we reach the required pulse width, interrupt me
      setup_ccp1(CCP_COMPARE_INT);
      lwheel.servo_high = 1;                               // next time, process the low side
   }
}

/******************************************************************************/
/* CCP2 ISR                                                                   */
/* See the CCP1 ISR's comment block to understand how this works.             */
/******************************************************************************/
#int_CCP2
CCP2_isr() {
   if (rwheel.servo_high)
   {
      output_low(SERVO_R);
      CCP_2 = 50000;
      rwheel.servo_high = 0;
   }
   else
   {
      if (servos_on)
         output_high(SERVO_R);
      CCP_2 = rwheel.servo_position;
      rwheel.servo_high = 1;
   }
}



/******************************************************************************/
/* Set Servo Position (in degrees)                                            */
/*                                                                            */
/* This function is useful if you are driving an unmodified RC servo to a     */
/* given angle.                                                               */
/* Convert servo position to CCP counter value; 50000 counts = 20ms           */
/* 0 = all the way one way; 90 = centered; 180 = all the way the other way    */
/******************************************************************************/
void calc_servo_position(ROBOTWHEEL *w, long deg)
{
   long temp;
   long pos;

   temp = deg << 2;
   pos = temp;
   temp <<= 1;
   pos += temp;
   temp <<= 1;
   pos += temp + (long)1240;
   w->servo_position = pos;
   servos_on = TRUE;
}

/******************************************************************************/
/* Set Motor Speed Dir                                                        */
/*                                                                            */
/* this routine assumes that speed is normalized already for the linear       */
/* range of speed control for the servo, and correctly drives the right servo */
/* in reverse compared to the left for Pete's MarkIII proto, a range of 256   */
/* for speed gives good control                                               */
/******************************************************************************/
void set_motor_speed_dir(ROBOTWHEEL *w)                    // , int speed, short fwdd)
{
   if (w->pos_fwd_pw)
   {
      if (w->v_dir)
         w->servo_position = CENTER_PERIOD + w->v_out;
      else
         w->servo_position = CENTER_PERIOD - w->v_out;
   }
   else
   {
      if (w->v_dir)
         w->servo_position = CENTER_PERIOD - w->v_out;
      else
         w->servo_position = CENTER_PERIOD + w->v_out;
   }
   servos_on = TRUE;
}

#else
/******************************************************************************/
/* drive motors with H bridge instead of servo control pulses                 */
/******************************************************************************/

/******************************************************************************/
/* Init PWM-driven brushed DC motors                                          */
/*                                                                            */
/* NOTE: this example assumes that the PWM inputs of the H bridges are        */
/* connected to the output pins of the two CCP units, and that the direction  */
/* control lines of the H bridges are connected directly to PIC I/O lines.    */
/******************************************************************************/
void init_motors()
{
   setup_ccp1(CCP_PWM);
   setup_ccp2(CCP_PWM);
   set_pwm1_duty(0);
   set_pwm2_duty(0);
#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
   pcf8574_value = 0;
   write_pcf8574(pcf8574_value);
#else
   output_high(RMOT_DIR);
   output_high(LMOT_DIR);
#endif
}

#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
/******************************************************************************/
/* Write PCF8574                                                              */
/*                                                                            */
/* This routine uses I2C to output 8 bits.                                    */
/******************************************************************************/
void write_pcf8574(int value)
{
   i2c_start();
   i2c_write(PCF8574_ID);
   i2c_write(value);
   i2c_stop();
}
#endif



/******************************************************************************/
/* Set Motor Speed Dir                                                        */
/*                                                                            */
/******************************************************************************/
void set_motor_speed_dir(ROBOTWHEEL *w)                    // , int speed, short fwdd)
{
   int8 out;
   short dir;
   if (w->v_out == 0)
      out = 0;
   else if (w->v_out <= (255 - MIN_OUT))
      out = (int8)(w->v_out + MIN_OUT);                    // start PWM at a minimum value needed to cause rotation
   else
      out = 255;
   dir = w->v_dir;
   if (!w->pos_fwd_pw)
      dir = !dir;
   if (w == lw)                                            // left wheel
   {
#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
      if (dir)
         pcf8574_value |= LMOT_DIR;                        // control the H bridge direction using the I2C parallel port chip
      else
         pcf8574_value &= ~LMOT_DIR;
#else
      if (dir)
         output_high(LMOT_DIR);                            // directly control the H bridge direction
      else
         output_low(LMOT_DIR);
#endif
      set_pwm2_duty(out);
   }
   else
   {
#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
      if (dir)
         pcf8574_value |= RMOT_DIR;
      else
         pcf8574_value &= ~RMOT_DIR;
#else
      if (dir)
         output_high(RMOT_DIR);
      else
         output_low(RMOT_DIR);
#endif
      set_pwm1_duty(out);
   }

#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
   write_pcf8574(pcf8574_value);                           // write both at the same time
#endif
}
#endif



/******************************************************************************/
/* Update Wheel                                                               */
/*                                                                            */
/* Keep track of current position, and record time for later calcuating speed.*/
/******************************************************************************/
void update_wheel(ROBOTWHEEL *w, short dirval)
{
   if (!w->high_fwd_dir)
      w->enc_fwddir = !dirval;
   else
      w->enc_fwddir = dirval;
   if (w->enc_fwddir)
      w->enc_pos++;
   else
      w->enc_pos--;
   w->enc_ticks++;
   w->enc_period_end = timer2_ticks;
   w->enc_read = TRUE;
   w->enc_no_motion_periods = 0;
}

/******************************************************************************/
/* Timer0 ISR (Right Encoder Clk)                                             */
/*                                                                            */
/******************************************************************************/
#int_timer0
void timer0_isr()
{
   set_timer0(255);                                        // max count so first tick gives us an interrupt
   update_wheel(rw, input(ENC_R_DIR));
}


/******************************************************************************/
/* Left Encoder Clk ISR                                                       */
/*                                                                            */
/******************************************************************************/
#ifdef LEFT_CLK_ON_TIMER_1
#int_timer1
void timer1_isr()
{
   set_timer1(65535);
#else
#int_ext
void ext_isr()
{
#endif
   update_wheel(lw, input(ENC_L_DIR));
}

/******************************************************************************/
/* Timer 2 Interrupt Handler                                                  */
/*                                                                            */
/* this keeps track of time and updates the servos                            */
/* on a fixed interval                                                        */
/******************************************************************************/
#int_timer2
void timer2_isr()
{
   timer2_ticks++;
   control_loop_counter++;
   if (control_loop_counter == CONTROL_LOOP_COUNT)         // once per 40.96ms = 24.4 Hz
   {
      update_servo = 1;
      seconds_elapsed++;                                   // count timer ticks, one per 40.96 ms, 24.4 Hz
      control_loop_counter = 0;
      if (rwheel.enc_no_motion_periods < NO_MOTION_LIMIT)
         rwheel.enc_no_motion_periods++;
      if (lwheel.enc_no_motion_periods < NO_MOTION_LIMIT)
         lwheel.enc_no_motion_periods++;
   }
   if (update_pwm)                                         // update the PWM or servo control pulse value synchronous to the clock
   {
      update_pwm = 0;
      set_motor_speed_dir(rw);
      set_motor_speed_dir(lw);
   }
}




/******************************************************************************/
/* Calc Encoder Speeds                                                        */
/*                                                                            */
/* I've found that working in units of tenths of an inch per second to be     */
/* useful.  This allows one to measure down to 6" per minute.                 */
/*                                                                            */
/* Speed in 0.1"/sec = (Cwh / NCLKS) * TSCALE / (TICK * PER) = Ktps / PER     */
/* where Cwh = 8.64" wheel circumference, NCLKS = 128 (32 stripe disk),       */
/* TSCALE = 10 (to convert inches to 0.1"), TICK = 205us per timer2 tick,     */
/* and PER is the measured period in timer 2 ticks                            */
/* Ktps = 3296                                                                */
/******************************************************************************/
void calc_enc_speeds(ROBOTWHEEL *w)
{
   if (w->enc_read)                                       // if the wheel is spinning
   {
      w->enc_period = w->enc_period_end - w->enc_period_start;// calculate period since last update
      if (w->enc_ticks > 1)
         w->enc_period /= w->enc_ticks;                    // average period of all ticks during sampling interval
      if (w->enc_period)
         w->enc_speed = (signed int16)(Ktps / w->enc_period); // converts from 205us ticks per edge to multiples of 0.1 inches per second
      else
         w->enc_speed = 0;                                 // no speed
      w->enc_period_start = w->enc_period_end;             // it starts from timer tick of last one
      w->enc_ticks = 0;                                    // need at least one new tick to measure period
      if (!w->enc_fwddir)
         w->enc_speed = -w->enc_speed;
   }
   else if (w->enc_no_motion_periods >= NO_MOTION_LIMIT)   // have we timed out after receiving no encoder ticks?
   {
      w->enc_period = Ktps + 1;                            // yes, assume zero speed
      w->enc_speed = 0;
      w->enc_read = TRUE;                                  // force update to velocity control loop
   }
   // else we leave enc_speed alone, as we don't have any new information, but we leave enc_read false, so control loop does
   // not overcompensate
}


/******************************************************************************/
/* Print Encoder Speeds                                                       */
/******************************************************************************/
void print_enc_speeds()
{
#ifdef DEBUG_SNS
   printf("R: EncSpd %ld; Pos %ld Fwd %d; ", rwheel.enc_speed, rwheel.enc_pos, (int)rwheel.enc_fwddir);
   printf("L: EncSpd %ld; Pos %ld Fwd %d\r\n", lwheel.enc_speed, lwheel.enc_pos, (int)lwheel.enc_fwddir);
#endif
}

/******************************************************************************/
/* Sat16                                                                      */
/* Limit value to be between positive and negative limits.                    */
/******************************************************************************/
#separate
signed long sat16(signed long value, signed long limit)
{
   if (!bit_test(value, 15))                               // positive
   {
      if (value > limit)
         value = limit;
   }
   else
   {
      if (value < -limit)
         value = -limit;
   }
   return value;
}

/******************************************************************************/
/* Sat32                                                                      */
/* Limit value to be between positive and negative limits.                    */
/******************************************************************************/
#separate
signed long sat32(signed int32 value, signed int32 limit)
{
   if (!bit_test(value, 31))                               // positive
   {
      if (value > limit)
         value = limit;
   }
   else
   {
      if (value < -limit)
         value = -limit;
   }
   return value;
}


/******************************************************************************/
/* Set Velocity                                                               */
/*                                                                            */
/* Set velocity in terms of tenths of an inch per second.                     */
/******************************************************************************/
void set_velocity(ROBOTWHEEL *w, signed long speed)
{
   speed = sat16(speed, MAX_SPEED);
   if (bit_test(w->req_speed, 15) ^ bit_test(speed, 15))
      w->v_i_err = 0;                                      // reset integral on direction change
   w->v_prev_err = 0;                                      // reset previous differential error
   w->req_speed = speed;
}


/******************************************************************************/
/* Print Control                                                              */
/*                                                                            */
/******************************************************************************/
void print_vel_control(ROBOTWHEEL *w)
{
#ifdef DEBUG_SNS
   printf("V%c:", (w == lw) ? 'L' : 'R');
   printf("t %ld p %ld i %ld d %ld", w->v_err, w->v_p_err, w->v_i_err, w->v_d_err);
   printf(" f %ld req %ld out %ld dir %ld\r\n", w->v_f_err, w->req_speed, w->v_out, (int16)w->v_dir);
#endif
}


/******************************************************************************/
/* Control Motor                                                              */
/*                                                                            */
/* This converts the total control error to PWM or servo control value.       */
/*                                                                            */
/* Futaba S3003 servo rotates at 43RPM max, with an injection molded wheel    */
/* = 5.91 inches / sec = 59.1 tenths per sec;                                 */
/* a command of 255 to the set_servo_speed_dir function is equivalent to this;*/
/* so Kp = 255 / 59.1 = ~4.3 = 17/4                                           */
/* for 54 RPM (S03NTXF servos), Kp = 255 / 74 ~= 27 / 8;                      */
/* for 63 RPM (S03NF servos), with O-ring wheel circumference of 8.64",       */
/* = 9.07 inches / sec = 90.7 tenths per sec;                                 */
/* a command of 255 is equivalent to this, so Kp = 255 / 90.7 = ~2.8 = 45/16  */
/*                                                                            */
/* This then sets the motor output and direction values, and requests the     */
/* timer2 interrupt handler to use these new values.                          */
/******************************************************************************/

void control_motor(ROBOTWHEEL *w)
{
   w->v_out = (w->v_err * KoVN) / KoVD;                    // calculate motor outputs

   if (!bit_test(w->v_out, 15))                            // convert signed values to sign/magnitude for motor drivers
      w->v_dir = TRUE;
   else
   {
      w->v_dir = FALSE;
      w->v_out = -w->v_out;
   }
   if (w->v_out > 255)                                     // limit to maximum of 255
      w->v_out = 255;
// print_vel_control(w);
   update_pwm = 1;                                         // tell interrupt handler to update servo; this ensures it happens at a fixed rate
}


   /******************************************************************************/
/* Control Velocity                                                           */
/*                                                                            */
/* This performs the PI (Proportional / Integral) control loop calculations.  */
/* This adds 1/6th of the current error to the integral, but saturates        */
/* it to a limit of INTEG_LIMIT.  This prevents the integral term from growing*/
/* so large that it takes a long time to reduce it once the desired velocity  */
/* is reached.                                                                */
/******************************************************************************/

void control_velocity(ROBOTWHEEL *w)
{
   update_servo = 0;                                       // wait until next interval to do this again

   if (w->enc_read)
   {
      w->enc_read = FALSE;                                 // we are using up the current velocity measurement
      w->v_p_err = (w->req_speed - w->enc_speed) * FIXED_POINT_FACTOR;// calculate proportional term; scale by 16 for fixed-point calculations
      w->v_i_err = sat16(w->v_i_err + w->v_p_err * KiV, VEL_INTEG_LIMIT);// calculate integral term, with saturation to prevent windup
      w->v_d_err = (w->v_prev_err - w->v_p_err) * KdV;
      w->v_prev_err = w->v_p_err;
      w->v_p_err *= KpV;                                      // scale proportional error now
      w->v_f_err = w->req_speed * KfV * FIXED_POINT_FACTOR;   // calculate velocity feed forward term

      w->v_err = w->v_p_err + w->v_i_err + w->v_d_err + w->v_f_err;// sum total error
      w->v_err = w->v_err / FIXED_POINT_FACTOR;

      control_motor(w);
   }
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
void set_position(ROBOTWHEEL *w, signed int16 pos, signed long max_velocity, short delta)
{
   w->pos_reached = FALSE;
   if (delta)
      w->req_pos += pos;
   else
      w->req_pos = pos;
   set_velocity(w, max_velocity);                          // do this to calculate goal max vel
   w->max_vel = w->req_speed;

/*
  v = dx / dt     - velocity is change in distance per change in time
  a = dv / dt     - acceleration is change in velocity per change in time
  vn+1 = vn + dv  - next velocity increment is previous one plus change in velocity
  dv = a * dt     - change in velocity is same as acceleration time change in time
*/

   w->accel_counter = (w->req_speed - w->enc_speed) / DV;  // calculate the number of steps to accelerate in
   if (bit_test(w->accel_counter, 15))
      w->accel_counter = -w->accel_counter;                // make sure it is a positive value

   if (!w->accel_counter)                                  // less than one DV change, so just go in one jump
   {
      w->vel_incr = 0;
      w->cur_max_vel = w->req_speed;
   }
   else
   {
      if (w->req_speed < w->enc_speed)
         w->vel_incr = -DV;
      else
         w->vel_incr = DV;
      w->cur_max_vel = w->enc_speed + w->vel_incr;
   }
   w->p_i_err = 0;                                         // reset integral so old value does not hurt us
   w->p_prev_err = 0;                                      // reset differential previous error value
}


/******************************************************************************/
/* Print Position Control Values                                              */
/*                                                                            */
/******************************************************************************/
void print_pos_control(ROBOTWHEEL *w)
{
#ifdef DEBUG_STAT
   printf("%cP:" , (w == lw) ? 'L' : 'R');
   printf("t %ld p %ld i %ld d %ld", w->p_err, w->p_p_err, w->p_i_err, w->p_d_err);
   printf(" req %ld enc %ld out %ld dir %ld\r\n", w->req_pos, w->enc_pos, w->v_out, (int16)w->v_dir);
#endif
}



/******************************************************************************/
/* Control Position                                                           */
/*                                                                            */
/* This routine calculates the position PI control loop.  It then uses        */
/* the resulting velocity values to set the inner velocity control loop.      */
/* The constant '150' is empirically found for a given system.  In effect,    */
/* it sets how quickly the robot decelerates as it gets close to the desired  */
/* position.                                                                  */
/* Since req_pos_cur_max (the current allowed maximum velocity) is in tenths  */
/* per second, and value is the error term in units of position in encoder    */
/* ticks, the constant 150 is in units of encoder ticks too.  Thus, the       */
/* value returned is velocity in tenths of an inch per second.                */
/******************************************************************************/
void control_position(ROBOTWHEEL *w)
{
   signed int16 spd;

   calc_enc_speeds(w);
   if (w->enc_read)
   {
      w->p_p_err = sat16(w->req_pos - w->enc_pos, MAX_POS_ERR);// calculate current error in position
      w->p_p_err *= KpP * FIXED_POINT_FACTOR;                 // scale up by 16 to allow us to use fractional constants

      w->p_i_err += w->p_p_err * KiP;                         // integrate the error to ensure we reach the destination despite various frictional loads
      w->p_i_err = sat16(w->p_i_err, POS_INTEG_LIMIT);        // saturate the integrator to prevent windup / overshoot / instability

      w->p_d_err = (w->p_prev_err - w->p_p_err) * KdP;
      w->p_prev_err = w->p_p_err;

      w->p_err = w->p_p_err + w->p_i_err + w->p_d_err;

      spd = (signed int16)(((signed int32)w->cur_max_vel * w->p_err) / (signed int32)KPDEN);// convert the total position error term to desired velocity

      if (!w->pos_reached)                                       // check to see if we are close enough
      {
         if (abs(w->p_p_err) < PpTh)                          // allow up to 1 stripe in error (too small, and the system is unstable because we can't stop that accurately)
         {
            if (abs(spd) < PvTh)                              // also make sure we've slowed down enough
               w->pos_reached = TRUE;
         }
         if (w->accel_counter)                                // have we finished accelerating?
         {
            w->accel_counter--;
            w->cur_max_vel += w->vel_incr;                    // not yet -- bump up the speed
         }
         else
            w->cur_max_vel = w->max_vel;                      // we've reached the speed limit
      }

#ifdef NESTED_LOOPS
      set_velocity(w, spd);                                   // request our calculated speed
      control_velocity(w);                                    // use inner velocity control loop
#else
      w->enc_read = FALSE;                                    // this needs to be reset, since control_velocity() normally does it
      w->v_err = spd * KPOV;                                  // scale to provide an equivalent velocity error
      control_motor(w);                                       // then feed that straight to the motor
#endif
   }
}


/******************************************************************************/
/* Stop                                                                       */
/*                                                                            */
/******************************************************************************/
void stop(void)
{
   set_velocity(rw, 0);
   set_velocity(lw, 0);
   rwheel.v_out = 0;
   lwheel.v_out = 0;
   set_motor_speed_dir(rw);
   set_motor_speed_dir(lw);
#ifdef RC_SERVO_MODE
  servos_on = FALSE;
  update_pwm = FALSE;
#else
   update_pwm = TRUE;
#endif
}


/******************************************************************************/
/* Run to Position                                                            */
/*                                                                            */
/* Keep executing the control loops until the desired position is reached.    */
/******************************************************************************/
void run_to_position(void)
{
   seconds_elapsed = 0;
   while (!lw->pos_reached || !rw->pos_reached)
   {
      if (update_servo)
      {
         control_position(lw);
         control_position(rw);
      }
      if (seconds_elapsed > 30)
      {
         print_pos_control(lw);
         print_pos_control(rw);
         seconds_elapsed = 0;
      }
   }
   stop();
}


/******************************************************************************/
/* Wait Seconds                                                               */
/*                                                                            */
/* This uses timer2's interrupt handler to delay for a while.                 */
/******************************************************************************/
void wait_seconds(long int seconds)
{
   seconds_elapsed = 0;
   seconds *= 24;
   while (seconds_elapsed < seconds)
   {
   }
}


/******************************************************************************/
/* Encoder Test                                                               */
/*                                                                            */
/* Turn the wheels and watch the outputs.  Make sure the values make sense -- */
/* that the sign is positive for forward rotation, for example.               */
/******************************************************************************/
void encoder_activity_test(void)
{
   stop();
   printf("Encoder Test\r\n");
   printf("hit enter to stop\r\n");
   for (;;)
   {
      if (rwheel.enc_read || lwheel.enc_read)
      {
         if (rwheel.enc_read)
         {
            calc_enc_speeds(rw);
            rwheel.enc_read = FALSE;
         }
         if (lwheel.enc_read)
         {
            calc_enc_speeds(lw);
            lwheel.enc_read = FALSE;
         }
         print_enc_speeds();
      }
      if (kbhit())
      {
         print_enc_speeds();
         if (getc() == '\r')
            break;
      }
   }
}


/******************************************************************************/
/* Motor Test                                                                 */
/*                                                                            */
/* This steps the motors through moving forward and reverse.                  */
/******************************************************************************/
void motor_action_test(void)
{
   printf("Motor Test\r\n");

   printf("hit key to turn left wheel forward: ");
   while (!kbhit());
   getc();
   printf("\r\n");
   rwheel.v_out = 0;
   rwheel.v_dir = TRUE;
   lwheel.v_out = 255;
   lwheel.v_dir = TRUE;
   update_pwm = TRUE;

   printf("hit key to turn right wheel forward: ");
   while (!kbhit());
   getc();
   printf("\r\n");
   lwheel.v_out = 0;
   lwheel.v_dir = TRUE;
   rwheel.v_out = 255;
   rwheel.v_dir = TRUE;
   update_pwm = TRUE;

   printf("hit key to turn left wheel backward: ");
   while (!kbhit());
   getc();
   printf("\r\n");
   rwheel.v_out = 0;
   rwheel.v_dir = FALSE;
   lwheel.v_out = 255;
   lwheel.v_dir = FALSE;
   update_pwm = TRUE;

   printf("hit key to turn right wheel backward: ");
   while (!kbhit());
   getc();
   printf("\r\n");
   lwheel.v_out = 0;
   lwheel.v_dir = FALSE;
   rwheel.v_out = 255;
   rwheel.v_dir = FALSE;
   update_pwm = TRUE;

   printf("hit key to stop: ");
   while (!kbhit());
   getc();
   printf("\r\nDone.\r\n");
   stop();
}

/******************************************************************************/
/* Speed Control Test                                                         */
/*                                                                            */
/* This ramps the speed of each wheel down to zero from MAX_SPEED forward, to */
/* MAX_SPEED in reverse.  The robot should go in a straight line.             */
/******************************************************************************/
void zero_servos()
{
   int cmd;
   
// zero servos
   printf("Zeroing Servos\r\n");
   printf("adjust potentiometer in each servo to stop them from turning\r\n");
   printf("if servo neutral positions are not adjustable (e.g., Parallax servos),\r\n");
   printf("then adjust the pulse widths using the keyboard, then modify and\r\n");
   printf("recompile the program to use these new values.\r\n");
   printf("Left Wheel: A=inc S=dec; Right Wheel: K=inc L=dec; Enter=Done.\r\n");
   stop();                                                 // set to zero speed
   servos_on = TRUE;                                       // force servo output on
   update_pwm = TRUE;                                      // force output to them
   for(;;)
   {
      cmd = getc();
      switch(toupper(cmd))
      {
         case A:
            break;
         case S:
            break;
         case K:
            break;
         case L:
            break;
      }
      if (cmd == 0x0d)
         break;
   }
   printf("\r\nDone.\r\n"
}

/******************************************************************************/
/* Speed Control Test                                                         */
/*                                                                            */
/* This ramps the speed of each wheel down to zero from MAX_SPEED forward, to */
/* MAX_SPEED in reverse.  The robot should go in a straight line.             */
/******************************************************************************/
void speed_control_test()
{
   signed long spd_R, spd_L;
   int i;

   printf("Speed ramp test\r\n");
   spd_L = spd_R = MAX_SPEED;
   set_velocity(rw, spd_R);
   set_velocity(lw, spd_L);
   seconds_elapsed = 0;
   for (i = 0; i < ((MAX_SPEED * 2) / 5); )
   {
      if (update_servo) // NOTE: your code will need to do this in its main loop
      {
         calc_enc_speeds(rw);
         calc_enc_speeds(lw);
         control_velocity(rw);
         control_velocity(lw);
      }
#ifdef DEBUG_STAT
      if (!(seconds_elapsed & 0x1f))                        // print debug information
      {
         print_enc_speeds();
         print_vel_control(lw);
         print_vel_control(rw);
      }
#endif
      if (seconds_elapsed > 120)                            // adjust speeds
      {
         print_enc_speeds();
         print_vel_control(lw);
         print_vel_control(rw);
         seconds_elapsed = 0;
         spd_R -= 5;
         spd_L -= 5;
         if (spd_R < -MAX_SPEED)
            spd_R = spd_L = MAX_SPEED;
         set_velocity(rw, spd_R);
         set_velocity(lw, spd_L);
#ifdef DEBUG_STAT
         printf("--> Requested SpdR = %ld; SpdL = %ld\r\n", spd_R, spd_L);
#endif
         i++;
      }
   }
   printf("Done.\r\n");
   stop();
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

   printf("Position test\r\n");

   rwheel.enc_pos = 0;
   lwheel.enc_pos = 0;
   for (i = 0; i < 16; i++)
   {
      printf("Fwd 1 foot\r\n");
      output_bit(RED_LED, 1);
      output_bit(YELLOW_LED, 1);

      set_position(rw, 178, MAX_SPEED, TRUE);
      set_position(lw, 178, MAX_SPEED, TRUE);              // both forward 12"
      run_to_position();

      output_bit(RED_LED, 0);
      wait_seconds(2);
      printf("Turning 90CC\r\n");
      output_bit(RED_LED, 1);
      output_bit(YELLOW_LED, 0);


      set_position(rw, 41, MAX_SPEED/2, TRUE);
      set_position(lw, -41, MAX_SPEED/2, TRUE);            // rotate counter clockwise 90 degrees
      run_to_position();

      output_bit(RED_LED, 0);
      wait_seconds(2);
   }
   printf("Done.\r\n");
   stop();
}

/******************************************************************************/
/* Main Program Entry Point                                                   */
/*                                                                            */
/******************************************************************************/

#zero_ram                                                  // don't need to bother to set variables to zero initially; this does it for us
void main()
{
#byte port_d = 8

   setup();


#ifdef ENCODER_TEST
   encoder_activity_test();
#endif

#ifdef MOTOR_TEST
   motor_action_test();
#endif

#ifdef ZERO_SERVOS
   zero_servos();
#endif

#ifdef SPEED_TEST
   speed_control_test();
#endif

#ifdef POSITION_TEST
   position_control_test();
#endif

   stop();
   for (;;);
}


