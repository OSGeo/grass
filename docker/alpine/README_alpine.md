# Docker GRASS GIS (alpine linux)

Dockerfile with an [Alpine Linux](https://www.alpinelinux.org/) image with
[GRASS GIS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support and
[grass-session](https://github.com/zarch/grass-session/).

Download size of this image is only approximately 80 MB.

Clone this repository and change directory:

```bash
git clone https://github.com/OSGeo/grass.git
cd grass
```

__Build the docker with__:

```bash
docker build \
         --file docker/alpine/Dockerfile_alpine \
         --tag grass-py3-pdal:latest-alpine .
```

View the images available using `sudo docker images` and open a bash terminal
with:

```bash
$ docker run -it grass-py3-pdal:latest-alpine /bin/bash
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
         -f docker/alpine/Dockerfile_alpine \
         -t grass-py3-pdal:stable-alpine .

$ docker run -it grass-py3-pdal:stable-alpine /bin/bash
bash-5.0#
```
