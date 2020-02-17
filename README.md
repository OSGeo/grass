[![Build Status](https://travis-ci.com/OSGeo/grass.svg?branch=master)](https://travis-ci.com/OSGeo/grass)

# GRASS GIS Repository

## Description

GRASS GIS ([https://grass.osgeo.org/](https://grass.osgeo.org/)) is
a Geographic Information System used for geospatial data management and
analysis, image processing, graphics/map production, spatial modeling, and
visualization.

## How to get write access here

In general: you don't really need write access as you can simply open
a [pull request](https://github.com/OSGeo/grass/pulls) to contribute to
GRASS GIS. See [CONTRIBUTING file](CONTRIBUTING.md) for more details.

Want to become a core developer? See
[Procedure for gaining Git write access](https://trac.osgeo.org/grass/wiki/HowToContribute#WriteaccesstotheGRASScorerepository)

## How to compile GRASS

> See INSTALL file.

Yes, you should really read [INSTALL](INSTALL).

## How to generate the 'Programmer's Manual'

This needs doxygen (<http://www.doxygen.org>) and optionally
Graphviz dot (<http://www.research.att.com/sw/tools/graphviz/>).

To build the GRASS programmer's documentation, run

```
make htmldocs
```

or to generate documentation as single html file
(recommended for simple reading)

```
make htmldocs-single
```

here. This takes quite some time. The result is in `lib/html/index.html`
which refers to further document repositories in

```
lib/vector/html/index.html
lib/db/html/index.html
lib/gis/html/index.html
```

The master file is: `./grasslib.dox` where all sub-documents have to
be linked into.

To generate the documents in PDF format, run

```
make pdfdocs
```

An online version is available [here](https://grass.osgeo.org/programming7/)

## Docker

Build using the downloaded source code (in the directory with the
source code):

```
docker build -t grassgis78 .
```

A test run (assuming you have existing GRASS GIS location; it can be
downloaded from
[here](https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.zip))

```
# case 1: launching in the grassdata directory in which the location is stored:
docker run -it --rm --user=$(id -u):$(id -g) --volume $(pwd):/data \
    --env HOME=/data/ grassgis78 grass --text nc_basic_spm_grass7/user1 \
        --exec g.region -p

# case 2: launching anywhere
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ grassgis78 \
        grass /data/nc_basic_spm_grass7/PERMANENT --exec g.region -p
```

Note that the first `grassgis78` is the name of the image while the second
`grass` is the name of the executable.

To run the tests (again assuming local location):

```
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ -w /code/grass \
        grassgis78 grass /data/nc_basic_spm_grass7/PERMANENT --exec \
            python -m grass.gunittest.main \
                --location nc_basic_spm_grass7 --location-type nc
```

Note: If you compiled locally before building the Docker image, you may
encounter problems as the local configuration and locally compiled file
are copied to and used in the Docker image. To make sure you don't have
this issue, clean all the compiled files from the source code:

```
make distclean
```
