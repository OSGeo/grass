@echo off

rem --------------------------------------------------------------------------------------------------------------------------
rem Set the script variables
rem --------------------------------------------------------------------------------------------------------------------------

set GRASS_RELEASE_PACKAGE_DIR=.\GRASS-Release-Package
set GRASS_6_DEV_PACKAGE_DIR=.\GRASS-6-Dev-Package
set GRASS_7_DEV_PACKAGE_DIR=.\GRASS-7-Dev-Package

set OSGEO4W_DIR=c:\osgeo4w

set GRASS_RELEASE_INSTALL_FOLDER=%OSGEO4W_DIR%\apps\grass\grass-6.4.0RC4
set GRASS_6_DEV_INSTALL_FOLDER=%OSGEO4W_DIR%\apps\grass\grass-6.5.svn
set GRASS_7_DEV_INSTALL_FOLDER=%OSGEO4W_DIR%\apps\grass\grass-7.0.svn

@echo -----------------------------------------------------------------------------------------------------------------------
@echo Self Contained GRASS Automated Packager
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Edited by: Marco Pasetti
@echo Revised for OSGeo4W by: Colin Nielsen
@echo Last Update: 30 March 2009
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Select the GRASS version to pack:
@echo.
@echo 1. Current GRASS Release Version
@echo.
@echo 2. Current GRASS-6 Development Version
@echo.
@echo 3. Current GRASS-7 Development Version
@echo.

set /p SELECTION=Enter your selection (1/2/3):

if %SELECTION%==1 (
set PACKAGE_DIR=%GRASS_RELEASE_PACKAGE_DIR%
set GRASS_PREFIX=%GRASS_RELEASE_INSTALL_FOLDER%
)

if %SELECTION%==2 (
set PACKAGE_DIR=%GRASS_6_DEV_PACKAGE_DIR%
set GRASS_PREFIX=%GRASS_6_DEV_INSTALL_FOLDER%
)

if %SELECTION%==3 (
set PACKAGE_DIR=%GRASS_7_DEV_PACKAGE_DIR%
set GRASS_PREFIX=%GRASS_7_DEV_INSTALL_FOLDER%
)

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Remove the previous Selected Package and create a new PACKAGE_DIR
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

pause

if exist %PACKAGE_DIR% rmdir /S/Q %PACKAGE_DIR%
mkdir %PACKAGE_DIR%

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy %GRASS_PREFIX% content to PACKAGE_DIR
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

xcopy %GRASS_PREFIX% %PACKAGE_DIR% /S/V/F

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Extralibs to PACKAGE_DIR\extralib
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\extralib

copy %OSGEO4W_DIR%\bin\*.dll %PACKAGE_DIR%\extralib
rem copy %OSGEO4W_DIR%\pgsql\lib\libpq.dll %PACKAGE_DIR%\extralib

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Extrabins to PACKAGE_DIR\extrabin
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\extrabin

copy %OSGEO4W_DIR%\bin\*.exe %PACKAGE_DIR%\extrabin

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy SQLite content to PACKAGE_DIR\sqlite
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\sqlite
mkdir %PACKAGE_DIR%\sqlite\bin
mkdir %PACKAGE_DIR%\sqlite\include
mkdir %PACKAGE_DIR%\sqlite\lib

xcopy %OSGEO4W_DIR%\bin\sqlite3.dll %PACKAGE_DIR%\sqlite\bin /S/V/F/I
copy %OSGEO4W_DIR%\include\btree.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\fts1*.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\hash.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\keywordhash.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\opcodes.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\os.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\os_common.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\pager.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\parse.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\sqlite*.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\vdbe.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\include\vdbeInt.h %PACKAGE_DIR%\sqlite\include
copy %OSGEO4W_DIR%\lib\sqlite3_i.lib %PACKAGE_DIR%\sqlite\lib

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy GPSBABEL executable and dll to PACKAGE_DIR\gpsbabel
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\gpsbabel

