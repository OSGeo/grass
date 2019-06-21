FROM alpine:edge

# Based on:
# https://github.com/mundialis/docker-grass-gis/blob/master/Dockerfile
LABEL authors="Pietro Zambelli"
LABEL maintainer="peter.zamb@gmail.com"

# PACKAGES VERSIONS
ARG GRASS_VERSION=7.7
ARG PYTHON_VERSION=3
ARG PROJ_VERSION=5.2.0
ARG PROJ_DATUMGRID_VERSION=1.8

# ================
# CONFIG VARIABLES
# ================

# set configuration options
ENV GRASS_CONFIG="\
      --enable-largefile \
      --with-cxx \
      --with-proj --with-proj-share=/usr/share/proj \
      --with-gdal \
      --with-python \
      --with-geos \
      --with-sqlite \
      --with-bzlib \
      --with-zstd \
      --with-cairo --with-cairo-ldflags=-lfontconfig \
      --with-fftw \
      --with-postgres --with-postgres-includes='/usr/include/postgresql' \
      --without-freetype \
      --without-openmp \
      --without-opengl \
      --without-nls \
      --without-mysql \
      --without-odbc \
      --without-openmp \
      --without-ffmpeg \
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


# List of packages to be installed
ENV PACKAGES="\
      attr \
      bash \
      bison \
      bzip2 \
      cairo \
      fftw \
      flex \
      freetype \
      gdal \
      gettext \
      geos \
      gnutls \
      libbz2 \
      libjpeg-turbo \
      libpng \
      musl \
      musl-utils \
      ncurses \
      openjpeg \
      openblas \
      py3-numpy \
      py3-pillow \
      py3-six \
      postgresql \
      proj4 \
      sqlite \
      sqlite-libs \
      tiff \
      zstd \
      zstd-libs \
    " \
    # These packages are required to compile GRASS GIS.
    GRASS_BUILD_PACKAGES="\
      build-base \
      bzip2-dev \
      cairo-dev \
      fftw-dev \
      freetype-dev \
      g++ \
      gcc \
      gdal-dev \
      geos-dev \
      gnutls-dev \
      libc6-compat \
      libjpeg-turbo-dev \
      libpng-dev \
      make \
      openjpeg-dev \
      openblas-dev \
      postgresql-dev \
      proj4-dev \
      sqlite-dev \
      tar \
      tiff-dev \
      unzip \
      vim \
      wget \
      zip \
      zstd-dev \
    "

# ====================
# INSTALL DEPENDENCIES
# ====================

WORKDIR /src

ENV PYTHONBIN=python$PYTHON_VERSION

RUN echo "Install Python";\
    apk add --no-cache $PYTHONBIN && \
    $PYTHONBIN -m ensurepip && \
    rm -r /usr/lib/python*/ensurepip && \
    pip$PYTHON_VERSION install --upgrade pip setuptools && \
    if [ ! -e /usr/bin/pip ]; then ln -s pip$PYTHON_VERSION /usr/bin/pip ; fi && \
    if [[ ! -e /usr/bin/python ]]; then ln -sf /usr/bin/$PYTHONBIN /usr/bin/python; fi && \
    rm -r /root/.cache

# Add the packages
RUN echo "Install main packages";\
    apk update; \
    apk add --no-cache \
            --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing \
            --repository http://dl-cdn.alpinelinux.org/alpine/edge/main \
            $PACKAGES; \
    # Add packages just for the GRASS build process
    apk add --no-cache \
            --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing \
            --repository http://dl-cdn.alpinelinux.org/alpine/edge/main \
            --virtual .build-deps $GRASS_BUILD_PACKAGES; \
    # echo LANG="en_US.UTF-8" > /etc/default/locale;
    #
    # install the latest projection library for GRASS GIS
    #
    echo "Install PROJ4";\
    echo "  => Dowload proj-$PROJ_VERSION";\
    wget http://download.osgeo.org/proj/proj-$PROJ_VERSION.tar.gz && \
    tar xzvf proj-$PROJ_VERSION.tar.gz && \
    cd /src/proj-$PROJ_VERSION/ && \
    echo "  => Dowload datumgrid-$PROJ_DATUMGRID_VERSION" &&\
    wget http://download.osgeo.org/proj/proj-datumgrid-$PROJ_DATUMGRID_VERSION.zip && \
    cd nad && \
    unzip ../proj-datumgrid-$PROJ_DATUMGRID_VERSION.zip && \
    cd .. && \
    echo "  => configure" &&\
    ./configure --prefix=/usr/ && \
    echo "  => compile" &&\
    make && \
    echo "  => install" &&\
    make install && \
    ldconfig /etc/ld.so.conf.d; \
    #
    # Checkout and install GRASS GIS
    #
    echo "Install GRASS GIS";\
    echo "  => Dowload grass-$GRASS_VERSION";\
    wget https://grass.osgeo.org/grass`echo $GRASS_VERSION | tr -d .`/source/snapshot/grass-$GRASS_VERSION.svn_src_snapshot_latest.tar.gz && \
    # unpack source code package and remove tarball archive:
    mkdir /src/grass_build && \
    tar xfz grass-$GRASS_VERSION.svn_src_snapshot_latest.tar.gz --strip=1 -C /src/grass_build && \
    rm -f grass-$GRASS_VERSION.svn_src_snapshot_latest.tar.gz; \
    #
    # Configure compile and install GRASS GIS
    #
    echo "  => Configure and compile grass";\
    cd /src/grass_build && \
    /src/grass_build/configure $GRASS_CONFIG && \
    make -j $NUMTHREADS && \
    make install && \
    ldconfig /etc/ld.so.conf.d; \
    #
    # enable simple grass command regardless of version number
    #
    ln -s `find /usr/local/bin -name "grass*"` /usr/local/bin/grass; \
    #
    # Reduce the image size
    #
    rm -rf /src/*; \
    # remove build dependencies and any leftover apk cache
    apk del --no-cache --purge .build-deps; \
    rm -rf /var/cache/apk/*; \
    rm -rf /root/.cache; \
    # Remove unnecessary grass files
    rm -rf /usr/local/grass77/demolocation; \
    rm -rf /usr/local/grass77/docs; \
    rm -rf /usr/local/grass77/fonts; \
    rm -rf /usr/local/grass77/gui; \
    rm -rf /usr/local/grass77/share;


# Unset environmental variables to avoid later compilation issues
ENV INTEL="" \
    MYCFLAGS="" \
    MYLDFLAGS="" \
    MYCXXFLAGS="" \
    LD_LIBRARY_PATH="" \
    LDFLAGS="" \
    CFLAGS="" \
    CXXFLAGS="" \
    # set SHELL var to avoid /bin/sh fallback in interactive GRASS GIS sessions in docker
    SHELL="/bin/bash"


# =====================
# INSTALL GRASS-SESSION
# =====================

# install external Python API
RUN pip install grass-session

# set GRASSBIN
ENV GRASSBIN="/usr/local/bin/grass"

# ========
# FINALIZE
# ========

# Data workdir
WORKDIR /grassdb
VOLUME /grassdb

# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    LC_ALL="en_US.UTF-8"

# debug
RUN $GRASSBIN --config revision version

CMD [$GRASSBIN, "--version"]
