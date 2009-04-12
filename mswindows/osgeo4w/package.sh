set -e

(
echo
echo
echo

if ! [ -f configure-stamp ]; then
	echo $(date): STARTING configure

	./configure \
		--with-libs="/c/OSGeo4W/lib /c/MinGW/lib" \
		--with-includes="/c/OSGeo4W/include /c/MinGW/include" \
		--exec-prefix=/c/OSGeo4W/bin \
		--libexecdir=/c/OSGeo4W/bin \
		--prefix=/c/OSGeo4W/apps/grass \
		--includedir=/c/OSGeo4W/include \
		--disable-x --without-x \
		--with-cxx \
		--enable-shared \
		--enable-largefile \
		--with-opengl=windows \
		--with-fftw \
		--with-freetype \
		--with-proj-share=/c/OSGeo4W/share/proj \
		--with-gdal=/C/OSGeo4W/bin/gdal-config \
		--with-tcltk \
		--with-sqlite \
		--with-postgres \
		--with-curses \
		--with-regex \
		--with-nls \
		--without-swig \
		--with-freetype-includes=/c/OSGeo4W/include/freetype2

	echo $(date): STARTING make clean
	make clean

	touch configure-stamp
fi


echo $(date): STARTING make
make || make

echo $(date): STARTING make install
make install
echo $(date): END

) | tee -a package.log
