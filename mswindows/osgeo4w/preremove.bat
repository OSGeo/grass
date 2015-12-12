del "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\GRASS @VERSION@ GUI.lnk"
del "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\GRASS @VERSION@ Command Line.lnk"
rmdir "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@" 

del "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@.lnk"

del "%OSGEO4W_ROOT%"\bin\@GRASS_EXECUTABLE@.bat
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\etc\fontcap

del "%OSGEO4W_ROOT%"\bin\libintl-8.dll
del "%OSGEO4W_ROOT%"\bin\libiconv-2.dll
del "%OSGEO4W_ROOT%"\bin\libfontconfig-1.dll
if exist "%OSGEO4W_ROOT%"\bin\libgcc_s_seh-1.dll "%OSGEO4W_ROOT%"\bin\libgcc_s_seh-1.dll
if exist "%OSGEO4W_ROOT%"\bin\libgcc_s_dw2-1.dll "%OSGEO4W_ROOT%"\bin\libgcc_s_dw2-1.dll
del "%OSGEO4W_ROOT%"\bin\libwinpthread-1.dll
del "%OSGEO4W_ROOT%"\bin\libexpat-1.dll
del "%OSGEO4W_ROOT%"\bin\libfreetype-6.dll
del "%OSGEO4W_ROOT%"\bin\libbz2-1.dll
del "%OSGEO4W_ROOT%"\bin\libharfbuzz-0.dll
del "%OSGEO4W_ROOT%"\bin\libglib-2.0-0.dll
del "%OSGEO4W_ROOT%"\bin\libpng16-16.dll
