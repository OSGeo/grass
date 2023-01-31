# GRASS GIS docker
<<<<<<< HEAD

## GRASS GIS docker matrix
<<<<<<< HEAD
<<<<<<< HEAD

[![Docker Pulls](https://img.shields.io/docker/pulls/osgeo/grass-gis.svg)](https://grass.osgeo.org/download/docker/)

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
docker pull osgeo/grass-gis:<tag>
```

=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

<!-- markdownlint-disable line-length -->
| Base image   | Docker tag    | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python | Image size |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 22.04 | latest-ubuntu | 8.3.dev    | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 | 2.89 GB    |
| Debian 11    | latest-debian | 8.3.dev    | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | latest-alpine | 8.3.dev    | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 22.04 | stable-ubuntu | 8.2 branch | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 | 2.89 GB    |
| Debian 11    | stable-debian | 8.2 branch | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | stable-alpine | 8.2 branch | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
<!-- markdownlint-enable line-length -->

Last update: 22 Jan 2023 (source: <https://github.com/OSGeo/grass/actions/workflows/docker.yml>
and <https://hub.docker.com/r/mundialis/grass-py3-pdal/tags>)

## Requirements

* docker or podman

## Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

=======

## GRASS GIS docker matrix

[![Docker Pulls](https://img.shields.io/docker/pulls/mundialis/grass-py3-pdal.svg)](https://grass.osgeo.org/download/software/docker-images/)

<!-- markdownlint-disable line-length -->
| Base image   | Docker tag    | GRASS GIS  | PROJ  | GDAL  | PDAL  | Python | Image size |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 22.04 | latest-ubuntu | 8.3.dev    | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 | 2.89 GB    |
| Debian 11    | latest-debian | 8.3.dev    | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | latest-alpine | 8.3.dev    | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
|--------------|---------------|------------|-------|-------|-------|--------|------------|
| Ubuntu 22.04 | stable-ubuntu | 8.2 branch | 8.2.1 | 3.4.1 | 2.4.3 | 3.10.6 | 2.89 GB    |
| Debian 11    | stable-debian | 8.2 branch | 7.2.1 | 3.2.2 | 2.4.3 | 3.9.2  | 2.93 GB    |
| Alpine 3.12  | stable-alpine | 8.2 branch | 7.0.1 | 3.1.4 | 2.1.0 | 3.8.10 |  186 MB    |
<!-- markdownlint-enable line-length -->

Last update: 22 Jan 2023 (source: <https://github.com/OSGeo/grass/actions/workflows/docker.yml>
and <https://hub.docker.com/r/mundialis/grass-py3-pdal/tags>)

## Requirements

* docker or podman

## Installation

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the table above):

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
```bash
docker pull mundialis/grass-py3-pdal:<tag>
```

<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
See also: <https://grass.osgeo.org/download/docker/>
