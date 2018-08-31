@Echo OFF 

:: Software and documentation is (c) 2013 GKX Associates Inc. and 
:: licensed under [GPL 2.0](http://www.gnu.org/licenses/gpl-2.0.html).

:: Help is at bottom of script or just run script with single argument: help

:: can optionally uncomment and change these to force certain values. This
:: is normally unnecessary. Rather one can usually rely on the heuristics to
:: set them.
:: set R_CMD=R
:: set R_HOME=%ProgramFiles%\R\R-2.14.0
:: set R_ARCH=64
:: set R_MIKTEX_PATH=%ProgramFiles%\MiKTeX 2.9\miktex\bin
:: set R_TOOLS=C:\Rtools
:: set MYSQL_HOME=%ProgramFiles%\MySQL\MysQL Server 5.1

:: 1 means read registry and 0 means ignore registry
if not defined R_REGISTRY set R_REGISTRY=1
set CYGWIN=nodosfilewarning

SetLocal EnableExtensions EnableDelayedExpansion 

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: R_CMD
:: 1. if 1st arg is CMD then set R_CMD to R
:: 2. else if 1st arg is Rshow, Rpath, Rgui, Rcmd, R or Rscript set R_CMD to it
::    and remove it from args
:: 3. else use R_CMD if set
:: 4. else use %0
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

set args=%*
set cmd=
if /i "%~1"=="registry" goto:Rregistry
if /i "%~1"=="cd" set cmd=Rcd
if /i "%~1"=="CMD" set cmd=Rcmd
if /i "%~1"=="dir" set cmd=Rdir
if /i "%~1"=="ls" set cmd=Rdir
if /i "%~1"=="gui" set cmd=Rgui
if /i "%~1"=="help" goto:Rhelp
if /i "%~1"=="path" set cmd=Rpath
if /i "%~1"=="R" set cmd=R
if /i "%~1"=="script" set cmd=Rscript
if /i "%~1"=="show" set cmd=RShow
if /i "%~1"=="SetReg" set cmd=RSetReg
if /i "%~1"=="tools" set cmd=Rtools
if /i "%~1"=="touch" set cmd=Rtouch

if "%cmd%"=="" goto:R_CMD_cont
if "%2"=="" (set args=) && goto:R_CMD_cont
set args=xxx%*
call set args=%%args:xxx%1=%%
:R_CMD_cont
if defined cmd set R_CMD=%cmd%
if not defined R_CMD set R_CMD=%0
:: set "R_CMD=%R_CMD:.bat=%"
for %%i in ("%R_CMD%") do set R_CMD=%%~ni
if /i "%R_CMD%"=="#Rscript" set R_CMD=Rscript
rem echo R_CMD:%R_CMD% args=[%args%]

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: 1. If .\Rgui.exe exist use implied R_PATH and skip remaining points.
:: 2. If .\{x64,i386}\Rgui.exe or .\bin\{x64,i386}\Rgui.exe exists use implied R_HOME.
:: 3. if R_HOME defined then derive any of R_ROOT and R_VER that 
::    are not already defined.
:: 4. if R_PATH defined then derive any of R_ROOT, R_HOME, R_VER and R_ARCH that
::    are not already defined.
:: 4a. If R_REGISTRY=1 and R found in registry derive any of R_HOME, R_ROOT and 
::     R_VER that are not already defined.
:: 5. If R_ROOT not defined try %ProgramFiles%\R\*, %ProgramFiles(x86)%\R\*
::    and then %SystemRoot%\R else error
:: 6. If R_VER not defined use last directory in cd %R_ROOT% & dir /od
:: 7. if R_ARCH not defined try %R_ROOT%\%R_VER%\bin\x64\Rgui.exe and then
::    %R_ROOT%\%R_VER%\bin\i386\Rgui.exe
:: 8. If R_ROOT, R_VER and R_ARCH defined skip remaining points.
:: 9. If Rgui.exe found on PATH use implied R_PATH.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: 1
if exist Rgui.exe (
    pushd ..\..
    set R_HOME=!CD!
    popd
   goto:R_exe_end
)

:: 2
if exist x64\Rgui.exe (
    pushd ..
    set R_PATH=!CD!\bin\x64
    popd
    goto:R_exe_end
)
if exist i386\Rgui.exe (
    pushd ..
    set R_PATH=!CD!\bin\i386
    popd
    goto:R_exe_end
)
if exist bin\x64\Rgui.exe set R_PATH=%CD%\bin\x64 & goto:R_exe_end
if exist bin\i386\Rgui.exe set R_PATH=%CD%\bin\i386
:R_exe_end

:: 3

if defined R_HOME (
    pushd
    cd %R_HOME%
    if not defined R_VER for /f "delims=" %%a in ("!CD!") do set R_VER=%%~na
    cd ..
    if not defined R_ROOT set R_ROOT=!CD!
    popd
)

:: 4
if defined R_PATH (
    pushd
    cd %R_PATH%
    if not defined R_ARCH for /f "delims=" %%a in ("!CD!") do set R_ARCH=%%~na
    cd ..\..
    if not defined R_HOME set R_HOME=!CD!
    if not defined R_VER for /f "delims=" %%a in ("!CD!") do set R_VER=%%~na
    cd ..
    if not defined R_ROOT set R_ROOT=!CD!
    popd
)


:: 4a

if not defined R_HOME for /f "tokens=2*" %%a in (
    'reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2^>NUL ^| findstr InstallPath'
    ) do set R_HOME=%%~b

if not defined R_HOME for /f "tokens=2*" %%a in (
    'reg query hklm\software\R-core\R /v InstallPath 2^>NUL ^| findstr InstallPath'
    ) do set R_HOME=%%~b

if defined R_HOME (
    if not defined R_ROOT (
	pushd %R_HOME%
	cd ..
	set R_ROOT=!CD!
        popd
    )
    if not defined R_VER (
        for /f "delims=" %%a in ("%R_HOME%") do set R_VER=%%~nxa
    )
)

	
:: 5

if defined R_ROOT goto:R_ROOT_end
if exist "%ProgramFiles%\R" set R_ROOT=%ProgramFiles%\R
if defined R_ROOT goto:R_ROOT_end
if exist %SystemDrive%\R set R_ROOT=%SystemDrive%\R
:R_ROOT_end

:: 6
if defined R_VER goto:R_VER_end
for /f "delims=" %%a in (
    'dir /b /od /ad "%R_ROOT%" 2^>NUL'
) do set R_VER=%%a
:R_VER_end

:: do we need this?
if defined R_ROOT if defined R_VER set R_HOME=%R_ROOT%\%R_VER%

:: 7
if defined R_ARCH goto:R_ARCH_cont
set R_ARCH=i386
if exist "%R_HOME%\bin\x64" set R_ARCH=x64
if exist "%R_ROOT%\%R_VER%\bin\x64" set R_ARCH=x64
:R_ARCH_cont
if "%R_ARCH%"=="64" set R_ARCH=x64
if "%R_ARCH%"=="32" set R_ARCH=i386
if "%R_ARCH%"=="386" set R_ARCH=i386

:: 8
if not defined R_ROOT goto:where
if not defined R_VER goto:where
if not defined R_ARCH goto:where
set R_PATH=%R_ROOT%\%R_VER%\bin\%R_ARCH%
goto:path_end

echo "R not found" & exit /b 1

:: 9
:where
where Rgui.exe 1>NUL 2>NUL
if not errorlevel 1 for /f "delims=" %%a in ('where Rgui.exe') do set R_PATH=%%~pa

:path_end

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: R_TOOLS
:: 1. use R_TOOLS if defined. If not
:: 2. check if ls.exe, rsync.exe and gcc.exe are on PATH. If not
:: 3. check if Rtools found in registry. If not
:: 4. check if C:\Rtools exists. If not
:: 5. R_TOOLS not found.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if defined R_TOOLS goto:RtoolsEnd

where ls.exe 1> NUL 2> NUL
if errorlevel 1 goto:RtoolsNotOnPATH

where rsync.exe 1> NUL 2> NUL
if errorlevel 1 goto:RtoolsNotOnPATH

where gcc.exe 1> NUL 2> NUL
if errorlevel 1 goto:RtoolsNotOnPATH

for /f "delims=" %%a in ('where rsync.exe') do set R_TOOLS=%%~pa
pushd %R_TOOLS%
cd ..
set R_TOOLS=%CD%
popd
goto:RtoolsEnd

:RtoolsNotOnPATH

if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\R-core\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
 ) do set R_TOOLS=%%~b
