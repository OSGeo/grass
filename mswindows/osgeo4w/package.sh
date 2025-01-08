#!/bin/bash

set -e

export ARCH=x86_64-w64-mingw32
export SRC=$PWD

if ! [ -d mswindows ]; then
    echo Start from GRASS toplevel dir
    exit 1
fi

mkdir -p /tmp
if ! [ -d /tmp ]; then
    echo /tmp does not exists
    exit 1
fi

# package patch number
# e.g. 'r65400-1' for daily builds, '-1' for release
if [ -z  $PACKAGE_PATCH ]; then
    PACKAGE_PATCH=1
fi

# package name
# eg. '-daily' -> 'grass-daily', empty for release
if [ -z $PACKAGE_POSTFIX ]; then
    PACKAGE_POSTFIX=""
fi

[ -n "$OSGEO4W_ROOT_MSYS" ]
echo "OSGEO4W_ROOT_MSYS:$OSGEO4W_ROOT_MSYS OSGEO4W_ROOT:$OSGEO4W_ROOT"

fetchenv() {
    local IFS
    IFS=
    batch=$1
    shift
    srcenv=$(mktemp /tmp/srcenv.XXXXXXXXXX)
    dstenv=$(mktemp /tmp/dstenv.XXXXXXXXXX)
    diffenv=$(mktemp /tmp/diffenv.XXXXXXXXXX)
    args="$@"
    cmd.exe //c set >$srcenv
    cmd.exe //c "call `cygpath -sw $batch` $args \>nul 2\>nul \& set" >$dstenv
    diff -u $srcenv $dstenv | sed -f mswindows/osgeo4w/envdiff.sed >$diffenv
    . $diffenv
    PATH=$PATH:/usr/bin:/mingw64/bin/:$PWD/mswindows/osgeo4w/lib:$PWD/mswindows/osgeo4w:/c/windows32/system32:/c/windows:/c/windows32/system32:/c/windows
    rm -f $srcenv $dstenv $diffenv
}

fetchenv $OSGEO4W_ROOT_MSYS/bin/o4w_env.bat

PATH=/mingw64/lib/ccache/bin:$PATH

T0=$(date +%s)
LT=$T0
CS=""

log() {
    local D T
    NOW=$(date)
    T=$(date +%s)

    if [ -n "$CS" ]; then
        local D H M S
	S=$(( $T-$LT ))
	M=$(( S/60 )); S=$(( S%60 ))
	H=$(( M/60 )); M=$(( M%60 ))
	D=$(( H/24 )); H=$(( H%24 ))

	echo -n "$NOW: FINISHED $CS AFTER "
	(( D>0 )) && echo -n "${D}d"
	(( H>0 )) && echo -n "${H}h"
	(( M>0 )) && echo -n "${M}m"
	echo "${S}s"
    fi

    CS="$@"
    LT=$T
    if [ -n "$CS" ]; then
        echo $NOW: STARTING $CS
    elif [ -n "$T0" ]; then
	CS="COMPLETE RUN"
	LT=$T0
	T0=""
	log
    fi
}

exec 3<include/VERSION
read MAJOR <&3
read MINOR <&3
read PATCH <&3

export VERSION=${MAJOR}.${MINOR}.${PATCH}
export POSTFIX=${MAJOR}${MINOR}

GRASS_EXECUTABLE=grass${MAJOR}${MINOR}

if [ -f mswindows/osgeo4w/package.log ]; then
    i=0
    while [ -f mswindows/osgeo4w/package.log.$i ]; do
	(( i+=1 ))
    done
    mv mswindows/osgeo4w/package.log mswindows/osgeo4w/package.log.$i
fi

exec 3>&1 > >(tee mswindows/osgeo4w/package.log) 2>&1

