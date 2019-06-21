# Docker GRASS GIS (alpine linux)

Dockerfile with an [Alpine Linux](https://www.alpinelinux.org/) image with [GRASS GIS](https://grass.osgeo.org/) and [grass-session](https://github.com/zarch/grass-session/).

Download size of this image is only approximately 80 MB.

Build the docker with:

```bash
$ sudo docker build --build-arg GRASS_VERSION=7.6 \
                    --build-arg PYTHON_VERSION=3  \
                    --build-arg PROJ_VERSION=5.2.0 \
                    --build-arg PROJ_DATUMGRID_VERSION=1.8 \
                    .
```

View the images available using `sudo docker images` and open a bash terminal with:

```bash
$ sudo docker run -i -t 15550df91610 /bin/bash
bash-5.0#
```
