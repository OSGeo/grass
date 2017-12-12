#!/usr/bin/sh

set -e

PWD="$(pwd)"

if ! [ -d mswindows ]; then
	echo Start from GRASS toplevel dir
	exit 1
fi

if ! [ -d /tmp ]; then 
    mkdir /tmp 
    if ! [ -d /tmp ]; then 
 	echo /tmp does not exists 
 	exit 1 
    fi 
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
export OSGEO4W_ROOT="C:\\\OSGeo4W${OSGEO4W_POSTFIX}"

export PATH=/usr/bin:/mingw${MINGW_POSTFIX}/bin/:$OSGEO4W_ROOT_MSYS/bin:$PWD/mswindows/osgeo4w/lib:$PWD/mswindows/osgeo4w

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

export VERSION=$MAJOR.$MINOR.$PATCH

if [[ "$PATCH" == *svn* ]] ; then
    GRASS_EXECUTABLE=grass${MAJOR}${MINOR}svn
else
    GRASS_EXECUTABLE=grass${MAJOR}${MINOR}
fi

if [ -f mswindows/osgeo4w/package.log ]; then 
    i=0 
    while [ -f mswindows/osgeo4w/package.log.$i ]; do 
 	(( i+=1 )) 
    done 
    mv mswindows/osgeo4w/package.log mswindows/osgeo4w/package.log.$i 
fi 

exec 3>&1 >> mswindows/osgeo4w/package.log 2>&1 

dll_to_a() {
        # http://sourceforge.net/apps/trac/mingw-w64/wiki/Answer%2064%20bit%20MSVC-generated%20x64%20.lib
        echo "$1 => $2"
        gendef - $1 >$2.def
	if [ "$MINGW_POSTFIX" = "64" ]; then
            dlltool --as-flags=--64 -m i386:x86-64 -k --output-lib $2.dll.a --input-def $2.def
	else
	    dlltool -k --output-lib $2.dll.a --input-def $2.def
	fi
}

log dll.to.a
[ -d mswindows/osgeo4w/lib ] || mkdir mswindows/osgeo4w/lib 
dll_to_a $OSGEO4W_ROOT_MSYS/bin/proj.dll        mswindows/osgeo4w/lib/libproj
dll_to_a $OSGEO4W_ROOT_MSYS/bin/iconv.dll       mswindows/osgeo4w/lib/libiconv
dll_to_a $OSGEO4W_ROOT_MSYS/bin/gdal202.dll     mswindows/osgeo4w/lib/libgdal
dll_to_a $OSGEO4W_ROOT_MSYS/bin/liblas_c.dll    mswindows/osgeo4w/lib/liblas_c
dll_to_a $OSGEO4W_ROOT_MSYS/bin/geos_c.dll      mswindows/osgeo4w/lib/libgeos_c
dll_to_a $OSGEO4W_ROOT_MSYS/bin/libtiff.dll     mswindows/osgeo4w/lib/libtiff
dll_to_a $OSGEO4W_ROOT_MSYS/bin/libpq.dll       mswindows/osgeo4w/lib/libpq
dll_to_a $OSGEO4W_ROOT_MSYS/bin/libmysql.dll       mswindows/osgeo4w/lib/libmysqlclient
dll_to_a $OSGEO4W_ROOT_MSYS/bin/sqlite3.dll     mswindows/osgeo4w/lib/libsqlite3
dll_to_a $OSGEO4W_ROOT_MSYS/bin/cairo.dll       mswindows/osgeo4w/lib/libcairo
dll_to_a $OSGEO4W_ROOT_MSYS/bin/libfftw3-3.dll  mswindows/osgeo4w/lib/libfftw3
dll_to_a $OSGEO4W_ROOT_MSYS/bin/pdcurses.dll  mswindows/osgeo4w/lib/libpdcurses
if [ "$MINGW_POSTFIX" = "64" ]; then
    dll_to_a $OSGEO4W_ROOT_MSYS/bin/zlib1.dll       mswindows/osgeo4w/lib/libz
    dll_to_a $OSGEO4W_ROOT_MSYS/bin/libpng16.dll    mswindows/osgeo4w/lib/libpng
    dll_to_a $OSGEO4W_ROOT_MSYS/bin/jpeg.dll        mswindows/osgeo4w/lib/libjpeg
else
# TODO (related to dependencies)
    dll_to_a $OSGEO4W_ROOT_MSYS/bin/zlib_osgeo.dll       mswindows/osgeo4w/lib/libz
#   dll_to_a $OSGEO4W_ROOT_MSYS/bin/libpng12-0.dll  mswindows/osgeo4w/lib/libpng
    dll_to_a $OSGEO4W_ROOT_MSYS/bin/jpeg_osgeo.dll  mswindows/osgeo4w/lib/libjpeg
fi

cp -uv $OSGEO4W_ROOT_MSYS/lib/libxdr.a          mswindows/osgeo4w/lib
#cp -uv $OSGEO4W_ROOT_MSYS/lib/libregex.a        mswindows/osgeo4w/lib
cp -uv $OSGEO4W_ROOT_MSYS/lib/libfreetype.dll.a mswindows/osgeo4w/lib

