@echo off
rem #########################################################################
rem #
rem #		GRASS Initialization
rem #
rem #########################################################################

rem *******Environment variables***********
rem Uncomment and set the following values if they differ from the indicated default

rem Directory where your .grassrc7 file will be stored
rem set HOME=%USERPROFILE%

rem Name of the wish (Tk) executable
rem set GRASS_WISH=wish.exe

rem Path to the shell command 
rem (adjust to where you installed msys or another shell)
rem set GRASS_SH=c:\msys\1.0\bin\sh.exe

rem Path to utilities used by some scripts, such as awk, sed, etc
rem (adjust to where you installed msys, gnuwin32, or other similar utilises)
rem set PATH=c:\msys\1.0\bin;c:\msys\1.0\lib;%PATH%

rem Path to your web browser
rem set GRASS_HTML_BROWSER=%SYSTEMDRIVE%/PROGRA~1/INTERN~1/IEXPLORE.EX

rem Path to the proj files (notably the epsg projection list)
rem set GRASS_PROJSHARE=c:/grass/share/proj

set WINGISBASE=GISBASE_VALUE
"%WINGISBASE%\etc\Init.bat" %*

