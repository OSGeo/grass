# Docker GRASS GIS (Debian Linux)

Dockerfile with an [Debian Linux](https://www.debian.org/) image with
[GRASS GIS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support and
[grass-session](https://github.com/zarch/grass-session/).

Download size of this image is of approximately 2.6 GB.

Clone this repository and change directory:

```bash
git clone https://github.com/OSGeo/grass.git
cd grass
```

__Build the docker with__:

```bash
docker build \
         --file docker/debian/Dockerfile_debian_pdal \
         --tag grass-py3-pdal:latest-debian .
```

View the images available using `sudo docker images` and open a bash terminal with:

```bash
$ docker run -it grass-py3-pdal:latest-debian /bin/bash
bash-5.0#
```

__To build a stable version__:

change to the releasebranch or tag you want to build:

```bash
git checkout remotes/origin/releasebranch_7_8
```

and build and enter with:

```bash
$ docker build \
         -f docker/debian/Dockerfile_debian_pdal \
         -t grass-py3-pdal:stable-debian .

$ docker run -it grass-py3-pdal:stable-debian /bin/bash
bash-5.0#
```
