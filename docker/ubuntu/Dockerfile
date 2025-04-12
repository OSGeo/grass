# syntax=docker/dockerfile:1.14@sha256:4c68376a702446fc3c79af22de146a148bc3367e73c25a5803d453b6b3f722fb

# Note: This file must be kept in sync in ./Dockerfile and ./docker/ubuntu/Dockerfile.
#       Changes to this file must be copied over to the other file.

ARG GUI=without

FROM ubuntu:22.04@sha256:d80997daaa3811b175119350d84305e1ec9129e1799bba0bd1e3120da3ff52c3 AS common_start

LABEL authors="Carmen Tawalika,Markus Neteler,Anika Weinmann,Stefan Blumentrath"
LABEL maintainer="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

WORKDIR /tmp

ARG GUI

# Todo: re-consider required dev packages for addons (~400MB in dev packages)
ARG GRASS_RUN_PACKAGES="\
    bison \
    build-essential \
    bzip2 \
    curl \
    flex \
    g++ \
    gcc \
    gdal-bin \
    geos-bin \
    git \
    language-pack-en-base \
    libcairo2 \
    libcurl4-gnutls-dev \
    libfftw3-bin \
    libfftw3-dev \
    libfreetype6 \
    libgdal-dev \
    libgeos-dev \
    libgsl-dev \
    libgsl27 \
    libjpeg-turbo8 \
    libjsoncpp-dev \
    liblapacke-dev \
    libmagic-mgc \
    libmagic1 \
    libncurses5 \
    libomp-dev \
    libomp5 \
    libopenblas-base \
    libopenblas-dev \
    libopenjp2-7 \
    libpdal-base13 \
    libpdal-dev \
    libpdal-plugin-hdf \
    libpdal-plugins \
    libpdal-util13 \
    libpnglite0 \
    libpq-dev \
    libpq5 \
    libproj-dev \
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
    netcdf-bin \
    pdal \
    proj-bin \
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
ENV GRASS_RUN_PACKAGES=${GRASS_RUN_PACKAGES}

# Define build packages
ARG GRASS_BUILD_PACKAGES="\
    cmake \
    libbz2-dev \
    libcairo2-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libncurses5-dev \
    libnetcdf-dev \
    libopenjp2-7-dev \
    libpnglite-dev \
    libreadline-dev \
    libsqlite3-dev \
    libtiff-dev \
    libzstd-dev \
    mesa-common-dev \
    zlib1g-dev \
    "
ENV GRASS_BUILD_PACKAGES=${GRASS_BUILD_PACKAGES}

ARG GRASS_CONFIG="\
  --enable-largefile \
  --with-blas \
  --with-bzlib \
  --with-cairo --with-cairo-ldflags=-lfontconfig \
  --with-cxx \
  --with-fftw \
  --with-freetype --with-freetype-includes=/usr/include/freetype2/ \
  --with-gdal=/usr/bin/gdal-config \
  --with-geos \
  --with-lapack \
  --with-netcdf \
  --with-odbc \
  --with-openmp \
  --with-pdal \
  --with-postgres --with-postgres-includes=/usr/include/postgresql \
  --with-proj-share=/usr/share/proj \
  --with-readline \
  --with-sqlite \
  --with-zstd \
  --without-mysql \
  "

ARG GRASS_PYTHON_PACKAGES="\
    matplotlib \
    numpy \
    Pillow \
    pip \
    ply \
    psycopg2 \
    python-dateutil \
    python-magic \
    setuptools \
  "
ENV GRASS_PYTHON_PACKAGES=${GRASS_PYTHON_PACKAGES}


FROM common_start AS grass_without_gui

ARG GRASS_CONFIG="${GRASS_CONFIG} --without-opengl"
ENV GRASS_CONFIG=${GRASS_CONFIG}

FROM common_start AS grass_with_gui

ARG GRASS_RUN_PACKAGES="${GRASS_RUN_PACKAGES} \
  adwaita-icon-theme-full \
  freeglut3 \
  gettext \
  libglu1-mesa \
  libgstreamer-plugins-base1.0 \
  libgtk-3-0 \
  libjpeg8 \
  libnotify4 \
  libpng16-16 \
  librsvg2-common \
  libsdl2-2.0-0 \
  libsm6 \
  libtiff5 \
  libwebkit2gtk-4.0 \
  libxtst6 \
"
# librsvg2-common \
# (fix error (wxgui.py:7782): Gtk-WARNING **: 19:53:09.774:
# Could not load a pixbuf from /org/gtk/libgtk/theme/Adwaita/assets/check-symbolic.svg.
# This may indicate that pixbuf loaders or the mime database could not be found.)

ARG GRASS_BUILD_PACKAGES="${GRASS_BUILD_PACKAGES} \
  adwaita-icon-theme-full \
  freeglut3-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
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

ARG GRASS_CONFIG="${GRASS_CONFIG} \
  --with-nls \
  --with-opengl \
  --with-readline \
  --with-x \
  "
