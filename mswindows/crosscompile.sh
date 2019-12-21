#!/bin/sh
#
# Script for building a cross-compiled portable package of GRASS GIS
#
# Written by Huidae Cho
#
# Requires MXE <https://mxe.cc/> for cross-compilation
#
# Tested on Slackware 14.2 x86_64 with up-to-date packages from slackpkg and
# sbopkg
#
# Basic steps:
#
# mkdir -p ~/usr/src
# cd ~/usr/src
# git clone https://github.com/mxe/mxe.git
# cd mxe
# echo MXE_TARGETS=x86_64-w64-mingw32.shared > settings.mk
# make cc blas bzip2 cairo fftw freetype gdal geos lapack netcdf libpng \
#      pthreads readline libgnurx sqlite tiff zstd proj
#
# cd ~/usr/src
# git clone https://github.com/OSGeo/grass.git
# cd grass
# crosscompile.sh --pull --package > crosscompile.log 2>&1
#

# stop on errors
set -e

# default paths, but can be overriden from the command line
MXE=${MXE-$HOME/usr/local/src/mxe}
FREETYPE_INC=${FREETYPE_INC-/usr/include/freetype2}

# process options
pull=0
package=0
for opt; do
	case $opt in
	-h|--help)
		cat<<'EOT'
Usage: crosscompile.sh [OPTIONS]

-h, --help               display this help message
    --mxe=PATH           MXE path (default: $HOME/usr/local/src/mxe)
    --freetype-inc=PATH  FreeType include path (default: /usr/include/freetype2)
    --pull               update the current branch
    --package            package the cross-compiled build as
                         grass79-x86_64-w64-mingw32-YYYYMMDD.zip
EOT
		exit
		;;
	--pull)
		pull=1
		;;
	--package)
		package=1
		;;
	--mxe=*)
		MXE=`echo $opt | sed 's/^--mxe=//'`
		;;
	--freetype-inc=*)
		FREETYPE_INC=`echo $opt | sed 's/^--freetype-inc=//'`
		;;
	*)
		echo "$opt: unknown option"
		exit 1
		;;
	esac
done

# see if we're inside the root of the GRASS source code
if [ ! -e grass.pc.in ]; then
	echo "Please run this script from the root of the GRASS source code"
	exit 1
fi

# check paths
errors=0
if [ ! -e $MXE ]; then
	echo "$MXE: not found"
	errors=1
fi
if [ ! -e $FREETYPE_INC ]; then
	echo "$FREETYPE_INC: not found"
	errors=1
fi
if [ $errors -eq 1 ]; then
	exit 1
fi

# update the current branch if requested
if [ $pull -eq 1 ]; then
	if [ ! -e .git ]; then
		echo "not a git repository"
		exit 1
	fi
	git pull
fi

################################################################################
# Compile the native architecture for generating document files

CFLAGS="-g -O2 -Wall" \
CXXFLAGS="-g -O2 -Wall" \
LDFLAGS="-lcurses" \
./configure \
--with-nls \
--with-cxx \
--with-pthread \
--with-blas \
--with-lapack \
--with-openmp \
--with-postgres \
--with-sqlite \
--with-motif \
--with-freetype \
--with-freetype-includes=$FREETYPE_INC \
--with-readline \
--with-python \
--with-wxwidgets \
--with-geos \
--with-netcdf \
--with-zstd \
--with-liblas \
--with-bzlib \
--with-pdal \
> /dev/stdout

make clean default

BUILD_ARCH=`sed -n '/^ARCH[ \t]*=/{s/^.*=[ \]*//; p}' include/Make/Platform.make`
for i in \
	config.log \
	include/Make/Platform.make \
	include/Make/Doxyfile_arch_html \
	include/Make/Doxyfile_arch_latex \
	error.log \
; do
	cp -a $i $i.$BUILD_ARCH
done

################################################################################
# Cross-compile the target architecture

ARCH=x86_64-w64-mingw32
SHARED=$ARCH.shared
MXE_BIN=$MXE/usr/bin/$SHARED
MXE_SHARED=$MXE/usr/$SHARED

CC=$MXE_BIN-gcc \
CXX=$MXE_BIN-g++ \
CFLAGS="-g -O2 -Wall" \
CXXFLAGS="-g -O2 -Wall" \
AR=$MXE_BIN-ar \
RANLIB=$MXE_BIN-ranlib \
WINDRES=$MXE_BIN-windres \
PKG_CONFIG=$MXE_BIN-pkg-config \
./configure \
--host=$ARCH \
--with-nls \
--with-pthread \
--with-blas \
--with-lapack \
--with-openmp \
--with-postgres \
--with-sqlite \
--with-cairo-includes=$MXE_SHARED/include/cairo \
--with-freetype \
--with-freetype-includes=$MXE_SHARED/include/freetype2 \
--with-readline \
--with-python \
--with-wxwidgets \
--with-geos=$MXE_SHARED/bin/geos-config \
--with-netcdf=$MXE_SHARED/bin/nc-config \
--with-zstd \
--with-bzlib \
--with-gdal=$MXE_SHARED/bin/gdal-config \
--with-opengl=windows \
> /dev/stdout

