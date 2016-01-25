'******************************************************************************
'* WW-01 BASCOM AVR for Atmel ATMEGA16                                        *
'* Simple Odometry Example Program                                            *
'*                                                                            *
'* Copyright 2004, Noetic Design, Inc.                                        *
'* v 1.0, written by Pete Skeggs                                              *
'*                                                                            *
'* This demonstation program shows off the features                           *
'* of the WheelWatcher.                                                       *
'******************************************************************************
'******************************************************************************
'* TARGET NOTE:                                                               *
'*                                                                            *
'* This example is for use with an ARC 1.1 board from barello.net.  This      *
'* uses the ATMEGA16 processor with a 16MHz resonator.  The following         *
'* fuses are required; use AVR Studio or GNU avrdude to reprogram:            *
'* CKOPT = 1, CKSEL3 = CKSEL2 = CKSEL1 = 1, CKSEL0 = 1, SUT1 = 0, SUT0 = 0    *
'*                                                                            *
'* NOTE: you MUST set the hardware stack size in Bascom's Options -> Compiler *
'* -> Chip >= 40, or else the stack will overflow during execution of INT1 or *
'* INT0 ISRs, due to the call made to get_timer().  The default of 32 is too  *
'* small in this case.                                                        *
'*                                                                            *
'******************************************************************************
'* Wiring:                                                                    *
'*                                                                            *
'* Right WW-01:                                                               *
'* Red (+5V)   - JP10 pin 3 / +5V                                             *
'* Black (Gnd) - JP10 pin 16 / GND                                            *
'* Orange (DIR)- JP3 pin 18 / PC0                                             *
'* Purple (CLK)- RIGHT motor connector pin 6 / INT0 / PD2                     *
'*                                                                            *
'* Left WW-01:                                                                *
'* Red (+5V)   - JP3 pin 9 / +5V                                              *
'* Black (Gnd) - JP3 pin 10 / GND                                             *
'* Orange (DIR)- JP3 pin 17 / PC1                                             *
'* Purple (CLK)- LEFT motor connector pin 6 / INT1 / PD3                      *
'*                                                                            *
'******************************************************************************
'* The following note is just about all that remains of the original demo.c   *
'* that came with WinAVR.  We are retaining it as required, though little     *
'* remains of the original code.                                              *
'* ---------------------------------------                                    *
'* "THE BEER-WARE LICENSE" (Revision 42):                                     *
'* <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you*
'* can do whatever you want with this stuff. If we meet some day, and you     *
'* think this stuff is worth it, you can buy me a beer in return. Joerg Wunsch*
'* ---------------------------------------                                    *
'******************************************************************************

$regfile = "M16def.dat"
$crystal = 16000000
$baud = 19200


' timer variables
Defword Timer_ticks                                         ' in multiples of 128us; rolls over every 10 seconds
Deflng Seconds
Seconds = 0
Defbyte Seconds_fraction
Seconds_fraction = 0
Defbyte Seconds_decimal
Seconds_decimal = 0
Defbyte Pcount
Pcount = 0
Defword Tmrr
Tmrr = 0
Defword Tmrl
Tmrl = 0

' encoder variables
Defbit Enc_fwdir_r                                          ' most recent value read from the right WW-01's DIR line
Defbit Enc_fwdir_l

' position variables
Deflng Enc_pos_r
Deflng Enc_pos_l

' velocity control variables
Defword Out_r
Out_r = 0
Defword Out_l
Out_l = 0
Defbyte Direction_r
Defbyte Direction_r

' velocity measurement variables
Defword Enc_period_l_prev
Defword Enc_period_r_prev
Defword Enc_period_l
Defword Enc_period_r
Defint Enc_speed_r
Defint Enc_speed_l

' program state flags
Defbit Do_print
Do_print = 0
Defbyte Reset_source
Defword Temp
Temp = 0

' constants
Const Max_servo = 500
Const Min_servo = 250
Const Mid_servo = 375
Const False = 0
Const True = 1

' hardware constants
Rservo Alias Portc.5                                        ' right servo control line on ARC 1.1 board
Lservo Alias Portc.2                                        ' left servo control line on ARC 1.1 board
'Rdir Alias Portc.0                                          ' right WW-01 DIR signal
'Ldir Alias Portc.1                                          ' left WW-01 DIR signal
Prog_led Alias Portb.4

Const Up = 0
Const Down = 1
Const Steady = 2

