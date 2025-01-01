# Singularity GRASS GIS (Debian Linux)

Singularityfile with an [Debian Linux](https://www.debian.org/) image with
[GRASS GIS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support.

Download size of this image is of approximately 560 MB.

Clone this repository and change directory:

```shell
git clone https://github.com/OSGeo/grass.git
cd grass
```

__Build the singularity with__:

```shell
sudo singularity build grass_gis.simg singularity/debian/singularity_debian
```

__To build a stable version__:

change to the releasebranch or tag you want to build:

```shell
git checkout {tag or branch}
```

and build and enter with:

```shell
sudo singularity build grass_gis.simg singularity/debian/singularity_debian
```

The image can be used as:

```shell
singularity exec containers/grass_gis.simg grass --version
```
