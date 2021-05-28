#!/usr/bin/bash

# osgeo4w-setup -g -k -a x86_64 -q -P gdal -P proj -P geos -P fftw -P libjpeg -P liblas-devel -P libpng -P libpq -P libtiff -P libxdr -P pdcurses -P regex-devel -P sqlite3 -P zstd-devel -P zstd -P laszip2 -P python3-core -P python3-six

set -e

PWD="$(pwd)"

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

# OSGeo4W directory postfix
# eg. '64' for 64bit, empty for 32bit
if [ -z $OSGEO4W_POSTFIX ]; then
    OSGEO4W_POSTFIX=""
fi
if [ "$OSGEO4W_POSTFIX" = "64" ]; then
    MINGW_POSTFIX=64
else
    MINGW_POSTFIX=32
fi

export OSGEO4W_ROOT_MSYS="/c/OSGeo4W${OSGEO4W_POSTFIX}"
export OSGEO4W_ROOT=$(cygpath -w "$OSGEO4W_ROOT_MSYS")

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
    cmd.exe //c "call `cygpath -w $batch` $args \>nul 2\>nul \& set" >$dstenv
    diff -u $srcenv $dstenv | sed -f mswindows/osgeo4w/envdiff.sed >$diffenv
    . $diffenv
    PATH=$PATH:/usr/bin:/mingw${MINGW_POSTFIX}/bin/:$PWD/mswindows/osgeo4w/lib:$PWD/mswindows/osgeo4w:/c/windows32/system32:/c/windows:/c/windows32/system32:/c/windows
    rm -f $srcenv $dstenv $diffenv
}

# Avoid GRASS' old msys
! [ -f $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat ] || mv $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat.off

fetchenv $OSGEO4W_ROOT_MSYS/bin/o4w_env.bat
fetchenv $OSGEO4W_ROOT_MSYS/bin/py3_env.bat

! [ -f $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat.off ] || mv $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat.off $OSGEO4W_ROOT_MSYS/etc/ini/msys.bat

PATH=/mingw${MINGW_POSTFIX}/lib/ccache/bin:$PATH

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

if [ "$MINGW_POSTFIX" = "64" ]; then
	mingw_libgcc=libgcc_s_seh-1.dll
else
	mingw_libgcc=libgcc_s_dw2-1.dll
fi

DLLS="/mingw${MINGW_POSTFIX}/bin/zlib1.dll
	/mingw${MINGW_POSTFIX}/bin/libbz2-1.dll
	/mingw${MINGW_POSTFIX}/bin/libiconv-2.dll
	/mingw${MINGW_POSTFIX}/bin/libexpat-1.dll
	/mingw${MINGW_POSTFIX}/bin/libfontconfig-1.dll
	/mingw${MINGW_POSTFIX}/bin/libintl-8.dll
	/mingw${MINGW_POSTFIX}/bin/libsystre-0.dll
	/mingw${MINGW_POSTFIX}/bin/libtre-5.dll
	/mingw${MINGW_POSTFIX}/bin/libwinpthread-1.dll
	/mingw${MINGW_POSTFIX}/bin/libcairo-2.dll
	/mingw${MINGW_POSTFIX}/bin/libpixman-1-0.dll
	/mingw${MINGW_POSTFIX}/bin/libpng16-16.dll
	/mingw${MINGW_POSTFIX}/bin/libfreetype-6.dll
	/mingw${MINGW_POSTFIX}/bin/libharfbuzz-0.dll
	/mingw${MINGW_POSTFIX}/bin/libglib-2.0-0.dll
	/mingw${MINGW_POSTFIX}/bin/libgraphite2.dll
	/mingw${MINGW_POSTFIX}/bin/libpcre-1.dll
	/mingw${MINGW_POSTFIX}/bin/libstdc++-6.dll
	/mingw${MINGW_POSTFIX}/bin/$mingw_libgcc"

if [ "$MINGW_POSTFIX" = "64" ]; then
	conf_host=x86_64-w64-mingw32
	# https://trac.osgeo.org/osgeo4w/ticket/550
	conf_opts="--with-liblas=$PWD/mswindows/osgeo4w/liblas-config"
else
	conf_host=i386-w64-mingw32
	# https://trac.osgeo.org/osgeo4w/ticket/539
	#  LAS support hopefully only temporarily disabled on 32bit
	conf_opts=
fi

if ! [ -f mswindows/osgeo4w/configure-stamp ]; then
	if [ -e include/Make/Platform.make ] ; then
	    log make distclean
	    make distclean
	fi

	log remove old logs
	rm -f mswindows/osgeo4w/package.log.*

	mkdir -p dist.$conf_host/bin
	cp -uv $DLLS dist.$conf_host/bin

	mkdir -p mswindows/osgeo4w/lib
	cp -uv $OSGEO4W_ROOT_MSYS/lib/libpq.lib mswindows/osgeo4w/lib/pq.lib
	cp -uv $OSGEO4W_ROOT_MSYS/lib/proj_i.lib mswindows/osgeo4w/lib/proj.lib
	cp -uv $OSGEO4W_ROOT_MSYS/lib/sqlite3_i.lib mswindows/osgeo4w/lib/sqlite3.lib

	log configure
	./configure \
	        --host=$conf_host \
		--with-libs="$OSGEO4W_ROOT/lib" \
		--with-includes=$OSGEO4W_ROOT_MSYS/include \
                --libexecdir=$OSGEO4W_ROOT_MSYS/bin \
                --prefix=$OSGEO4W_ROOT_MSYS/apps/grass \
                --bindir=$OSGEO4W_ROOT_MSYS/bin \
                --includedir=$OSGEO4W_ROOT_MSYS/include \
		--without-x \
		--with-cxx \
		--enable-shared \
		--enable-largefile \
		--with-fftw \
		--with-freetype \
		--with-freetype-includes=/mingw${MINGW_POSTFIX}/include/freetype2 \
		--with-proj-share=$OSGEO4W_ROOT_MSYS/share/proj \
		--with-proj-includes=$OSGEO4W_ROOT_MSYS/include \
		--with-proj-libs=$PWD/mswindows/osgeo4w/lib \
		--with-postgres \
		--with-postgres-includes=$OSGEO4W_ROOT_MSYS/include \
		--with-postgres-libs=$PWD/mswindows/osgeo4w/lib \
		--with-gdal=$PWD/mswindows/osgeo4w/gdal-config \
		--with-geos=$PWD/mswindows/osgeo4w/geos-config \
		--with-sqlite \
		--with-sqlite-includes=$OSGEO4W_ROOT_MSYS/include \
		--with-sqlite-libs=$PWD/mswindows/osgeo4w/lib \
		--with-regex \
		--with-nls \
		--with-zstd \
		--with-odbc \
	        --with-cairo \
	        --with-opengl=windows \
                --with-bzlib $conf_opts
# see #3047
#	        --with-mysql

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
    OSGEO4W_POSTFIX=$OSGEO4W_POSTFIX sh \
        mswindows/osgeo4w/mklibs.sh $OSGEO4W_ROOT_MSYS/apps/grass/grass$POSTFIX/lib/*.${MAJOR}.${MINOR}.dll
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

    # copy dependencies (TODO: to be reduced)
    cp -uv $DLLS apps/grass/grass$POSTFIX/bin
    cp -uv /mingw${MINGW_POSTFIX}/etc/fonts/fonts.conf \
	apps/grass/grass$POSTFIX/etc

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
