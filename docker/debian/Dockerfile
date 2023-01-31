<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
FROM debian:stable@sha256:84725fb1debbf461f54a23030be0bcde8449f2fa4eba8adf857b1a4c707fec8d
# currently Debian 12

# docker run -it --rm debian:stable bash
# apt-get update && apt-get install lsb-release -y && lsb_release -a
=======
FROM debian:10.1
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
FROM debian:10.1
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
FROM debian:10.1
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
FROM debian:stable
# currently Debian 11

# docker run -it --rm debian:stable bash
# apt-get update && apt-get install lsb-release -y && lsb_release -a
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann"
LABEL maintainer="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"

ENV DEBIAN_FRONTEND noninteractive

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
# define versions to be used (PDAL is not available on Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.5.5
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
# define versions to be used
ARG PDAL_VERSION=1.8.0
ARG PROJ_VERSION=5.2.0
ARG PROJ_DATUMGRID_VERSION=1.8
ARG LAZ_PERF_VERSION=1.3.0
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
# define versions to be used (PDAL is not available on Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

SHELL ["/bin/bash", "-c"]

WORKDIR /tmp

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends --no-install-suggests \
    build-essential \
    bison \
    bzip2 \
    cmake \
    curl \
    flex \
    g++ \
    gcc \
    gdal-bin \
    git \
    libbz2-dev \
    libcairo2 \
    libcairo2-dev \
    libcurl4-gnutls-dev \
    libfftw3-bin \
    libfftw3-dev \
<<<<<<< HEAD
<<<<<<< HEAD
    libfreetype-dev \
=======
    libfreetype6-dev \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    libfreetype6-dev \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    libgdal-dev \
    libgeos-dev \
    libgsl0-dev \
    libjpeg-dev \
    libjsoncpp-dev \
    libnetcdf-dev \
<<<<<<< HEAD
<<<<<<< HEAD
    libncurses-dev \
=======
    libncurses5-dev \
    libopenblas-base \
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    libncurses5-dev \
    libopenblas-base \
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    libopenblas-dev \
    libopenjp2-7 \
    libopenjp2-7-dev \
    libpnglite-dev \
    libpq-dev \
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    libproj-dev \
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    libproj-dev \
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
=======
    libproj-dev \
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    libpython3-all-dev \
    libsqlite3-dev \
    libtiff-dev \
    libzstd-dev \
    locales \
    make \
    mesa-common-dev \
    moreutils \
    ncurses-bin \
    netcdf-bin \
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    proj-bin \
    proj-data \
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    proj-bin \
    proj-data \
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
=======
    proj-bin \
    proj-data \
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    python3 \
    python3-dateutil \
    python3-dev \
    python3-magic \
    python3-numpy \
    python3-pil \
    python3-pip \
    python3-ply \
    python3-setuptools \
    python3-venv \
    software-properties-common \
    sqlite3 \
    subversion \
    unzip \
    vim \
    wget \
    zip \
