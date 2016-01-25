@echo off

D:
cd D:\microtools\atmel\AVR Tools\AvrDragon
:start

echo Doing full programming sequence in HVSP mode...
avrdragon -d ATtiny85 -mp -s -e -pf -if ww11.hex -vf -f0x55E2 -E0xFE -F0x55E2 -G0xFE
rem avrdude -p t85 -c dragon_hvsp -P usb -e -U flash:w:ww11.hex:i -v -U efuse:w:0xFE:m -U hfuse:w:0x55:m -U lfuse:w:0xE2:m
if errorlevel 1 goto error

echo Complete -- no error!
goto exit

:error
echo ERROR -- %errorlevel%

:exit