copy %OSGEO4W_DIR%\gpsbabel.exe %PACKAGE_DIR%\gpsbabel
rem copy %OSGEO4W_DIR%\libexpat.dll %PACKAGE_DIR%\gpsbabel

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy shared PROJ.4 files to PACKAGE_DIR\proj
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

xcopy %OSGEO4W_DIR%\share\proj %PACKAGE_DIR%\proj /S/V/F/I

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Tcl/Tk content to PACKAGE_DIR\tcl-tk
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\tcl-tk
mkdir %PACKAGE_DIR%\tcl-tk\bin
mkdir %PACKAGE_DIR%\tcl-tk\include
mkdir %PACKAGE_DIR%\tcl-tk\lib
mkdir %PACKAGE_DIR%\tcl-tk\lib\tcl8.5
mkdir %PACKAGE_DIR%\tcl-tk\lib\tk8.5

xcopy %OSGEO4W_DIR%\bin\tclpip85.dll %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\tcl85.dll %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\tclsh.exe %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\tclsh85.exe %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\tk85.dll %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\wish.exe %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\bin\wish85.exe %PACKAGE_DIR%\tcl-tk\bin /S/V/F/I

copy %OSGEO4W_DIR%\include\tcl*.h %PACKAGE_DIR%\tcl-tk\include
copy %OSGEO4W_DIR%\include\tk*.h %PACKAGE_DIR%\tcl-tk\include
copy %OSGEO4W_DIR%\include\tommath_class.h %PACKAGE_DIR%\tcl-tk\include
copy %OSGEO4W_DIR%\include\tommath_superclass.h %PACKAGE_DIR%\tcl-tk\include
copy %OSGEO4W_DIR%\include\ttkDecls.h %PACKAGE_DIR%\tcl-tk\include

copy %OSGEO4W_DIR%\lib\tcl8.5\*.tcl %PACKAGE_DIR%\tcl-tk\lib\tcl8.5
copy %OSGEO4W_DIR%\lib\tcl8.5\tclIndex %PACKAGE_DIR%\tcl-tk\lib\tcl8.5

copy %OSGEO4W_DIR%\lib\tk8.5\*.tcl %PACKAGE_DIR%\tcl-tk\lib\tk8.5
copy %OSGEO4W_DIR%\lib\tk8.5\tclIndex %PACKAGE_DIR%\tcl-tk\lib\tk8.5

xcopy %OSGEO4W_DIR%\lib\tk8.5\ttk %PACKAGE_DIR%\tcl-tk\lib\tk8.5\ttk /S/V/F/I

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy MSYS files to PACKAGE_DIR\msys
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\msys

copy %OSGEO4W_DIR%\apps\msys\* %PACKAGE_DIR%\msys

xcopy %OSGEO4W_DIR%\apps\msys\bin %PACKAGE_DIR%\msys\bin /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\msys\doc %PACKAGE_DIR%\msys\doc /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\msys\etc %PACKAGE_DIR%\msys\etc /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\msys\info %PACKAGE_DIR%\msys\info /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\msys\lib %PACKAGE_DIR%\msys\lib /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\msys\man %PACKAGE_DIR%\msys\man /S/V/F/I
del %OSGEO4W_DIR%\apps\msys\etc\fstab


@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Python content to PACKAGE_DIR\Python25
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\Python25

copy %OSGEO4W_DIR%\apps\Python25\* %PACKAGE_DIR%\Python25

xcopy %OSGEO4W_DIR%\apps\Python25\DLLs %PACKAGE_DIR%\Python25\DLLs /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\Python25\include %PACKAGE_DIR%\Python25\include /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\Python25\Lib %PACKAGE_DIR%\Python25\Lib /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\Python25\libs %PACKAGE_DIR%\Python25\libs /S/V/F/I
xcopy %OSGEO4W_DIR%\apps\Python25\Scripts %PACKAGE_DIR%\Python25\Scripts /S/V/F/I

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Packaging Completed
@echo Remove lines 11-16 from %PACKAGE_DIR%\msys\msys.bat
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.
pause
