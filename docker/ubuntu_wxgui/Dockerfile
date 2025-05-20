FROM ubuntu:22.04@sha256:d80997daaa3811b175119350d84305e1ec9129e1799bba0bd1e3120da3ff52c3

LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Tomas Zigo"
LABEL maintainer="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"

ENV DEBIAN_FRONTEND=noninteractive

# define versions to be used (PDAL is not available on Ubuntu/Debian, so we compile it here)
# https://github.com/PDAL/PDAL/releases
# renovate: datasource=github-tags depName=PDAL/PDAL
ARG PDAL_VERSION=2.7.1

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
    bison \
    build-essential \
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
    libgdal-dev \
    libgeos-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libgsl0-dev \
    libgtk-3-0 \
    libgtk-3-dev \
    libjpeg-dev \
    libjsoncpp-dev \
    libncurses5-dev \
    libnetcdf-dev \
    libnotify4 \
    libopenblas-base \
    libopenblas-dev \
    libopenjp2-7 \
    libopenjp2-7-dev \
    libpnglite-dev \
    libpq-dev \
    libproj-dev \
    libpython3-all-dev \
    libreadline-dev \
    librsvg2-common \
    libsecret-1-0 \
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
    python-is-python3 \
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
    python3-wxgtk4.0 \
    software-properties-common \
    sqlite3 \
    subversion \
    unzip \
    vim \
    wget \
    zip \
    zlib1g-dev \
    && apt-get clean all && rm -rf /var/lib/apt/lists/*

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

## fetch vertical datums for PDAL and store into PROJ dir
WORKDIR /src
RUN mkdir vdatum && \
    cd vdatum && \
    wget -q http://download.osgeo.org/proj/vdatum/egm08_25/egm08_25.gtx && mv egm08_25.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/egm96_15/egm96_15.gtx && mv egm96_15.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid1999.zip && unzip -j -u usa_geoid1999.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2003.zip && unzip -j -u usa_geoid2003.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2009.zip && unzip -j -u usa_geoid2009.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/usa_geoid2012.zip && unzip -j -u usa_geoid2012.zip -d /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertconc.gtx && mv vertconc.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertcone.gtx && mv vertcone.gtx /usr/share/proj; \
    wget -q http://download.osgeo.org/proj/vdatum/vertcon/vertconw.gtx && mv vertconw.gtx /usr/share/proj; \
    cd .. && \
    rm -rf vdatum

## compile and install PDAL (not available on Debian/Ubuntu) with laz-perf enabled
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
      -DBUILD_PGPOINTCLOUD_TESTS=OFF \
      -DBUILD_PLUGIN_CPD=OFF \
      -DBUILD_PLUGIN_GREYHOUND=ON \
      -DBUILD_PLUGIN_HEXBIN=ON \
      -DBUILD_PLUGIN_ICEBRIDGE=ON \
      -DBUILD_PLUGIN_NITF=OFF \
      -DBUILD_PLUGIN_PGPOINTCLOUD=OFF \
      -DBUILD_PLUGIN_PYTHON=ON \
      -DBUILD_PLUGIN_SQLITE=ON \
      -DHEXER_INCLUDE_DIR=/usr/include/ \
      -DWITH_LASZIP=OFF \
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
ENV MYCFLAGS="-O2 -std=gnu99 -m64"
ENV MYLDFLAGS="-s"
# CXX stuff:
ENV LD_LIBRARY_PATH="/usr/local/lib"
ENV LDFLAGS="$MYLDFLAGS"
ENV CFLAGS="$MYCFLAGS"
ENV CXXFLAGS="$MYCXXFLAGS"

# Configure compile and install GRASS GIS
# wxGUI require
# --with-nls
# --with-x
ENV NUMTHREADS=4
RUN make distclean || echo "nothing to clean"
RUN /src/grass_build/configure \
  --enable-largefile \
  --with-bzlib \
  --with-cairo --with-cairo-ldflags=-lfontconfig \
  --with-cxx \
  --with-fftw \
  --with-freetype --with-freetype-includes="/usr/include/freetype2/" \
  --with-gdal=/usr/bin/gdal-config \
  --with-geos \
  --with-netcdf \
  --with-nls \
  --with-pdal \
  --with-postgres --with-postgres-includes="/usr/include/postgresql" \
  --with-proj-share=/usr/share/proj \
  --with-readline \
  --with-sqlite \
  --with-x \
  --with-zstd \
  --without-mysql \
  --without-odbc \
  --without-openmp \
    && make -j $NUMTHREADS \
    && make install && ldconfig

# Unset environmental variables to avoid later compilation issues
ENV INTEL=""
ENV MYCFLAGS=""
ENV MYLDFLAGS=""
ENV MYCXXFLAGS=""
ENV LD_LIBRARY_PATH=""
ENV LDFLAGS=""
ENV CFLAGS=""
ENV CXXFLAGS=""

# set SHELL var to avoid /bin/sh fallback in interactive GRASS GIS sessions
ENV SHELL=/bin/bash
ENV LC_ALL="en_US.UTF-8"
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1

# https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
ENV PROJ_NETWORK=ON

# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass85 /usr/local/grass

# show GRASS GIS, PROJ, GDAL etc versions
RUN grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python --version

# Reduce the image size
RUN apt-get autoremove -y
RUN apt-get clean -y
RUN rm -r /src/grass_build/.git

WORKDIR /scripts

# enable GRASS GIS Python session support
## grass --config python-path
ENV PYTHONPATH="/usr/local/grass/etc/python:${PYTHONPATH}"
# enable GRASS GIS ctypes imports
## grass --config path
ENV LD_LIBRARY_PATH="/usr/local/grass/lib:$LD_LIBRARY_PATH"

WORKDIR /tmp
COPY docker/testdata/simple.laz .
WORKDIR /scripts
COPY docker/testdata/test_grass_session.py .
## run GRASS GIS python session and scan the test LAZ file
RUN python /scripts/test_grass_session.py
RUN rm -rf /tmp/grasstest_epsg_25832
# test LAZ file
RUN grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

WORKDIR /grassdb
VOLUME /grassdb
CMD ["$GRASSBIN", "--version"]
