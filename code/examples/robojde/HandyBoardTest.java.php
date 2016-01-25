<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html><!-- #BeginTemplate "/Templates/code.dwt" -->
<?php include 'nubotics/support/header.html';?>
<!-- #BeginEditable "doctitle" --> 
<?php
	$filenames[0] = "HandyBoardTest.java";
	$filedescs[0] = "Java source";
	$filedisps[0] = 1;
	$filenames[1] = "Sumo11Test.java";
	$filedescs[1] = "Java source";
	$filedisps[1] = 1;
	$filenames[2] = "HBTNativ.asm";
	$filedescs[2] = "assembly source";
	$filedisps[2] = 1;
	$filenames[3] = "HandyBoardTest.rjp";
	$filedescs[3] = "RoboJDE project file";
	$filedisps[3] = 1;
	$filenames[4] = "Sumo11Test.rjp";
	$filedescs[4] = "RoboJDE project file";
	$filedisps[4] = 0;
?>
<!-- #EndEditable -->
<blockquote> <br>
  <h3 align="center"><br>
<!-- #BeginEditable "subject" --> <b><i>Example Source Code:<br>
    RoboJDE Java Test Program<br>
    For HandyBoards and Sumo11s</i></b><br>
    <!-- #EndEditable --><br>
    </h3>
  <h3>Description</h3>
  <p align="left">
  <!-- #BeginEditable "body" --> This is a custom version of 
    the test program distributed with RoboJDE. We've added a test for the WheelWatcher, 
    similar to the one in the Interactive C <a href="../ic/ww01_ic_s11tests.ic.php">test 
    program</a>. Note that RoboJDE does not use the same pin numbers for encoder 
    objects as the Interactive C encoder.ic file does. See the comments.<br>
    <br>
    The good news is that the encoder objects built into the HandyBoard class 
    work just fine with the WheelWatcher -- and, they count all edges of ChA and 
    ChB, resulting in 128 counts per wheel rotation.<!-- #EndEditable --> 
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