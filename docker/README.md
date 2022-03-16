[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

# GRASS GIS 7 docker matrix

| Base image   | Docker tag      | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python | Image size |
|--------------|-----------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 20.04 | latest-ubuntu   | 7.8.x      | 6.3.1 | 3.0.4 | 2.2.0 | 3.8.10 | 1.20 GB    |
| Debian 10.1  | latest-debian   | 7.8.x      | 5.2.0 | 2.4.0 | 1.8.0 | 3.7.3  | 1.16 GB    |
| Alpine 3.12  | latest-alpine   | 7.8.x      | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  185 MB    |


Last update: 16 Mar 2022 (source: https://github.com/OSGeo/grass/actions/workflows/docker.yml and https://hub.docker.com/r/mundialis/grass-py3-pdal/tags)

# Requirements

 * docker or podman

# Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from the table above):

```
docker pull mundialis/grass-py3-pdal:<tag>
```

See also: https://grass.osgeo.org/download/docker/
