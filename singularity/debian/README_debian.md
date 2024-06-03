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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
sudo singularity build grass_development.simg singularity/debian/singularity_debian
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
sudo singularity build grass_gis.simg singularity/debian/singularity_debian
=======
sudo singularity build grass_development.simg singularity/debian/singularity_debian
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
sudo singularity build grass_development.simg singularity/debian/singularity_debian
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
```

__To build a stable version__:

change to the releasebranch or tag you want to build:

```bash
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
git checkout remotes/origin/releasebranch_8_2
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
git checkout {tag or branch}
=======
git checkout remotes/origin/releasebranch_8_2
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
git checkout remotes/origin/releasebranch_8_2
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
```

and build and enter with:

```bash
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
sudo singularity build grass_8_2.simg singularity/debian/singularity_debian
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
sudo singularity build grass_gis.simg singularity/debian/singularity_debian
=======
sudo singularity build grass_8_2.simg singularity/debian/singularity_debian
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
sudo singularity build grass_8_2.simg singularity/debian/singularity_debian
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
```

The image can be used as:

```bash
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
singularity exec containers/grass_8.2.simg grass --version
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
singularity exec containers/grass_gis.simg grass --version
=======
singularity exec containers/grass_8.2.simg grass --version
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
singularity exec containers/grass_8.2.simg grass --version
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
```