<<<<<<< HEAD
<<<<<<< HEAD
    zlib1g-dev \
    && apt-get clean all && rm -rf /var/lib/apt/lists/*
=======
    zlib1g-dev
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    zlib1g-dev
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

RUN echo LANG="en_US.UTF-8" > /etc/default/locale
RUN echo en_US.UTF-8 UTF-8 >> /etc/locale.gen && locale-gen

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
## install a more recent projection library for GRASS GIS
WORKDIR /src
RUN wget -q http://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz && \
    tar xzf proj-${PROJ_VERSION}.tar.gz && \
    cd /src/proj-${PROJ_VERSION}/ && \
    wget -q http://download.osgeo.org/proj/proj-datumgrid-${PROJ_DATUMGRID_VERSION}.zip && \
    cd nad && \
    unzip ../proj-datumgrid-${PROJ_DATUMGRID_VERSION}.zip && \
    cd .. && \
    ./configure --prefix=/usr/ && \
    make && \
    make install

## install laz-perf
RUN apt-get install cmake
WORKDIR /src
RUN wget -q https://github.com/hobu/laz-perf/archive/${LAZ_PERF_VERSION}.tar.gz -O laz-perf-${LAZ_PERF_VERSION}.tar.gz && \
    tar -zxf laz-perf-${LAZ_PERF_VERSION}.tar.gz && \
    cd laz-perf-${LAZ_PERF_VERSION} && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
## fetch vertical datums for PDAL and store into PROJ dir
WORKDIR /src
RUN mkdir vdatum && \
    cd vdatum && \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2012.zip && unzip -j -u usa_geoid2012.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2009.zip && unzip -j -u usa_geoid2009.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2003.zip && unzip -j -u usa_geoid2003.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid1999.zip && unzip -j -u usa_geoid1999.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertconc.gtx && mv vertconc.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertcone.gtx && mv vertcone.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertconw.gtx && mv vertconw.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/egm96_15/egm96_15.gtx && mv egm96_15.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/egm08_25/egm08_25.gtx && mv egm08_25.gtx /usr/share/proj; \
    cd .. && \
    rm -rf vdatum

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
=======
## install pdal
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
## install pdal
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
## install pdal
=======
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
ENV NUMTHREADS=4
WORKDIR /src
RUN wget -q \
 https://github.com/PDAL/PDAL/releases/download/${PDAL_VERSION}/PDAL-${PDAL_VERSION}-src.tar.gz && \
    tar xfz PDAL-${PDAL_VERSION}-src.tar.gz && \
    cd /src/PDAL-${PDAL_VERSION}-src && \
    mkdir build && \
    cd build && \
    cmake .. \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_C_COMPILER=gcc \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_MAKE_PROGRAM=make \
      -DBUILD_PLUGIN_PYTHON=ON \
      -DBUILD_PLUGIN_CPD=OFF \
      -DBUILD_PLUGIN_GREYHOUND=ON \
      -DBUILD_PLUGIN_HEXBIN=ON \
      -DHEXER_INCLUDE_DIR=/usr/include/ \
      -DBUILD_PLUGIN_NITF=OFF \
      -DBUILD_PLUGIN_ICEBRIDGE=ON \
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
      -DBUILD_PLUGIN_PGPOINTCLOUD=OFF \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=OFF \
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
      -DBUILD_PLUGIN_PGPOINTCLOUD=ON \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=ON \
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
      -DBUILD_PLUGIN_PGPOINTCLOUD=OFF \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=OFF \
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
      -DWITH_LAZPERF=ON \
      -DWITH_TESTS=ON && \
    make -j $NUMTHREADS && \
    make install

# copy grass gis source
WORKDIR /src
COPY . /src/grass_build/
WORKDIR /src/grass_build
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
# Cleanup potentially leftover GISRC file with wrong path to "demolocation"
RUN rm -f /src/grass_build/dist.*/demolocation/.grassrc*
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
# Cleanup potentially leftover GISRC file with wrong path to "demolocation"
RUN rm -f /src/grass_build/dist.*/demolocation/.grassrc*
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
=======
# Cleanup potentially leftover GISRC file with wrong path to "demolocation"
RUN rm -f /src/grass_build/dist.*/demolocation/.grassrc*
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

# Set environmental variables for GRASS GIS compilation, without debug symbols
# Set gcc/g++ environmental variables for GRASS GIS compilation, without debug symbols
ENV MYCFLAGS "-O2 -std=gnu99 -m64"
ENV MYLDFLAGS "-s"
# CXX stuff:
ENV LD_LIBRARY_PATH "/usr/local/lib"
ENV LDFLAGS "$MYLDFLAGS"
ENV CFLAGS "$MYCFLAGS"
ENV CXXFLAGS "$MYCXXFLAGS"

