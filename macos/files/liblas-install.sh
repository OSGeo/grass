#!/bin/bash

############################################################################
#
# TOOL:         liblas-install.sh
# AUTHOR(s):    Nicklas Larsson
# PURPOSE:      Downloads, compiles and installs libLAS.
# COPYRIGHT:    (c) 2021-2025 Nicklas Larsson and the GRASS Development Team
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

liblas_commit="6ada875661c46842433a13f28637f8d3d2c393bc"
liblas_zipfile_name="libLAS_${liblas_commit}.zip"
liblas_zipfile_url="https://github.com/libLAS/libLAS/archive/${liblas_commit}.zip"
liblas_source_dir_name="libLAS-${liblas_commit}"
liblas_build_dir_name=libLAS-build

liblas_zipfile="${cache_dir}/${liblas_zipfile_name}"
liblas_source_dir="${cache_dir}/${liblas_source_dir_name}"
liblas_build_dir="${cache_dir}/${liblas_build_dir_name}"

PREFIX=$(python3 -c 'import sys; print(sys.prefix)')
export PREFIX
export PATH="${PREFIX}/bin:/usr/bin:/bin:/usr/sbin:/etc:/usr/lib"
export CC="${PREFIX}/bin/clang"
export CXX="${PREFIX}/bin/clang++"
export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
export CFLAGS="-O2 -pipe -arch ${arch}"
export CXXFLAGS="-O2 -pipe -arch ${arch} -stdlib=libc++"
export LDFLAGS="-fuse-ld=lld"

cmake="${PREFIX}/bin/cmake"

if [ ! -f "$liblas_zipfile" ]; then
    echo "Downloading libLAS..."
    curl -L "$liblas_zipfile_url" --output "$liblas_zipfile" || exit 1
fi

rm -rf "$liblas_source_dir"
rm -rf "$liblas_build_dir"
mkdir -p "$liblas_source_dir"
mkdir -p "$liblas_build_dir"

unzip "$liblas_zipfile" -d "$cache_dir" &> /dev/null

# patch needed for using now outdated GDAL api
patch -d "$liblas_source_dir" -p0 << EOF
--- src/gt_wkt_srs.cpp.orig	2020-12-14 19:56:40.000000000 +0100
+++ src/gt_wkt_srs.cpp	2021-03-27 19:31:30.000000000 +0100
@@ -299,7 +299,6 @@
                 oSRS.SetFromUserInput(pszWKT);
                 oSRS.SetExtension( "PROJCS", "PROJ4",
                                    "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs" );
-                oSRS.FixupOrdering();
                 CPLFree(pszWKT);
                 pszWKT = NULL;
                 oSRS.exportToWkt(&pszWKT);
@@ -505,7 +504,6 @@
         {
             char	*pszWKT;
             oSRS.morphFromESRI();
-            oSRS.FixupOrdering();
             if( oSRS.exportToWkt( &pszWKT ) == OGRERR_NONE )
                 return pszWKT;
         }
@@ -1107,7 +1105,6 @@
 /* ==================================================================== */
     char	*pszWKT;

-    oSRS.FixupOrdering();

     if( oSRS.exportToWkt( &pszWKT ) == OGRERR_NONE )
         return pszWKT;

EOF

# patch for missing architecture in endian detection
patch -d "$liblas_source_dir" -p0 << EOF
--- include/liblas/detail/endian.hpp.orig	2021-01-20 18:24:39.000000000 +0100
+++ include/liblas/detail/endian.hpp	2022-09-27 18:21:23.000000000 +0200
@@ -88,6 +88,7 @@
    || defined(_M_ALPHA) || defined(__amd64) \\
    || defined(__amd64__) || defined(_M_AMD64) \\
    || defined(__x86_64) || defined(__x86_64__) \\
+   || defined(__arm64) || defined(__arm64__) \\
    || defined(_M_X64)

 # define LIBLAS_LITTLE_ENDIAN
EOF

LIBLAS_CONFIGURE_FLAGS="
  -DCMAKE_OSX_SYSROOT=${sdk} \
  -DCMAKE_INCLUDE_PATH=${PREFIX}/include \
  -DCMAKE_INSTALL_PREFIX=${PREFIX} \
  -DCMAKE_MACOSX_RPATH=ON \
  -DCMAKE_INSTALL_RPATH=${PREFIX} \
  -DWITH_GEOTIFF=ON \
  -DGEOTIFF_INCLUDE_DIR=${PREFIX}/include \
  -DGEOTIFF_LIBRARY=${PREFIX}/lib/libgeotiff.dylib \
  -DWITH_GDAL=ON \
  -DGDAL_CONFIG=${PREFIX}/bin/gdal-config \
  -DPROJ4_INCLUDE_DIR=${PREFIX}/include/proj \
  -DPROJ4_LIBRARY=${PREFIX}/lib/proj7 \
  -DWITH_LASZIP=OFF \
  -DWITH_PKGCONFIG=OFF
"

pushd "$liblas_build_dir" > /dev/null

echo
echo "Configuring libLAS..."
"$cmake" -G "Unix Makefiles" $LIBLAS_CONFIGURE_FLAGS "$liblas_source_dir"

echo "Compiling and installing libLAS..."
if ! make &> "${cache_dir}/libLAS_install.log"
then
    echo "...libLAS compilation failed. See ${cache_dir}/libLAS_install.log."
    popd > /dev/null
    exit_nice 1
fi

if ! make install &> "${cache_dir}/libLAS_install.log"
then
    echo "...libLAS installations failed. See ${cache_dir}/libLAS_install.log."
    popd > /dev/null
    exit_nice 1
fi
echo "...libLAS installed successfully."

popd > /dev/null

export CFLAGS=""
export CXXFLAGS=""
