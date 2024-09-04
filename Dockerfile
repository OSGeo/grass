# syntax=docker/dockerfile:1.9@sha256:fe40cf4e92cd0c467be2cfc30657a680ae2398318afd50b0c80585784c604f28

# Note: This file must be kept in sync in ./Dockerfile and ./docker/ubuntu/Dockerfile.
#       Changes to this file must be copied over to the other file.

ARG GUI=without
#ARG LDFLAGS="-s -Wl,--no-undefined -lblas"
#ARG CFLAGS="-O2 -std=gnu99 -m64"
#ARG CXXFLAGS=""
#ARG PYTHONPATH="$PYTHONPATH"
#ARG LD_LIBRARY_PATH="/usr/local/lib"
ARG BUILD_DATE="$(date +'%Y-%m-%dT%H:%M:%S%:z')"
ARG REVISION="$(git log -n 1 --pretty=format:'%h')"
ARG BRANCH="$(git branch --show-current)"
ARG VERSION="$(head include/VERSION -n 3 | sed ':a;N;$!ba;s/\n/ /g')"
ARG AUTHORS="Carmen Tawalika,Markus Neteler,Anika Weinmann,Stefan Blumentrath"
ARG MAINTAINERS="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"
ARG SOURCE="https://github.com/OSGeo/grass/tree/${BRANCH}/docker/ubuntu"
ARG VENDOR="GRASS GIS Development Team"
ARG LICENSE="GPL-2.0-or-later"
ARG TITLE="GRASS GIS ${VERSION}"
ARG DESCRIPTION="GRASS GIS (https://grass.osgeo.org/) is a Geographic Information System used for geospatial data management and analysis, image processing, graphics/map production, spatial modeling, and visualization."
ARG URL="https://github.com/OSGeo/grass/tree/${BRANCH}/docker"
ARG BASE_NAME="ubuntu:22.04"
ARG DOCUMENTATION="https://github.com/OSGeo/grass/tree/${BRANCH}/docker/README.md"
ARG REF_NAME="grass-gis"
ARG DIGEST="sha256:340d9b015b194dc6e2a13938944e0d016e57b9679963fdeb9ce021daac430221"

FROM ubuntu:22.04@sha256:340d9b015b194dc6e2a13938944e0d016e57b9679963fdeb9ce021daac430221 AS common_start

ARG BUILD_DATE
ARG REVISION
ARG BRANCH
ARG AUTHORS
ARG MAINTAINERS
ARG VERSION
ARG TITLE
ARG DESCRIPTION
ARG LICENSE
ARG SOURCE
ARG VENDOR
ARG URL
ARG GUI
ARG BASE_NAME
ARG DOCUMENTATION
ARG REF_NAME
ARG DIGEST

LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      org.opencontainers.image.license="$LICENSE" \
      org.opencontainers.image.created="$BUILD_DATE" \
      org.opencontainers.image.revision="$REVISION" \
      org.opencontainers.image.url="$URL" \
      org.opencontainers.image.source="$SOURCE" \
      org.opencontainers.image.version="$VERSION" \
      org.opencontainers.image.title="$TITLE" \
      org.opencontainers.image.description="$DESCRIPTION" \
      org.opencontainers.image.base.name="$BASE_NAME" \
      org.opencontainers.image.base.digest="$DIGEST" \
      org.opencontainers.image.ref.name="$REF_NAME" \
      org.opencontainers.image.documentation="$DOCUMENTATION" \
      maintainers="$MAINTAINERS"


ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

# WORKDIR /tmp

# Todo: re-consider required dev packages for addons (~400MB in dev packages)
ARG GRASS_RUN_PACKAGES="build-essential \
  bison \
  bzip2 \
  curl \
  flex \
  g++ \
  gcc \
  gdal-bin \
  geos-bin \
  proj-bin \
  netcdf-bin \
  git \
  language-pack-en-base \
  libcairo2 \
  libcurl4-gnutls-dev \
  libfftw3-bin \
  libfftw3-dev \
  libfreetype6 \
  libgdal-dev \
  libgsl27 \
  libjpeg-turbo8 \
  libjsoncpp-dev \
  libmagic1 \
  libmagic-mgc \
  libncurses5 \
  libopenblas-dev \
  libopenblas-base \
  libopenjp2-7 \
  libomp5 \
  libomp-dev \
  libgeos-dev \
  libpdal-dev \
  libproj-dev \
  libpq-dev \
  libgsl-dev \
  libpdal-base13 \
  libpdal-plugin-hdf \
  libpdal-plugins \
  libpdal-util13 \
  libpnglite0 \
  libpq5 \
  libpython3-all-dev \
  libreadline8 \
  libsqlite3-0 \
  libtiff-tools \
  libzstd1 \
  locales \
  make \
  mesa-utils \
  moreutils \
  ncurses-bin \
  pdal \
  proj-data \
  python-is-python3 \
  python3 \
  python3-dev \
  python3-venv \
  sqlite3 \
  unzip \
  vim \
  wget \
  zip \
  zlib1g \
"

