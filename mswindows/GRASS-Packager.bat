rem -----------------------------------------------------------------------------------------------------------------------
rem Self Contained GRASS Automated Packager
rem -----------------------------------------------------------------------------------------------------------------------
rem Edited by: Marco Pasetti
rem Revised for OSGeo4W by: Colin Nielsen, Helmut Kudrnovsky, and Martin Landa
rem Last Update: $Id$
rem -----------------------------------------------------------------------------------------------------------------------

@echo off

rem --------------------------------------------------------------------------------------------------------------------------
rem Set the script variables
rem --------------------------------------------------------------------------------------------------------------------------

set PACKAGE_DIR=.\GRASS-70-Devel-Package

set OSGEO4W_DIR=c:\osgeo4w

set GRASS_PREFIX=%OSGEO4W_DIR%\apps\grass\grass-7.0.svn
set GRASS_BIN_PREFIX=%OSGEO4W_DIR%\apps\grass\bin

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Remove the previous Selected Package and create a new PACKAGE_DIR
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

if exist %PACKAGE_DIR% rmdir /S/Q %PACKAGE_DIR%
mkdir %PACKAGE_DIR%

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy %GRASS_PREFIX% & %GRASS_BIN_PREFIX% content to PACKAGE_DIR
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

xcopy %GRASS_BIN_PREFIX%\grass70* %PACKAGE_DIR% /S/V/F
xcopy %GRASS_PREFIX% %PACKAGE_DIR% /S/V/F

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Extralibs to PACKAGE_DIR\extralib
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\extralib

copy %OSGEO4W_DIR%\bin\*.dll %PACKAGE_DIR%\extralib
del %PACKAGE_DIR%\extralib\libgrass_*6.4*.dll
del %PACKAGE_DIR%\extralib\libgrass_*6.5*.dll
del %PACKAGE_DIR%\extralib\Qt*4.dll
rem copy %OSGEO4W_DIR%\pgsql\lib\libpq.dll %PACKAGE_DIR%\extralib

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Move GRASS libs from extralib to lib
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

move %PACKAGE_DIR%\extralib\libgrass_*.dll %PACKAGE_DIR%\lib

@echo.
@echo -----------------------------------------------------------------------------------------------------------------------
@echo Copy Extrabins to PACKAGE_DIR\extrabin
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.

mkdir %PACKAGE_DIR%\extrabin

copy %OSGEO4W_DIR%\bin\*.exe %PACKAGE_DIR%\extrabin
del %PACKAGE_DIR%\extrabin\svn*.exe
del %PACKAGE_DIR%\extrabin\osgeo4w-*.exe

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
del %PACKAGE_DIR%\msys\etc\fstab
rem delete msys.bat from osgeo4w because there is an adaption (for spaces in installation path) written by GRASS-Installer.nsi
del %PACKAGE_DIR%\msys\msys.bat


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
@echo -----------------------------------------------------------------------------------------------------------------------
@echo.