make clean default

ARCH=`sed -n '/^ARCH[ \t]*=/{s/^.*=[ \]*//; p}' include/Make/Platform.make`
for i in \
	config.log \
	include/Make/Platform.make \
	include/Make/Doxyfile_arch_html \
	include/Make/Doxyfile_arch_latex \
	error.log \
; do
	cp -a $i $i.$BUILD_ARCH
done

################################################################################
# Copy document files from the native build

BUILD_DIST=dist.$BUILD_ARCH
DIST=dist.$ARCH

for i in \
	docs \
	gui/wxpython/xml \
; do
	rm -rf $DIST/$i
	cp -a $BUILD_DIST/$i $DIST/$i
done

################################################################################
# Copy MXE files

for i in \
	libblas.dll \
	libbz2.dll \
	libcairo-2.dll \
	libcrypto-1_1-x64.dll \
	libcurl-4.dll \
	libdf-0.dll \
	libexpat-1.dll \
	libfftw3-3.dll \
	libfontconfig-1.dll \
	libfreetype-6.dll \
	libfreexl-1.dll \
	libgcc_s_seh-1.dll \
	libgcrypt-20.dll \
	libgdal-20.dll \
	libgeos-3-6-2.dll \
	libgeos_c-1.dll \
	libgeotiff-2.dll \
	libgfortran-3.dll \
	libgif-7.dll \
	libglib-2.0-0.dll \
	libgnurx-0.dll \
	libgomp-1.dll \
	libgpg-error-0.dll \
	libgta-0.dll \
	libharfbuzz-0.dll \
	libhdf5-8.dll \
	libhdf5_hl-8.dll \
	libiconv-2.dll \
	libidn2-0.dll \
	libintl-8.dll \
	libjpeg-9.dll \
	libjson-c-4.dll \
	liblapack.dll \
	liblzma-5.dll \
	libmfhdf-0.dll \
	libmysqlclient.dll \
	libnetcdf.dll \
	libopenjp2.dll \
	libpcre-1.dll \
	libpixman-1-0.dll \
	libpng16-16.dll \
	libportablexdr-0.dll \
	libpq.dll \
	libproj-13.dll \
	libquadmath-0.dll \
	libreadline8.dll \
	libspatialite-7.dll \
	libsqlite3-0.dll \
	libssh2-1.dll \
	libssl-1_1-x64.dll \
	libstdc++-6.dll \
	libtermcap.dll \
	libtiff-5.dll \
	libunistring-2.dll \
	libwebp-7.dll \
	libwinpthread-1.dll \
	libxml2-2.dll \
	libzstd.dll \
	zlib1.dll \
; do
	cp -a $MXE_SHARED/bin/$i $DIST/lib
done

for i in \
	proj \
	gdal \
; do
	rm -rf $DIST/share/$i
	cp -a $MXE_SHARED/share/$i $DIST/share/$i
done

################################################################################
# Post-compile process

VERSION=`sed -n '/^INST_DIR/{s/^INST_DIR.*grass//; p}' include/Make/Platform.make`

rm -f $DIST/grass$VERSION.tmp
cp -a bin.$ARCH/grass$VERSION.py $DIST/etc

cat<<'EOT' > $DIST/grass$VERSION.bat
@echo off

rem Change this variable to override auto-detection of python.exe in PATH
set GRASS_PYTHON=C:\Python38\python.exe

rem For portable installation, use %~d0 for the changing drive letter
rem set GRASS_PYTHON=%~d0\Python38\python.exe

set GISBASE=%~dp0
set GRASS_PROJSHARE=%GISBASE%\share\proj

set PROJ_LIB=%GISBASE%\share\proj
set GDAL_DATA=%GISBASE%\share\gdal

rem XXX: Do we need these variables?
rem set GEOTIFF_CSV=%GISBASE%\share\epsg_csv
rem set FONTCONFIG_FILE=%GISBASE%\etc\fonts.conf

if not exist %GISBASE%\etc\fontcap (
	pushd .
	set GISRC=dummy
	cd %GISBASE%\lib
	%GISBASE%\bin\g.mkfontcap.exe
	popd
)

if not exist "%GRASS_PYTHON%" (
	set GRASS_PYTHON=
	for /f usebackq %%i in (`where python.exe`) do set GRASS_PYTHON=%%i
)
if "%GRASS_PYTHON%"=="" (
	echo.
	echo python.exe not found in PATH
	echo Please set GRASS_PYTHON in %~f0
	echo.
	pause
	goto:eof
)
rem XXX: Do we need PYTHONHOME?
rem for %%i in (%GRASS_PYTHON%) do set PYTHONHOME=%%~dpi

"%GRASS_PYTHON%" "%GISBASE%\etc\grass79.py" %*
if %ERRORLEVEL% geq 1 pause
EOT
unix2dos $DIST/grass$VERSION.bat

# package if requested
if [ $package -eq 1 ]; then
	DATE=`date +%Y%m%d`
	rm -f grass
	ln -s $DIST grass
	rm -f grass*-$ARCH-*.zip
	zip -r grass$VERSION-$ARCH-$DATE.zip grass
	rm -f grass
fi