# Define build packages
ARG GRASS_BUILD_PACKAGES="cmake \
  libbz2-dev \
  libcairo2-dev \
  libfreetype6-dev \
  zlib1g-dev \
  libnetcdf-dev \
  libopenjp2-7-dev \
  libreadline-dev \
  libjpeg-dev \
  libpnglite-dev \
  libsqlite3-dev \
  libtiff-dev \
  libzstd-dev \
  libncurses5-dev \
  mesa-common-dev \
  zlib1g-dev \
"

ARG GRASS_CONFIG="--with-cxx \
  --enable-largefile \
  --with-proj-share=/usr/share/proj \
  --with-gdal=/usr/bin/gdal-config \
  --with-geos \
  --with-sqlite \
  --with-cairo --with-cairo-ldflags=-lfontconfig \
  --with-freetype --with-freetype-includes=/usr/include/freetype2/ \
  --with-fftw \
  --with-postgres --with-postgres-includes=/usr/include/postgresql \
  --with-netcdf \
  --with-zstd \
  --with-bzlib \
  --with-pdal \
  --without-mysql \
  --with-blas \
  --with-lapack \
  --with-readline \
  --with-odbc \
  --with-openmp \
"

ARG GRASS_PYTHON_PACKAGES="pip \
  setuptools \
  python-dateutil \
  python-magic \
  numpy<2 \
  Pillow \
  ply \
  matplotlib \
  psycopg2 \
"

FROM common_start AS grass_without_gui

ARG GRASS_CONFIG="${GRASS_CONFIG} --without-opengl --without-x"

FROM common_start AS grass_with_gui

ARG GRASS_RUN_PACKAGES="${GRASS_RUN_PACKAGES} adwaita-icon-theme-full \
  libglu1-mesa \
  libgtk-3-0 \
  libnotify4 \
  libsdl2-2.0-0 \
  libxtst6 \
  librsvg2-common \
  gettext \
  freeglut3 \
  libgstreamer-plugins-base1.0 \
  libjpeg8 \
  libpng16-16 \
  libsm6 \
  libtiff5 \
  libwebkit2gtk-4.0 \
"

# librsvg2-common \
# (fix error (wxgui.py:7782): Gtk-WARNING **: 19:53:09.774:
# Could not load a pixbuf from /org/gtk/libgtk/theme/Adwaita/assets/check-symbolic.svg.
# This may indicate that pixbuf loaders or the mime database could not be found.)

ARG GRASS_BUILD_PACKAGES="${GRASS_BUILD_PACKAGES} adwaita-icon-theme-full \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  freeglut3-dev \
  libgstreamer-plugins-base1.0-dev \
  libgtk-3-dev \
  libjpeg-dev \
  libnotify-dev \
  libpng-dev \
  libsdl2-dev \
  libsm-dev \
  libtiff-dev \
  libwebkit2gtk-4.0-dev \
  libxtst-dev \
"

ARG GRASS_ADDITIONAL_CONFIG="--with-opengl \
  --with-x \
  --with-nls \
  --with-readline \
"
ARG GRASS_PYTHON_PACKAGES="${GRASS_PYTHON_PACKAGES} wxPython"
# If you do not use any Gnome Accessibility features, to suppress warning
# WARNING **: Couldn't connect to accessibility bus:
# execute programs with
ENV NO_AT_BRIDGE=1

# hadolint ignore=DL3006
FROM grass_${GUI}_gui AS grass_gis

LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      org.opencontainers.image.license="$LICENSE" \
      org.opencontainers.image.created="$BUILD_DATE" \
      org.opencontainers.image.revision="$REVISION" \
      org.opencontainers.image.url="$URL" \
      org.opencontainers.image.source="$SOURCE" \
      org.opencontainers.image.version="$VERSION" \
      org.opencontainers.image.title="$TITLE" \
      org.opencontainers.image.description="$DESCRIPTION" \
      maintainers="$MAINTAINERS"

# Add ubuntugis unstable and fetch packages
RUN apt-get update \
    && apt-get install  -y --no-install-recommends --no-install-suggests \
    software-properties-common \
    gpg \
    gpg-agent \
    && LC_ALL=C.UTF-8 add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable \
    && apt-get update -y && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    $GRASS_RUN_PACKAGES \
    # $GRASS_ADDITIONAL_RUN_PACKAGES \
    && apt-get remove -y gpg gpg-agent \
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && echo LANG="en_US.UTF-8" > /etc/default/locale \
    && echo en_US.UTF-8 UTF-8 >> /etc/locale.gen \
    && locale-gen

## fetch vertical datums for PDAL and store into PROJ dir
# WORKDIR /src

# # Get datum grids
# # Currently using https://proj.org/en/9.3/usage/network.html#how-to-enable-network-capabilities
# FROM ubuntu:22.04 as datum_grids

