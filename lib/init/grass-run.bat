@echo off
rem Batch file to launch GRASS commands

rem Change console title to name of module
title GRASS: %1

rem Force command line startup mode
set GRASS_UI_TERM=1

rem Run command
%*

if not %errorlevel% == 0 goto error

title GRASS: %1 Done.
echo.
echo %1 complete.
pause

:end
exit %errorlevel%

:error
title GRASS: %1 Done. (error %errorlevel%)
echo.
echo -----
echo ERROR: %1 exited abnormally.
echo -----
pause
goto end
