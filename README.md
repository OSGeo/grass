[![Build Status](https://travis-ci.com/OSGeo/grass.svg?branch=main)](https://travis-ci.com/OSGeo/grass)
[![GCC C/C++ standards check](https://github.com/OSGeo/grass/workflows/GCC%20C/C++%20standards%20check/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22GCC+C%2FC%2B%2B+standards+check%22)
[![Python code quality check](https://github.com/OSGeo/grass/workflows/Python%20code%20quality%20check/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22Python+code+quality+check%22)
[![General linting](https://github.com/OSGeo/grass/workflows/General%20linting/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22General+linting%22)
[![Ubuntu](https://github.com/OSGeo/grass/workflows/Ubuntu/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3AUbuntu)
[![OSGeo4W](https://github.com/OSGeo/grass/workflows/OSGeo4W/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3AOSGeo4W)
[![CentOS](https://github.com/OSGeo/grass/workflows/CentOS/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3ACentOS)

# GRASS GIS Repository

## Description

GRASS GIS ([https://grass.osgeo.org/](https://grass.osgeo.org/)) is
a Geographic Information System used for geospatial data management and
analysis, image processing, graphics/map production, spatial modeling, and
visualization.

Launch this repository in Binder and experiment with GRASS's Python API in Jupyter Notebooks by clicking the button below:

[![Binder](https://camo.githubusercontent.com/581c077bdbc6ca6899c86d0acc6145ae85e9d80e6f805a1071793dbe48917982/68747470733a2f2f6d7962696e6465722e6f72672f62616467655f6c6f676f2e737667)](https://mybinder.org/v2/gh/OSGeo/grass/main?urlpath=lab%2Ftree%2Fdoc%2Fnotebooks%2Fbasic_example.ipynb)


## How to get write access here

In general: you don't really need write access as you can simply open
a [pull request](https://github.com/OSGeo/grass/pulls) to contribute to
GRASS GIS. See [CONTRIBUTING file](CONTRIBUTING.md) for more details.

Want to become a core developer? See
[Procedure for gaining Git write access](https://trac.osgeo.org/grass/wiki/HowToContribute#WriteaccesstotheGRASScorerepository)

## How to compile GRASS

> See INSTALL file.

Yes, you should really read [INSTALL](INSTALL). In addition, there are detailed [compile instructions](https://grasswiki.osgeo.org/wiki/Compile_and_Install) in the Wiki.

## Docker

Build a docker image using the downloaded source code (run this in the directory
containing the source code):

```
docker build -t grassgis80 .
```

A test run (assuming you have the existing GRASS GIS test location; it can be
downloaded from
[here](https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.zip))

```
# case 1: launching in the grassdata directory in which the location is stored:
docker run -it --rm --user=$(id -u):$(id -g) --volume $(pwd):/data \
    --env HOME=/data/ grassgis80 grass --text nc_basic_spm_grass7/user1 \
        --exec g.region -p

# case 2: launching anywhere
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ grassgis80 \
        grass /data/nc_basic_spm_grass7/PERMANENT --exec g.region -p
```

Note that the first `grassgis80` is the name of the image while the second
`grass` is the name of the executable.

To run the tests (again assuming local location):

```
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ -w /code/grass \
        grassgis80 grass /data/nc_basic_spm_grass7/PERMANENT --exec \
            python -m grass.gunittest.main \
                --location nc_basic_spm_grass7 --location-type nc
```

Note: If you locally compile before building the Docker image, you may
encounter problems as the local configuration and the locally compiled files
are copied to and used in the Docker image. To make sure you don't have
this issue, clean all the compiled files from the source code:

```
make distclean
```

## How to generate the 'Programmer's Manual'

You can locally generate the [GRASS GIS Programmer's Manual](https://grass.osgeo.org/programming8/).

This needs doxygen (<http://www.doxygen.org>) and optionally
Graphviz dot (<http://www.research.att.com/sw/tools/graphviz/>).

To build the GRASS programmer's documentation, run

```
make htmldocs
```

Or to generate documentation as single html file
(recommended for simple reading)

```
make htmldocs-single
```

This takes quite some time. The result is in `lib/html/index.html`
which refers to further document repositories in

```
lib/vector/html/index.html
lib/db/html/index.html
lib/gis/html/index.html
```

The master file is: `./grasslib.dox` where all sub-documents have to
be linked to.

To generate the documents in PDF format, run

```
make pdfdocs
```
