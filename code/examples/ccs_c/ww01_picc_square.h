//-----------------------------------------
// define target processor and resources
//-----------------------------------------

#include <16F877.h>
#device ADC=8
#device *=16
//#DEVICE CCSICD=TRUE
#use delay(clock=20000000)
#use rs232(baud=38400,parity=n,bits=8,xmit=PIN_C6,rcv=PIN_C7)
#fuses HS,NOWDT,PUT,NOLVP,NOBROWNOUT


//-----------------------------------------
// define software structures and constants
//-----------------------------------------

// constants
#define CONTROL_LOOP_COUNT 200                             // number of timer2 overflows per control loop period (200 = 24.4Hz, because overflows at 4883 Hz)
#define LEFT_MOTOR  TRUE
#define RIGHT_MOTOR FALSE
#define FWD TRUE
#define REV FALSE
#define MAX_SPEED ((signed int16)63)                       // maximum velocity in tenths of an inch per second (depends on the servo brand and other factors)
#define INTEG_LIMIT MAX_SPEED                              // maximum error value that we allow the velocity integral term to grow to to prevent windup
#define VEL_INTEG_LIMIT ((signed int16)1000)               // maximum integral for velocity control to prevent windup
#define POS_INTEG_LIMIT ((signed int16)1000)                // maximum integral for position control to prevent windup
#define MAX_POS_ERR ((signed int16)128)                    // limit position error to keep calculations within 16 bits
#define CENTER_PERIOD ((signed int16)3760)                 // value to write to CCP register to output 1.5ms pulses


/*
   NO_MOTION_LIMIT should be set to the number of control periods needed
   so that the measured period >= (Ktps + 1), which results in enc_speed = 0.
   The control period is the 24.4 Hz = 41ms.
   If Cwh = 8.2", Ktps = 3295, so measured period = 3296.  3296 * 204.8 us = 0.675
   seconds, which is the same as 0.675 / (41ms) = 16.46.  So, if we go more than
   16.46 control periods without a tick, we are below the limit of measurability.
   Note that because Ktps is proportional to wheel circumference, this lower
   limit on speed is really platform-independent, and depends on the scale factor
   for the measurement of the wheel circumference.  If the scale is 0.1", then
   the minimum velocity is 0.1" / sec = 6"/minute.
*/
#define NO_MOTION_LIMIT 16                                 // number of control loops before we declare zero velocity (see note above)


// calculate the speed conversion factor
#define Dwh 2.75                                           // wheel diameter
#define PI 3.14159
#define Cwh (Dwh * PI)                                     // wheel circumference
#define TSCALE 10                                          // convert to 10ths of an inch
#define INVTICK ((signed int16)4883)                       // this is 1 / 204.8 us, to avoid using floating point math
#define NCLKS 128                                          // number of encoder clocks per wheel rotation
#define Ktps ((Cwh * TSCALE * INVTICK) / NCLKS)            // 3295

// PID tuning constants
#define FIXED_POINT_FACTOR ((signed int16)16)
#define KpV ((signed int16)8)
#define KiV ((signed int16)1)
#define KdV ((signed int16)1)
#define KfV ((signed int16)12)
#define KoVN ((signed int16)1)
#define KoVD ((signed int16)9)
#define DV ((signed int16)6)                               // delta V = acceleration step; amount to change velocity per servo control loop
#define KpP ((signed int16)4)
#define KiP ((signed int16)2)
#define KdP ((signed int16)1)
#define KoP ((signed int16)1)
#define PpTh (FIXED_POINT_FACTOR * KpP * 4)
#define PvTh ((signed int16)10)
#define KPOV ((signed int16)37)
#define KPDEN ((signed int16)8192)
#define MIN_OUT ((signed int16)140) // minimum PWM value to cause motor to rotate


// control data for a single wheel
typedef struct robot_wheel
{
// RC servo variables
#ifdef RC_SERVO_MODE
   short servo_high;
   long servo_position;
#endif

// wheel-specific info
   short pos_fwd_pw;                                       // if true, servo pulsewidth is larger than 1.5ms when rotating forward
   short high_fwd_dir;                                     // if true, a high level on the dir line means forward direction

// encoder direction variables
   short enc_fwddir;                                       // most recent value read from the WW-01 DIR line
   short enc_fwddir_prev;                                  // previous value

// encoder position variables
   signed int16 enc_pos;                                   // in encoder ticks, about 0.0675" per tick; max 2212 inches = 184 feet

// velocity measurement variables
   int32 prev_t2;                                          // the value of timer2_ticks the last time we saw a clock pulse from the WW-01
   int index;                                              // current position in the array of periods
   int enc_no_motion_periods;                              // number of sampling intervals during which no encoder ticks have occurred
   short enc_read;                                         // we have received at least one pulse from the encoder
   int enc_ticks;                                          // number of encoder ticks during the last sampling interval
   int32 enc_period_start;
   int32 enc_period_end;
   int32 enc_period;                                       // (end - start) / enc_ticks
   signed int16 enc_speed;                                 // current signed rotation rate, in units of 0.1 inches per second

// velocity PID variables
   signed int16 v_err;                                     // total error
   signed int16 v_p_err;                                   // proportional velocity error
   signed int16 v_i_err;                                   // integral error
   signed int16 v_d_err;                                   // differential error
   signed int16 v_f_err;                                   // velocity feed-forward error
   signed int16 v_prev_err;                                // for calculating differential error
   signed int16 v_out;                                     // motor output value
   short v_dir;                                            // direction line for motor

// velocity control variables
   signed int16 req_speed;                                 // desired or commanded velocity, in tenths of an inch per second

// position control variables
   signed int16 req_pos;                                   // desired or commanded position, in encoder ticks
   signed int16 max_vel;                                   // desired maximum velocity while moving to the new position
   signed int16 cur_max_vel;                               // current maximum
   signed int16 vel_incr;                                  // amount to add to velocity each increment (for initial acceleration)
   signed int16 accel_counter;
   signed int16 p_err;                                     // total error
   signed int16 p_p_err;                                   // proportional position error
   signed int16 p_i_err;                                   // integral error
   signed int16 p_d_err;                                   // differential error
   signed int16 p_prev_err;                                // for calculating differential error
   short pos_reached;                                      // we're done with this move
} ROBOTWHEEL;


