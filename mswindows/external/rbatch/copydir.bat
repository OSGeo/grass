@echo off
setlocal
if not "%2"=="" goto:run
echo Usage: copydir fromdir todir
echo All files/directories in fromdir that do not also exist in todir are 
echo recurisvely copied.
echo e.g. 
echo      cd \Program Files\R
echo      copydir R-2.2.0\library R-2.3.0\library
goto:eof
:run
:: Notes on code:
:: on xcopy command /e copies subdir/files incl empty ones
:: on xcopy command /i causes target to be created
for /D  %%a in ("%~1\*") do if not exist %2\%%~na xcopy /e/i "%%a" "%~2\%%~nxa"
endlocal