if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_TOOLS=%%~b

if exist "C:\Rtools" set R_TOOLS=C:\Rtools

:RToolsEnd

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: R_TOOLS_PATH
:: Extract path from: %R_TOOLS%\unins000.dat
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if defined R_TOOLS call :extract_string {app} %R_TOOLS%\unins000.dat
call set R_TOOLS_PATH=%%final:{app}=%R_TOOLS%%%
call :trimPath:R_TOOLS_PATH

if defined R_TOOLS for /f "tokens=3" %%a in (%R_TOOLS%\Version.txt) do set R_TOOLS_VERSION=%%a

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: R_MIKTEX
:: If R_MIKTEX defined use that
:: else if pdflatex.exe on PATH use that else
:: check %ProgramFiles%\miktex* else
:: check %ProgramFiles(x86)%\miktex* else
:: check %SystemDrive%\miktex*
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if defined R_MIKTEX_PATH goto:miktex_end

:: if pdflatex.exe found in PATH use implied path
where pdflatex.exe 1>NUL 2>NUL
if errorlevel 1 goto:miktex_continue
set MIKTEX_ALREADY_ON_PATH=1
for /f "delims=" %%a in ('where pdflatex.exe') do set R_MIKTEX_PATH=%%~pa
:: remove trailing \, if any
IF "%R_MIKTEX_PATH:~-1%"=="\" SET R_MIKTEX_PATH=%R_MIKTEX_PATH:~0,-1%