# # See: https://github.com/OSGeo/PROJ-data
# RUN apt-get update \
#     && apt-get install  -y --no-install-recommends --no-install-suggests \
#     wget \
#     && apt-get clean all \
#     && rm -rf /var/lib/apt/lists/*

# WORKDIR /tmp
# RUN wget -q --no-check-certificate -r -l inf -A tif https://cdn.proj.org/

# Start build stage
FROM grass_gis AS build

# Add build and Python packages
# hadolint ignore=SC2086
RUN apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    ${GRASS_BUILD_PACKAGES} \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && (echo "Install Python" \
    && wget --progress="dot:giga" -q https://bootstrap.pypa.io/pip/get-pip.py \
    # && apt-get install -y python3-ensurepip \
    && python get-pip.py \
    # && python3 -m ensurepip --upgrade \
    && rm -r get-pip.py \
    && mkdir -p /src/site-packages \
    # && cd /src \
    && python -m pip install --no-cache-dir -t /src/site-packages --upgrade \
    ${GRASS_PYTHON_PACKAGES} \
    && rm -r /root/.cache \
    && rm -rf /tmp/pip-* \
    )

# copy grass gis source
COPY . /src/grass_build/

WORKDIR /src/grass_build

# Set environmental variables for GRASS GIS compilation, without debug symbols
# Set gcc/g++ environmental variables for GRASS GIS compilation, without debug symbols
#ENV MYCFLAGS="-O2 -std=gnu99 -m64"
#ENV MYLDFLAGS="-s"
# CXX stuff:

ENV LD_LIBRARY_PATH="/usr/local/lib" \
    LDFLAGS="-s -Wl,--no-undefined -lblas" \
    CFLAGS="-O2 -std=gnu99 -m64" \
    CXXFLAGS="" \
    # PYTHONPATH="$PYTHONPATH" \
    NUMTHREADS=4

# Configure compile and install GRASS GIS
# and the GDAL-GRASS plugin
# hadolint ignore=DL3003
RUN make distclean || echo "nothing to clean" \
    && ./configure ${GRASS_CONFIG} \
    && make -j $NUMTHREADS \
    && make install && ldconfig \
    &&  cp /usr/local/grass85/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass85/demolocation; \
    rm -rf /usr/local/grass85/fonts; \
    rm -rf /usr/local/grass85/gui; \
    rm -rf /usr/local/grass85/share; \
    mkdir -p /usr/local/grass85/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass85/gui/wxpython/xml/module_items.xml \
    && git clone https://github.com/OSGeo/gdal-grass \
    && cd "gdal-grass" \
    && ./configure \
      --with-gdal=/usr/bin/gdal-config \
      --with-grass=/usr/local/grass85 \
    && make -j $NUMTHREADS \
    && make install -j $NUMTHREADS \
    && cd /src \
    && rm -rf "gdal-grass"

# Leave build stage
FROM grass_gis AS grass_gis_final
LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      org.opencontainers.image.license="$LICENSE" \
      org.opencontainers.image.created="$BUILD_DATE" \
      org.opencontainers.image.revision="$REVISION" \
      org.opencontainers.image.url="$URL" \
      org.opencontainers.image.source="$SOURCE" \
      org.opencontainers.image.version="$VERSION" \
      org.opencontainers.image.title="$TITLE" \
      org.opencontainers.image.description="$DESCRIPTION" \
      maintainers="$MAINTAINERS"

# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python/" \
    LD_LIBRARY_PATH="/usr/local/grass/lib:/usr/local/lib" \
    GDAL_DRIVER_PATH="/usr/lib/gdalplugins"

# Copy GRASS GIS from build image
COPY --link --from=build /usr/local/bin/* /usr/local/bin/
COPY --link --from=build /usr/local/grass85 /usr/local/grass85/
COPY --link --from=build /src/site-packages /usr/lib/python3.10/
COPY --link --from=build /usr/lib/gdalplugins /usr/lib/gdalplugins
# COPY --link --from=datum_grids /tmp/cdn.proj.org/*.tif /usr/share/proj/

# Create generic GRASS GIS lib name regardless of version number
# and show GRASS GIS, PROJ, GDAL etc versions
RUN ln -sf /usr/local/grass85 /usr/local/grass \
    && grass --tmp-project EPSG:4326 --exec g.version -rge \
    && pdal --version \
    && python --version

# WORKDIR /scripts

# enable GRASS GIS Python session support
## grass --config python-path
# ENV PYTHONPATH="/usr/local/grass/etc/python:${PYTHONPATH}"
# enable GRASS GIS ctypes imports
## grass --config path
# ENV LD_LIBRARY_PATH="/usr/local/grass/lib:$LD_LIBRARY_PATH"

WORKDIR /tmp
COPY docker/testdata/simple.laz .
WORKDIR /scripts
COPY docker/testdata/test_grass_session.py .
## run GRASS GIS python session and scan the test LAZ file
RUN python /scripts/test_grass_session.py \
    && rm -rf /tmp/grasstest_epsg_25832 \
    && grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

WORKDIR /grassdb
VOLUME /grassdb
