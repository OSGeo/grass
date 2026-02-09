# syntax=docker/dockerfile:1.21@sha256:27f9262d43452075f3c410287a2c43f5ef1bf7ec2bb06e8c9eeb1b8d453087bc

# Note: This file must be kept in sync in ./Dockerfile and ./docker/ubuntu/Dockerfile.
#       Changes to this file must be copied over to the other file.
ARG GUI=without

FROM ubuntu:24.04@sha256:cd1dba651b3080c3686ecf4e3c4220f026b521fb76978881737d24f200828b2b AS common_start

ARG BASE_NAME="ubuntu:24.04"
ARG PYTHON_VERSION=3.12
# renovate: datasource=github-tags depName=libgeos/geos
ARG GEOS_VERSION=3.14.1
# renovate: datasource=github-tags depName=OSGeo/PROJ
ARG PROJ_VERSION=9.7.1
# renovate: datasource=github-tags depName=OSGeo/gdal
ARG GDAL_VERSION=3.12.1
# renovate: datasource=github-tags depName=PDAL/PDAL
ARG PDAL_VERSION=2.9.2
# renovate: datasource=github-tags depName=OSGeo/gdal-grass
ARG GDAL_GRASS_VERSION=2.0.0
# renovate: datasource=pypi depName=wxPython
ARG WXPYTHON_VERSION=4.2.4

# Have build parameters as build arguments?
# ARG LDFLAGS="-s -Wl,--no-undefined -lblas"
# ARG CFLAGS="-O2 -std=gnu99"
# ARG CXXFLAGS=""
# ARG PYTHONPATH="$PYTHONPATH"
# ARG LD_LIBRARY_PATH="/usr/local/lib"

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

# Todo: re-consider required dev packages for addons (~400MB in dev packages)
ARG GRASS_RUN_PACKAGES="\
  bison \
  build-essential \
  bzip2 \
  curl \
  flex \
  freetds-bin \
  freetds-common \
  g++ \
  gcc \
  git \
  language-pack-en-base \
  libarmadillo12 \
  libcairo2 \
  libcurl4-gnutls-dev \
  libfftw3-bin \
  libfftw3-dev \
  libfreetype6 \
  libfyba0t64 \
  libgeotiff5 \
  libgif7 \
  libgsl-dev \
  libgsl27 \
  libhdf5-dev \
  libjpeg-turbo8 \
  libjson-c5 \
  libjsoncpp-dev \
  libkmlbase1t64 \
  libkmldom1t64 \
  libkmlengine1t64 \
  libkmlxsd1t64 \
  libkmlconvenience1t64 \
  liblapacke-dev \
  liblz4-1 \
  libmagic-mgc \
  libmagic1 \
  libmuparser2v5 \
  libncurses6 \
  libomp-dev \
  libomp5 \
  libopenblas-dev \
  libopenblas0 \
  libopenjp2-7 \
  libpnglite0 \
  libpq-dev \
  libpq5 \
  libpython3-all-dev \
  libreadline8 \
  libspatialite8t64 \
  libsqlite3-0 \
  libsqlite3-mod-spatialite \
  libsvm3 \
  libtiff-tools \
  libxerces-c3.2t64 \
  libzstd1 \
  locales \
  make \
  mesa-utils \
  moreutils \
  ncurses-bin \
  netcdf-bin \
  pcre2-utils \
  python-is-python3 \
  python3 \
  python3-dev \
  sqlite3 \
  tdsodbc \
  unixodbc \
  unzip \
  vim \
  wget \
  zip \
  zlib1g \
"

ARG GRASS_GUI_RUN_PACKAGES=" \
  adwaita-icon-theme-full \
  gettext \
  libglu1-mesa \
  libglut3.12 \
  libgstreamer-plugins-base1.0 \
  libgtk-3-0 \
  libjpeg8 \
  libnotify4 \
  libpng16-16 \
  librsvg2-common \
  libsdl2-2.0-0 \
  libsm6 \
  libtiff6 \
  libwebkit2gtk-4.1 \
  libxtst6 \
  python3-wxgtk4.0 \
"

# Define build packages
ARG GRASS_BUILD_PACKAGES="\
  cmake \
  freetds-dev \
  libbz2-dev \
  libarmadillo-dev \
  libcairo2-dev \
  libfreetype6-dev \
  libfyba-dev \
  libgeotiff-dev \
  libgif-dev \
  libjpeg-dev \
  libjson-c-dev \
  libkml-dev \
  liblz4-dev \
  libmuparser-dev \
  libncurses-dev \
  libnetcdf-dev \
  libopenjp2-7-dev \
  libpcre2-dev \
  libpnglite-dev \
  libreadline-dev \
  libsqlite3-dev \
  libspatialite-dev \
  libsvm-dev \
  libtiff-dev \
  libxerces-c-dev \
  libzstd-dev \
  mesa-common-dev \
  ninja-build \
  unixodbc-dev \
  zlib1g-dev \
  swig \
"