if ! [ -f mswindows/osgeo4w/configure-stamp ]; then
	if [ -e include/Make/Platform.make ] ; then
	    log make distclean
	    make distclean
	fi

	log remove old logs
	rm -f mswindows/osgeo4w/package.log.*

	mkdir -p mswindows/osgeo4w/lib
	cp -uv $OSGEO4W_ROOT_MSYS/lib/sqlite3_i.lib mswindows/osgeo4w/lib/sqlite3.lib


	log configure
	CFLAGS="$CFLAGS -pipe" \
	CXXFLAGS="$CXXFLAGS -pipe" \
	./configure \
		--bindir=${OSGEO4W_ROOT_MSYS}/bin \
		--enable-largefile \
		--enable-shared \
		--host=${ARCH} \
		--includedir=${OSGEO4W_ROOT_MSYS}/include \
		--libexecdir=${OSGEO4W_ROOT_MSYS}/bin \
		--prefix=${OSGEO4W_ROOT_MSYS}/apps/grass \
		--with-blas \
		--with-bzlib \
		--with-cairo \
		--with-cairo-includes=${OSGEO4W_ROOT_MSYS}/include \
		--with-cairo-ldflags="-L${SRC}/mswindows/osgeo4w/lib -lcairo" \
		--with-cairo-libs=${OSGEO4W_ROOT_MSYS}/lib \
		--with-cxx \
		--with-fftw \
		--with-freetype \
		--with-freetype-includes=${OSGEO4W_ROOT_MSYS}/include/freetype2 \
		--with-gdal=${SRC}/mswindows/osgeo4w/gdal-config \
		--with-geos=${SRC}/mswindows/osgeo4w/geos-config \
		--with-includes=${OSGEO4W_ROOT_MSYS}/include \
		--with-lapack \
		--with-liblas=${SRC}/mswindows/osgeo4w/liblas-config \
		--with-libpng=${SRC}/mswindows/osgeo4w/libpng-config \
		--with-libs="${OSGEO4W_ROOT_MSYS}/lib ${OSGEO4W_ROOT_MSYS}/bin" \
		--with-netcdf=${OSGEO4W_ROOT_MSYS}/bin/nc-config \
		--with-nls \
		--with-odbc \
		--with-opengl=windows \
		--with-openmp \
		--with-postgres \
		--with-postgres-includes=${OSGEO4W_ROOT_MSYS}/include \
		--with-postgres-libs=${OSGEO4W_ROOT_MSYS}/lib \
		--with-proj-includes=${OSGEO4W_ROOT_MSYS}/include \
		--with-proj-libs=${OSGEO4W_ROOT_MSYS}/lib \
		--with-proj-share=${OSGEO4W_ROOT_MSYS}/share/proj \
		--with-readline \
		--with-regex \
		--with-sqlite \
		--with-sqlite-includes=${OSGEO4W_ROOT_MSYS}/include \
		--with-sqlite-libs=${OSGEO4W_ROOT_MSYS}/lib \
		--with-zstd \
		--without-pdal \
		--without-x

	touch mswindows/osgeo4w/configure-stamp
fi


log make
make -k || ( cat error.log >&3 && false )

log make install
make install

log cleanup
rm -f d*.o

log prepare packaging
mv $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/include/grass/config.h \
   $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/include/grass/config.h.mingw
cp mswindows/osgeo4w/config.h.switch $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/include/grass/config.h
cp mswindows/osgeo4w/config.h.vc $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/include/grass
# rename grass.py to avoid ModuleNotFoundError
mv $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/etc/grass.py $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/etc/grass${POSTFIX}.py

mkdir -p $OSGEO4W_ROOT_MSYS/etc/preremove $OSGEO4W_ROOT_MSYS/etc/postinstall
sed -e "s#@POSTFIX@#$POSTFIX#g" \
    mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}.bat
sed -e "s#@POSTFIX@#$POSTFIX#g" \
    mswindows/osgeo4w/python-grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/python-${GRASS_EXECUTABLE}.bat
sed -e "s#@POSTFIX@#$POSTFIX#g" \
    mswindows/osgeo4w/env.bat.tmpl >$OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/etc/env.bat
sed -e "s#@POSTFIX@#$POSTFIX#g" -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/postinstall.bat >$OSGEO4W_ROOT_MSYS/etc/postinstall/grass${PACKAGE_POSTFIX}.bat
sed -e "s#@POSTFIX@#$POSTFIX#g" -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/preremove.bat >$OSGEO4W_ROOT_MSYS/etc/preremove/grass${PACKAGE_POSTFIX}.bat

if [ -n "$PACKAGE_PATCH" ]; then
    log building vc libraries
    sh mswindows/osgeo4w/mklibs.sh $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/lib/*.${MAJOR}.${MINOR}.dll
    mv mswindows/osgeo4w/vc/grass*.lib $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/lib

    log creating package
    mkdir -p mswindows/osgeo4w/package

    PDIR=$PWD/mswindows/osgeo4w/package
    SRC=$PWD
    cd $OSGEO4W_ROOT_MSYS

    # bat files - unix2dos
    unix2dos bin/${GRASS_EXECUTABLE}.bat
    unix2dos bin/python-${GRASS_EXECUTABLE}.bat
    unix2dos etc/postinstall/grass${PACKAGE_POSTFIX}.bat
    unix2dos etc/preremove/grass${PACKAGE_POSTFIX}.bat

    # copy dependencies
    cp -uv $(/usr/bin/find apps/grass/grass$POSTFIX -iname "*.dll" -o -iname "*.exe" | PATH=$PWD/apps/grass/grass$POSTFIX/lib:$PWD/bin:/mingw64/bin:/usr/bin /usr/bin/xargs /usr/bin/ldd | /usr/bin/sed -ne 's#^.* => \(/mingw64/bin/.*\) (.*)$#\1#p' | /usr/bin/sort -u) apps/grass/grass$POSTFIX/bin

    # copy R batch files
    cp -uv $SRC/mswindows/external/rbatch/* apps/grass/grass$POSTFIX/bin

    # creating grass package
    /bin/tar -cjf $PDIR/grass$PACKAGE_POSTFIX-$VERSION-$PACKAGE_PATCH.tar.bz2 \
	apps/grass/grass$POSTFIX \
	bin/${GRASS_EXECUTABLE}.bat \
	bin/python-${GRASS_EXECUTABLE}.bat \
	etc/postinstall/grass${PACKAGE_POSTFIX}.bat \
	etc/preremove/grass${PACKAGE_POSTFIX}.bat
fi

log

exit 0
