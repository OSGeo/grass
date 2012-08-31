
@echo off
rem if /i "%1"==path (path %2) && goto:eof

setlocal
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Placing this file in your path will allow rcmd to be run anywhere
:: without changing your path environment variable.  See comments
:: below on how it finds where R is.  Your path can be listed by
:: the Windows console command:  path
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

rem at one tine this script had only been tested on XP
rem recent tests have only been on Vista
rem ver | findstr XP >NUL
rem if errorlevel 1 echo Warning: This script has only been tested on Windows XP.

set scriptdir_=%~dp0
set lookin=.;%userprofile%;%scriptdir_%
if not defined R_BATCHFILES_RC (
	for %%f in ("rbatchfilesrc.bat") do set "R_BATCHFILES_RC=%%~$lookin:f"
)
if defined R_BATCHFILES_RC (
	if exist "%R_BATCHFILES_RC%" call %R_BATCHFILES_RC%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: use environment variable R_HOME if defined
:: else current folder if bin\rcmd.exe exists 
:: else most current R as determined by registry entry
:: else error
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if not defined R_HOME if exist bin\r.exe set R_HOME=%CD%
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\R-core\R /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME echo "Error: R not found" & goto:eof

call :process_arch %*

:: add R_MIKTEX to PATH if defined.  Otherwise if its not 
:: in the PATH already then check \Program Files\miktex* or \miktex* 
:: and if found add that to PATH.

:: if miktex found in PATH skip searching for it
PATH | findstr /i miktex > nul
if not errorlevel 1 goto:end_miktex

:: check for presence of %ProgramFiles%\miktex* or \miktex*

if not defined R_MIKTEX for /f "delims=" %%a in (
    'dir /b /on "%ProgramFiles%"\miktex* 2^>NUL'
) do set R_MIKTEX=%ProgramFiles%\%%a

if not defined R_MIKTEX for /f "delims=" %%a in (
    'dir /b /on %SystemDrive%:\miktex* 2^>NUL'
) do set R_MIKTEX=%SystemDrive%:\miktex\%%a

:end_miktex
if defined R_MIKTEX PATH %R_MIKTEX%\miktex\bin;%PATH%

if not defined MYSQL_HOME for /f "delims=" %%a in (
    'dir /b /on "%ProgramFiles%"\MySQL\* 2^>NUL'
) do set MYSQL_HOME=%ProgramFiles%\MySQL\%%a

if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\R-core\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
 ) do set R_TOOLS=%%~b
if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_TOOLS=%%~b

set PATHQ=%PATH%
:WHILE
    if "%PATHQ%"=="" goto WEND
    for /F "delims=;" %%i in ("%PATHQ%") do if exist "%%~sfi" set PATH2=%PATH2%;%%~sfi
    for /F "delims=; tokens=1,*" %%i in ("%PATHQ%") do set PATHQ=%%j
    goto WHILE 
:WEND

set path2=%path2:~1%

if defined R_TOOLS (
    set path2=%R_TOOLS%\bin;%R_TOOLS%\perl\bin;%R_TOOLS%\MinGW\bin;%PATH2%
)

path %path2%

set here=%CD%
set args=%*

:: get name by which this command was called
:: this allows same file to be used for Rgui, Rterm, etc. by just renaming it
for %%i in (%0) do set cmd=%%~ni.exe

if /i %cmd%==rtools.exe (endlocal & set path=%path2%) && goto:eof

cd %R_HOME%\bin
if /i not %cmd%==rguistart.exe goto:notRguiStart
  set cmd=rgui.exe
  set firstArgument=%1
  if defined firstArgument (
    dir %1 | findstr "<DIR>" > nul
    if errorlevel 1 goto:notRguiStart
    set here=%~1
    set firstArgument=
  )
  set args=
  shift
  :startloop
  set firstArgument=%1
  if defined firstArgument (
     set args=%args% "%~1" 
     shift
     goto:startloop
  )
:notRguiStart

set st=
if /i %cmd%==rgui.exe set st=start

if /i not %cmd%==#Rscript.exe goto:not#Rscript
set cmd=Rscript.exe
if [%1]==[] goto:help#Rscript
call :rsarg1 %*
goto:not#Rscript
:rsarg1
set args=%*
set arg1=%~1
set arg1=%arg1:.bat.bat=.bat%
set last4=%arg1:~-4%
if /i not "%last4%"==".bat" set arg1=%arg1%.bat
for %%a in ("%R_HOME%\bin\Rscript.exe") do set RSCRIPT=%%~sfa
call set args=%%args:%1="%arg1%"%%
rem call set args=%%args:%1=%%
goto:eof
:not#Rscript

cd %here%

:: Look in architecture specific subdirectory of bin. If not there look in bin.
set cmdpath=%R_HOME%\bin\%R_ARCH0%\%cmd%
if exist "%cmdpath%" goto:cmdpathfound
set cmdpath=%R_HOME%\bin\%cmd%
if exist "%cmdpath%" goto:cmdpathfound
echo "Error: %cmd% not found" & goto:eof
:cmdpathfound

:: if called as jgr.bat locate the JGR package to find jgr.exe
if /i not %cmd%==jgr.exe goto:notJGR
  set st=start
  set cmdpath=jgr.exe
  if not defined JGR_LIBS set JGR_LIBS=%R_LIBS%
  for %%a in ("%R_HOME%\bin\Rscript.exe") do set RSCRIPT=%%~sfa
  if not defined JGR_LIBS for /f "usebackq delims=" %%a in (
		`%RSCRIPT% -e "cat(.libPaths(),sep=';')"`
  ) do set JGR_LIBS=%%~a
  if not defined JGR_LIBS (
	echo "Error: JGR package not found in R library" & goto:eof
  )
  for %%f in ("JGR") do set "jgrpkg=%%~$JGR_LIBS:f"
  set JGR_LIB=%jgrpkg:~0,-4%
  for %%a in ("%JGR_LIB%") do set JGR_LIB_SHORT=%%~sfa
  for %%a in ("%R_HOME%") do set R_HOME_SHORT=%%~sfa
  set args=--libpath=%JGR_LIB_SHORT% --rhome=%R_HOME_SHORT%

:notJGR

rem set R_ARCH
rem set R_ARCH0
rem set cmdpath
rem if defined st set st
rem set args

set cygwin=nodosfilewarning
if not defined args goto:noargs
if defined st (start "" "%cmdpath%" %args%) else "%cmdpath%" %args%
goto:eof
:noargs
if defined st (start "" "%cmdpath%") else "%cmdpath%"
goto:eof

:help#Rscript
echo Usage: #Rscript %%0 %%*
echo If the above is the first line in a file 
echo containing only R code and the file is 
echo given a .bat extension then it can be 
echo run as a batch file.
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
    set arg=%~1
    shift 
    if not defined arg goto :process_arch_cont 
	if "%arg%"=="--arch" set R_ARCH=%1
	if defined R_ARCH goto:process_arch_cont
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
