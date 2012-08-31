@echo off
setlocal
rem rem ver | findstr XP >NUL

for %%i in (%0) do set cmd0=%%~ni
set cmd=Sweave
if /i "%cmd0%"=="Stangle" set cmd=Stangle

if not defined SWEAVE_STYLEPATH_DEFAULT set SWEAVE_STYLEPATH_DEFAULT=TRUE

set scriptdir_=%~dp0
set lookin=.;%userprofile%;%scriptdir_%

if not defined R_BATCHFILES_RC (
	for %%f in ("rbatchfilesrc.bat") do set "R_BATCHFILES_RC=%%~$lookin:f"
)
if defined R_BATCHFILES_RC (
	set R_BATCHFILES_RC
	if exist "%R_BATCHFILES_RC%" call %R_BATCHFILES_RC%
)

if "%1"=="" goto:help
if "%1"=="-h" goto:help
if "%1"=="--help" goto:help
if "%1"=="/?" goto:help

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: use environment variable R_HOME if defined
:: else check for .\bin\R.exe
:: else check registry
:: else check for existence of bin\R.exe in:
::   %ProgramFiles%\R\R\R-*
:: where more recently dated R-* directories would be matched over older ones
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not defined R_HOME if exist bin\R.exe set R_HOME=%CD%
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b

if not defined R_HOME echo "Error: R not found" & goto:eof

	if not defined R_HOME echo "Error: R not found" & goto:eof

if not "%1"==":Rterm" goto:notRterm

	set here=%CD%
	set args=%2 %3 %4 %5 %6 %7 %8 %9

	set cmd=:Rterm

	goto:Rterm
)
:notRterm
goto:continue
:help
echo Usage: %0 abc.Rnw
echo    or  %0 abc
if /i "%cmd%"=="stangle" goto:eof
echo switches:
echo    -t or --tex or     produce tex file and exit
echo    -p or --pdf or     produce pdf file and exit
echo    -n or --nobck.pdf  do not create .bck.pdf; instead display pdf directly
echo Runs sweave producing a .tex file.  Then it runs pdflatex producing
echo  a .pdf file and a .bck.pdf file.  Finally the .bck.pdf file is 
echo  displayed on screen.
echo.
echo Examples:
echo.
echo 1. Run sweave, pdflatex, create backup pdf with unique name, display it
echo       sweave mydoc.Rnw
echo 2. Same
echo       sweave mydoc
echo 3. Run sweave to create tex file.  Do not run pdflatex or display.
echo       sweave mydoc --tex
echo 4. Run sweave and pdflatex creating pdf file.  Do not create .bck.pdf
echo    file and do not display file.
echo       sweave mydoc --pdf
echo 5. Run sweave and pdflatex. Do not create .bck.pdf. Display .pdf file.
echo       sweave mydoc --nobck
goto:eof
:continue

call :process_arch %*

:: argument processing
:: - returns 'file' as file argument

    :loop 
    (set arg=%~1) 
    shift 
    if not defined arg goto :cont 
    (set prefix1=%arg:~0,1%) 
    if "%prefix1%"=="-" goto:switch
    set file=%arg%
    goto:loop
	:switch
	set switch0=%arg:-=%
	set switch0=%switch0:~0,1%
	rem architecture switch was previously handled so skip over it here
	if "%switch0%"=="a" goto:loop
	set switch=%switch0%
    goto:loop
    :cont

if errorlevel 1 echo Warning: This script has only been tested on Windows XP.
if exist "%file%.Rtex" set infile="%file%.Rtex"
if exist "%file%.Snw" set infile="%file%.Snw"
if exist "%file%.Rnw" set infile="%file%.Rnw"
if exist "%file%" set infile="%file%" 
set infilslsh=%infile:\=/%
:: call sweave
echo library('utils'); %cmd%(%infilslsh%) | %cmd0%.bat :Rterm --no-restore --slave
if /i "%cmd%"=="stangle" goto:eof
:: echo on
if errorlevel 1 goto:eof
if /i "%switch%"=="t" goto:eof

:: echo %cd%
for %%a in ("%file%") do set base=%%~sdpna
if not exist "%base%.tex" goto:eof
for /f "delims=" %%a in ('dir %infile% "%base%.tex" /od/b ^| more +1'
) do set ext=%%~xa

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