ARG GRASS_PYTHON_PACKAGES="\
    matplotlib \
    numpy \
    packaging>=25.0 \
    Pillow>=10.3.0 \
    psycopg2 \
    python-dateutil \
    python-magic \
    setuptools==80.9.0 \
    cython \
    "

# If you do not use any Gnome Accessibility features, to suppress warning
# WARNING **: Couldn't connect to accessibility bus:
# execute programs with
ENV NO_AT_BRIDGE=1

# Install runtime-packages
# hadolint ignore=SC2086,DL3008
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    $GRASS_RUN_PACKAGES \
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/*

## fetch vertical datums for PDAL and store into PROJ dir
# WORKDIR /src

# # Get datum grids
# # Currently using https://proj.org/en/9.3/usage/network.html#how-to-enable-network-capabilities
# FROM ubuntu:24.04 AS datum_grids

# # See: https://github.com/OSGeo/PROJ-data
# RUN apt-get update \
#     && apt-get install  -y --no-install-recommends --no-install-suggests \
#     wget \
#     && apt-get clean all \
#     && rm -rf /var/lib/apt/lists/*

# WORKDIR /tmp
# RUN wget -q --no-check-certificate -r -l inf -A tif https://cdn.proj.org/

# Start build stage
FROM common_start AS build_common

# Add build packages
# hadolint ignore=SC2086,DL3008,DL3013
RUN apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    ${GRASS_BUILD_PACKAGES} ca-certificates \
    && apt-get autoremove -y \
    && apt-get clean all \
    && echo "Install Python" \
    && wget --progress="dot:giga" https://bootstrap.pypa.io/pip/get-pip.py \
    && python get-pip.py --break-system-packages \
    && rm -r get-pip.py \
    # Deactivate python3-packaging to allow installation of newer version
    && apt-get remove -y python3-packaging \
    && apt-get clean all \
    && python3 -m pip install --break-system-packages --no-cache-dir --upgrade \
    ${GRASS_PYTHON_PACKAGES} \
    # Add back dev-packages that depend on python3-packaging
    && apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    libcairo2-dev libglib2.0-dev libglib2.0-dev-bin \
    # Clean up
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && rm -r /root/.cache \
    && rm -rf /tmp/pip-*

# Set environmental variables for GRASS compilation, without debug symbols
# Set gcc/g++ environmental variables for GRASS compilation, without debug symbols
ARG NUMTHREADS=4
ENV CMAKE_BUILD_PARALLEL_LEVEL=$NUMTHREADS
WORKDIR /src

COPY ./utils/build_ubuntu_dependencies.sh /src/

# Build and install PROJ, GEOS, GDAL and PDAL from source
FROM build_common AS build_proj
RUN /src/build_ubuntu_dependencies.sh proj "$PROJ_VERSION" "$NUMTHREADS"

FROM build_common AS build_geos
RUN /src/build_ubuntu_dependencies.sh geos "$GEOS_VERSION" "$NUMTHREADS"

FROM build_common AS build_gdal_pdal
COPY --link --from=build_proj /usr/local /usr/local
COPY --link --from=build_geos /usr/local /usr/local

# hadolint ignore=DL3059
RUN /src/build_ubuntu_dependencies.sh gdal "$GDAL_VERSION" "$NUMTHREADS"

# hadolint ignore=DL3059
RUN /src/build_ubuntu_dependencies.sh pdal "$PDAL_VERSION" "$NUMTHREADS"

# With all the libraries needed compiled from source, now build grass and gdal-grass
FROM build_gdal_pdal AS build_grass_config

ARG GRASS_CONFIG="\
  --enable-largefile \
  --with-blas \
  --with-bzlib \
  --with-cairo --with-cairo-ldflags=-lfontconfig --with-cairo-includes=/usr/include/cairo/ \
  --with-cxx \
  --with-fftw \
  --with-freetype --with-freetype-includes=/usr/include/freetype2/ \
  --with-gdal=/usr/local/bin/gdal-config \
  --with-geos \
  --with-lapack \
  --with-libsvm \
  --with-netcdf \
  --with-odbc \
  --with-openmp \
  --with-pcre \
  --with-pdal \
  --with-postgres --with-postgres-includes=/usr/include/postgresql \
  --with-proj-share=/usr/local/share/proj \
  --with-readline \
  --with-sqlite \
  --with-zstd \
  --without-mysql \
"

ARG GRASS_NO_GUI_CONFIG="--without-nls --without-opengl --without-x"

ARG GRASS_GUI_CONFIG="\
  --with-nls \
  --with-opengl \
  --with-x \
"

# librsvg2-common \
# (fix error (wxgui.py:7782): Gtk-WARNING **: 19:53:09.774:
# Could not load a pixbuf from /org/gtk/libgtk/theme/Adwaita/assets/check-symbolic.svg.
# This may indicate that pixbuf loaders or the mime database could not be found.)
ARG GRASS_GUI_BUILD_PACKAGES=" \
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
  libwebkit2gtk-4.1-dev \
  libxtst-dev \
"

COPY . /src/grass_build/

# copy grass source
WORKDIR /src/grass_build

ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/lib" \
    LDFLAGS="-s -Wl,--no-undefined -lblas" \
    CFLAGS="-O2 -std=gnu99" \
    CXXFLAGS="" \
    NUMTHREADS=$NUMTHREADS

FROM build_grass_config AS build_grass_with_gui

ARG GRASS_CONFIG="$GRASS_CONFIG $GRASS_GUI_CONFIG"
ARG GRASS_GUI_PACKAGES="$GRASS_GUI_RUN_PACKAGES $GRASS_GUI_BUILD_PACKAGES"

# hadolint ignore=DL3008
RUN echo "Installing GRASS GUI packages: $GRASS_GUI_PACKAGES" \
    && apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    $GRASS_GUI_PACKAGES \
    && python3 -m pip install  -U --break-system-packages --no-cache-dir --upgrade \
    -f https://extras.wxpython.org/wxPython4/extras/linux/gtk3/ubuntu-24.04 \
    "wxpython==${WXPYTHON_VERSION}" \
    # Clean up
    && pip cache purge \
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    # Need to generate locale when NLS is enabled
    && echo LANG="en_US.UTF-8" > /etc/default/locale \
    && echo en_US.UTF-8 UTF-8 >> /etc/locale.gen \
    && locale-gen

FROM build_grass_config AS build_grass_without_gui

ARG GRASS_CONFIG="$GRASS_CONFIG $GRASS_NO_GUI_CONFIG"

# hadolint ignore=DL3006
FROM build_grass_${GUI}_gui AS build_grass

# Configure compile and install GRASS
# hadolint ignore=SC2086,DL3008
RUN make -j $NUMTHREADS distclean || echo "nothing to clean" \
    && ./configure $GRASS_CONFIG \
    && make -j $NUMTHREADS \
    && make install && ldconfig \
    && rm -rf /usr/local/grass86/demolocation \
    && cp /usr/local/grass86/gui/wxpython/xml/module_items.xml module_items.xml

FROM build_grass AS build_grass_with_gui_built
RUN echo "GUI selected, skipping GUI related cleanup"

FROM build_grass AS build_grass_without_gui_built
RUN echo "No GUI selected, removing GUI related files" \
    && rm -rf /usr/local/grass86/fonts \
    && rm -rf /usr/local/grass86/gui \
    && rm -rf /usr/local/grass86/docs/html \
    && rm -rf /usr/local/grass86/docs/mkdocs \
    && rm -rf /usr/local/grass86/share

# hadolint ignore=DL3006
FROM build_grass_${GUI}_gui_built AS build_grass_plugin
# Build the GDAL-GRASS plugin
# hadolint ignore=DL3003,DL3059
RUN git clone --branch "$GDAL_GRASS_VERSION" --depth 1 https://github.com/OSGeo/gdal-grass.git \
    && cd "gdal-grass" \
    && cmake -B build -DAUTOLOAD_DIR=/usr/local/lib/gdalplugins -DBUILD_TESTING=OFF \
    && cmake --build build \
    && cmake --install build \
    && cd /src \
    && rm -rf "gdal-grass"

ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python" \
    LD_LIBRARY_PATH="/usr/local/grass/lib:/usr/local/lib" \
    GDAL_DRIVER_PATH="/usr/local/lib/gdalplugins"

ENV LD_LIBRARY_PATH="/usr/local/grass/lib:/usr/local/lib:/usr/lib"

# show GRASS, PROJ, GDAL etc versions
RUN /usr/local/bin/grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python3 --version && \
    python3 -c "import numpy; print('numpy', numpy.__version__)" && \
    python3 -c "import matplotlib as mpl; print('matplotlib', mpl.__version__)" && \
    python3 -c "from osgeo import gdal; print('GDAL_GRASS driver:', 'available' if gdal.GetDriverByName('GRASS') else 'not-available')"

# Leave build stage
FROM common_start AS grass_final_without_gui
RUN echo "No additional steps needed without GUI."

FROM common_start AS grass_final_with_gui

# hadolint ignore=SC2086,DL3008
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends --no-install-suggests \
    $GRASS_GUI_RUN_PACKAGES \
    && apt-get autoremove -y \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/*

# hadolint ignore=DL3006
FROM grass_final_${GUI}_gui AS grass_final

# Set with "$(git branch --show-current)"
ARG BRANCH=main
# Set with "$(git log -n 1 --pretty=format:'%h')"
ARG REVISION=latest
# Set with "$(head include/VERSION -n 3 | sed ':a;N;$!ba;s/\n/ /g')"
ARG VERSION=8.5.dev
# Set with "$(date +'%Y-%m-%dT%H:%M:%S%:z')"
ARG BUILD_DATE
ARG TITLE="GRASS ${VERSION}"
ARG URL="https://github.com/OSGeo/grass/tree/${BRANCH}/docker"
ARG DOCUMENTATION="https://github.com/OSGeo/grass/tree/${BRANCH}/docker/README.md"
ARG SOURCE="https://github.com/OSGeo/grass/tree/${BRANCH}/docker/ubuntu"
ARG AUTHORS="Carmen Tawalika,Markus Neteler,Anika Weinmann,Stefan Blumentrath"
ARG MAINTAINERS="tawalika@mundialis.de,neteler@mundialis.de,weinmann@mundialis.de"
ARG VENDOR="GRASS Development Team"
ARG LICENSE="GPL-2.0-or-later"
ARG DESCRIPTION="GRASS (Geographic Resources Analysis Support System, https://grass.osgeo.org/) "
ARG DESCRIPTION="$DESCRIPTION is a powerful computational engine for geospatial processing."
ARG DESCRIPTION="$DESCRIPTION Build with:"
ARG DESCRIPTION="$DESCRIPTION PROJ ${PROJ_VERSION},"
ARG DESCRIPTION="$DESCRIPTION GEOS ${GEOS_VERSION},"
ARG DESCRIPTION="$DESCRIPTION GDAL ${GDAL_VERSION},"
ARG DESCRIPTION="$DESCRIPTION PDAL ${PDAL_VERSION}."
ARG REF_NAME="grass"

# Add OCI annotations (see: https://github.com/opencontainers/image-spec/blob/main/annotations.md)
LABEL org.opencontainers.image.authors="$AUTHORS" \
      org.opencontainers.image.vendor="$VENDOR" \
      org.opencontainers.image.licenses="$LICENSE" \
      org.opencontainers.image.created="$BUILD_DATE" \
      org.opencontainers.image.revision="$REVISION" \
      org.opencontainers.image.url="$URL" \
      org.opencontainers.image.source="$SOURCE" \
      org.opencontainers.image.version="$VERSION" \
      org.opencontainers.image.title="$TITLE" \
      org.opencontainers.image.description="$DESCRIPTION" \
      org.opencontainers.image.base.name="$BASE_NAME" \
      org.opencontainers.image.ref.name="$REF_NAME" \
      org.opencontainers.image.documentation="$DOCUMENTATION" \
      maintainers="$MAINTAINERS"

# GRASS specific
# allow work with MAPSETs that are not owned by current user
ENV GRASS_SKIP_MAPSET_OWNER_CHECK=1 \
    SHELL="/bin/bash" \
    # https://proj.org/usage/environmentvars.html#envvar-PROJ_NETWORK
    PROJ_NETWORK=ON \
    LC_ALL="en_US.UTF-8" \
    PYTHONPATH="/usr/local/grass/etc/python" \
    LD_LIBRARY_PATH="/usr/local/grass/lib:/usr/local/lib:/usr/lib" \
    GDAL_DRIVER_PATH="/usr/local/lib/gdalplugins"

# Copy GRASS, GDAL-GRASS-plugin and compiled dependencies from build image
COPY --link --from=build_grass_plugin /usr/local /usr/local
COPY --link --from=build_grass_plugin /src/grass_build/module_items.xml /usr/local/grass86/gui/wxpython/xml/module_items.xml
# COPY --link --from=datum_grids /tmp/cdn.proj.org/*.tif /usr/share/proj/

# Create generic GRASS lib name regardless of version number
RUN ln -sf /usr/local/grass86 /usr/local/grass \
    && ldconfig

# show GRASS, PROJ, GDAL etc versions
RUN grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python3 --version && \
    python3 -c "import numpy; print('numpy', numpy.__version__)" && \
    python3 -c "import matplotlib as mpl; print('matplotlib', mpl.__version__)" && \
    python3 -c "import packaging; print('packaging', packaging.__version__)" && \
    python3 -c "from osgeo import gdal; print('GDAL_GRASS driver:', 'available' if gdal.GetDriverByName('GRASS') else 'not-available')"

WORKDIR /tmp
COPY docker/testdata/simple.laz .
WORKDIR /scripts
COPY docker/testdata/test_grass_session.py .
# Run GRASS python session and scan the test LAZ file
RUN python3 /scripts/test_grass_session.py \
    && rm -rf /tmp/grasstest_epsg_25832 \
    && grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g \
    && rm /tmp/simple.laz \
    && rm /scripts/test_grass_session.py

WORKDIR /grassdb
VOLUME /grassdb
