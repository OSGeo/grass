# syntax=docker/dockerfile:1.18@sha256:dabfc0969b935b2080555ace70ee69a5261af8a8f1b4df97b9e7fbcf6722eddf

# Note: This file must be kept in sync in ./Dockerfile and ./docker/ubuntu/Dockerfile.
#       Changes to this file must be copied over to the other file.

ARG GUI=without
# Have build parameters as build arguments?
# ARG LDFLAGS="-s -Wl,--no-undefined -lblas"
# ARG CFLAGS="-O2 -std=gnu99"
# ARG CXXFLAGS=""
# ARG PYTHONPATH="$PYTHONPATH"
# ARG LD_LIBRARY_PATH="/usr/local/lib"
ARG NUMTHREADS=4

ARG BRANCH=$(git branch --show-current)
ARG AUTHORS="Carmen Tawalika,Markus Neteler,Anika Weinmann,Stefan Blumentrath"
ARG MAINTAINERS="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"
ARG VENDOR="GRASS Development Team"
ARG BASE_NAME="ubuntu:22.04"
ARG DOCUMENTATION="https://github.com/OSGeo/grass/tree/${BRANCH}/docker/README.md"
ARG REF_NAME="grass"
ARG DIGEST="sha256:4e0171b9275e12d375863f2b3ae9ce00a4c53ddda176bd55868df97ac6f21a6e"

FROM ubuntu:24.04@sha256:353675e2a41babd526e2b837d7ec780c2a05bca0164f7ea5dbbd433d21d166fc AS common_start

ARG AUTHORS
ARG MAINTAINERS
ARG VENDOR
ARG GUI
ARG BASE_NAME
ARG DOCUMENTATION
ARG REF_NAME
ARG DIGEST
ARG NUMTHREADS

LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      org.opencontainers.image.base.name="$BASE_NAME" \
      org.opencontainers.image.base.digest="$DIGEST" \
      org.opencontainers.image.ref.name="$REF_NAME" \
      org.opencontainers.image.documentation="$DOCUMENTATION" \
      maintainers="$MAINTAINERS"


ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

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
  libsvm3 \
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
  libsvm-dev \
  libtiff-dev \
  libzstd-dev \
  mesa-common-dev \
  zlib1g-dev \
"

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
  --with-libsvm \
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
    Pillow==10.3.0 \
    pip \
    psycopg2 \
    python-dateutil \
    python-magic \
    setuptools \
  "
FROM common_start AS grass_without_gui

ARG GRASS_ADDITIONAL_CONFIG="--without-opengl --without-x"

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
  python3-wxgtk4.0 \
"

# librsvg2-common \
# (fix error (wxgui.py:7782): Gtk-WARNING **: 19:53:09.774:
# Could not load a pixbuf from /org/gtk/libgtk/theme/Adwaita/assets/check-symbolic.svg.
# This may indicate that pixbuf loaders or the mime database could not be found.)

ARG GRASS_BUILD_PACKAGES="${GRASS_BUILD_PACKAGES} \
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

ARG GRASS_ADDITIONAL_CONFIG="\
  --with-nls \
  --with-opengl \
  --with-readline \
  --with-x \
"
# ARG GRASS_PYTHON_PACKAGES="${GRASS_PYTHON_PACKAGES} wxPython"
# If you do not use any Gnome Accessibility features, to suppress warning
# WARNING **: Couldn't connect to accessibility bus:
# execute programs with
ENV NO_AT_BRIDGE=1

# hadolint ignore=DL3006
FROM grass_${GUI}_gui AS grass

LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      maintainers="$MAINTAINERS"