if "%ext%"==".tex" (pdflatex "%base%.tex") else goto:eof
if errorlevel 1 goto:eof
if /i "%switch%"=="p" goto:eof

if not exist "%base%.pdf" goto:eof
for /f "delims=" %%a in ('dir "%base%.pdf" "%base%.tex" /od/b ^| more +1'
) do set ext=%%~xa
if not "%ext%"==".pdf" goto:eof
set pdffile=%base%.pdf
if /i "%switch%"=="n" start "" "%pdffile%" && goto:eof
set tmpfile=%date%-%time%
set tmpfile=%tmpfile: =-%
set tmpfile=%tmpfile::=.%
set tmpfile=%tmpfile:/=.%
set tmpfile=%base%-%tmpfile%.bck.pdf
copy "%pdffile%" "%tmpfile%"
start "" "%tmpfile%"
echo *** delete *.bck.pdf files when done ***
goto:eof



@echo off
setlocal
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Placing this file in your path will allow rcmd to be run anywhere
:: without changing your path environment variable.  See comments
:: below on how it finds where R is.  Your path can be listed by
:: the Windows console command:  path
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: use environment variable R_HOME if defined
:: else check for .\bin\R.exe
:: else check registry
:: else check for existence of bin\R.exe in each of the following:
::   %ProgramFiles%\R\R\R-*
:: where more recently created R-* directories would be matched over
:: older ones.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not defined R_HOME if exist bin\R.exe set R_HOME=%CD%
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b
if not defined R_HOME for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_HOME=%%~b

if not defined R_HOME for /f "delims=" %%a in (
	'dir/b/od "%ProgramFiles%"\R\R-* 2^>NUL'
) do if exist "%ProgramFiles%\R\%%a\bin\R.exe" (set R_HOME=%ProgramFiles%\R\%%a)

if not defined R_HOME echo "Error: R not found" & goto:eof


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

if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\R-core\Rtools /v InstallPath 2^>NUL ^|
findstr InstallPath'
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
    PATH %R_TOOLS%\bin;%R_TOOLS%\perl\bin;%R_TOOLS%\MinGW\bin;%PATH2%
)

set here=%CD%
set args=%*

:: get name by which this command was called
:: this allows same file to be used for Rgui, Rterm, etc. by just renaming it
for %%i in (%0) do set cmd=%%~ni 

goto %cmd%
goto:eof

:: note that RguiStart sets cmd to rgui.exe and then 
:: jumps to :Rgui.exe where processing is finished
:RguiStart
:RguiStart.exe
cd %R_HOME%\bin
if /i not %cmd%==rguistart.exe goto:Rgui
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

:Rcmd2
:Rcmd2.exe
:Rcmd
:Rcmd.exe
(set cmd=rcmd.exe)&goto:main

:Rterm
:Rterm.exe
(set cmd=rterm.exe)&goto:main

:Rgui
:Rgui.exe
(set cmd=rgui.exe)&goto:main

:R
:R.exe
(set cmd=r)&goto:main

:Rjgr
:Rjgr.exe
(set cmd=rjgr)&goto:main

:#Rscript
:#Rscript.exe
:Rscript
:Rscript.exe
(set cmd=rscript.exe)&goto:main

# main portion of program
:main
set st=
if /i %cmd%==rgui.exe set st=start
:: if /i %cmd%==#rscript.exe set cmd=rscript.exe
cd %here%

:: Look in architecture specific subdirectory of bin. If not there look in bin.
set cmdpath=%R_HOME%\bin\%R_ARCH%\%cmd%
if exist "%cmdpath%" goto:cmdpathfound
set cmdpath=%R_HOME%\bin\%cmd%
if exist "%cmdpath%" goto:cmdpathfound
echo "Error: %cmd% not found" & goto:eof
:cmdpathfound
rem set cmdpath

if defined st (start "" "%cmdpath%" %args%) else "%cmdpath%" %args%
goto:eof

endlocal


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
	rem set R_HOME
	if defined R_ARCH goto:process_arch_cont
	:: The loop searches for --arch and sets R_ARCH to the next argument
    :process_arch_loop 
    (set arg=%~1) 
    shift 
    if not defined arg goto :process_arch_cont 
	if "%arg%"=="--arch" (set R_ARCH=%1) & goto:process_arch_cont
	goto:process_arch_loop
    :process_arch_cont
	if defined R_ARCH goto:process_arch_defined
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



