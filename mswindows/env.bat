rem Environmental variables for GRASS stand-alone installer

set GRASS_PYTHON=%GISBASE%\extrabin\python.exe
set GRASS_PROJSHARE=%GISBASE%\proj
set GRASS_HTML_BROWSER=explorer

set PYTHONHOME=%GISBASE%\Python27
set GDAL_DATA=%GISBASE%\share\gdal
set PROJ_LIB=%GISBASE%\proj
set GEOTIFF_CSV=%GISBASE%\share\epsg_csv

set PATH=%GISBASE%\msys\bin;%PATH%
set PATH=%GISBASE%\extrabin;%GISBASE%\extralib;%PATH%
set PATH=%GISBASE%\sqlite\bin;%PATH%
