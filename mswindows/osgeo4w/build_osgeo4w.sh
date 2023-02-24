#!/bin/sh

#
# The following environment variables are supposed to be passed to the build script
# - SRC: the directory where the grass source code lives
# - OSGEO4W_ROOT_MSYS: the root directory of OSGeo4W
# - UNITTEST: If this variable is defined, addition files for unittests are created
#
# By default, the script will look for the source code in the current directory
# and create bin.x86_64-w64-mingw32\grass$ver.bat (run this batch file to start
# GRASS GIS) and dist.x86_64-w64-mingw32\etc\env.bat.
#

# stop on errors
set -e


# compile
export PATH=${OSGEO4W_ROOT_MSYS}/bin:/usr/bin:/mingw64/bin
export C_INCLUDE_PATH=".:${OSGEO4W_ROOT_MSYS}/include:${SRC}/dist.${ARCH}/include:/c/msys64/mingw64/include"
export PYTHONHOME=${OSGEO4W_ROOT_MSYS}/apps/Python39
export ARCH=x86_64-w64-mingw32

./configure \
    --host=${ARCH} \
    --with-libs="${OSGEO4W_ROOT_MSYS}/lib ${OSGEO4W_ROOT_MSYS}/bin" \
    --with-includes=${OSGEO4W_ROOT_MSYS}/include \
    --libexecdir=${OSGEO4W_ROOT_MSYS}/bin \
    --prefix=${OSGEO4W_ROOT_MSYS}/apps/grass \
    --bindir=${OSGEO4W_ROOT_MSYS}/bin \
    --includedir=${OSGEO4W_ROOT_MSYS}/include \
    --without-x \
    --with-cxx \
    --enable-shared \
    --enable-largefile \
    --with-openmp \
    --with-fftw \
    --with-nls \
    --with-readline \
    --with-blas \
    --with-lapack-includes=/mingw64/include/lapack \
    --with-freetype \
    --with-freetype-includes=${OSGEO4W_ROOT_MSYS}/include/freetype2 \
    --with-proj-share=${OSGEO4W_ROOT_MSYS}/share/proj \
    --with-proj-includes=${OSGEO4W_ROOT_MSYS}/include \
    --with-proj-libs=${OSGEO4W_ROOT_MSYS}/lib \
    --with-postgres \
    --with-postgres-includes=${OSGEO4W_ROOT_MSYS}/include \
    --with-postgres-libs=${OSGEO4W_ROOT_MSYS}/lib \
    --with-gdal=${SRC}/mswindows/osgeo4w/gdal-config \
    --with-geos=${SRC}/mswindows/osgeo4w/geos-config \
    --with-sqlite \
    --with-sqlite-includes=${OSGEO4W_ROOT_MSYS}/include \
    --with-sqlite-libs=${OSGEO4W_ROOT_MSYS}/lib \
    --with-regex \
    --with-nls \
    --with-zstd \
    --with-odbc \
    --with-cairo \
    --with-cairo-includes=${OSGEO4W_ROOT_MSYS}/include \
    --with-cairo-libs=$OSGEO4W_ROOT_MSYS/lib \
    --with-cairo-ldflags="-L${SRC}/mswindows/osgeo4w/lib -lcairo" \
    --with-opengl=windows \
    --with-bzlib \
    --with-liblas=${SRC}/mswindows/osgeo4w/liblas-config \
    --with-netcdf=${OSGEO4W_ROOT_MSYS}/bin/nc-config \
    --without-pdal

make

# install

bin=bin.${ARCH}
dist=dist.${ARCH}
ver=$(sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make)

rm -f $dist/grass$ver.tmp $dist/etc/fontcap

if [ "$UNITTEST" ]; then
    # Add executables for bash scripts in unittests
    bash_bin=bash_bin
    mkdir "${SRC}/dist.${ARCH}/$bash_bin"

    for f in ${SRC}/dist.${ARCH}/scripts/*.py
    do
        bash_exe=$(echo $f | sed -e "s/\/scripts\//\/${bash_bin}\//;s/\.py$//")
        cp "$f" "$bash_exe"
        # dos2unix "$bash_exe"
        chmod ugo+x "$bash_exe"
    done
    bash_exe_path=$(echo "${SRC}/dist.${ARCH}/$bash_bin" | sed -e "s/^\/c/\;c:/;s/^\/d/d:/")
    export PATH="$PATH:${SRC}/dist.${ARCH}/$bash_bin"
fi

# create batch files
src_esc=$(echo ${SRC} | sed 's#^/\([a-z]\)#\1:#; s#/#\\\\\\\\#g')
dist_esc="$src_esc\\\\$dist"

(
    sed 's/^\(set GISBASE=\).*/\1'$dist_esc'/' \
        mswindows/osgeo4w/env.bat.tmpl
    cat <<EOT


set PATH=%PATH%;C:\\msys64\\mingw64\\bin;C:\\msys64\\usr\\bin

