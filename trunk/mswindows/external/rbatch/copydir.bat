@echo off
:: Software and documentation is (c) 2013 GKX Associates Inc. and 
:: licensed under [GPL 2.0](http://www.gnu.org/licenses/gpl-2.0.html).
setlocal
if not "%2"=="" goto:run
echo Usage: copydir fromdir todir
echo All files/directories in fromdir that do not also exist in todir are 
echo recurisvely copied.
echo e.g. 
echo      cd "%userprofile%\Documents\R\win-library"
echo      copydir 2.14 2.15
echo Now start up R 2.15.x and issue update.packages()
goto:eof
:run
:: Notes on code:
:: on xcopy command /e copies subdir/files incl empty ones
:: on xcopy command /i causes target to be created
for /D  %%a in ("%~1\*") do if not exist %2\%%~na xcopy /e/i "%%a" "%~2\%%~nxa"
endlocal