' calculate the speed conversion factors
Const Dwh = 2.75                                            ' wheel diameter
Const Pi = 3.14159
Const Cwh =(dwh * Pi)                                       ' wheel circumference
Const Tscale = 60                                           ' seconds per minute
Const Invtick = 7812                                        ' this is 1 / 128 us, to avoid using floating point math
Const Nclks = 128                                           ' number of encoder clocks per wheel rotation
Const Kips =((cwh * Invtick) / Nclks)                       ' inches per second (IPS) = Kips / PER = 527 / PER
Const Krpm =((tscale * Invtick) / Nclks)                    ' revolutions per minute (RPM) = Krpm / PER = 3662 / PER

' function prototypes
Declare Function Get_timer_ticks() As Word
Declare Sub Setup()
Declare Sub Dump_reset()
Declare Sub Calc_enc_speeds()
Declare Sub Print_enc_speeds()

'******************************************************************************
'* Main Program Entry Point                                                   *
'*                                                                            *
'******************************************************************************
Main:
   Print "Atmel ATMEGA16 / ARC 1.1 WW-01 Monitor Example"
   Setup
   Dump_reset

   '* loop forever, the interrupts are doing the rest *

   Do
      If Do_print = 1 Then                                  ' only print this 10 times a second
         Do_print = 0
         Reset Prog_led
         Calc_enc_speeds
         Seconds_decimal = Seconds_fraction * 2
         Print Seconds ; "." ; Seconds_decimal;             ' display current run time
         Print " ldist " ; Enc_pos_l ; ", rdist " ; Enc_pos_r;       ' display current position
         Print ", lspd " ; Enc_speed_l ; ", rspd " ; Enc_speed_r;       ' display current speed
         Print ", out_L " ; Out_l ; ", out_R " ; Out_r;     ' display current servo control pulse values
         Print ", lper " ; Enc_period_l ; ", rper " ; Enc_period_r;       ' display the measured periods
         Print ", Ldir " ; Enc_fwdir_l ; ", Rdir " ; Enc_fwdir_r;       ' display direction lines from WW-01
         Temp = Get_timer_ticks()
         Print ", ticks " ; Temp                            ' display current timer tick (used to measure period of CLK lines)
         Set Prog_led
      End If
   Loop
End

'******************************************************************************
'*  Setup the Hardware                                                        *
'******************************************************************************
Sub Setup()

   '* as early as possible, grab the current reason for being reset and then clear it *
   Reset_source = Mcucsr                                    ' find out why we were reset
   Mcucsr = $1f                                             ' clear reset source

   '***********************
   ' set the direction
   ' control info for the
   ' I/O ports
   '***********************
   Config Porta = Input
   Config Portb = Input
   Config Pinb.4 = Output                                   ' Program LED
   Config Portc = Input
   Config Pinc.5 = Output                                   ' right servo
   Config Pinc.2 = Output                                   ' left servo
   Config Portd = Input
   Config Pind.5 = Output                                   ' OC1A and OC1B as output (you could hook the servos here instead)
   Config Pind.4 = Output


   '***********************
   ' set up WW-01 interface
   '***********************
   '* enable interrupts on timer 1 overflow and both output compares *
   Enable Ovf1
   On Ovf1 Overflow1_isr
   Enable Oc1a
   On Oc1a Oc1a_isr
   Enable Oc1b
   On Oc1b Oc1b_isr

   '* enable external interrupts 0 and 1 for falling edge triggering *
   Config Int0 = Falling
   Config Int1 = Falling
   Enable Int0
   On Int0 Rclk_isr
   Enable Int1
   On Int1 Lclk_isr

   '***********************
   ' set up hardware-based
   ' servo control pulse
   ' generation; replace
   ' this with code to
   ' drive the H bridges if
   ' you use DC motors
   ' instead of servos
   ' HOWEVER, you would still
   ' need to make timer_ticks
   ' work as a 16 bit 128 us
   ' resolution timer for
   ' measuring wheel speed.
   '***********************
   '* tmr1 is PWM with TOP set by ICR1, OC1A and OC1B output waveforms in fast PWM mode *
   Tccr1a = 0
   Tccr1a.com1a1 = 1
   Tccr1a.com1b1 = 1
   Tccr1a.wgm11 = 1

   '* tmr1 running on fosc/64 = 250,000 Hz clock *
   Tccr1b = 0
   Tccr1b.wgm13 = 1
   Tccr1b.wgm12 = 1
   Tccr1b.cs11 = 1
   Tccr1b.cs10 = 1

   '* set PWM frequency to 50Hz *
   Capture1 = 5000

   '* set duty cycle such that 1.5 ms pulses are generated; 1 ms = 250, 1.5ms = 375, 2ms = 500 *
   Out_r = 500
   Out_l = 250                                              ' 375;
   Pwm1a = Out_r
   Pwm1b = Out_l
   Direction_r = Down
   Direction_l = Up

  'finally we must turn on the global interrupt

   Enable Interrupts