if not exist %GISBASE%\\etc\\fontcap (
	pushd .
	%~d0
	cd %GISBASE%\\lib
	set GISRC=dummy
	%GISBASE%\\bin\\g.mkfontcap.exe
	popd
)
EOT
) >$dist/etc/env.bat
unix2dos $dist/etc/env.bat

(
    sed -e 's/^\(call "\)%~dp0\(.*\)$/\1C:\\OSGeo4W\\bin\2/' \
        -e 's/^\(call "\).*\(\\etc\\env\.bat"\)$/\1'$dist_esc'\2/' \
        -e 's/^\(.* "\)%GISBASE%\\etc\(\\grass.*\)$/\1%GISBASE%\\..\\'$bin'\2/' \
        -e 's/@POSTFIX@/'$ver'/g' \
        mswindows/osgeo4w/grass.bat.tmpl
) >$bin/grass$ver.bat
unix2dos $bin/grass$ver.bat

opt_path=${OSGEO4W_ROOT_MSYS}/opt
grass_path=$opt_path/grass

if [ "$UNITTEST" ]; then
    msys_path=";C:/msys64/usr/bin;C:/msys64/mingw64/bin"
fi

mkdir -p $opt_path
cp -a $dist $grass_path
# have a versionless and versioned startup script
cp -a $bin/grass.py $bin/grass$ver.py
cp -a $bin/grass$ver.py $grass_path/etc
cp -a $(ldd $dist/lib/*.dll | awk '/mingw64/{print $3}' |
    sort -u | grep -v 'lib\(crypto\|ssl\)') $grass_path/lib

(
    sed -e 's/^\(set GISBASE=\).*/\1%OSGEO4W_ROOT%\\opt\\grass/' \
        mswindows/osgeo4w/env.bat.tmpl
    cat <<EOT

set PATH=%OSGEO4W_ROOT%\\bin${msys_path};%PATH%;$bash_exe_path

if not exist %GISBASE%\\etc\\fontcap (
	pushd .
	%~d0
	cd %GISBASE%\\lib
	set GISRC=dummy
	%GISBASE%\\bin\\g.mkfontcap.exe
	popd
)
EOT
) >$grass_path/etc/env.bat
unix2dos $grass_path/etc/env.bat

cat $grass_path/etc/env.bat

(
    sed -e 's/^\(call "%~dp0\)\(.*\)$/\1\\..\\..\\bin\2/' \
        -e 's/^\(call "%OSGEO4W_ROOT%\\\).*\(\\etc\\env\.bat"\)$/\1opt\\grass\2/' \
        -e 's/@POSTFIX@/'$ver'/g' \
        mswindows/osgeo4w/grass.bat.tmpl
) >$grass_path/grass$ver.bat
unix2dos $grass_path/grass$ver.bat

if [ "$UNITTEST" ]; then
    cp $grass_path/grass$ver.bat $grass_path/grass.bat
    cp -a $bin/grass.py ${SRC}/dist.${ARCH}/$bash_bin/grass
    dos2unix ${SRC}/dist.${ARCH}/$bash_bin/grass
    chmod ugo+x ${SRC}/dist.${ARCH}/$bash_bin/grass
    # Set path for bash if not called from OSGeo shell
    arch_esc=$(sed 's/[\/\*\.]/\\&/g' <<<"${ARCH}")
    src_esc=$(sed 's/[\/\*\.]/\\&/g' <<<"${SRC}")
    osgeo4w_esc=$(sed 's/[\/\*\.]/\\&/g' <<<"${OSGEO4W_ROOT_MSYS}")
    pythonhome_esc=$(sed 's/[\/\*\.]/\\&/g' <<<"${PYTHONHOME}")
    sed -i "5s/^/export PATH=\"${osgeo4w_esc}\/bin:\/usr\/bin:\/mingw64\/bin:$src_esc\/dist\.$arch_esc\/bin:$src_esc\/dist\.$arch_esc\/$bash_bin\"\nexport PYTHONHOME=\"${pythonhome_esc}\"/" ${SRC}/.github/workflows/test_simple.sh
    printf "export PATH=\"${OSGEO4W_ROOT_MSYS}/bin:/usr/bin:/mingw64/bin:${SRC}/dist.${ARCH}/bin:${SRC}/dist.${ARCH}/$bash_bin\"\nexport PYTHONHOME=\"${PYTHONHOME}\"\nexport PYTHONUTF8=1" > $HOME/.bash_profile
    printf "export PATH=\"${OSGEO4W_ROOT_MSYS}/bin:/usr/bin:/mingw64/bin:${SRC}/dist.${ARCH}/bin:${SRC}/dist.${ARCH}/$bash_bin\"\nexport PYTHONHOME=\"${PYTHONHOME}\"\nexport PYTHONUTF8=1" > $HOME/.profile
    printf "export PATH=\"${OSGEO4W_ROOT_MSYS}/bin:/usr/bin:/mingw64/bin:${SRC}/dist.${ARCH}/bin:${SRC}/dist.${ARCH}/$bash_bin\"\nexport PYTHONHOME=\"${PYTHONHOME}\"\nexport PYTHONUTF8=1" > $HOME/.bashrc
fi
