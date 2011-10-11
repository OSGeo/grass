#!/c/OSGeo4W/apps/msys/bin/sh

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

export PACKAGE=${1:-1} 
export OSGEO4W_ROOT_MSYS="/c/OSGeo4W"
export OSGEO4W_ROOT="C:\\\OSGeo4W"
export PATH=.:/c/mingw/bin:/usr/local/bin:/bin:$OSGEO4W_ROOT_MSYS/bin:/c/WINDOWS/system32:/c/WINDOWS:/c/WINDOWS/System32/Wbem:/c/Subversion:$PWD/mswindows/osgeo4w

T0=$(date +%s) 
LT=$T0 
CS="" 

log() { 
    local D T 
    NOW=$(date) 
    T=$(date +%s) 
    
    if [ -n "$CS" ]; then 
        local D H M S 
 	(( S=T-$LT )) 
 	(( M=S/60 )); (( S=S%60 )) 
 	(( H=M/60 )); (( M=M%60 )) 
 	(( D=H/24 )); (( H=H%24 )) 
 	
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

export GRASS_PYTHON="/c/OSGeo4W/bin/python.exe"
export PYTHONHOME="/c/OSGeo4W/apps/Python25"

if [ -f mswindows/osgeo4w/package.log ]; then 
    i=0 
    while [ -f mswindows/osgeo4w/package.log.$i ]; do 
 	(( i++ )) 
    done 
    mv mswindows/osgeo4w/package.log mswindows/osgeo4w/package.log.$i 
fi 

exec 3>&1 >> mswindows/osgeo4w/package.log 2>&1 

[ -d mswindows/osgeo4w/lib ] || mkdir mswindows/osgeo4w/lib 
cp -uv $OSGEO4W_ROOT_MSYS/lib/sqlite3_i.lib mswindows/osgeo4w/lib/libsqlite3.a 
cp -uv $OSGEO4W_ROOT_MSYS/lib/proj.lib mswindows/osgeo4w/lib/libproj.a 
cp -uv $OSGEO4W_ROOT_MSYS/lib/libtiff_i.lib mswindows/osgeo4w/lib/libtiff.a 
cp -uv $OSGEO4W_ROOT_MSYS/lib/libpq.lib mswindows/osgeo4w/lib/libpq.a 
cp -uv $OSGEO4W_ROOT_MSYS/lib/jpeg_i.lib mswindows/osgeo4w/lib/libjpeg.a 
cp -uv $OSGEO4W_ROOT_MSYS/lib/zlib.lib mswindows/osgeo4w/lib/libz.a 

if ! [ -f mswindows/osgeo4w/configure-stamp ]; then

	if [ -e include/Make/Platform.make ] ; then
	    log make distclean
	    make distclean
	fi

	log remove old logs
	rm -f mswindows/osgeo4w/package.log.[0-9][0-9][0-9]

	log configure
	./configure \
		--with-libs="$OSGEO4W_ROOT_MSYS/lib $PWD/mswindows/osgeo4w/lib" \
		--with-includes=$OSGEO4W_ROOT_MSYS/include \
		--libexecdir=$OSGEO4W_ROOT_MSYS/bin \
		--prefix=$OSGEO4W_ROOT_MSYS/apps/grass \
	        --bindir=$OSGEO4W_ROOT_MSYS/bin \
		--includedir=$OSGEO4W_ROOT_MSYS/include \
		--disable-x --without-x \
		--with-cxx \
		--enable-shared \
		--enable-largefile \
		--with-opengl=windows \
		--with-fftw \
		--with-freetype \
		--with-proj-share=$OSGEO4W_ROOT_MSYS/share/proj \
		--with-gdal=$PWD/mswindows/osgeo4w/gdal-config \
		--with-geos=$PWD/mswindows/osgeo4w/geos-config \
		--with-tcltk \
		--with-sqlite \
		--with-postgres \
		--with-curses \
		--with-regex \
		--with-nls \
		--with-freetype-includes=$OSGEO4W_ROOT_MSYS/include/freetype2 \
		--with-odbc \
	        --without-cairo

	touch mswindows/osgeo4w/configure-stamp
fi

log make 
make -k || ( cat error.log >&3 && false ) 

log make install
make install

log cleanup
mv $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h \
    $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h.mingw
cp mswindows/osgeo4w/config.h.switch $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h
cp mswindows/osgeo4w/config.h.vc $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT@#$OSGEO4W_ROOT#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/grass$MAJOR$MINOR.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#$OSGEO4W_ROOT_MSYS#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/grass.tmpl >$OSGEO4W_ROOT_MSYS/bin/grass$MAJOR$MINOR
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#$OSGEO4W_ROOT#g" \
    mswindows/osgeo4w/env.bat.tmpl >$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/env.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/postinstall.bat >$OSGEO4W_ROOT_MSYS/etc/postinstall/grass$MAJOR$MINOR.bat 
sed -e "s#@VERSION@#$VERSION#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/preremove.bat >$OSGEO4W_ROOT_MSYS/etc/preremove/grass$MAJOR$MINOR.bat 

if [ -f /c/mingw/bin/libgnurx-0.dll ]; then
    cp /c/mingw/bin/libgnurx-0.dll $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/bin 
    cp /c/mingw/bin/libiconv-2.dll $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/bin 
    cp /c/mingw/bin/libintl-8.dll $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/bin
fi

if [ -n "$PACKAGE" ]; then
    log building vc libraries 
    sh mswindows/osgeo4w/mklibs.sh $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/lib/*.$VERSION.dll 
    mv mswindows/osgeo4w/vc/grass*.lib $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/lib
    # rm $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/lib/*.dll
    
    # log BUILDING GDAL GRASS plugins 
    # $COMSPEC /c "mswindows\\osgeo4w\\gdalplugins.cmd $VERSION" 
    
    log CREATING PACKAGES 
    mkdir -p mswindows/osgeo4w/package
    
    PDIR=$PWD/mswindows/osgeo4w/package
    SRC=$PWD
    cd $OSGEO4W_ROOT_MSYS 

    sed -e "s#@VERSION@#$VERSION#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
	$SRC/mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/grass$MAJOR$MINOR.bat.tmpl
    sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#@OSGEO4W_ROOT@#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
	$SRC/mswindows/osgeo4w/grass.tmpl >$OSGEO4W_ROOT_MSYS/bin/grass$MAJOR$MINOR.tmpl
    
    tar -cjf $PDIR/grass-$VERSION-$PACKAGE.tar.bz2 \
    apps/grass/grass-$VERSION \
    bin/grass$MAJOR$MINOR.bat.tmpl \
    bin/grass$MAJOR$MINOR.tmpl \
    bin/libintl3.dll \
    bin/libiconv2.dll \
    bin/regex2.dll \
    etc/postinstall/grass$MAJOR$MINOR.bat \
    etc/preremove/grass$MAJOR$MINOR.bat
    
    rm bin/grass$MAJOR$MINOR.tmpl
    rm bin/grass$MAJOR$MINOR.bat.tmpl
    
    cd $PDIR/.. 
    svn diff >/tmp/grass-$VERSION.diff
    tar -C /tmp -cjf $PDIR/grass-$VERSION-$PACKAGE-src.tar.bz2 grass-$VERSION.diff
fi

log 

exit 0