End Sub


'******************************************************************************
'* RCLK / INTERRUPT0 ISR                                                      *
'*                                                                            *
'* This interrupt service routine is called on each pulse of the              *
'* right WW-01 CLK line.  We grab the current time stamp, adjust              *
'* the current position based on the RDIR pin, then calculate a               *
'* new encoder clock period using the new time stamp.  This is done           *
'* by subtracting the previous time stamp from the current to get             *
'* the current period, then using that and the last period value              *
'* to calculate a new one using a running average algorithm.                  *
'******************************************************************************
Rclk_isr:
   Tmrr = Get_timer_ticks()
   If Pinc.0 = 1 Then
      Enc_fwdir_r = True
      Incr Enc_pos_r
   Else
      Enc_fwdir_r = False
      Decr Enc_pos_r
   End If
   ' this calculates a running average to filter alignment noise
   Enc_period_r = Enc_period_r * 3
   Enc_period_r = Enc_period_r + Tmrr
   Enc_period_r = Enc_period_r - Enc_period_r_prev
   Shift Enc_period_r , Right , 2
   Enc_period_r_prev = Tmrr
Return


'******************************************************************************
'* LCLK / INTERRUPT1 ISR                                                      *
'*                                                                            *
'* This interrupt service routine is called on each pulse of the              *
'* left  WW-01 CLK line.  We grab the current time stamp, adjust              *
'* the current position based on the LDIR pin, then calculate a               *
'* new encoder clock period using the new time stamp.  This is done           *
'* by subtracting the previous time stamp from the current to get             *
'* the current period, then using that and the last period value              *
'* to calculate a new one using a running average algorithm.                  *
'******************************************************************************
Lclk_isr:
   Tmrl = Get_timer_ticks()
   If Pinc.1 = 1 Then
      Enc_fwdir_l = False
      Decr Enc_pos_l
   Else
      Enc_fwdir_l = True
      Incr Enc_pos_l
   End If
   ' this calculates a running average to filter alignment noise
   Enc_period_l = Enc_period_l * 3
   Enc_period_l = Enc_period_l + Tmrl
   Enc_period_l = Enc_period_l - Enc_period_l_prev
   Shift Enc_period_l , Right , 2                           ' divide by 4
   Enc_period_l_prev = Tmrl
Return


'******************************************************************************
'******************************************************************************
'*                                                                            *
'* SERVO SUPPORT CODE                                                         *
'*                                                                            *
'******************************************************************************
'******************************************************************************

'******************************************************************************
'* OUTPUT COMPARE 1A ISR                                                      *
'*                                                                            *
'* This interrupt occurs each time the TCNT1 value matches OCR1A.  We use this*
'* routine to lower the RSERVO pin on the ARC board.  This is needed because  *
'* the ARC board connects the right servo control signal to PC5 instead of the*
'* OC1A / PD5 signal which is automatically output by the hardware.           *
'******************************************************************************
Oc1a_isr:
   Rservo = 0
Return

'******************************************************************************
'* OUTPUT COMPARE 1B ISR                                                      *
'*                                                                            *
'* This interrupt occurs each time the TCNT1 value matches OCR1B.  We use this*
'* routine to lower the LSERVO pin on the ARC board.  This is needed because  *
'* the ARC board connects the left  servo control signal to PC2 instead of the*
'* OC1B / PD4 signal which is automatically output by the hardware.           *
'******************************************************************************
Oc1b_isr:
   Lservo = 0
Return

