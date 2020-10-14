[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

# GRASS GIS docker matrix

| Base image   | Docker tag      | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python | Image size |
|--------------|-----------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 18.04 | latest-ubuntu   | 7.9.dev    | 4.9.3 | 2.2.3 | 1.8.0 | 3.6.9  | 1.04 GB    |
| Ubuntu 19.10 | latest-ubuntu19 | 7.9.dev    | 5.2.0 | 2.4.2 | 1.9.1 | 3.7.5  |  850 MB    |
| Debian 10.1  | latest-debian   | 7.9.dev    | 5.2.0 | 2.4.0 | 1.8.0 | 3.7.3  | 1.14 GB    |
| Alpine 3.12  | latest-alpine   | 7.9.dev    | 7.0.1 | 3.1.2 | 2.1.0 | 3.5.8  |  385 MB    |
|--------------|-----------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 18.04 | stable-ubuntu   | 7.8 branch | 4.9.3 | 2.2.3 | 1.8.0 | 3.6.9  | 1.04 GB    |
| Ubuntu 19.10 | stable-ubuntu19 | 7.8 branch | 5.2.0 | 2.4.2 | 1.9.1 | 3.7.5  |  850 MB    |
| Debian 10.1  | stable-debian   | 7.8 branch | 5.2.0 | 2.4.0 | 1.8.0 | 3.7.3  | 1.14 GB    |
| Alpine 3.12  | stable-alpine   | 7.8 branch | 7.0.1 | 3.1.2 | 2.1.0 | 3.5.8  |  385 MB    |

Last update: 17 Sep 2020 (source: https://hub.docker.com/r/mundialis/grass-py3-pdal/builds)

# Requirements

 * docker or podman

# Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from the table above):

```
docker pull mundialis/grass-py3-pdal:<tag>
```

See also: https://grass.osgeo.org/download/docker/
