# GRASS GIS docker

## GRASS GIS docker matrix

[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

<!-- markdownlint-disable line-length -->
| Base image   | Docker tag    | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python |
|--------------|---------------|------------|-------|-------|-------|--------|
| Ubuntu 22.04 | latest-ubuntu | 8.4.0dev   | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 |
| Debian 11    | latest-debian | 8.4.0dev   | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  |
| Alpine 3.17  | latest-alpine | 8.4.0dev   | 9.1.0 | 3.5.3 | 2.4.3| 3.10.11 |
|--------------|---------------|------------|-------|-------|-------|--------|
| Ubuntu 22.04 | stable-ubuntu | 8.3 branch | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 |
| Debian 11    | stable-debian | 8.3 branch | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  |
| Alpine 3.17  | stable-alpine | 8.3 branch | 9.1.0 | 3.5.3 | 2.4.3| 3.10.11 |
<!-- markdownlint-enable line-length -->

Last update: May 17 2023 (source: <https://github.com/OSGeo/grass/actions/workflows/docker.yml>
and <https://hub.docker.com/r/mundialis/grass-py3-pdal/tags>)

## Requirements

* docker or podman

## Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

```bash
docker pull mundialis/grass-py3-pdal:<tag>
```

See also: <https://grass.osgeo.org/download/docker/>