goto:miktex_end

:miktex_continue
if not defined R_MIKTEX_PATH for /f "delims=" %%a in (
    'dir /b /on "%ProgramFiles%"\miktex* 2^>NUL'
) do set R_MIKTEX_PATH=%ProgramFiles%\%%a\miktex\bin

if not defined R_MIKTEX_PATH for /f "delims=" %%a in (
    'dir /b /on "%ProgramFiles(x86)%"\miktex* 2^>NUL'
) do set R_MIKTEX_PATH=%ProgramFiles%\%%a\miktex\bin

if not defined R_MIKTEX_PATH for /f "delims=" %%a in (
    'dir /b /on %SystemDrive%:\miktex* 2^>NUL'
) do set R_MIKTEX_PATH=%SystemDrive%:\%%a\mixtex\bin

:miktex_end

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: MySQL
:: If MYSQL_HOME defined use that else
:: check %ProgramFiles%\MySQL\* else
:: check %SystemDrive%:\MySQL\*
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: if not defined MYSQL_HOME for /f "delims=" %%a in (
::    'dir /b /on "%ProgramFiles%"\MySQL\* 2^>NUL'
:: ) do set MYSQL_HOME=%ProgramFiles%\MySQL\%%a
::
:: if not defined MYSQL_HOME for /f "delims=" %%a in (
::    'dir /b /on %SystemDrive%:\MySQL* 2^>NUL'
:: ) do set R_MIKTEX=%SystemDrive%:\MySQL\%%a

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: get name by which program was called - $0
:: or use R_CMD environment variable if that was defined (mainly for testing)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if not defined R_CMD (set R_CMD=%0)
for %%i in ("%R_CMD%") do set R_CMD=%%~ni
if /i "%R_CMD%"=="dir" goto:Rdir
if /i "%R_CMD%"=="cd" goto:Rcd
if /i "%R_CMD%"=="touch" goto:Rtouch
if /i "%R_CMD%"=="Rversions" goto:RSetReg

:: add MiKTeX to PATH if not already on it
if not defined R_MIKTEX_PATH goto :miktex_add_path_end
echo %PATH% | findstr /i miktex 1>NUL 2>NUL
if errorlevel 1 path %R_MIKTEX_PATH%;%PATH%
:miktex_add_path_end

:: add Rtools paths to PATH if not already on it
if not defined R_TOOLS_PATH goto :Rtools_add_path_end
echo %PATH% | findstr /i Rtools 1>NUL 2>NUL
if errorlevel 1 path %R_TOOLS_PATH%;%PATH%
:Rtools_add_path_end

if /i "%R_CMD%"=="Rpath" goto:Rpath
if /i "%R_CMD%"=="Rtools" goto:Rtools
if /i "%R_CMD%"=="Rcd" goto:Rcd
if /i "%R_CMD%"=="Rdir" goto:Rdir
if /i "%R_CMD%"=="Rshow" goto:Rshow
if /i "%R_CMD%"=="Rtouch" goto:Rtouch
if /i "%R_CMD%"=="RSetReg" goto:RSetReg


if /i not "%R_CMD%"=="Rgui" goto:notRgui
start "Rgui.exe" "%R_PATH%\Rgui.exe" %args%
goto:eof

:notRgui
"%R_PATH%\%R_CMD%.exe" %args%

goto:eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: output the set statements
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Rshow