if ! [ -f mswindows/osgeo4w/configure-stamp ]; then

	if [ -e include/Make/Platform.make ] ; then
	    log make distclean
	    make distclean
	fi

	log remove old logs
	rm -f mswindows/osgeo4w/package.log.[0-9][0-9][0-9]

	if [ "$MINGW_POSTFIX" = "64" ]; then
	    conf_host=x86_64-w64-mingw32
            # https://trac.osgeo.org/osgeo4w/ticket/550
            # LAS support hopefully only temporarily disabled on 64bit
            conf_opts=
	else
	    conf_host=i386-w64-mingw32
            # https://trac.osgeo.org/osgeo4w/ticket/539
            #  LAS support hopefully only temporarily disabled on 32bit
            conf_opts=
	fi
	
	log configure
	LDFLAGS="-L$PWD/mswindows/osgeo4w/lib -lz" ./configure \
	        --host=$conf_host \
		--with-libs="$PWD/mswindows/osgeo4w/lib "\
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
		--with-proj-share=$OSGEO4W_ROOT_MSYS/share/proj \
		--with-gdal=$PWD/mswindows/osgeo4w/gdal-config \
		--with-geos=$PWD/mswindows/osgeo4w/geos-config \
		--with-sqlite \
		--with-regex \
		--with-nls \
		--with-freetype-includes=$OSGEO4W_ROOT_MSYS/include/freetype2 \
		--with-odbc \
	        --with-cairo \
                --with-postgres \
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
rm -f diib*

log prepare packaging
mv $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h \
    $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h.mingw
cp mswindows/osgeo4w/config.h.switch $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h
cp mswindows/osgeo4w/config.h.vc $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass
mkdir -p $OSGEO4W_ROOT_MSYS/etc/preremove $OSGEO4W_ROOT_MSYS/etc/postinstall
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT@#$OSGEO4W_ROOT#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#$OSGEO4W_ROOT#g" \
    mswindows/osgeo4w/env.bat.tmpl >$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/env.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/postinstall.bat >$OSGEO4W_ROOT_MSYS/etc/postinstall/grass${PACKAGE_POSTFIX}.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/preremove.bat >$OSGEO4W_ROOT_MSYS/etc/preremove/grass${PACKAGE_POSTFIX}.bat 

if [ -n "$PACKAGE_PATCH" ]; then
    log building vc libraries 
    OSGEO4W_POSTFIX=$OSGEO4W_POSTFIX sh \
        mswindows/osgeo4w/mklibs.sh $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/lib/*.$VERSION.dll 
    mv mswindows/osgeo4w/vc/grass*.lib $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/lib
    
    log creating package
    mkdir -p mswindows/osgeo4w/package
    
    PDIR=$PWD/mswindows/osgeo4w/package
    SRC=$PWD
    cd $OSGEO4W_ROOT_MSYS 

    # update startup script
    sed -e "s#@VERSION@#$VERSION#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
	$SRC/mswindows/osgeo4w/grass.bat.tmpl > bin/${GRASS_EXECUTABLE}.bat.tmpl
    
    # bat files - unix2dos
    unix2dos bin/${GRASS_EXECUTABLE}.bat.tmpl
    unix2dos etc/postinstall/grass${PACKAGE_POSTFIX}.bat
    unix2dos etc/preremove/grass${PACKAGE_POSTFIX}.bat
    
    # copy dependencies (TODO: to be reduced)
    if [ "$MINGW_POSTFIX" = "64" ]; then
	mingw_libgcc=libgcc_s_seh-1.dll
    else
	mingw_libgcc=libgcc_s_dw2-1.dll
    fi
    cp -uv /mingw${MINGW_POSTFIX}/bin/libintl-8.dll \
        /mingw${MINGW_POSTFIX}/bin/libiconv-2.dll \
	/mingw${MINGW_POSTFIX}/bin/libfontconfig-1.dll \
	/mingw${MINGW_POSTFIX}/bin/$mingw_libgcc \
	/mingw${MINGW_POSTFIX}/bin/libwinpthread-1.dll \
	/mingw${MINGW_POSTFIX}/bin/libexpat-1.dll \
	/mingw${MINGW_POSTFIX}/bin/libfreetype-6.dll \
	/mingw${MINGW_POSTFIX}/bin/libbz2-1.dll \
        /mingw${MINGW_POSTFIX}/bin/libharfbuzz-0.dll \
	/mingw${MINGW_POSTFIX}/bin/libglib-2.0-0.dll \
	/mingw${MINGW_POSTFIX}/bin/libpng16-16.dll \
	/mingw${MINGW_POSTFIX}/bin/libsystre-0.dll \
	/mingw${MINGW_POSTFIX}/bin/libtre-5.dll \
        /mingw${MINGW_POSTFIX}/bin/zlib1.dll \
        /mingw${MINGW_POSTFIX}/bin/libstdc++-6.dll \
	apps/grass/grass-$VERSION/bin
    cp -uv /mingw${MINGW_POSTFIX}/etc/fonts/fonts.conf \
	apps/grass/grass-$VERSION/etc
    
    # creating grass package
    tar -cjf $PDIR/grass$PACKAGE_POSTFIX-$VERSION-$PACKAGE_PATCH.tar.bz2 \
	apps/grass/grass-$VERSION \
	bin/${GRASS_EXECUTABLE}.bat.tmpl \
	etc/postinstall/grass${PACKAGE_POSTFIX}.bat \
	etc/preremove/grass${PACKAGE_POSTFIX}.bat
    
    # clean up
    rm bin/${GRASS_EXECUTABLE}.bat.tmpl
fi

log 

exit 0
