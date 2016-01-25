<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filenames[0] = "ww01_ic_s11tests.ic";
	$filedescs[0] = "Interactive C source";
	$filedisps[0] = 1;
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    Interactive C for Sumo11</i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This example is simply an update 
    to the standard test program that comes with the SORC Sumo11 board. It uses 
    the standard encoder.ic code that comes with Interactive C to interface to 
    the WheelWatchers -- which is good news as you can reuse any code that depends 
    on that file, like the various HandyBoard examples and RugWarrior examples.<br>
    <br>
    Encoder.ic relies on the ChA and ChB signals only, which limits you to 32 
    counts per wheel rotation. However, the test itself will display all the WW-01 
    signals if hooked up. <!-- #EndEditable --> 
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