if defined R_PATH (
    set old=%CD%
    cd %R_PATH%
    for /f "delims=" %%a in ("!CD!") do set R_ARCH=%%~nxa
    cd ..\..
    set R_HOME=!CD!
    for /f "delims=" %%a in ("!CD!") do set R_VER=%%~nxa
    cd ..
    set R_ROOT=!CD!
    cd !old!
)

:: echo set R_PATH=%R_PATH%
:: echo set R_HOME=%R_HOME%
:: echo set R_ROOT=%R_ROOT%
:; echo set R_VER=%R_VER%
:: echo set R_ARCH=%R_ARCH%
:: echo set R_TOOLS=%R_TOOLS%
:: echo set R_TOOLS_PATH=%R_TOOLS_PATH%
:: :: echo set MYSQL_HOME=%MYSQL_HOME%
::echo set R_MIKTEX_PATH=%R_MIKTEX_PATH%
set R
goto:eof

:Rcd
endlocal & cd %R_ROOT%
goto:eof

:Rdir
dir/od "%R_ROOT%"
goto:eof

:RSetReg
cd %R_PATH%
RSetReg %args%
goto:eof

:: if not XP then check if running with Admin privs. If not give msg and exit.
:Rtouch
ver | findstr XP >NUL
if not errorlevel 1 goto:Rtouch_next
if not exist "%ProgramFiles%\R" goto:Rtouch_next
reg query "HKU\S-1-5-19" >NUL 2>&1 && ( goto Rtouch_next ) || ( 
        echo Please run this as Administator.
        goto :eof
) 
:Rtouch_next

if not defined R_HOME set R_HOME=%R_ROOT%\%R_VER%
pushd %R_HOME%
echo > dummy.txt
del dummy.txt
popd
goto:eof



:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: set path 
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Rpath
endlocal & PATH %PATH%;%R_PATH%
goto:eof

:Rtools
endlocal & PATH %PATH%
goto:eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: list R versions
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Rversions
if defined args goto:Rversions_cont
pushd %R_HOME%
cd ..
echo R packages found. Most recent (last listed) is default:
for /f "delims=" %%a in ('dir/b /od') do echo %%~fa
popd
goto:eof
:Rversions_cont
set args=###%args%
set args=%args:### =%
set args=%args:###=%
pushd %R_HOME%
cd ..
(for /f "delims=" %%a in ('dir /b /od') do echo %%~fa) | findstr /L /C:"%args%" 1>NUL 2>NUL
if errorlevel 1 echo %args% not found & goto:eof
echo Run the following command (may need an elevated cmd window):
for /f "delims=" %%a in ('dir /b /on ^| findstr /L /C:"%args%"') do @echo echo ^> "%%~fa\dummy.txt" /Y
popd
goto:eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: list registry entries
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:Rregistry
reg query hklm\software\R-core\R /v InstallPath 2>NUL | findstr InstallPath
reg query hklm\software\wow6432Node\r-core\r /v InstallPath 2>NUL | findstr InstallPath

goto:eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Extract text from file:
::   %1 = input string that starts text
::   %2 = input file
::   final = output variable holding text from and including %1 until 
::    binary data encountered
::
:: Needs: SetLocal EnableExtensions EnableDelayedExpansion 
::
:: Example:  
::      call :extract_string {app} C:\Rtools\unins000.dat
::      echo %final%
::   where {app} is the string that starts extraction and 
::         C:\Rtoolsiunins000.dat is the file
::
:: Based on code by Frank Westlake, https://github.com/FrankWestlake
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

   :extract_string

   SetLocal EnableExtensions EnableDelayedExpansion 

   Set "string=%1" 
   Set "file=%2"

   For /F "delims=" %%a in ( 
       'findstr /C:"%string%" "%file%"^|MORE' 
     ) Do ( 
     Set "$=%%~a" 
     If /I "!$:~0,5!" EQU "%string%" ( 
       Set $=!$:;=" "! 
       For %%b in ("!$!") Do ( 
         Set "#=%%~b" 
         If "!#:~0,5!" EQU "%string%" ( 
           CALL :work "!#!"
         ) 
       ) 
     ) 
   ) 
   endlocal & set final=%final%
   Goto :EOF 
   :work 
   set final=%final%!#!;
   Goto :EOF 

 :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 
  :trimPath:<variable to trim> [segment to add] 
  :: Eliminates redundant path segments from the variable and 
  :: optionally adds new segmants. 
  :: Example: CALL :trimPath:PATH 
  :: Example: CALL :trimPath:PATH "C:\A & B" C:\a\b\c 
  :: 
  :: Note that only a colon separates the subroutine name and 
  :: the name of the variable to be edited. 
  :: - Frank Westlake, https://github.com/FrankWestlake
  SetLocal EnableExtensions EnableDelayedExpansion 
  For /F "tokens=2 delims=:" %%a in ("%0") Do ( 
    For %%a in (%* !%%a!) Do ( 
      Set "#=%%~a" 
      For %%b in (!new!) Do If /I "!#!" EQU "%%~b" Set "#=" 
      If DEFINED # ( 
        If DEFINED new (Set "new=!new!;!#!") Else ( Set "new=!#!") 
      ) 
    ) 
  ) 
  EndLocal & For /F "tokens=2 delims=:" %%a in ("%0") Do Set "%%a=%new%" 
  Goto :EOF 

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 

