FROM ubuntu:18.04

LABEL authors="Vaclav Petras,Markus Neteler"
LABEL maintainer="wenzeslaus@gmail.com,neteler@osgeo.org"

# system environment
ENV DEBIAN_FRONTEND noninteractive

# data directory - not using the base images volume because then the permissions cannot be adapted
ENV DATA_DIR /data

# GRASS GIS compile dependencies
RUN apt-get update \
    && apt-get install -y --no-install-recommends --no-install-suggests \
        build-essential \
        libblas-dev \
        libbz2-dev \
        libcairo2-dev \
        libfftw3-dev \
        libfreetype6-dev \
        libgdal-dev \
        libgeos-dev \
        libglu1-mesa-dev \
        libgsl0-dev \
        libjpeg-dev \
        liblapack-dev \
        libncurses5-dev \
        libnetcdf-dev \
        libopenjp2-7 \
        libopenjp2-7-dev \
        libpdal-dev pdal \
        libpdal-plugin-python \
        libpng-dev \
        libpq-dev \
        libproj-dev \
        libreadline-dev \
        libsqlite3-dev \
        libtiff-dev \
        libxmu-dev \
        libzstd-dev \
        bison \
        flex \
        g++ \
        gettext \
        gdal-bin \
        language-pack-en-base \
        libfftw3-bin \
        make \
        ncurses-bin \
        netcdf-bin \
        proj-bin \
        proj-data \
        python3 \
        python3-dateutil \
        python3-dev \
        python3-numpy \
        python3-pil \
        python3-pip \
        python3-ply \
        python3-six \
        python3-wxgtk4.0 \
        python3-gdal \
        python3-matplotlib \
        sqlite3 \
        subversion \
        unixodbc-dev \
        zlib1g-dev \
    && apt-get autoremove \
    && apt-get clean && \
    mkdir -p $DATA_DIR

RUN echo LANG="en_US.UTF-8" > /etc/default/locale
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

RUN mkdir /code
RUN mkdir /code/grass

# add repository files to the image
COPY . /code/grass

WORKDIR /code/grass

# Set gcc/g++ environmental variables for GRASS GIS compilation, without debug symbols
ENV MYCFLAGS "-O2 -std=gnu99 -m64"
ENV MYLDFLAGS "-s"
# CXX stuff:
ENV LD_LIBRARY_PATH "/usr/local/lib"
ENV LDFLAGS "$MYLDFLAGS"
ENV CFLAGS "$MYCFLAGS"
ENV CXXFLAGS "$MYCXXFLAGS"

# Configure, compile and install GRASS GIS
ENV NUMTHREADS=2
RUN ./configure \
    --enable-largefile \
    --with-cxx \
    --with-nls \
    --with-readline \
    --with-sqlite \
    --with-bzlib \
    --with-zstd \
    --with-cairo --with-cairo-ldflags=-lfontconfig \
    --with-freetype --with-freetype-includes="/usr/include/freetype2/" \
    --with-fftw \
    --with-netcdf \
    --with-pdal \
    --with-proj --with-proj-share=/usr/share/proj \
    --with-geos=/usr/bin/geos-config \
    --with-postgres --with-postgres-includes="/usr/include/postgresql" \
    --with-opengl-libs=/usr/include/GL \
    && make -j $NUMTHREADS && make install && ldconfig

# enable simple grass command regardless of version number
RUN ln -s /usr/local/bin/grass* /usr/local/bin/grass

# Reduce the image size
RUN apt-get autoremove -y
RUN apt-get clean -y

# set SHELL var to avoid /bin/sh fallback in interactive GRASS GIS sessions in docker
ENV SHELL /bin/bash

# Fix permissions
RUN chmod -R a+rwx $DATA_DIR

# create a user
RUN useradd -m -U grass

# declare data volume late so permissions apply
VOLUME $DATA_DIR
WORKDIR $DATA_DIR

# Further reduce the docker image size 
RUN rm -rf /code/grass

# switch the user
USER grass

CMD ["/usr/local/bin/grass", "--version"]
