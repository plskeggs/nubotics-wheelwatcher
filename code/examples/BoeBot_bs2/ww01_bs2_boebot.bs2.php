<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filename = "ww01_bs2_boebot.bs2";
?>
<!-- #EndEditable -->
<blockquote> <br>
<h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
Parallax BS2 / Boe-Bot</i></b><br>
<br>
<a href="../../example-bots/Boe-Bot/index.htm"><img src="../../example-bots/Boe-Bot/thumbnails/thww-01-boe-bot.jpg" width="160" height="120" border="0"></a> 
<br>
<a href="../../example-bots/Boe-Bot/index.htm">Photo Gallery of Example Robot</a><!-- #EndEditable --><br>
</h3>
<h3>Description</h3>
<p align="left"> <!-- #BeginEditable "body" --> This example uses a Basic Stamp 
2 with two WheelWatchers, on a Parallax Boe-Bot. Somewhat surprisingly, we are 
able to pulse the servos while also counting both edges of the Channel A quadrature 
signal and monitoring the hardware-decoded direction line. These actions are used 
to implement a drive-in-a-square movement.<br>
<br>
In order to use the WW-01 encoders on a Boe-Bot platform, certain changes were 
required:<br>
<ol>
<li>replace wheels with Acroname wheels which are designed for our codewheel stickers; 
the Parallax wheels' hubs are too large for the sticker to fit</li>
<li>mount the servos outside of the chassis rather than inside, so that our standard 
spacers can be used to mount the WW-01 circuit boards at the correct height<br>
</li>
</ol>
<p>The WW-01 encoders were wired up so that, for each wheel, the BS2 acesses the 
Channel A and Direction lines, not the Channel B nor the Clock lines. The reason 
is that performing a full decode of Channel A and Channel B for each wheel would 
take too much time for the BS2, but, the Clock line produces pulses that are too 
short for the BS2 to detect. Instead, we can take advantage of the decoded Direction 
line along with the more-slowly-changing Channel A signal (slow compared to the 
Clock line), and still get 64 pulses per revolution of the wheel, which is not 
too bad.</p>
<p>The connections are:</p>
<ul>
<li>BS2 Pin 0: Left Channel A (yellow)</li>
<li>BS2 Pin 1: Left Direction (orange)</li>
<li>BS2 Pin 2: Right Channel A (yellow)</li>
<li>BS2 Pin 3: Right Direction (orange)</li>
<li>Left and Right Vdd (red) to the Vdd proto board holes</li>
<li>Left and Right Vss (black) to the Vss proto board holes</li>
</ul>
<p>
<!-- #EndEditable --> <br>
&nbsp;</p>
<h3>Download</h3>
<p align="left"> <!-- #BeginEditable "download" --> 
<?php 
	print "<a href='$filename'>$filename</a><br>";
?>
<!-- #EndEditable --> <br>
&nbsp;</p>
<h3>Source Code</h3>
<blockquote> <code> <!-- #BeginEditable "code" --> 
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
<!-- #EndEditable --> <br>
&nbsp;</code> </blockquote>
</blockquote>
<?php include 'nubotics/support/footer.html';?>
<!-- #EndTemplate -->