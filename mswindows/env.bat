REM
REM Environmental variables for GRASS stand-alone installer
REM

REM Default prompt: cmd.exe
REM To enable bash prompt please uncomment the line bellow
REM set GRASS_SH=%GISBASE%\msys\bin\sh.exe

set GRASS_PYTHON=%GISBASE%\extrabin\python.exe
set PYTHONHOME=%GISBASE%\Python27

set GRASS_PROJSHARE=%GISBASE%\share\proj

set PROJ_LIB=%GISBASE%\share\proj 
set GDAL_DATA=%GISBASE%\share\gdal
set GEOTIFF_CSV=%GISBASE%\share\epsg_csv

set PATH=%GISBASE%\msys\bin;%PATH%
set PATH=%GISBASE%\extrabin;%PATH%
set PATH=%GISBASE%\bin;%PATH%
