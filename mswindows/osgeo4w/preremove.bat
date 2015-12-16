set BATCH=%OSGEO4W_ROOT%\bin\@GRASS_EXECUTABLE@.bat

del "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@.lnk"
del "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@.lnk"

del "%BATCH%"
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\etc\fontcap

del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libintl-8.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libiconv-2.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libfontconfig-1.dll
if exist "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libgcc_s_seh-1.dll "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libgcc_s_seh-1.dll
if exist "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libgcc_s_dw2-1.dll "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libgcc_s_dw2-1.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libwinpthread-1.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libexpat-1.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libfreetype-6.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libbz2-1.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libharfbuzz-0.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libglib-2.0-0.dll
del "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\lib\libpng16-16.dll
