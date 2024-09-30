## GRASS GIS Docker installation and usage

[![Docker Pulls](https://img.shields.io/docker/pulls/osgeo/grass-gis.svg)](https://grass.osgeo.org/download/docker/)

The following instructions will guide you through setting up and using the
GRASS GIS via Docker or Podman.

## Requirements

Have [Docker](https://www.docker.com/) or [podman](https://podman.io/) podman
installed.

## Overview

For a list of GRASS GIS versions, see the
[docker tag list](https://hub.docker.com/r/osgeo/grass-gis/tags).

## General installation notes

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

```bash
docker pull osgeo/grass-gis:<tag>
```

## Provided images

Choose the image and running method that best suits your needs based on
the required features, available disk space, and whether you need GUI support.

### Alpine-based image

**Installation:**

```bash
docker pull osgeo/grass-gis:releasebranch_8_4-alpine
```

**Running:**

```bash
docker run -it --rm osgeo/grass-gis:releasebranch_8_4-alpine grass -c EPSG:4326 /tmp/grassproject
```

### Debian-based image

**Installation:**

```bash
docker pull osgeo/grass-gis:releasebranch_8_4-debian
```

**Running:**

```bash
docker run -it --rm osgeo/grass-gis:releasebranch_8_4-debian grass -c EPSG:4326 /tmp/grassproject
```

### Ubuntu-based image

**Installation:**

```bash
docker pull osgeo/grass-gis:releasebranch_8_4-ubuntu
```

**Running:**

```bash
docker run -it --rm osgeo/grass-gis:releasebranch_8_4-ubuntu grass -c EPSG:4326 /tmp/grassproject
```

### Ubuntu-based image with GUI

**Installation:**

```bash
docker pull osgeo/grass-gis:releasebranch_8_4-ubuntu_wxgui
```

**Running:**

```bash
docker run -it --rm -e DISPLAY=$DISPLAY \
       -v /tmp/.X11-unix:/tmp/.X11-unix \
       osgeo/grass-gis:releasebranch_8_4-ubuntu_wxgui grass
```

## Additional usage tips

- To work with data on your host system, use the `-v` (volume) option
  to mount a local directory:

```bash
docker run -it --rm -v "$(pwd):/data" osgeo/grass-gis:releasebranch_8_4-alpine grass
```

- To run a specific GRASS GIS command, append it to the end of the docker run command:

```bash
docker run --rm osgeo/grass-gis:releasebranch_8_4-alpine grass --exec r.info map=elevation
```

- For persistent sessions, consider creating a named container:

```bash
docker run -it --name my_grass_session osgeo/grass-gis:releasebranch_8_4-alpine grass
```

- To resume a named session:

```bash
docker start -ai my_grass_session
```

- Show GRASS GIS compilation metadata and dependencies

Find out which version of GDAL, GEOS, PROJ, PDAL, Python and GRASS GIS are
included in a specific image (example for Alpine):

```bash
docker run -it --rm osgeo/grass-gis:releasebranch_8_4-alpine \
    grass --tmp-project XY --exec g.version -rge \
    && pdal --version \
    && python3 --version
```

## Resources

See also: <https://grass.osgeo.org/download/docker/>

For a detailed guide on GRASS GIS commands, check the
[GRASS GIS documentation](https://grass.osgeo.org/learn/).
