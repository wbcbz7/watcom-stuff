@echo off
if [%1] == [] goto usage
echo -------------------------------- >> %1
vidbench.exe -m13 -i -q               >> %1
echo -------------------------------- >> %1
vidbench.exe -mx -i -q                >> %1
echo -------------------------------- >> %1
vidbench.exe -m101 -i -q              >> %1
echo -------------------------------- >> %1
vidbench.exe -m101 -lfb -i -q         >> %1
echo results written to %1 file
goto exit

:usage
echo usage: test.bat [file.txt], i.e. "test.bat results.txt"
:exit