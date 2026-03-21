#!/bin/bash

# Exit on error
set -e

LIB_NAME=$(echo "$1" | tr '[:upper:]' '[:lower:]')
LIB_NAME_UPPER=$(echo "$1" | tr '[:lower:]' '[:upper:]')
LIB_VERSION=$2
NPROCS=$3

echo "Downloading ${LIB_NAME_UPPER} ${LIB_VERSION}..."

if [ "$LIB_NAME" == "gdal" ]; then
    wget "https://github.com/OSGeo/gdal/releases/download/v${LIB_VERSION}/gdal-${LIB_VERSION}.tar.gz"
    tar -xzf "./gdal-${LIB_VERSION}.tar.gz"
    cd "./gdal-${LIB_VERSION}"

    echo "Configuring ${LIB_NAME_UPPER} build with CMake..."
    cmake -G Ninja -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_PYTHON_BINDINGS=ON \
        -DBUILD_TESTING=OFF \
        -DGDAL_USE_CURL=ON \
        -DGDAL_USE_EXPAT=ON \
        -DGDAL_USE_FREEXL=ON \
        -DGDAL_USE_GEOTIFF=ON \
        -DGDAL_USE_GIF=ON \
        -DGDAL_USE_HDF5=ON \
        -DGDAL_USE_JPEG=ON \
        -DGDAL_USE_LIBLZMA=ON \
        -DGDAL_USE_LIBXERCESC=ON \
        -DGDAL_USE_LIBXML2=ON \
        -DGDAL_USE_NETCDF=ON \
        -DGDAL_USE_OPENJPEG=ON \
        -DGDAL_USE_PNG=ON \
        -DGDAL_USE_PROJ=ON \
        -DGDAL_USE_SPATIALITE=ON \
        -DGDAL_USE_SQLITE3=ON \
        -DGDAL_USE_TIFF=ON \
        -DGDAL_USE_WEBP=ON \
        -DGDAL_USE_ZSTD=ON \
        -DPython_NumPy_INCLUDE_DIRS="$(python3 -c "import numpy; print(numpy.get_include())")"
fi

if [ "$LIB_NAME" == "proj" ]; then
    wget "https://download.osgeo.org/proj/proj-${LIB_VERSION}.tar.gz"
    tar -xzf "proj-${LIB_VERSION}.tar.gz"
    cd "proj-${LIB_VERSION}"

    echo "Configuring PROJ build with CMake..."
    cmake -G Ninja -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_TESTING=OFF
fi


if [ "$LIB_NAME" == "pdal" ]; then
    wget -q "https://github.com/PDAL/PDAL/releases/download/${LIB_VERSION}/PDAL-${LIB_VERSION}-src.tar.gz"
    tar xfz "PDAL-${LIB_VERSION}-src.tar.gz"
    cd "PDAL-${LIB_VERSION}-src"

    echo "Configuring PDAL build with CMake..."
    cmake -G Ninja -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_C_COMPILER=gcc \
        -DBUILD_PGPOINTCLOUD_TESTS=OFF \
        -DBUILD_PLUGIN_CPD=OFF \
        -DBUILD_PLUGIN_GREYHOUND=ON \
        -DBUILD_PLUGIN_HDF=ON \
        -DBUILD_PLUGIN_HEXBIN=ON \
        -DBUILD_PLUGIN_ICEBRIDGE=ON \
        -DBUILD_PLUGIN_NITF=OFF \
        -DBUILD_PLUGIN_PGPOINTCLOUD=ON \
        -DBUILD_PLUGIN_PYTHON=ON \
        -DBUILD_PLUGIN_SQLITE=ON \
        -DHEXER_INCLUDE_DIR=/usr/include/ \
        -DWITH_LASZIP=OFF \
        -DWITH_LAZPERF=ON \
        -DWITH_TESTS=OFF \
        -DWITH_ZLIB=ON \
        -DWITH_ZSTD=ON

fi

if [ "$LIB_NAME" == "geos" ]; then
    echo "Downloading GEOS ${LIB_VERSION}..."
    wget "https://download.osgeo.org/geos/geos-${LIB_VERSION}.tar.bz2"

    # Extract the archive
    tar -xjf "geos-${LIB_VERSION}.tar.bz2"
    cd "geos-${LIB_VERSION}"

    echo "Configuring GEOS build with CMake..."
    cmake -G Ninja -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_DOCUMENTATION=OFF \
        -DBUILD_TESTING=OFF

fi


echo "Compiling ${LIB_NAME_UPPER}..."
cmake --build build

echo "Installing ${LIB_NAME_UPPER}..."
cmake --install build
# Update shared library cache
ldconfig

echo "${LIB_NAME_UPPER} ${LIB_VERSION} has been successfully installed."