:Rhelp

echo (c) 2013 G. Grothendieck 
echo License: GPL 2.0 ( http://www.gnu.org/licenses/gpl-2.0.html )
echo Launch script for R and associated functions.  
echo Usage:  R.bat [subcommand] [arguments]
echo Subcommands where (0) means takes no arguments; (A) means may need Admin priv
echo   cd - cd to R_ROOT, typically to C:\Program Files\R (0)
echo   cmd - Run Rcmd.exe
echo   dir - List contents of R_ROOT in chronological order showing R versions (0)
echo   gui - Run Rgui.exe
echo   help - Help info (0)
echo   path - Add R_TOOLS, R_MIKTEX ^& R_PATH to path for this cmd line session (0)
echo   R - Run R.exe (0)
echo   script - Run Rscript.exe
echo   show - Show R_ variable values used. R_PATH, etc. (0)
echo   SetReg - Run RSetReg; see 2.17 in R FAQ for Windows (A)
echo   tools - Add R_TOOLS and R_MIKTEX to path for this cmd line session (0)
echo   touch - Change date on R_HOME to now (0) (A)
echo Examples
echo   R     -- invoke R.exe                R gui -- invoke Rgui.exe
echo   R dir -- show R versions             R show -- show R_ variables
echo   R CMD build mypkg -- builds mypkg
echo   cmd /c set R_VER=R-2.14.0 ^& R gui -- run indicated Rgui version
echo   cmd /c set R_ARCH=32 ^^^& R gui -- run 32 bit Rgui
echo   cmd /c R_VER=R-2.14.0 ^^^& R setreg - make 2.14.0 current in registry
echo   cmd /c R_VER=R-2.14.0 ^^^& R.bat touch - change date on R-2.14.0 dir to now
goto:eof
echo.
echo Run Rgui using a different version of R.  R_HOME only affects R session
echo but not cmd line session.
echo   cmd /c set R_HOME=%ProgramFiles%\R\R-2.14.0 ^& R gui
echo
echo Launch a new cmd line window in which R_HOME is as set and launch R:
echo   start set R_HOME=%ProgramFiles%\R\R-2.14.0 ^& R gui
echo
echo ==Customization by renaming==
echo.
echo If the optional first argument is missing then it uses the value of 
echo the environment variable R_CMD or if that is not set it uses the name of 
echo the script file as the default first argument.  The idea is one could have 
echo multiple versions of the script called R.bat, Rgui.bat, etc. which invoke
echo the corresponding functionality without having to specify first argument.
echo.
echo ==Customization by setting environment variables at top of script==
echo.
echo It can be customized by setting any of R_CMD, R_HOME, R_ARCH, 
echo R_MIKTEX_PATH, R_TOOLS after the @echo off command at the top of the 
echo script.  R_CMD will be used as the default first argument (instead of the 
echo script name).  
echo.
echo e.g. use the following after @echo off to force 32-bit
echo set R_ARCH=32
echo.
echo e.g.  use the following after @echo off to force a particular version of 
echo R to be used
echo set R_HOME=%ProgramFiles%\R\R-2.14.0
echo.
echo e.g. use the following after @echo off to change the default command to 
echo Rgui even if the script is called myRgui.bat, say:
echo set R_CMD=Rgui
echo.
echo ==Installation==
echo. 
echo The script is self contained so just place it anywhere on your Windows
echo PATH.  (From the Windows cmd line the command PATH shows your current
echo Windows path.)  You may optionally make copies of this script with names 
echo like R.bat, Rscript.bat, Rcmd.bat so that each has a different default.
echo.

