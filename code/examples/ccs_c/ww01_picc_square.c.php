<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filenames[0] = "ww01_picc_square.c";
	$filedescs[0] = "C source";
	$filedisps[0] = 1;
	$filenames[1] = "ww01_picc_square.h";
	$filedescs[1] = "header file";
	$filedisps[1] = 1;
	$filenames[2] = "ww01_picc_square.pjt";
	$filedescs[2] = "CCS project file";
	$filedisps[2] = 0;
	$filenames[3] = "ww01_picc_square.hex";
	$filedescs[3] = "Intel Hex file";
	$filedisps[3] = 0;
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
PIC 16F877 Using CCS C</i></b><br>
Advanced Velocity and Position Control<br>
For WW-01 and WW-02<br>
<b><i><font color="#FF0000">New Version 11/11/05</font></i></b> <br>
<!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This example is quite complex, 
as it demonstrates many advanced control loop techniques. It has two control loops: 
a velocity loop and a position loop. It includes code to drive RC servos in the 
background using the two CCP hardware units. Alteratively, it can drive motors 
using PWM sign/magitude control with external H bridges, either directly wired 
to the PIC or on a board like the MarkIII sensor board. It measures the actual 
velocity of each wheel by measuring the time between pulses on the CLK line on 
the WheelWatchers.<br>
<br>
This new version of the example is a complete rewrite of many parts. We changed 
the design to store each wheel's variables (PID values, states, etc.) in a ROBOTWHEEL 
data structure, which is then passed to the various set_() and control_() routines. 
This reuses the code and makes it easier to read. Unfortunately, it also makes 
the code larger, due to the poor ability of a PIC to handle pointers to structures.<br>
<br>
This example applies to both the WW-01 (for servos) as well as the WW-02 (for 
Solarbotics gearhead motors). In the case of the WW-02, comment out the definition 
of RC_SERVO_MODE to put it in H bridge mode. It assumes a sign-magnitude style 
H bridge with the PWM inputs wired to CCP1 and CCP2 on the PIC, and the direction 
lines wired to pin C3 and Pin C4.<br>
<br>
The velocity loop is used by calling set_velocity(wheel, speed), where wheel points 
to the wheel data structure for the wheel to change. Then, in a loop (usually 
your main loop, where you also check sensors and change behaviors), whenever the 
global flag update_servo is non-zero, call calc_enc_speeds(wheel) for each wheel, 
and then control_velocity(wheel) for each wheel.<br>
<br>
The position loop is used by calling set_position(wheel, position, max_velocity, 
delta), where position is the goal absolute position or relative position (depending 
on delta), and max_velocity indicates the maximum allowed speed during the move. 
Then, in your main loop, you can either call run_to_position(), which blocks until 
the move is complete, or do it yourself. You do it yourself by checking update_servo. 
When non-zero, call control_position(wheel) for each wheel. When the pos_reached 
flag within each wheel's wheel structure is non-zero, the motion is complete.<br>
<br>
The position loop by default directly generates motor control values, which are 
PWM values either deviating from the servo control pulse center value (1.5ms) 
or the actual PWM duty cycle when driving DC motors. Alternatively, by defining 
the symbol NESTED_LOOPS, this example will feed the position loop's velocity request 
to set_velocity(), and then calls control_velocity(). This tends to be unstable 
for short moves and low speeds.<br>
<br>
There are 4 separate tests that can be enabled or disabled by commenting out the 
symbol that enables it:<br>
<ul>
<li>#define ENCODER_TEST 1 // use this to verify that your encoders are working</li>
<li>#define MOTOR_TEST 1 // use this to verify that your motors or servos are 
wired right, and spin the right way</li>
<li>#define SPEED_TEST 1 // use this to test velocity control</li>
<li>#define POSITION_TEST 1 // use this to test position control</li>
</ul>
When you start to debug this example on your robot, it's a good idea to enable 
all tests, to ensure that the encoders and motors are wired correctly and working. 
You may need to change the values of each wheel's high_fwd_dir and pos_fwd_pw 
flags (at the start of setup()) to correct for problems you might find using the 
encoder and motor tests. The flag high_fwd_dir adjusts, for each wheel, the direction 
of rotation of that wheel in order to see positive vs. negative position values 
from the encoders. The flag pos_fwd_pw is used to control the direction of rotation 
of a given wheel's motor or servo; change this if one or more of the wheels spin 
the wrong way during the motor test.<br>
<br>
Values most likely to be changed are in the header file:<br>
<ul>
<li>MIN_OUT - the minimum PWM value to cause the motor or servo to rotate</li>
<li>MAX_SPEED - the maximum effective speed of the robot, in tenths of an inch 
per second</li>
<li>DV - delta V, the amount to change velocity per servo control loop</li>
<li>KoVN / KoVD - the numerator and denominator used for scaling the velocity 
error term to produce a motor control value ranging from approximately -255 to 
+255</li>
<li>KPDEN - the scale factor for converting the position control loop's error 
value to a goal speed</li>
<li>KPOV - the scale factor for converting the position control loop's goal speed 
directly to a velocity error term, when NESTED_LOOPS is disabled</li>
<li>PpTh - a threshold on position errors, below which the move is considered 
reached</li>
</ul>
<p>You may also need to tune the various PID constants, though you should start 
with the list above.</p>
<!-- #EndEditable --> 
  <br>
  &nbsp;</p>
  
  <h3>Download</h3>
  <p align="left">
  <!-- #BeginEditable "download" --> 
<?php
	$i = 0;
	foreach ($filenames as $filename) {
		$filedesc = $filedescs[$i]; 
		print "<a href='$filename'>$filename - $filedesc</a><br>";
		$i = $i + 1;
	}
	?>
<!-- #EndEditable --> 
  <br>
  &nbsp;</p>

  <h3>Source Code</h3>
  <blockquote>
  <code>
<!-- #BeginEditable "code" --> 
<?php
	$i = 0;
	foreach ($filenames as $filename) {
		if ($filedisps[$i] != 0) {
			print "</code><hr><h3>$filename</h3><br><code>";
			$f = fopen($filename, "r");
			while (!feof($f)) {
	   			$buffer = fgets($f, 4096);
				$line = htmlentities($buffer, ENT_QUOTES);
				$line = str_replace(" ", "&nbsp;", $line);
			    print "$line<br>";
			}
			fclose($f);
		}
		$i = $i + 1;
	}
	?>
<!-- #EndEditable --> 
  <br>
  &nbsp;</code>
  </blockquote>
</blockquote>
<?php include 'nubotics/support/footer.html';?>
<!-- #EndTemplate -->