#!/c/OSGeo4W/apps/msys/bin/sh

###set -e

if ! [ -d mswindows ]; then
	echo Start from GRASS toplevel dir
	exit 1
fi

OSGEO4W_ROOT_MSYS=/c/OSGeo4W
PATH=.:/c/mingw/bin:/usr/local/bin:/bin:$OSGEO4W_ROOT_MSYS/bin:/c/WINDOWS/system32:/c/WINDOWS:/c/WINDOWS/System32/Wbem
export OSGEO4W_ROOT_MSYS PATH

version() {
	(
		read MAJOR
		read MINOR
		read PATCH
		echo $MAJOR.$MINOR.$PATCH
	) < include/VERSION
}

VERSION=$(version)
PACKAGE=3
export VERSION PACKAGE

GRASS_PYTHON=/c/OSGeo4W/apps/Python25
PYTHONHOME=/c/OSGeo4W/apps/Python25
export GRASS_PYTHON PYTHONHOME

LOG=mswindows/osgeo4w/package.log

report() {
    echo $(date): STARTING $1
    echo $(date): STARTING $1 >>$LOG
}

echo $(date): START > $LOG

if ! [ -f mswindows/osgeo4w/configure-stamp ]; then

	if [ -e include/Make/Grass.make ] ; then
	    report "distclean"
	    make distclean >>$LOG 2>&1
	fi

	report "configure"
	./configure \
		--with-libs="$OSGEO4W_ROOT_MSYS/apps/gdal-16/lib $OSGEO4W_ROOT_MSYS/lib" \
		--with-includes="$OSGEO4W_ROOT_MSYS/apps/gdal-16/include $OSGEO4W_ROOT_MSYS/include" \
		--exec-prefix=$OSGEO4W_ROOT_MSYS/apps/grass \
		--libexecdir=$OSGEO4W_ROOT_MSYS/bin \
		--prefix=$OSGEO4W_ROOT_MSYS/apps/grass \
		--includedir=$OSGEO4W_ROOT_MSYS/include \
		--disable-x --without-x \
		--with-cxx \
		--enable-shared \
		--with-opengl=windows \
		--with-fftw \
		--with-freetype \
		--with-proj-share=$OSGEO4W_ROOT_MSYS/share/proj \
		--with-gdal=$OSGEO4W_ROOT_MSYS/bin/gdal-config \
		--with-tcltk \
		--with-sqlite \
		--with-postgres \
		--with-curses \
		--with-regex \
		--with-nls \
		--with-freetype-includes=$OSGEO4W_ROOT_MSYS/include/freetype2 \
		--with-odbc \
	        --without-cairo \
	        --enable-largefile
		--with-odbc >>$LOG 2>&1
		
	touch mswindows/osgeo4w/configure-stamp
fi

report "make"
make >>$LOG 2>&1

report "make install"
make install >>$LOG 2>&1

report "cleanup"
mv "$OSGEO4W_ROOT_MSYS"/apps/grass/grass-$VERSION/lib/*.$VERSION.dll \
   "$OSGEO4W_ROOT_MSYS/bin"

mv "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h" \
   "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h.mingw"
cp mswindows/osgeo4w/config.h.switch \
   "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass/config.h"
cp mswindows/osgeo4w/config.h.vc \
   "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/include/grass"
cp mswindows/osgeo4w/ini.bat.tmpl \
   "$OSGEO4W_ROOT_MSYS/etc/ini/grass.bat.tmpl"
cp mswindows/osgeo4w/postinstall.bat \
   "$OSGEO4W_ROOT_MSYS/etc/postinstall/grass.bat"
cp mswindows/osgeo4w/preremove.bat \
   "$OSGEO4W_ROOT_MSYS/etc/preremove/grass.bat"

P="$(pwd -W)"
# portable? how about dist.amd64-pc-mingw32?
P="${P//\//\\\\}\\\\dist.i686-pc-mingw32"

sed -e "s#$P#@osgeo4w@#" "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/fontcap" \
    > $OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/fontcap.tmpl
sed -e "s#$P#@osgeo4w_msys@#" "$OSGEO4W_ROOT_MSYS/apps/grass/bin/grass70" \
    > $OSGEO4W_ROOT_MSYS/apps/grass/bin/grass70.tmpl
rm "$OSGEO4W_ROOT_MSYS/apps/grass/grass-$VERSION/etc/fontcap"

echo $(date): END >> $LOG

exit 0