ARG GRASS_PYTHON_PACKAGES="${GRASS_PYTHON_PACKAGES} wxPython"
# If you do not use any Gnome Accessibility features, to suppress warning
# WARNING **: Couldn't connect to accessibility bus:
# execute programs with
ENV NO_AT_BRIDGE=1


FROM grass_${GUI}_gui AS grass_gis
# Add ubuntugis unstable and fetch packages
RUN apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    software-properties-common \
    gpg \
    gpg-agent \
    && LC_ALL=C.UTF-8 add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable \
    && apt-get update -y && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    $GRASS_RUN_PACKAGES \
    && apt-get remove -y gpg gpg-agent \
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/*

RUN echo LANG="en_US.UTF-8" > /etc/default/locale \
    && echo en_US.UTF-8 UTF-8 >> /etc/locale.gen \
    && locale-gen

## fetch vertical datums for PDAL and store into PROJ dir
WORKDIR /src

# # Get datum grids
# # Currently using https://proj.org/en/9.3/usage/network.html#how-to-enable-network-capabilities
# FROM ubuntu:22.04 AS datum_grids

# # See: https://github.com/OSGeo/PROJ-data
# RUN apt-get update \
#     && apt-get install  -y --no-install-recommends --no-install-suggests \
#     wget \
#     && apt-get clean all \
#     && rm -rf /var/lib/apt/lists/*

# WORKDIR /tmp
# RUN wget --no-check-certificate -r -l inf -A tif https://cdn.proj.org/

# Start build stage
FROM grass_gis AS build

# Add build packages
RUN apt-get update \
    && apt-get install  -y --no-install-recommends --no-install-suggests \
    $GRASS_BUILD_PACKAGES \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/*

# Add pip and Python packages
RUN (echo "Install Python" \
    && wget https://bootstrap.pypa.io/pip/get-pip.py \
    # && apt-get install -y python3-ensurepip \
    && python get-pip.py \
    # && python3 -m ensurepip --upgrade \
    && rm -r get-pip.py \
    && mkdir -p /src/site-packages \
    && cd /src \
    && python -m pip install --no-cache-dir -t /src/site-packages --upgrade \
    $GRASS_PYTHON_PACKAGES \
    && rm -r /root/.cache \
    && rm -rf /tmp/pip-* \
    )

# copy grass gis source
COPY . /src/grass_build/

WORKDIR /src/grass_build

# Set environmental variables for GRASS GIS compilation, without debug symbols
# Set gcc/g++ environmental variables for GRASS GIS compilation, without debug symbols
ENV MYCFLAGS="-O2 -std=gnu99"
ENV MYLDFLAGS="-s"
# CXX stuff:
ENV LD_LIBRARY_PATH="/usr/local/lib"
ENV LDFLAGS="$MYLDFLAGS"
ENV CFLAGS="$MYCFLAGS"
ENV CXXFLAGS="$MYCXXFLAGS"

# Configure compile and install GRASS GIS
ENV NUMTHREADS=4
RUN make distclean || echo "nothing to clean"
RUN ./configure $GRASS_CONFIG \
    && make -j $NUMTHREADS \
    && make install && ldconfig \
    &&  cp /usr/local/grass85/gui/wxpython/xml/module_items.xml module_items.xml; \
    rm -rf /usr/local/grass85/demolocation; \
    rm -rf /usr/local/grass85/fonts; \
    rm -rf /usr/local/grass85/gui; \
    rm -rf /usr/local/grass85/share; \
    mkdir -p /usr/local/grass85/gui/wxpython/xml/; \
    mv module_items.xml /usr/local/grass85/gui/wxpython/xml/module_items.xml;

# Build the GDAL-GRASS plugin
# renovate: datasource=github-tags depName=OSGeo/gdal-grass
ARG GDAL_GRASS_VERSION=1.0.3
RUN git clone --branch $GDAL_GRASS_VERSION --depth 1 https://github.com/OSGeo/gdal-grass.git \
    && cd "gdal-grass" \
    && cmake -B build -DAUTOLOAD_DIR=/usr/lib/gdalplugins -DBUILD_TESTING=OFF \
    && cmake --build build \
    && cmake --install build \
    && cd /src \
    && rm -rf "gdal-grass"

# Leave build stage
FROM grass_gis AS grass_gis_final

# GRASS GIS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python/:${PYTHONPATH}" \
    LD_LIBRARY_PATH="/usr/local/grass/lib:$LD_LIBRARY_PATH" \
    GDAL_DRIVER_PATH="/usr/lib/gdalplugins"

# Copy GRASS GIS from build image
COPY --link --from=build /usr/local/bin/* /usr/local/bin/
COPY --link --from=build /usr/local/grass85 /usr/local/grass85/
COPY --link --from=build /src/site-packages /usr/lib/python3.10/
COPY --link --from=build /usr/lib/gdalplugins /usr/lib/gdalplugins
# COPY --link --from=datum_grids /tmp/cdn.proj.org/*.tif /usr/share/proj/

# Create generic GRASS GIS lib name regardless of version number
RUN ln -sf /usr/local/grass85 /usr/local/grass

# show GRASS GIS, PROJ, GDAL etc versions
RUN grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python --version

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
