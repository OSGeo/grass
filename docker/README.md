# GRASS docker

[![Docker Pulls](https://img.shields.io/docker/pulls/osgeo/grass-gis.svg)](https://grass.osgeo.org/download/docker/)

To install a docker image, run (replace `<tag>` with the desired Docker tag from
the [list of Docker images](https://grass.osgeo.org/download/docker/):

```bash
docker pull osgeo/grass-gis:<tag>
```

See also: <https://grass.osgeo.org/download/docker/>

Find out included version of GDAL, GEOS, PROJ, PDAL, Python and GRASS using

```bash
grass --tmp-project XY --exec g.version -rge \
    && pdal --version \
    && python3 --version
```

## Building the docker image

Build a docker image using the downloaded source code (run this in the directory
containing the source code):

### Docker image **without graphical user interface - wxGUI**

```bash
docker build -t grassgis .
```

A test run (assuming you have the existing GRASS test location;

[Download test project data here](https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.zip))

```bash
# case 1: launching in the grassdata directory in which the location is stored:
docker run -it --rm --user=$(id -u):$(id -g) --volume $(pwd):/data \
    --env HOME=/data/ grassgis grass --text nc_basic_spm_grass7/user1 \
        --exec g.region -p

# case 2: launching anywhere
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ grassgis \
        grass /data/nc_basic_spm_grass7/PERMANENT --exec g.region -p
```

Note that the first `grassgis` is the name of the image while the second
`grass` is the name of the executable.

To run the tests (again assuming local location):

```bash
docker run -it --rm --user=$(id -u):$(id -g) \
    --volume /your/test/grassdata/:/data --env HOME=/data/ -w /code/grass \
        grassgis grass /data/nc_basic_spm_grass7/PERMANENT --exec \
            python -m grass.gunittest.main \
                --location nc_basic_spm_grass7 --location-type nc
```

### Docker image **with graphical user interface - wxGUI**

```bash
docker build -t grassgis -f docker/ubuntu_wxgui/Dockerfile .
```

Note that the first `grassgis` is the name of the image while the second
`grass` is the name of the executable.

```bash
xhost local:$(id -u)
docker run -it --privileged --user=$(id -u):$(id -g) --rm \
    --volume="$(pwd)/:/data" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
    --env HOME=/data/ --env DISPLAY=$DISPLAY \
    --device="/dev/dri/card0:/dev/dri/card0" \
    grassgis grass --gui
```

Note: If you compiled locally before building the Docker image, you may
encounter problems as the local configuration and the locally compiled files
are copied to and used in the Docker image. To make sure you don't have
this issue, clean all the compiled files from the source code:

```bash
make distclean
```
