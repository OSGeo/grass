# GRASS GIS docker

## GRASS GIS docker matrix

[![Docker Pulls](https://img.shields.io/docker/pulls/osgeo/grass-gis.svg)](https://grass.osgeo.org/download/docker/)

Find out included version of GDAL, GEOS, PROJ, PDAL, Python and GRASS GIS using

```bash
grass --tmp-project XY --exec g.version -rge \
    && pdal --version \
    && python3 --version
```

## Requirements

* docker or podman

## Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

```bash
docker pull osgeo/grass-gis:<tag>
```

See also: <https://grass.osgeo.org/download/docker/>
