REM
REM Environmental variables for GRASS stand-alone installer
REM

set GRASS_PYTHON=%GISBASE%\extrabin\python3.exe
<<<<<<< HEAD
set PYTHONHOME=%GISBASE%\Python312
=======
set PYTHONHOME=%GISBASE%\Python39
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

set GRASS_PROJSHARE=%GISBASE%\share\proj

set PROJ_LIB=%GISBASE%\share\proj
set GDAL_DATA=%GISBASE%\share\gdal

set FONTCONFIG_FILE=%GISBASE%\etc\fonts.conf

set PATH=%GISBASE%\extrabin;%GISBASE%\bin;%PYTHONHOME%;%PATH%

REM set RStudio temporarily to %PATH% if it exists

IF EXIST "%ProgramFiles%\RStudio\bin\rstudio.exe" set PATH=%PATH%;%ProgramFiles%\RStudio\bin

REM set R_USER if %USERPROFILE%\Documents\R\ exists to catch most common cases of private R libraries
IF EXIST "%USERPROFILE%\Documents\R\" set R_USER=%USERPROFILE%\Documents\

REM set R temporarily to %PATH%

IF EXIST "%ProgramFiles%\R\" ("%GISBASE%\extrabin\R" path)
