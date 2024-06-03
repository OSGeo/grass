<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
FROM ubuntu:22.04
=======
<<<<<<< HEAD
FROM ubuntu:22.04@sha256:a6d2b38300ce017add71440577d5b0a90460d0e57fd7aec21dd0d1b0761bbfb2
>>>>>>> osgeo-main

<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Tomas Zigo"
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
FROM ubuntu:22.04@sha256:a6d2b38300ce017add71440577d5b0a90460d0e57fd7aec21dd0d1b0761bbfb2

<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Tomas Zigo"
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann"
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann"
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
FROM ubuntu:22.04

LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Tomas Zigo"
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
FROM ubuntu:22.04

LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Tomas Zigo"
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
LABEL maintainer="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"

ENV DEBIAN_FRONTEND noninteractive

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
# define versions to be used (PDAL is not available on Ubuntu/Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
# define versions to be used (PDAL is not available on Ubuntu/Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
# define versions to be used
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
# https://github.com/hobuinc/laz-perf/releases
ARG LAZ_PERF_VERSION=3.2.0
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
# define versions to be used (PDAL is not available on Ubuntu/Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
# define versions to be used (PDAL is not available on Ubuntu/Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
ARG PDAL_VERSION=2.4.3
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

SHELL ["/bin/bash", "-c"]

WORKDIR /tmp

# wxGUI require packages:
# adwaita-icon-theme-full
# libgl1-mesa-dev
# libglu1-mesa-dev
# libgtk-3-0
# libnotify4
# libsdl2-2.0-0
# libxtst6
# librsvg2-common (fix error (wxgui.py:7782): Gtk-WARNING **: 19:53:09.774:
# Could not load a pixbuf from /org/gtk/libgtk/theme/Adwaita/assets/check-symbolic.svg.
# This may indicate that pixbuf loaders or the mime database could not be found.)
# gettext (require for configure option --with-nls) enable wxGUI localization
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends --no-install-suggests \
    adwaita-icon-theme-full \
    build-essential \
    bison \
    bzip2 \
    cmake \
    curl \
    flex \
    g++ \
    gcc \
    gdal-bin \
    gettext \
    git \
    language-pack-en-base \
    libbz2-dev \
    libcairo2 \
    libcairo2-dev \
    libcurl4-gnutls-dev \
    libfftw3-bin \
    libfftw3-dev \
    libfreetype6-dev \
    libgl1-mesa-dev \
    libgdal-dev \
    libgeos-dev \
    libglu1-mesa-dev \
    libgsl0-dev \
    libgtk-3-0 \
    libjpeg-dev \
    libjsoncpp-dev \
    libnetcdf-dev \
    libncurses5-dev \
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    libnotify4 \
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
    libnotify4 \
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
    libnotify4 \
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
    libnotify4 \
=======
>>>>>>> osgeo-main
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
    libnotify4 \
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    libnotify4 \
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    libopenblas-base \
    libopenblas-dev \
    libopenjp2-7 \
    libopenjp2-7-dev \
    libpnglite-dev \
    libpq-dev \
    libproj-dev \
    libpython3-all-dev \
    librsvg2-common \
    libreadline-dev \
    libsdl2-2.0-0 \
    libsqlite3-dev \
    libtiff-dev \
    libxtst6 \
    libzstd-dev \
    locales \
    make \
    mesa-common-dev \
    moreutils \
    ncurses-bin \
    netcdf-bin \
    proj-bin \
    proj-data \
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
<<<<<<< HEAD
    zlib1g-dev
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
    zlib1g-dev \
    && apt-get clean all && rm -rf /var/lib/apt/lists/*
=======
    zlib1g-dev
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    zlib1g-dev
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

# wxGUI
RUN pip install -U \
    -f https://extras.wxpython.org/wxPython4/extras/linux/gtk3/ubuntu-22.04 \
    wxPython \
    && pip cache purge
# If you do not use any Gnome Accessibility features, to suppress warning
# WARNING **: Couldn't connect to accessibility bus:
# execute programs with
ENV NO_AT_BRIDGE=1

RUN echo LANG="en_US.UTF-8" > /etc/default/locale
RUN echo en_US.UTF-8 UTF-8 >> /etc/locale.gen && locale-gen

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
=======
## install the latest projection library for GRASS GIS
=======
## install a more recent projection library for GRASS GIS
>>>>>>> a8e084d159 (Dockerfile Debian: versionless link fix):docker/debian/Dockerfile_debian_pdal
=======
## install a more recent projection library for GRASS GIS
>>>>>>> aac6232e2b (Dockerfile Debian: versionless link fix):docker/debian/Dockerfile_debian_pdal
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
=======
## install laz-perf (missing from https://packages.ubuntu.com/)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
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
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
=======
>>>>>>> osgeo-main
## install pdal
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
      -DBUILD_PLUGIN_PGPOINTCLOUD=OFF \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=OFF \
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
      -DBUILD_PLUGIN_PGPOINTCLOUD=ON \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=ON \
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
      -DBUILD_PLUGIN_PGPOINTCLOUD=OFF \
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DWITH_LASZIP=OFF \
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
      -DWITH_LAZPERF=ON \
      -DWITH_TESTS=ON && \
    make -j $NUMTHREADS && \
    make install

# copy grass gis source
WORKDIR /src
COPY . /src/grass_build/
WORKDIR /src/grass_build

# Cleanup potentially leftover GISRC file with wrong path to "demolocation"
RUN rm -f /src/grass_build/dist.*/demolocation/.grassrc*

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
# wxGUI require
# --with-x
# --with-nls
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
  --with-x \
  --with-nls \
  --with-readline \
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
<<<<<<< HEAD
# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass83 /usr/local/grass
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
# https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
ENV PROJ_NETWORK=ON

=======
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/debian/Dockerfile_debian_pdal
=======
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610)):docker/debian/Dockerfile_debian_pdal
# Create generic GRASS GIS lib name regardless of version number
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
RUN ln -sf /usr/local/grass84 /usr/local/grass
=======
RUN ln -sf /usr/local/grass `grass --config path`
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
RUN ln -sf /usr/local/grass `grass --config path`
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
RUN ln -sf /usr/local/grass80 /usr/local/grass
>>>>>>> a8e084d159 (Dockerfile Debian: versionless link fix):docker/debian/Dockerfile_debian_pdal
=======
RUN ln -sf /usr/local/grass80 /usr/local/grass
>>>>>>> aac6232e2b (Dockerfile Debian: versionless link fix):docker/debian/Dockerfile_debian_pdal
=======
# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass83 /usr/local/grass
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass83 /usr/local/grass
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

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
RUN pip3 install grass-session

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
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
<<<<<<< HEAD:docker/ubuntu_wxgui/Dockerfile
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
=======
>>>>>>> osgeo-main
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> 756514063b (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -s
>>>>>>> c875f035a5 (Dockerfile: fix broken lib link (#1625)):docker/debian/Dockerfile_debian_pdal
=======
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
# test LAZ file
RUN grass --tmp-location EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

WORKDIR /grassdb
VOLUME /grassdb
