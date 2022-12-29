# Singularity GRASS GIS (Debian Linux)

Singularityfile with an [Debian Linux](https://www.debian.org/) image with
[GRASS GIS](https://grass.osgeo.org/), [PDAL](https://pdal.io) support and
[grass-session](https://github.com/zarch/grass-session/).

Download size of this image is of approximately 560 MB.

Clone this repository and change directory:

```bash
git clone https://github.com/OSGeo/grass.git
cd grass
```

__Build the singularity with__:

```bash
sudo singularity build grass_master.simg singularity/debian/singularity_debian
```

The image can be used as:

```bash
singularity exec containers/grass_7.8.simg grass --version
```

__To build a stable version__:

change to the releasebranch or tag you want to build:

```bash
git checkout remotes/origin/releasebranch_7_8
```

and build and enter with:

```bash
sudo singularity build grass_7_8.simg singularity/debian/singularity_debian
```