'******************************************************************************
'* TIMER 1 OVERFLOW ISR                                                       *
'*                                                                            *
'* This interrupt routine is called at the end of every 20ms servo control    *
'* pulse period, which is 5000 ticks of TCNT1 with a prescale value of 64.    *
'* Here is where the RSERVO (PC5) and LSERVO (PC2) lines are manually dropped,*
'* a new control pulse value is calculated and written to the corresponding   *
'* OCR1 register, and we also do some timekeeping work for use in measuring   *
'* the encoder clock period.                                                  *
'******************************************************************************
Overflow1_isr:

   '******************************
   ' take the servo control pulses
   ' high at the start of the
   ' 20ms period
   '******************************
   Rservo = 1
   Lservo = 1

   '******************************
   ' generate a new value for the
   ' control pulse lengths for
   ' fun -- this is just a demo
   '******************************
   Select Case Direction_r
      Case Up:
         Incr Out_r : If Out_r >= Max_servo Then : Direction_r = Down : End If
      Case Down:
         Decr Out_r : If Out_r <= Min_servo Then : Direction_r = Up : End If
   End Select
   Select Case Direction_l
      Case Up:
         Incr Out_l : If Out_l >= Max_servo Then : Direction_l = Down : End If
      Case Down:
         Decr Out_l : If Out_l <= Min_servo Then : Direction_l = Up : End If
   End Select
   Pwm1a = Out_r
   Pwm1b = Out_l

   '******************************
   ' take care of timers needed
   ' to display WW-01 information
   ' periodically
   '******************************
   Incr Seconds_fraction
   If Seconds_fraction = 50 Then
      Seconds_fraction = 0
      Seconds = Seconds + 1
   End If
   Incr Pcount
   If Pcount = 10 Then
      Pcount = 0
      Do_print = 1
   End If

   '******************************
   ' CRITICAL: generate running
   ' 16 bit, 128us resolution
   ' timer value for use in
   ' measuring wheel speed with
   ' the WW-01s
   '******************************
   Timer_ticks = Timer_ticks + 156                          ' this is the max count of 5000 / 32; unit of time is 128us
Return

'******************************************************************************
'******************************************************************************
'*                                                                            *
'* SUPPORT ROUTINES FOR WW-01                                                 *
'*                                                                            *
'******************************************************************************
'******************************************************************************

'******************************************************************************
'* Get Timer Ticks                                                            *
'*                                                                            *
'* This routine uses the overflow value calculated during the                 *
'* overflow ISR as well as the current TCNT1 value to calculate               *
'* a new time stamp for use in measuring an encoder period.                   *
'* One timer_tick unit is equivalent to 128us, and so it over-                *
'* flows every 10 seconds.  This allows us to measure the                     *
'* speed of a running servo very accurately, and measure it                   *
'* over the range of a maximum of 3600 RPM(!) and a minimum                   *
'* of 0.05 RPM.                                                               *
'******************************************************************************
Function Get_timer_ticks() As Word
   Local Tmp As Word

   Tmp = Tcnt1
   Shift Tmp , Right , 5
   Get_timer_ticks = Timer_ticks + Tmp                      ' add current count to overflow
End Function

'******************************************************************************
'* Calc Encoder Speeds                                                        *
'*                                                                            *
'* speed in RPM = (1 / NCLKS) * TSCALE / (TICK * PER),  where NCLKS = 128     *
'* (32 stripe disk) per rotation, TSCALE = 60 seconds per minute, TICK =128us *
'* per timer tick, and PER is the measured period in timer ticks;             *
'* so RPMs = 3662 / PER.                                                      *
'******************************************************************************
Sub Calc_enc_speeds()
   Enc_speed_r = Krpm / Enc_period_r
   If Enc_fwdir_r = 0 Then
      Enc_speed_r = -enc_speed_r
   End If

   Enc_speed_l = Krpm / Enc_period_l
   If Enc_fwdir_l = 1 Then
      Enc_speed_l = -enc_speed_l
   End If
End Sub

'******************************************************************************
'* Print Encoder Speeds                                                       *
'******************************************************************************
Sub Print_enc_speeds()
   Print "EncSpdR " ; Enc_speed_r ; " RFwd " ; Enc_fwdir_r;
   Print "EncSpdL " ; Enc_speed_l ; " LFwd " ; Enc_fwdir_l
End Sub



'******************************************************************************
'* Print Reset Source                                                         *
'*                                                                            *
'* This Dumps Strings Over The Serial Port To Indicate Why We Recently        *
'* Reset.                                                                     *
'******************************************************************************
Sub Dump_reset()
   Print "Reset Source:"
   If Reset_source.4 = 1 Then
      Print "JTAG Reset"
   End If
   If Reset_source.wdrf = 1 Then
      Print "Watchdog Reset"
   End If
   If Reset_source.borf = 1 Then
      Print "Brownout Reset"
   End If
   If Reset_source.extrf = 1 Then
      Print "External Reset"
   End If
   If Reset_source.porf = 1 Then
      Print "Power-on Reset"
   End If
End Sub