//-----------------------------------------
// define hardware
//-----------------------------------------

#define PIN_PWM_R    PIN_C2                                // tied to B1; = CCP1
#define PIN_PWM_L    PIN_C1                                // tied to B2; = CCP2

#define EYE_R        PIN_A2
#define EYE_R_AN     2
#define EYE_L        PIN_A3
#define EYE_L_AN     3

#define MARKIII_BOARD 1                                    // use the MarkIII board constants
#define PROTO_LED_SWITCHES 1

// DEFINE BOARD-VERSION-SPECIFIC CONSTANTS

#use fast_io(A)
#use fast_io(B)
#use fast_io(C)

#ifdef REV1_BOARD

#define RED_LED    PIN_A0                                  // output
#define YELLOW_LED PIN_A1                                  // output

#define ENC_R_CLK    PIN_A4                                // I/O 1, TOCKI
#define ENC_L_CLK    PIN_C0                                // I/O 5, T1CKI

#define ENC_R_DIR    PIN_B3                                // decoded direction
#define ENC_L_DIR    PIN_B2

#define SW_1         PIN_B4
#define SW_2         PIN_B5

#define TRIS_A_VAL   0x3C                                  // A0,A1 outputs, rest are inputs; 1 = input
#define TRIS_B_VAL   0xFF                                  // all inputs
#define TRIS_C_VAL   0xE1                                  // C1,C2,C3,C4 outputs; rest inputs

#endif

#ifdef REV2_BOARD

#define RED_LED PIN_A2                                     // output
#define YELLOW_LED PIN_A5                                  // output

#define ENC_R_CLK    PIN_A4                                // I/O 5, TOCKI
#define ENC_L_CLK    PIN_C0                                // I/O 7, T1CKI

#define ENC_L_DIR PIN_C5                                   // input
#define ENC_R_DIR PIN_B1                                   // input

#define SW_1 PIN_B3                                        // input
#define SW_2 PIN_B2                                        // input

#define TRIS_A_VAL 0x1B                                    // 1 = input; A2 and A5 are outputs
#define TRIS_B_VAL 0xFF                                    // all inputs
#define TRIS_C_VAL 0xE1                                    // C1 = PWM2, C2 = PWM1, C3 = DIR1, C4 = DIR2, all outputs

#endif

#ifdef MARKIII_BOARD

#ifdef RC_SERVO_MODE
#define SERVO_L     PIN_B2                                 // RC servo
#define SERVO_R     PIN_B1
#endif

#define LINE_L      PIN_E2
#define LINE_C      PIN_E1
#define LINE_R      PIN_E0

#define ENC_R_CLK   PIN_A4                                 // exp1: TOCKI
#define ENC_R_DIR   PIN_B4                                 // exp12: input
//#define ENC_R_CHA   PIN_D1
//#define ENC_R_CHB   PIN_D0

#ifdef LEFT_CLK_ON_TIMER_1
#define ENC_L_CLK   PIN_C0                                 // exp25: T1CKI
#else
#define ENC_L_CLK   PIN_B0                                 // exp20: INT0
#endif
#define ENC_L_DIR   PIN_B5                                 // exp10: input
//#define ENC_L_CHA   PIN_D3
//#define ENC_L_CHB   PIN_D2

#ifdef PROTO_LED_SWITCHES
#define RED_LED     PIN_D0                                 // output
#define YELLOW_LED  PIN_D1                                 // output
#define GREEN_LED   PIN_D2                                 // output
#define BLUE_LED    PIN_D3                                 // output
#endif

#define TRIS_A_VAL 0x1B                                    // 1 = input; A2 and A5 are outputs
#ifdef RC_SERVO_MODE
#define TRIS_B_VAL 0xF9                                    // all inputs
#else
#define TRIS_B_VAL 0xFF
#endif
#define TRIS_C_VAL 0xE1                                    // C1 = PWM2, C2 = PWM1, C3 = DIR1, C4 = DIR2, all outputs
#ifdef PROTO_LED_SWITCHES
#define TRIS_D_VAL 0xF0
#else
#define TRIS_D_VAL 0xCF
#endif
#define TRIS_E_VAL 0xFF

#use fast_io(D)
#use fast_io(E)


#endif

#ifdef MARKIII_SENSOR_BOARD_HBRIDGE_MODE
#use I2C(master, sda=PIN_C4, scl=PIN_C3, FORCE_HW)
#define PCF8574_ID   0x40
#define RMOT_DIR       0x01                                // R motor direction is controlled by a bit on the parallel port
#define LMOT_DIR       0x02
#else
#define RMOT_DIR     PIN_C3                                // R motor direction is controlled by a pin on the PIC
#define LMOT_DIR     PIN_C4
#endif

