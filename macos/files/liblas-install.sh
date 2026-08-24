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

liblas_commit="33097f17e27b853ac7b9651025a70354ffb10cfc"
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

LIBLAS_CONFIGURE_FLAGS="
  -DCMAKE_OSX_SYSROOT=${sdk} \
  -DCMAKE_INCLUDE_PATH=${PREFIX}/include \
  -DCMAKE_INSTALL_PREFIX=${PREFIX} \
  -DCMAKE_MACOSX_RPATH=ON \
  -DCMAKE_INSTALL_RPATH=${PREFIX} \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
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
