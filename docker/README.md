# GRASS GIS docker

## GRASS GIS docker matrix

[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

Find out included version of GDAL, GEOS, PROJ, PDAL, Python and GRASS GIS using

```bash
grass --tmp-location XY --exec g.version -rge \
    && pdal --version \
    && python3 --version
```

## Requirements

* docker or podman

## Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

```bash
docker pull mundialis/grass-py3-pdal:<tag>
```

See also: <https://grass.osgeo.org/download/docker/>
