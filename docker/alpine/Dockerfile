FROM alpine:3.21@sha256:a8560b36e8b8210634f77d9f7f9efd7ffa463e380b75e2e74aff4511df3ef88c as common

# Based on:
# https://github.com/mundialis/docker-grass-gis/blob/master/Dockerfile
LABEL authors="Carmen Tawalika,Pietro Zambelli,Markus Neteler"
LABEL maintainer="neteler@osgeo.org"

# PACKAGES VERSIONS
ARG PYTHON_VERSION=3

# List of packages to be installed (proj-data omitted: 570.04 MB)
ENV GRASS_RUN_PACKAGES="\
      attr \
      bash \
      bison \
      build-base \
      bzip2 \
      cairo \
      curl \
      fftw \
      flex \
      freetype \
      g++ \
      gcc \
      gdal \
      gdal-dev \
      gdal-driver-GMLAS \
      gdal-driver-HDF5 \
      gdal-driver-JP2OpenJPEG \
      gdal-driver-LIBKML \
      gdal-driver-MSSQLSpatial \
      gdal-driver-netCDF \
      gdal-driver-ODBC \
      gdal-driver-PG \
      gdal-driver-PNG \
      gdal-driver-WMS \
      gdal-tools \
      geos \
      geos-dev \
      gettext \
      git \
      gnutls \
      jsoncpp \
      laszip \
      libbz2 \
      libgeotiff \
      libjpeg-turbo \
      libpng \
      libpq-dev \
      libunwind \
      make \
      musl \
      musl-utils \
      ncurses \
      openblas \
      openjpeg \
      pdal \
      pdal-dev \
      postgresql15-client \
      proj-util \
      py3-numpy \
      py3-pillow \
      python3 \
      sqlite \
      sqlite-libs \
      subversion \
      tiff \
      zstd \
      zstd-libs \
    "
# ====================
# INSTALL DEPENDENCIES
# ====================

WORKDIR /src

# Add the packages
RUN echo "Install main packages";\
    apk update; \
    apk add --no-cache $GRASS_RUN_PACKAGES


FROM common AS build

# ================
# CONFIG VARIABLES
# ================

# set configuration options, without wxGUI
ENV GRASS_CONFIG="\
      --enable-largefile \
      --with-bzlib \
      --with-cairo --with-cairo-ldflags=-lfontconfig \
      --with-cxx \
      --with-fftw \
      --with-gdal \
      --with-geos \
      --with-openmp \
      --with-pdal \
      --with-postgres --with-postgres-includes=/usr/include/postgresql \
      --with-proj-share=/usr/share/proj \
      --with-sqlite \
      --with-zstd \
      --without-freetype \
      --without-mysql \
      --without-nls \
      --without-odbc \
      --without-opengl \
      "

# Set environmental variables for GRASS GIS compilation, without debug symbols
ENV MYCFLAGS="-O2 -std=gnu99 -m64" \
    MYLDFLAGS="-s -Wl,--no-undefined -lblas" \
    # CXX stuff:
    LD_LIBRARY_PATH="/usr/local/lib" \
    LDFLAGS="$MYLDFLAGS" \
    CFLAGS="$MYCFLAGS" \
    CXXFLAGS="$MYCXXFLAGS" \
    NUMTHREADS=2

# These packages are required to compile GRASS GIS.
ENV GRASS_BUILD_PACKAGES="\
      build-base \
      bzip2-dev \
      cairo-dev \
      cmake \
      fftw-dev \
      freetype-dev \
      geos-dev \
      git \
      gnutls-dev \
      libc6-compat \
      libjpeg-turbo-dev \
      libpng-dev \
      libpq-dev \
      openblas-dev \
      openjpeg-dev \
      pdal \
      pdal-dev \
      proj-dev \
      py3-numpy-dev \
      python3-dev \
      sqlite-dev \
      tar \
      tiff-dev \
      unzip \
      vim \
      wget \
      zip \
      zstd-dev \
    "

# Add the packages
RUN echo "Install main packages";\
    # Add packages just for the GRASS build process
    apk add --no-cache --virtual .build-deps $GRASS_BUILD_PACKAGES
    # echo LANG="en_US.UTF-8" > /etc/default/locale;

# Copy and install GRASS GIS
COPY . /src/grass_build/
WORKDIR /src/grass_build/

# Configure compile and install GRASS GIS
RUN echo "  => Configure and compile grass" && \
    /src/grass_build/configure $GRASS_CONFIG && \
    make -j $NUMTHREADS && \
    make install && \
    ldconfig /etc/ld.so.conf.d

# Build the GDAL-GRASS plugin
# renovate: datasource=github-tags depName=OSGeo/gdal-grass
ARG GDAL_GRASS_VERSION=1.0.3
RUN git clone --branch $GDAL_GRASS_VERSION --depth 1 https://github.com/OSGeo/gdal-grass.git /src/gdal-grass
WORKDIR /src/gdal-grass
RUN cmake -B build -DAUTOLOAD_DIR=/usr/lib/gdalplugins -DBUILD_TESTING=OFF && \
    cmake --build build && \
    cmake --install build

# Get rid of version number here, restore in next stage via symbolic link
RUN mv /usr/local/grass85 /usr/local/grass

# Reduce the image size - Remove unnecessary grass files
RUN cp /usr/local/grass/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass/demolocation; \
    rm -rf /usr/local/grass/fonts; \
    rm -rf /usr/local/grass/gui; \
    rm -rf /usr/local/grass/share; \
    mkdir -p /usr/local/grass/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass/gui/wxpython/xml/module_items.xml;


FROM common AS grass

# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python:$PYTHONPATH"

# Copy GRASS GIS and GDAL GRASS driver from build image
COPY --from=build /usr/local/bin/grass /usr/local/bin/grass
COPY --from=build /usr/local/grass* /usr/local/grass/
COPY --from=build /usr/lib/gdalplugins/*_GRASS.so /usr/lib/gdalplugins/
# Set GISBASE
ENV GISBASE=/usr/local/grass
ENV GRASS_GISBASE=/usr/local/grass

# run simple LAZ test
COPY docker/testdata/simple.laz /tmp/
COPY docker/testdata/test_grass_python.py docker/testdata/test_grass_session.py docker/alpine/grass_tests.sh /scripts/

# run GRASS GIS python session and scan the test LAZ file; some cleanup
# also show installed version
RUN ln -sf /usr/local/grass /usr/local/grass85; \
    rm -rf /tmp/grasstest_epsg_25832; \
    $SHELL /scripts/grass_tests.sh; \
    python /scripts/test_grass_session.py && rm -rf /tmp/grasstest_epsg_25832; \
    grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g \
    && rm -f /scripts/grass_tests.sh /tmp/simple.laz /scripts/test_grass_python.py; \
    apk del --no-cache gettext pdal-dev; \
    grass --tmp-project XY --exec g.version -rge \
    && pdal --version \
    && python --version

# Data workdir
WORKDIR /grassdb
VOLUME /grassdb

CMD ["$GRASSBIN", "--version"]
