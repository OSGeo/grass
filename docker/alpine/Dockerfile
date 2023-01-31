<<<<<<< HEAD
<<<<<<< HEAD
FROM alpine:3.19@sha256:c5b1261d6d3e43071626931fc004f70149baeba2c8ec672bd4f27761f8e1ad6b as common
=======
FROM alpine:3.15 as common
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
FROM alpine:3.15 as common
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

# Based on:
# https://github.com/mundialis/docker-grass-gis/blob/master/Dockerfile
LABEL authors="Carmen Tawalika,Pietro Zambelli,Markus Neteler"
LABEL maintainer="neteler@osgeo.org"

# PACKAGES VERSIONS
ARG PYTHON_VERSION=3

<<<<<<< HEAD
<<<<<<< HEAD
# List of packages to be installed (proj-data omitted: 570.04 MB)
ENV GRASS_RUN_PACKAGES="\
      attr \
      build-base \
=======
# List of packages to be installed
ENV PACKAGES="\
      attr \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
# List of packages to be installed
ENV PACKAGES="\
      attr \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      bash \
      bison \
      bzip2 \
      cairo \
      curl \
      fftw \
      flex \
      freetype \
<<<<<<< HEAD
<<<<<<< HEAD
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
      gettext \
      geos \
      geos-dev \
      git \
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      gdal \
      gdal-tools \
      gettext \
      geos \
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      gnutls \
      jsoncpp \
      laszip \
      libbz2 \
<<<<<<< HEAD
<<<<<<< HEAD
      libgeotiff \
      libjpeg-turbo \
      libpng \
      libpq-dev \
      libunwind \
      make \
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      libexecinfo \
      libgeotiff \
      libjpeg-turbo \
      libpng \
      libunwind \
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      musl \
      musl-utils \
      ncurses \
      openjpeg \
      openblas \
      py3-numpy \
      py3-pillow \
<<<<<<< HEAD
<<<<<<< HEAD
      python3 \
      pdal \
      pdal-dev \
      postgresql15-client \
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      py3-six \
      pdal \
      pdal-dev \
      postgresql \
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
# Add the packages
RUN echo "Install main packages";\
    apk update; \
    apk add --no-cache $GRASS_RUN_PACKAGES
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
    apk add --no-cache $PACKAGES
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))


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
<<<<<<< HEAD
<<<<<<< HEAD
=======
      g++ \
      gcc \
      gdal-dev \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
      g++ \
      gcc \
      gdal-dev \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      geos-dev \
      git \
      gnutls-dev \
      libc6-compat \
      libjpeg-turbo-dev \
      libpng-dev \
<<<<<<< HEAD
<<<<<<< HEAD
      libpq-dev \
=======
      make \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
      make \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
      openjpeg-dev \
      openblas-dev \
      pdal \
      pdal-dev \
<<<<<<< HEAD
<<<<<<< HEAD
=======
      postgresql-dev \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
      postgresql-dev \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/alpine/Dockerfile
<<<<<<< HEAD:docker/alpine/Dockerfile
# Get rid of version number here, restore in next stage via symbolic link
RUN mv $(grass --config path) /usr/local/grass

# Reduce the image size - Remove unnecessary grass files
RUN cp /usr/local/grass/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass/demolocation; \
    rm -rf /usr/local/grass/fonts; \
    rm -rf /usr/local/grass/gui; \
    rm -rf /usr/local/grass/share; \
    mkdir -p /usr/local/grass/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass/gui/wxpython/xml/module_items.xml;

RUN git clone https://github.com/OSGeo/gdal-grass /src/gdal-grass
WORKDIR /src/gdal-grass
RUN ./configure \
    --with-gdal=/usr/bin/gdal-config \
    --with-grass=/usr/local/grass && \
    make -j $NUMTHREADS && \
    make install -j $NUMTHREADS
=======
# Reduce the image size - Remove unnecessary grass files
=======
# Reduce the image size - Remove unnecessary grass files
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/alpine/Dockerfile_alpine
RUN cp /usr/local/grass80/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass80/demolocation; \
    rm -rf /usr/local/grass80/fonts; \
    rm -rf /usr/local/grass80/gui; \
    rm -rf /usr/local/grass80/share; \
    mkdir -p /usr/local/grass80/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass80/gui/wxpython/xml/module_items.xml;
<<<<<<< HEAD:docker/alpine/Dockerfile
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/alpine/Dockerfile_alpine
=======
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/alpine/Dockerfile_alpine
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
# Reduce the image size - Remove unnecessary grass files
RUN cp /usr/local/grass83/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass83/demolocation; \
    rm -rf /usr/local/grass83/fonts; \
    rm -rf /usr/local/grass83/gui; \
    rm -rf /usr/local/grass83/share; \
    mkdir -p /usr/local/grass83/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass83/gui/wxpython/xml/module_items.xml;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))


