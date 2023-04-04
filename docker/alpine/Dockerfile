FROM alpine:3.17 as common

# Based on:
# https://github.com/mundialis/docker-grass-gis/blob/master/Dockerfile
LABEL authors="Carmen Tawalika,Pietro Zambelli,Markus Neteler"
LABEL maintainer="neteler@osgeo.org"

# PACKAGES VERSIONS
ARG PYTHON_VERSION=3

# List of packages to be installed (proj-data omitted: 570.04 MB)
ENV PACKAGES="\
      attr \
      bash \
      bison \
      bzip2 \
      cairo \
      curl \
      fftw \
      flex \
      freetype \
      gdal \
      gdal-dev \
      gdal-tools \
      gettext \
      geos \
      geos-dev \
      gnutls \
      jsoncpp \
      laszip \
      libbz2 \
      libgeotiff \
      libjpeg-turbo \
      libpng \
      libunwind \
      make \
      musl \
      musl-utils \
      ncurses \
      openjpeg \
      openblas \
      py3-numpy \
      py3-pillow \
      py3-six \
      pdal \
      pdal-dev \
      postgresql \
      proj-util \
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

ENV PYTHONBIN=python$PYTHON_VERSION

RUN echo "Install Python";\
    apk add --no-cache $PYTHONBIN && \
    $PYTHONBIN -m ensurepip && \
    rm -r /usr/lib/python*/ensurepip && \
    pip$PYTHON_VERSION install --upgrade pip setuptools && \
    if [ ! -e /usr/bin/pip ]; then ln -s pip$PYTHON_VERSION /usr/bin/pip ; fi && \
    if [ ! -e /usr/bin/python ]; then ln -sf /usr/bin/$PYTHONBIN /usr/bin/python; fi && \
    rm -r /root/.cache

# Add the packages
RUN echo "Install main packages";\
    apk update; \
    apk add --no-cache $PACKAGES


FROM common as build

# ================
# CONFIG VARIABLES
# ================

# set configuration options, without wxGUI
ENV GRASS_CONFIG="\
      --enable-largefile \
      --with-cxx \
      --with-proj-share=/usr/share/proj \
      --with-gdal \
      --with-pdal \
      --with-geos \
      --with-sqlite \
      --with-bzlib \
      --with-zstd \
      --with-cairo --with-cairo-ldflags=-lfontconfig \
      --with-fftw \
      --with-postgres --with-postgres-includes=/usr/include/postgresql \
      --with-openmp \
      --without-freetype \
      --without-opengl \
      --without-nls \
      --without-mysql \
      --without-odbc \
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
      fftw-dev \
      freetype-dev \
      g++ \
      gcc \
      gdal-dev \
      geos-dev \
      git \
      gnutls-dev \
      libc6-compat \
      libjpeg-turbo-dev \
      libpng-dev \
      libpq-dev \
      make \
      openjpeg-dev \
      openblas-dev \
      pdal \
      pdal-dev \
      proj-dev \
      python3-dev \
      py3-numpy-dev \
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

# Reduce the image size - Remove unnecessary grass files
RUN cp /usr/local/grass83/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass83/demolocation; \
    rm -rf /usr/local/grass83/fonts; \
    rm -rf /usr/local/grass83/gui; \
    rm -rf /usr/local/grass83/share; \
    mkdir -p /usr/local/grass83/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass83/gui/wxpython/xml/module_items.xml;


FROM common as grass

ENV LC_ALL="en_US.UTF-8"

# Copy GRASS GIS from build image
COPY --from=build /usr/local/bin/grass /usr/local/bin/grass
COPY --from=build /usr/local/grass* /usr/local/grass/

# install external Python API
RUN pip3 install --upgrade pip six grass-session --ignore-installed six

RUN ln -sf /usr/local/grass $(grass --config path)
RUN grass --tmp-location XY --exec g.version -rge && \
    pdal --version && \
    python3 --version


FROM grass as test

## run simple LAZ test
COPY docker/testdata/simple.laz /tmp/simple.laz
COPY docker/testdata/test_grass_session.py /scripts/test_grass_session.py
ENV GRASSBIN=grass

# Test grass-session
# Not yet ready for GRASS GIS 8:
RUN /usr/bin/python3 /scripts/test_grass_session.py
# Test PDAL
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

# Test addon installation
RUN apk add --no-cache py3-scikit-learn
RUN grass --tmp-location XY --exec g.extension extension=r.learn.ml2


FROM grass as final

# These packages are required to run g.extension in GRASS GIS.
ENV GRASS_RUN_PACKAGES="\
      build-base \
      gcc \
      libpq-dev \
      make \
    "

# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASSBIN="/usr/local/bin/grass" \
    GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash"

# https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
ENV PROJ_NETWORK=ON

# Add packages for fully enabling g.extension
RUN apk add --no-cache $GRASS_RUN_PACKAGES

# addon test in final stage: does g.extension install also C-extensions?
# test raster C addon
RUN grass --tmp-location XY --exec g.extension extension=r.gwr operation=add
RUN grass --tmp-location XY --exec g.extension extension=r.gwr operation=remove -f
# test vector C addon
RUN grass --tmp-location XY --exec g.extension extension=v.centerpoint operation=add
RUN grass --tmp-location XY --exec g.extension extension=v.centerpoint operation=remove -f

# show installed version
RUN grass --tmp-location XY --exec g.version -rge && \
    pdal --version && \
    python3 --version

# test r.in.pdal in final image because test stage is not executed in github action
COPY docker/testdata/simple.laz /tmp/simple.laz
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

# Data workdir
WORKDIR /grassdb
VOLUME /grassdb

CMD ["$GRASSBIN", "--version"]
