@echo off

D:
cd D:\microtools\atmel\AVR Tools\AvrDragon
:start

echo Doing full programming sequence...
avrdragon -d ATtiny85 -mi -s -e -pf -if ww11.hex -vf -f0x55E2 -E0xFE -F0x55E2 -G0xFE
if errorlevel 1 goto error

echo Complete -- no error!
goto exit

:error
echo ERROR -- %errorlevel%

:exit
echo Ctrl-C to stop:
pause 
goto start
