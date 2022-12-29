set ICON=%OSGEO4W_ROOT%\apps\grass\grass@POSTFIX@\gui\icons\grass_osgeo.ico
set BATCH=%OSGEO4W_ROOT%\bin\@GRASS_EXECUTABLE@.bat
textreplace -std -t "%BATCH%"
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\grass@POSTFIX@\etc\fontcap

if not %OSGEO4W_MENU_LINKS%==0 xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@.lnk" "%BATCH%"  "--gui" \ "Launch GRASS GIS @VERSION@" 1 "%ICON%" 
if not %OSGEO4W_DESKTOP_LINKS%==0 xxmklink "%OSGEO4W_DESKTOP%\GRASS GIS @VERSION@.lnk" "%BATCH%"  "--gui" \ "Launch GRASS GIS @VERSION@" 1 "%ICON%" 

rem run g.mkfontcap outside a GRASS session during
rem an OSGeo4W installation for updating paths to fonts

rem set gisbase
set GISBASE=%OSGEO4W_ROOT%\apps\grass\grass@POSTFIX@

rem set path to freetype dll and its dependencies
set FREETYPEBASE=%OSGEO4W_ROOT%\bin;%OSGEO4W_ROOT%\apps\msys\bin;%GISBASE%\lib

rem set dependencies to path
set PATH=%FREETYPEBASE%;%PATH%

rem GISRC must be set
set GISRC=dummy

rem run g.mkfontcap outside a GRASS session
"%GISBASE%\bin\g.mkfontcap.exe" -o

del "%BATCH%.tmpl
