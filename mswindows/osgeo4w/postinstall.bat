set ICON=%OSGEO4W_ROOT%\apps\grass\grass-@VERSION@\gui\icons\grass_osgeo.ico
set BATCH=%OSGEO4W_ROOT%\bin\@GRASS_EXECUTABLE@.bat
textreplace -std -t "%BATCH%"
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\etc\fontcap

xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@.lnk"       "%BATCH%"  "-gui" \ "Launch GRASS GIS @VERSION@" 1 "%ICON%" 
xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@.lnk" "%BATCH%"  "-gui" \ "Launch GRASS GIS @VERSION@" 1 "%ICON%" 

rem run g.mkfontcap outside a GRASS session during
rem an OSGeo4W installation for updating paths to fonts

rem set gisbase
set GISBASE=%OSGEO4W_ROOT%\apps\grass\grass-@VERSION@

rem set path to freetype dll and its dependencies
set FREETYPEBASE=%OSGEO4W_ROOT%\bin;%OSGEO4W_ROOT%\apps\msys\bin;%GISBASE%\lib

rem set dependencies to path
set PATH=%FREETYPEBASE%;%PATH%

rem GISRC must be set
set GISRC=dummy

rem run g.mkfontcap outside a GRASS session
"%GISBASE%\bin\g.mkfontcap.exe" -o

del "%BATCH%.tmpl