FROM common as grass

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/alpine/Dockerfile
=======
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
ENV LC_ALL="en_US.UTF-8"

# Copy GRASS GIS from build image
COPY --from=build /usr/local/bin/grass /usr/local/bin/grass
COPY --from=build /usr/local/grass* /usr/local/grass/
<<<<<<< HEAD
<<<<<<< HEAD
# pip 20.0.0 fix
RUN apk add curl && curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py && python3 get-pip.py pip==20.0.2 && rm get-pip.py

# install external Python API
<<<<<<< HEAD:docker/alpine/Dockerfile
<<<<<<< HEAD:docker/alpine/Dockerfile
RUN pip3 install --upgrade pip six grass-session --ignore-installed six
=======
RUN pip3 install --upgrade grass-session
>>>>>>> da389cd55e (Dockerfile_alpine: fix broken pip six installation (#1568)):docker/alpine/Dockerfile_alpine
=======
RUN pip3 install --upgrade pip six grass-session --ignore-installed six
>>>>>>> 7c10386e82 (g.proj: fix reading input WKT (#1582)):docker/alpine/Dockerfile_alpine

RUN ln -sf /usr/local/grass `grass --config path`
RUN grass --tmp-location EPSG:4326 --exec g.version -rge && \
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

# install external Python API
RUN pip3 install --upgrade pip six grass-session --ignore-installed six

RUN ln -sf /usr/local/grass `grass --config path`
RUN grass --tmp-location XY --exec g.version -rge && \
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    pdal --version && \
    python3 --version


FROM grass as test

RUN apk add make gcc

## run simple LAZ test
COPY docker/testdata/simple.laz /tmp/simple.laz
COPY docker/testdata/test_grass_session.py /scripts/test_grass_session.py
ENV GRASSBIN=grass
<<<<<<< HEAD
<<<<<<< HEAD
RUN grass --tmp-location EPSG:4326 --exec g.extension extension=r.in.pdal

# Not yet ready for GRASS GIS 8:
#RUN /usr/bin/python3 /scripts/test_grass_session.py
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s

# Test addon installation
RUN grass --tmp-location EPSG:4326 --exec g.extension extension=r.learn.ml
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

# Test grass-session
# Not yet ready for GRASS GIS 8:
RUN /usr/bin/python3 /scripts/test_grass_session.py
# Test PDAL
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

# Test addon installation
RUN apk add py3-scikit-learn
RUN grass --tmp-location XY --exec g.extension extension=r.learn.ml2
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))


FROM grass as final

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> e526e86235 (Dockerfile_alpine: fix broken pip six installation (#1568)):docker/alpine/Dockerfile_alpine
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASSBIN="/usr/local/bin/grass" \
    GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
<<<<<<< HEAD
<<<<<<< HEAD
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    GRASSBIN=grass \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python:$PYTHONPATH"

# Copy GRASS GIS and GDAL GRASS driver from build image
COPY --from=build /usr/local/bin/grass /usr/local/bin/grass
COPY --from=build /usr/local/grass* /usr/local/grass/
COPY --from=build /usr/lib/gdalplugins/*_GRASS.so /usr/lib/gdalplugins/
# run simple LAZ test
COPY docker/testdata/simple.laz /tmp/
COPY docker/testdata/test_grass_python.py docker/alpine/grass_tests.sh /scripts/
COPY docker/testdata/test_grass_python.py /scripts/

# install external Python API
RUN ln -sf /usr/local/grass $(grass --config path); \
    # run some tests and cleanup
    $SHELL /scripts/grass_tests.sh \
    && rm -f /scripts/grass_tests.sh /tmp/simple.laz /scripts/test_grass_python.py; \
    # delete unused packages
    apk del --no-cache gettext pdal-dev; \
    # show installed version
    grass --tmp-location XY --exec g.version -rge \
    && pdal --version \
    && python3 --version
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    SHELL="/bin/bash"
ENV PROJ_NETWORK="ON"

<<<<<<< HEAD
=======
<<<<<<< HEAD

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
# show installed version
RUN grass --tmp-location XY --exec g.version -rge && \
    pdal --version && \
    python3 --version
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

# test r.in.pdal in final image because test stage is not executed in github action
COPY docker/testdata/simple.laz /tmp/simple.laz
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g


<<<<<<< HEAD
=======
# test r.in.pdal in final image because test stage is not executed in github action
COPY docker/testdata/simple.laz /tmp/simple.laz
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g


>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
# Data workdir
WORKDIR /grassdb
VOLUME /grassdb

<<<<<<< HEAD
<<<<<<< HEAD
CMD ["$GRASSBIN", "--version"]
=======
CMD $GRASSBIN --version
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
CMD $GRASSBIN --version
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
