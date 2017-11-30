FROM ubuntu:16.04

MAINTAINER Vaclav Petras <wenzeslaus@gmail.com>

# system environment
ENV DEBIAN_FRONTEND noninteractive

# data directory - not using the base images volume because then the permissions cannot be adapted
ENV DATA_DIR /data

# GRASS GIS compile dependencies
RUN apt-get update \
    && apt-get install -y --install-recommends \
        autoconf2.13 \
        autotools-dev \
        bison \
        flex \
        g++ \
        gettext \
        libblas-dev \
        libbz2-dev \
        libcairo2-dev \
        libfftw3-dev \
        libfreetype6-dev \
        libgdal-dev \
        libgeos-dev \
        libglu1-mesa-dev \
        libjpeg-dev \
        liblapack-dev \
        liblas-c-dev \
        libncurses5-dev \
        libnetcdf-dev \
        libpng-dev \
        libpq-dev \
        libproj-dev \
        libreadline-dev \
        libsqlite3-dev \
        libtiff-dev \
        libxmu-dev \
        make \
        netcdf-bin \
        proj-bin \
        python \
        python-dev \
        python-numpy \
        python-pil \
        python-ply \
        unixodbc-dev \
        zlib1g-dev \
    && apt-get autoremove \
    && apt-get clean && \
    mkdir -p $DATA_DIR

RUN mkdir /code
RUN mkdir /code/grass

# add repository files to the image
COPY . /code/grass

WORKDIR /code/grass

# install GRASS GIS
RUN ./configure \
    --enable-largefile=yes \
    --with-nls \
    --with-cxx \
    --with-readline \
    --with-bzlib \
    --with-pthread \
    --with-proj-share=/usr/share/proj \
    --with-geos=/usr/bin/geos-config \
    --with-cairo \
    --with-opengl-libs=/usr/include/GL \
    --with-freetype=yes --with-freetype-includes="/usr/include/freetype2/" \
    --with-sqlite=yes \
    --with-liblas=yes --with-liblas-config=/usr/bin/liblas-config \
    && make -j2 && make install && ldconfig

# enable simple grass command regardless of version number
RUN ln -s /usr/local/bin/grass* /usr/local/bin/grass

# Fix permissions
RUN chmod -R a+rwx $DATA_DIR

# create a user
RUN useradd -m -U grass

# declare volume late so permissions apply
VOLUME $DATA_DIR
WORKDIR $DATA_DIR

# switch the user
USER grass

CMD ["/usr/local/bin/grass", "--version"]
