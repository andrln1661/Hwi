@echo off
set PORT=%1
avrdude -v -patmega2560 -cwiring -P %PORT% -b115200 -D -Uflash:w:firmware.hex:i
pause
