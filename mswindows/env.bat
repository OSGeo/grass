set WINGISBASE=%GISBASE%

rem Name of the wish (Tk) executable
set GRASS_WISH=wish.exe

rem Path to the shell command
set GRASS_SH=%GISBASE%\msys\bin\sh.exe

rem Set Path to utilities (libraries and binaries) used by GRASS
set PATH=%GISBASE%\msys\bin;%PATH%
set PATH=%GISBASE%\extrabin;%GISBASE%\extralib;%PATH%
set PATH=%GISBASE%\tcl-tk\bin;%GISBASE%\sqlite\bin;%GISBASE%\gpsbabel;%PATH%
set PATH=%GISBASE%\bin;%GISBASE%\scripts;%PATH%

rem Set Path to default web browser
set GRASS_HTML_BROWSER=explorer

rem Path to the proj files (notably the epsg projection list)
set GRASS_PROJSHARE=%GISBASE%\proj

rem Set GDAL_DATA
set GDAL_DATA=%GISBASE%\share\gdal

rem Set PROJ_LIB
set PROJ_LIB=%GISBASE%\proj

rem Set GEOTIFF_CSV
set GEOTIFF_CSV=%GISBASE%\share\epsg_csv

rem Path to the python directory
set PYTHONHOME=%GISBASE%\Python25
if "x%GRASS_PYTHON%" == "x" set GRASS_PYTHON=python
