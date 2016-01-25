Programming and Building Notes

The batch files prog.bat and reprog.bat automate programming WW-11 or WW-12 
boards using an Atmel AVR Dragon programmer.

The reprog.bat is specifically for *reprogramming* an already programmed board.  
In that case, the high voltage programming mode is required, which in turn
requires you to remove R7, the 0 ohm resistor that connects TP1-DEBUGWIRE to
the junction between R2 and R4.  Obviously, the resistor needs to be replaced
(or simply replaced with a solder blob) afterwards.

The firmware was built using WinAVR 20100110:
http://winavr.sourceforge.net
This is a Windows package containing GCC and GNU Tools:
1. AVR GNU Binutils 2.19
2. AVR GNU Compiler Collection (GCC) 4.3.3
3. avr-libc 1.6.7cvs

January 2016, Noetic Design, Inc.