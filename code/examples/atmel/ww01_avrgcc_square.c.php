<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filenames[0] = "ww01_avrgcc_square.c";
	$filedescs[0] = "C source";
	$filedisps[0] = 1;
	$filenames[1] = "Makefile";
	$filedescs[1] = "build file";
	$filedisps[1] = 1;
	$filenames[2] = "ww01_avrgcc_square.hex";
	$filedescs[2] = "Intel Hex file";
	$filedisps[2] = 0;
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    Atmel AVR Processor using WinAVR / GCC<br>
    Advanced Velocity and Position Control<br>
    </i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" -->This example is quite complex, 
    as it demonstrates many advanced control loop techniques. It has two control 
    loops: an inner velocity loop and an outer position loop. It includes code 
    to drive RC servos in the background using the two TIMER1 Output Compare hardware 
    units. <br>
    It measures the actual velocity of each wheel by measuring the time between 
    pulses on the CLK line on the WheelWatchers. It also calculates a running 
    4 sample average of these clock pulses in order to filter out alignment errors.<br>
    This example was developed on a <a href="http://www.barello.net/ARC/index.htm">Barello.net 
    ARC 1.1 board</a> for the MarkIII chassis. <!-- #EndEditable --> 
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