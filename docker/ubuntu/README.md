# Docker GRASS (Ubuntu Linux) #

## Using Ubuntu docker builds maintained by the GRASS development team ##

Note: If the images are not found locally on your computer,
they will be downloaded from docker-hub (~ 2.6 GB).

__Run the command line image for the development branch with__:

```bash
docker run -it osgeo/grass-gis:main-ubuntu grass -h
```

__Run the command line image for releasebranch_8_4 with__:

```bash
docker run -it osgeo/grass-gis:releasebranch_8_4-ubuntu grass -h
```

__Run the image for the development branch with GUI support with__:

```bash
docker run -it osgeo/grass-gis:main-ubuntu_wxgui grass -h
```

## Building and running Ubuntu docker images ##

Dockerfile with an [Ubuntu Linux](https://ubuntu.com/) image with
[GRASS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support.

Download size of this image is of approximately 2.6 GB.

To build your own image, do as follows:

Clone this repository and change directory:

```bash
git clone https://github.com/OSGeo/grass.git
cd grass
```

__Build the development branch with__:

```bash
DOCKER_BUILDKIT=1 docker build . \
  --file docker/ubuntu/Dockerfile \
  --tag grass-main-ubuntu:latest \
  --build-arg GUI=without \
  --build-arg NUMTHREADS=8 \
  --build-arg BRANCH="$(git branch --show-current)" \
  --build-arg REVISION="$(git log -n 1 --pretty=format:'%h')" \
  --build-arg VERSION="$(head include/VERSION -n 3 | sed ':a;N;$!ba;s/\n/ /g')" \
  --build-arg BUILD_DATE="$(date +'%Y-%m-%dT%H:%M:%S%:z')"
```

View the images available using `sudo docker images` and open a bash terminal
with:

```bash
docker run -it grass-main-ubuntu:latest grass -h
```

__To build a stable version__:

Change to the releasebranch or tag you want to build:

```bash
git checkout remotes/origin/releasebranch_8_4
```

and build and enter with:

```bash
DOCKER_BUILDKIT=1 docker build . \
  --file docker/ubuntu/Dockerfile \
  --tag grass-stable-ubuntu:latest \
  --build-arg GUI=without \
  --build-arg NUMTHREADS=8 \
  --build-arg BRANCH="$(git branch --show-current)" \
  --build-arg REVISION="$(git log -n 1 --pretty=format:'%h')" \
  --build-arg VERSION="$(head include/VERSION -n 3 | sed ':a;N;$!ba;s/\n/ /g')" \
  --build-arg BUILD_DATE="$(date +'%Y-%m-%dT%H:%M:%S%:z')"

docker run -it grass-stable-ubuntu:latest grass -h
```

__To build and run a latest version with wxgui__:

The `GUI` build argument allows choosing if the GUI should
be included in the build (`GUI=with`) or not (`GUI=without`).

```bash
git checkout main
DOCKER_BUILDKIT=1 docker build . \
  --file docker/ubuntu/Dockerfile \
  --tag grass-main-ubuntu-wxgui:latest \
  --build-arg GUI=with \
  --build-arg NUMTHREADS=8 \
  --build-arg BRANCH="$(git branch --show-current)" \
  --build-arg REVISION="$(git log -n 1 --pretty=format:'%h')" \
  --build-arg VERSION="$(head include/VERSION -n 3 | sed ':a;N;$!ba;s/\n/ /g')" \
  --build-arg BUILD_DATE="$(date +'%Y-%m-%dT%H:%M:%S%:z')"

xhost local:$(id -u)
docker run -it --privileged --user=$(id -u):$(id -g) --rm \
  --volume="$(pwd)/:/data" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
  --env HOME=/data/ --env DISPLAY=$DISPLAY \
  --device="/dev/dri/card0:/dev/dri/card0" \
  grass-main-ubuntu-wxgui:latest grass --gui
```

## Test the docker image ##

Note: Adjust the volume mount to the path of the GRASS source code directory.

```bash
# Test basic functionality
docker run -ti \
  -v /opt/src/grass:/grassdb \
  grass-main-ubuntu:latest \
  bash docker/testdata/test_docker_image.sh

# Execute also the full test suite
docker run -ti \
  -e TESTSUITE=1 \
  -v /opt/src/grass:/grassdb \
  grass-main-ubuntu:latest \
  bash docker/testdata/test_docker_image.sh
```
