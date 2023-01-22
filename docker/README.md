# GRASS GIS docker

## GRASS GIS docker matrix

[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

<!-- markdownlint-disable line-length -->
| Base image   | Docker tag    | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python | Image size |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 20.04 | latest-ubuntu | 8.3.dev    | 6.3.1 | 3.0.4 | 2.2.0 | 3.8.10 | 1.20 GB    |
| Debian 11    | latest-debian | 8.3.dev    | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | latest-alpine | 8.3.dev    | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 20.04 | stable-ubuntu | 8.2 branch | 6.3.1 | 3.0.4 | 2.2.0 | 3.8.10 | 1.20 GB    |
| Debian 11    | stable-debian | 8.2 branch | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | stable-alpine | 8.2 branch | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
<!-- markdownlint-enable line-length -->

Last update: 21 Jan 2023 (source: <https://github.com/OSGeo/grass/actions/workflows/docker.yml>
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
