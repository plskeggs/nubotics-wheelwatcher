<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filename = "ww01_oopic2p_square.osc";
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    OOPIC II+ Simple Odometry</i></b><br>
    <a href="oopic_bot.jpg"><img src="tn_oopic_bot.jpg" width="320" height="240" border="0"></a> 
    <br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This example demonstrates the 
    use of a virtual circuit that automatically stops the servo when the encoder 
    reaches a certain count.<br>
    It uses this prinicple to drive the robot in a 6&quot; square pattern.<!-- #EndEditable --> 
  <br>
  &nbsp;</p>
  
  <h3>Download</h3>
  <p align="left">
  <!-- #BeginEditable "download" -->
<?php 
	print "<a href='$filename'>$filename</a> (fixed line termination problem, 10/2/2005)<br>";
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