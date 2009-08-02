path %SYSTEMROOT%\system32;%SYSTEMROOT%;%SYSTEMROOT%\System32\Wbem

set VS90COMNTOOLS=%PROGRAMFILES%\Microsoft Visual Studio 9.0\Common7\Tools\
call "%PROGRAMFILES%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86
set OSGEO4W_ROOT=%PROGRAMFILES%\OSGeo4W
set P=%PATH%
call "%OSGEO4W_ROOT%"\bin\o4w_env.bat
path %P%;%OSGEO4W_ROOT%\bin

set VERSION=1.6.0
set PACKAGE=1

set P=%CD%

echo OGR: %DATE% %TIME%
cd ..\gdal-1.6\ogr\ogrsf_frmts\grass
nmake /f makefile.vc clean
nmake /f makefile.vc GRASS_VERSION=%1 plugin

echo GDAL: %DATE% %TIME%
cd ..\..\..\frmts\grass
nmake /f makefile.vc clean
nmake /f makefile.vc GRASS_VERSION=%1 plugin

cd %P%

mkdir package\gdal16-grass
tar -C %OSGEO4W_ROOT% -cjf package/gdal16-grass-%VERSION%-%PACKAGE%.tar.bz2 ^
	apps/gdal-16/bin/gdalplugins/ogr_GRASS.dll ^
	apps/gdal-16/bin/gdalplugins/gdal_GRASS.dll

:end
echo END: %DATE% %TIME%
