<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filename = "ww01_dios_square.txt";
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    DIOS Simple Odometry</i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This is an example using a DIOS 
    Ares board from <a href="http://www.kronosrobotics.com">Kronos Robotics</a>. 
    The DIOS language is a variation on BASIC. Here, we count ChA and ChB signals 
    using interrrupts. The interrupt handlers calcuate the current position and 
    also count down a goal distance. This is used to determine when a desired 
    movement is done.Note that the Ares co-processor can be used to drive the 
    servos. Instead, this example uses only the DIOS functionality.<!-- #EndEditable --> 
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