# Configure compile and install GRASS GIS
ENV GRASS_PYTHON=/usr/bin/python3
ENV NUMTHREADS=4
RUN make distclean || echo "nothing to clean"
RUN /src/grass_build/configure \
  --with-cxx \
  --enable-largefile \
  --with-proj-share=/usr/share/proj \
  --with-gdal=/usr/bin/gdal-config \
  --with-geos \
  --with-sqlite \
  --with-cairo --with-cairo-ldflags=-lfontconfig \
  --with-freetype --with-freetype-includes="/usr/include/freetype2/" \
  --with-fftw \
  --with-postgres --with-postgres-includes="/usr/include/postgresql" \
  --with-netcdf \
  --with-zstd \
  --with-bzlib \
  --with-pdal \
  --without-mysql \
  --without-odbc \
  --without-openmp \
  --without-opengl \
    && make -j $NUMTHREADS \
    && make install && ldconfig

# Unset environmental variables to avoid later compilation issues
ENV INTEL ""
ENV MYCFLAGS ""
ENV MYLDFLAGS ""
ENV MYCXXFLAGS ""
ENV LD_LIBRARY_PATH ""
ENV LDFLAGS ""
ENV CFLAGS ""
ENV CXXFLAGS ""

# set SHELL var to avoid /bin/sh fallback in interactive GRASS GIS sessions
ENV SHELL /bin/bash
ENV LC_ALL "en_US.UTF-8"
ENV GRASS_SKIP_MAPSET_OWNER_CHECK 1

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/debian/Dockerfile
<<<<<<< HEAD:docker/debian/Dockerfile
# https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
ENV PROJ_NETWORK=ON

=======
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/ubuntu/Dockerfile_ubuntu_pdal
# Create generic GRASS GIS lib name regardless of version number
<<<<<<< HEAD:docker/debian/Dockerfile
<<<<<<< HEAD:docker/debian/Dockerfile
<<<<<<< HEAD:docker/debian/Dockerfile
<<<<<<< HEAD:docker/debian/Dockerfile
RUN ln -sf /usr/local/grass84 /usr/local/grass
=======
RUN ln -sf /usr/local/grass `grass --config path`
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
RUN ln -sf /usr/local/grass `grass --config path`
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
RUN ln -sf /usr/local/grass80 /usr/local/grass
>>>>>>> a8e084d159 (Dockerfile Debian: versionless link fix):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
RUN ln -sf /usr/local/grass80 /usr/local/grass
>>>>>>> aac6232e2b (Dockerfile Debian: versionless link fix):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass83 /usr/local/grass
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass83 /usr/local/grass
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

# show GRASS GIS, PROJ, GDAL etc versions
RUN grass --tmp-location EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python3 --version

# Reduce the image size
RUN apt-get autoremove -y
RUN apt-get clean -y
RUN rm -r /src/grass_build/.git

WORKDIR /scripts

# install external GRASS GIS session Python API
<<<<<<< HEAD
<<<<<<< HEAD
RUN pip3 install grass-session --break-system-packages
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
RUN pip3 install grass-session

<<<<<<< HEAD
# install GRASS GIS extensions
RUN grass --tmp-location EPSG:4326 --exec g.extension extension=r.in.pdal
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

=======
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
# add GRASS GIS envs for python usage
ENV GISBASE "/usr/local/grass/"
ENV GRASSBIN "/usr/local/bin/grass"
ENV PYTHONPATH "${PYTHONPATH}:$GISBASE/etc/python/"
ENV LD_LIBRARY_PATH "$LD_LIBRARY_PATH:$GISBASE/lib"

WORKDIR /tmp
COPY docker/testdata/simple.laz .
WORKDIR /scripts
COPY docker/testdata/test_grass_session.py .
## just scan the LAZ file
# Not yet ready for GRASS GIS 8:
#RUN /usr/bin/python3 /scripts/test_grass_session.py
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/debian/Dockerfile
<<<<<<< HEAD:docker/debian/Dockerfile
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/ubuntu/Dockerfile_ubuntu_pdal
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
=======
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

WORKDIR /grassdb
VOLUME /grassdb