# Add ubuntugis unstable and fetch packages
# hadolint ignore=SC2086
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
    && rm -rf /var/lib/apt/lists/* \
    && echo LANG="en_US.UTF-8" > /etc/default/locale \
    && echo en_US.UTF-8 UTF-8 >> /etc/locale.gen \
    && locale-gen

## fetch vertical datums for PDAL and store into PROJ dir
# WORKDIR /src

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
# RUN wget -q --no-check-certificate -r -l inf -A tif https://cdn.proj.org/

# Start build stage
FROM grass AS build

# Add build and Python packages
# hadolint ignore=SC2086
RUN apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    ${GRASS_BUILD_PACKAGES} \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && (echo "Install Python" \
    && wget --progress="dot:giga" -q https://bootstrap.pypa.io/pip/get-pip.py \
    && python get-pip.py \
    && rm -r get-pip.py \
    && mkdir -p /src/site-packages \
    && python -m pip install --no-cache-dir -t /src/site-packages --upgrade \
    ${GRASS_PYTHON_PACKAGES} \
    && rm -r /root/.cache \
    && rm -rf /tmp/pip-* \
    )

# copy grass gis source
COPY . /src/grass_build/

WORKDIR /src/grass_build

# Set environmental variables for GRASS compilation, without debug symbols
# Set gcc/g++ environmental variables for GRASS compilation, without debug symbols
ENV LD_LIBRARY_PATH="/usr/local/lib" \
    LDFLAGS="-s -Wl,--no-undefined -lblas" \
    CFLAGS="-O2 -std=gnu99" \
    CXXFLAGS="" \
    NUMTHREADS=$NUMTHREADS

# Configure compile and install GRASS
# hadolint ignore=SC2086
RUN make -j $NUMTHREADS distclean || echo "nothing to clean" \
    && ./configure $GRASS_CONFIG \
    $GRASS_ADDITIONAL_CONFIG \
    && make -j $NUMTHREADS
RUN make install && ldconfig \
    && rm -rf /usr/local/grass85/demolocation \
    && if [ "$GUI" = "with" ] ; then \
        echo "GUI selected, skipping GUI related cleanup"; \
    else \
        echo "No GUI selected, removing GUI related files"; \
        cp /usr/local/grass85/gui/wxpython/xml/module_items.xml module_items.xml \
        && rm -rf /usr/local/grass85/fonts \
        && rm -rf /usr/local/grass85/gui \
        && rm -rf /usr/local/grass85/docs/html \
        && rm -rf /usr/local/grass85/docs/mkdocs \
        && rm -rf /usr/local/grass85/share \
        && mkdir -p /usr/local/grass85/gui/wxpython/xml/ \
        && mv module_items.xml /usr/local/grass85/gui/wxpython/xml/module_items.xml; \
    fi

# Build the GDAL-GRASS plugin
ENV CMAKE_BUILD_PARALLEL_LEVEL=$NUMTHREADS
# renovate: datasource=github-tags depName=OSGeo/gdal-grass
ARG GDAL_GRASS_VERSION=1.0.4
# hadolint ignore=DL3003
RUN git clone --branch $GDAL_GRASS_VERSION --depth 1 https://github.com/OSGeo/gdal-grass.git \
    && cd "gdal-grass" \
    && cmake -B build -DAUTOLOAD_DIR=/usr/lib/gdalplugins -DBUILD_TESTING=OFF \
    && cmake --build build \
    && cmake --install build \
    && cd /src \
    && rm -rf "gdal-grass"

# Leave build stage
FROM grass AS grass_final
LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      maintainers="$MAINTAINERS"

# GRASS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python" \
    LD_LIBRARY_PATH="/usr/local/grass/lib:/usr/local/lib" \
    GDAL_DRIVER_PATH="/usr/lib/gdalplugins"

# Copy GRASS from build image
COPY --link --from=build /usr/local/bin/* /usr/local/bin/
COPY --link --from=build /usr/local/grass85 /usr/local/grass85/
COPY --link --from=build /src/site-packages /usr/lib/python3.10/
COPY --link --from=build /usr/lib/gdalplugins /usr/lib/gdalplugins
# COPY --link --from=datum_grids /tmp/cdn.proj.org/*.tif /usr/share/proj/

# Create generic GRASS lib name regardless of version number
RUN ln -sf /usr/local/grass85 /usr/local/grass

# show GRASS, PROJ, GDAL etc versions
RUN grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python --version

WORKDIR /tmp
COPY docker/testdata/simple.laz .
WORKDIR /scripts
COPY docker/testdata/test_grass_session.py .
## Run GRASS python session and scan the test LAZ file
RUN python /scripts/test_grass_session.py \
    && rm -rf /tmp/grasstest_epsg_25832 \
    && grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g \
    && rm /tmp/simple.laz \
    && rm /scripts/test_grass_session.py

WORKDIR /grassdb
VOLUME /grassdb
