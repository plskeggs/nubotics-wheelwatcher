<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filenames[0] = "ww01_avrgcc_monitor.c";
	$filedescs[0] = "C source";
	$filedisps[0] = 1;
	$filenames[1] = "Makefile";
	$filedescs[1] = "build file";
	$filedisps[1] = 1;
	$filenames[2] = "ww01_avrgcc_monitor.hex";
	$filedescs[2] = "Intel Hex file";
	$filedisps[2] = 0;
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    Atmel AVR Processor using WinAVR / GCC<br>
    Simple Position and Velocity Monitoring</i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This example ramps the speeds 
    of the two servos back and forth, and while doing so, displays the actual 
    current position and speed as measured using the WW-01s. <!-- #EndEditable --> 
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