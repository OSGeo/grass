@echo off
rem Batch file to launch GRASS commands

rem Change console title to name of module
title GRASS: %1

rem Force command line startup mode
set GRASS_UI_TERM=1

rem Run command
"%*"

title GRASS: %1 Done.
if %errorlevel% == 1 goto error

rem Pause for 2 seconds to allow user to read any output
ping 127.0.0.1 -n 3 -w 1000 >NUL:
:end
exit %errorlevel%

:error
echo -----
echo ERROR: %1 exited abnormally.
echo -----
pause
goto end
