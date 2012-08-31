
@echo off

:: Without args this lists the R_HOME directory for each version of R
:: on the system.  If one of those directories is given as an argument
:: then that version is set to the current version
:: Note: Use Rfind.bat and look on R_HOME line to find current version of R.

setlocal
rem ver | findstr XP >NUL
rem if errorlevel 1 echo Warning: This script has only been tested on Windows XP.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: use environment variable R_HOME if defined
:: else current folder if bin\rcmd.exe exists 
:: else most current R as determined by registry entry
:: else error
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not defined R_HOME if exist bin\rcmd.exe set R_HOME=%CD%
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME echo "Error: R not found" & goto:eof

:: look for architecture in these places in this order:
:: - environment variable R_ARCH
:: - first arg if its --arch
:: - check if R_HOME\bin\i386 exists
:: - if R_HOME\bin\x64 exists
:: - if none of the above then use i386

call :process_arch %*
rem set R

cd %R_HOME%
cd ..

if "%1"=="" (
	for /d %%a in (*) do if exist %%a\bin\R.exe echo %%a
	goto:eof
)

:: Look in architecture specific subdirectory of bin. If not there look in bin.
set cmdpath=%1\bin\%R_ARCH0%\RSetReg.exe
rem set cmdpath
if exist "%cmdpath%" goto:cmdpathfound
set cmdpath=%1\bin\RSetReg.exe
rem set cmdpath
if exist "%cmdpath%" goto:cmdpathfound
echo "Error: RSetReg.exe not found" & goto:eof
goto:eof
:cmdpathfound
"%cmdpath%"

goto:eof


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: processing of --arch= where value can be 32, 64, i386, x64, /i386, /x64

:: Call it like this: call :process_arch %*
:: On return R_ARCH will be set from --arch or R_ARCH or default
:: and R_ARCH0 will be R_ARCH without the / prefix
:: It will look for the architecture in these places in this order:
:: - first arg if its --arch
:: - environment variable R_ARCH
:: - check if R_HOME\bin\i386 exists
:: - if R_HOME\bin\x64 exists
:: - if none of the above then use R_ARCH=/i386
:: Note that R_HOME should be defined before calling this routine
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:process_arch
	if defined R_ARCH goto:process_arch_cont
	:: The loop searches for --arch and sets R_ARCH to the next argument
    :process_arch_loop 
    (set arg=%~1) 
    shift 
    if not defined arg goto :process_arch_cont 
	if "%arg%"=="--arch" (set R_ARCH=%1) & goto:process_arch_cont
	goto:process_arch_loop
    :process_arch_cont
	if defined process_arg_arch goto:process_arch_defined
	if exist %R_HOME%\bin\i386 (set R_ARCH=/i386) & goto:process_arch_defined
	if exist %R_HOME%\bin\x64 (set R_ARCH=/x64) & goto:process_arch_defined
	(set R_ARCH=/i386)
	:process_arch_defined
	if "%R_ARCH%"=="32" (set R_ARCH=/i386)
	if "%R_ARCH%"=="386" (set R_ARCH=/i386)
	if "%R_ARCH%"=="i386" (set R_ARCH=/i386)
	if "%R_ARCH%"=="64" (set R_ARCH=/x64)
	if "%R_ARCH%"=="x64" (set R_ARCH=/x64)
	:: if R_ARCH does not begin with a slash add one as a prefix
	(set first_char=%R_ARCH:~0,1%)
	if not "%first_char%" == "/" (set R_ARCH=/%R_ARCH%)
	:: R_ARCH0 is like R_ARCH but without the beginning /
	(set R_ARCH0=%R_ARCH:~1%)
	goto:eof

endlocal
