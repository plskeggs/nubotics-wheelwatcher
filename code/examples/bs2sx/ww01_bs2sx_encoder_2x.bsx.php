<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filename = "ww01_bs2sx_encoder_2x.bsx";
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    Parallax BS2SX</i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This example uses a Basic Stamp 
    2SX with two WheelWatchers, two proximity detectors (can be either bumpers 
    or IR), and a FerretTronics FT639 Serial Servo Controller Chip.<br>
    <br>
    The FT639 can easily be replaced with some other external servo controller, 
    such as Parallax's own 'Parallax Servo Controller.' Either way, an external 
    servo controller takes a large burden off of the Basic Stamp architecture, 
    as it is not designed to output servo control pulses in the background.<br>
    <br>
    Another thing to try is the Parallax PWMPAL. It provides both hardware pulse 
    generation for DC motors (e.g., stripped servos) or servos AND four 16 bit 
    hardware counters, accessible over a serial line from the Stamp. This should 
    allow you to use the CLK lines and get good odometry, even on a Basic Stamp.<br>
    <br>
    Currently, this example only counts the encoders, but does not yet do anything 
    with the counts. Stay tuned for more improvements.<!-- #EndEditable --> 
  <br>
  &nbsp;</p>
  
  <h3>Download</h3>
  <p align="left">
  <!-- #BeginEditable "download" --> 
    <?php 
	print "<a href='$filename'>$filename</a><br>";
?>
    <!-- #EndEditable --> 
  <br>
  &nbsp;</p>

  <h3>Source Code</h3>
  <blockquote>
  <code>
<!-- #BeginEditable "code" --> 
    <?php
	$f = fopen($filename, "r");
	while (!feof($f)) {
   		$buffer = fgets($f, 4096);
		$line = htmlentities($buffer, ENT_QUOTES);
		$line = str_replace(" ", "&nbsp;", $line);
	    print "$line<br>";
	}
	fclose($f);
?>
    <!-- #EndEditable --> 
  <br>
  &nbsp;</code>
  </blockquote>
</blockquote>
<?php include 'nubotics/support/footer.html';?>
<!-- #EndTemplate -->