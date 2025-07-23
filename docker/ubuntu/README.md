# Docker GRASS GIS (Ubuntu Linux)

Dockerfile with an [Ubuntu Linux](https://ubuntu.com/) image with
[GRASS GIS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support.

Download size of this image is of approximately 2.6 GB.

Clone this repository and change directory:

```bash
git clone https://github.com/OSGeo/grass.git
cd grass
```

## Ubuntu stable

__Build the docker with__:

```bash
docker build \
         --file docker/ubuntu/Dockerfile \
         --tag grass-py3-pdal:stable-ubuntu .
```

View the images available using `sudo docker images` and open a bash terminal
with:

```bash
$ docker run -it grass-py3-pdal:stable-ubuntu /bin/bash
bash-5.0#
```

__To build a stable version__:

change to the releasebranch or tag you want to build:

```bash
git checkout remotes/origin/releasebranch_8_2
```

and build and enter with:

```bash
$ docker build \
         -f docker/ubuntu/Dockerfile \
         -t grass-py3-pdal:stable-ubuntu .

$ docker run -it grass-py3-pdal:stable-ubuntu /bin/bash
bash-5.0#
```

## Ubuntu latest

__Build the docker with__:

```bash
$ docker build \
         --file docker/ubuntu/Dockerfile_ubuntu_latest_pdal \
         --tag grass-py3-pdal:latest-ubuntu .
```

View the images available using `sudo docker images` and open a bash terminal
with:

```bash
$ docker run -it grass-py3-pdal:latest-ubuntu /bin/bash
bash-5.0#
```

__To build a latest version__:

change to the releasebranch or tag you want to build:

```bash
git checkout remotes/origin/releasebranch_8_2
```

and build and enter with:

```bash
$ docker build \
         -f docker/ubuntu/Dockerfile_ubuntu_latest_pdal \
         -t grass-py3-pdal:latest-ubuntu .

$ docker run -it grass-py3-pdal:latest-ubuntu /bin/bash
bash-5.0#
```

__To build a latest version with wxgui__:

The `GUI` build argument allows choosing if the GUI should
be included in the build (`GUI=with`) or not (`GUI=without`).

```bash
$ DOCKER_BUILDKIT=1 docker build  \
    --file docker/ubuntu/Dockerfile \
    --tag grass-main-ubuntu-wxgui:latest \
    --build-arg GUI=with .
```

## Test the docker image

Note: Adjust the volume mount to the path of the GRASS GIS source code directory.

```bash
# Test basic functionality
$ docker run -ti \
         -v /opt/src/grass:/grassdb \
         grass-py3-pdal:latest-ubuntu \
         bash docker/testdata/test_docker_image.sh

# Execute also the full test suite
$ docker run -ti \
         -e TESTSUITE=1 \
         -v /opt/src/grass:/grassdb \
         grass-py3-pdal:latest-ubuntu \
         bash docker/testdata/test_docker_image.sh
```
