#!sh

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
# package name for osgeo4w
# eg. 70-dev -> grass70-dev, empty for release
export PACKAGE_NAME=$2
# OSGeo4W directory postfix, separate OSGeo4W installations are used
# for building GRASS 6.x and 7.x
if test -z "$3" ; then
    OSGEO4W_ROOT_POSTFIX=""
else
    OSGEO4W_ROOT_POSTFIX="$3"
fi
export OSGEO4W_ROOT_MSYS="/c/OSGeo4W${OSGEO4W_ROOT_POSTFIX}"
export OSGEO4W_ROOT="C:\\\OSGeo4W${OSGEO4W_ROOT_POSTFIX}"

export PATH=.:/usr/local/bin:/bin:$OSGEO4W_ROOT_MSYS/bin:/c/WINDOWS/system32:/c/WINDOWS:/c/WINDOWS/System32/Wbem:/c/Subversion:$PWD/mswindows/osgeo4w

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

if [[ "$PATCH" == *svn* ]] ; then
    GRASS_EXECUTABLE=grass${MAJOR}${MINOR}svn
else
    GRASS_EXECUTABLE=grass${MAJOR}${MINOR}
fi

export GRASS_PYTHON="$OSGEO4W_ROOT_MSYS/bin/python.exe"
export PYTHONHOME="$OSGEO4W_ROOT_MSYS/apps/Python27"

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
cp -uv $OSGEO4W_ROOT_MSYS/lib/proj_i.lib mswindows/osgeo4w/lib/libproj_i.a 
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
		--with-libs="$OSGEO4W_ROOT_MSYS/lib $OSGEO4W_ROOT_MSYS/apps/msys/lib $PWD/mswindows/osgeo4w/lib" \
		--with-includes="$OSGEO4W_ROOT_MSYS/include $OSGEO4W_ROOT_MSYS/apps/msys/include" \
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
	        --with-liblas=$PWD/mswindows/osgeo4w/liblas-config \
		--with-sqlite \
		--with-curses \
		--with-regex \
		--with-nls \
		--with-freetype-includes=$OSGEO4W_ROOT_MSYS/include/freetype2 \
		--with-odbc \
	        --with-cairo \
		--with-cairo-includes=$OSGEO4W_ROOT_MSYS/include/cairo

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
mkdir -p $OSGEO4W_ROOT_MSYS/etc/preremove $OSGEO4W_ROOT_MSYS/etc/postinstall
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT@#$OSGEO4W_ROOT#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#$OSGEO4W_ROOT_MSYS#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
    mswindows/osgeo4w/grass.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}
sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#$OSGEO4W_ROOT#g" \
    mswindows/osgeo4w/env.bat.tmpl >$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/env.bat
sed -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/postinstall.bat >$OSGEO4W_ROOT_MSYS/etc/postinstall/${GRASS_EXECUTABLE}.bat 
sed -e "s#@VERSION@#$VERSION#g" -e "s#@GRASS_EXECUTABLE@#$GRASS_EXECUTABLE#g" \
    mswindows/osgeo4w/preremove.bat >$OSGEO4W_ROOT_MSYS/etc/preremove/${GRASS_EXECUTABLE}.bat 

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
	$SRC/mswindows/osgeo4w/grass.bat.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}.bat.tmpl
    sed -e "s#@VERSION@#$VERSION#g" -e "s#@OSGEO4W_ROOT_MSYS@#@OSGEO4W_ROOT@#g" -e "s#@POSTFIX@#$MAJOR$MINOR#g" \
	$SRC/mswindows/osgeo4w/grass.tmpl >$OSGEO4W_ROOT_MSYS/bin/${GRASS_EXECUTABLE}.tmpl

    # grass package
    tar -cjf $PDIR/grass$PACKAGE_NAME-$VERSION-$PACKAGE.tar.bz2 \
	apps/grass/grass-$VERSION \
	bin/${GRASS_EXECUTABLE}.bat.tmpl \
	bin/${GRASS_EXECUTABLE}.tmpl \
	etc/postinstall/${GRASS_EXECUTABLE}.bat \
	etc/preremove/${GRASS_EXECUTABLE}.bat
    
    rm bin/${GRASS_EXECUTABLE}.tmpl
    rm bin/${GRASS_EXECUTABLE}.bat.tmpl
    
    # grass-devel package (obsolete)
    ###tar -cjf $PDIR/grass-devel-$VERSION-$PACKAGE.tar.bz2 \
    ###apps/grass/grass-$VERSION/include
    
    # grass-devel-mingw package (obsolete)
    ###tar -cjf $PDIR/grass-devel-mingw-$VERSION-$PACKAGE.tar.bz2 \
    ###apps/grass/grass-$VERSION/lib/*.a
    
    # grass-devel-vc package (obsolete)
    ###tar -cjf $PDIR/grass-devel-vc-$VERSION-$PACKAGE.tar.bz2 \
    ###apps/grass/grass-$VERSION/lib/*.lib
    
    # grass-locale package (obsolete)
    ###tar -cjf $PDIR/grass-locale-$VERSION-$PACKAGE.tar.bz2 \
    ###apps/grass/grass-$VERSION/locale
fi

log 

exit 0
