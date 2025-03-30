# GRASS GIS Repository

[![GCC C/C++ standards check](https://github.com/OSGeo/grass/workflows/GCC%20C/C++%20standards%20check/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22GCC+C%2FC%2B%2B+standards+check%22)
[![Python code quality check](https://github.com/OSGeo/grass/workflows/Python%20code%20quality%20check/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22Python+code+quality+check%22)
[![General linting](https://github.com/OSGeo/grass/workflows/General%20linting/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3A%22General+linting%22)
[![Ubuntu](https://github.com/OSGeo/grass/workflows/Ubuntu/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3AUbuntu)
[![OSGeo4W](https://github.com/OSGeo/grass/workflows/OSGeo4W/badge.svg)](https://github.com/OSGeo/grass/actions?query=workflow%3AOSGeo4W)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/OSGeo/grass/badge)](https://securityscorecards.dev/viewer/?uri=github.com/OSGeo/grass)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/2470/badge)](https://www.bestpractices.dev/projects/2470)
[![Coverity](https://scan.coverity.com/projects/1038/badge.svg)](https://scan.coverity.com/projects/grass)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.5176030.svg)](https://doi.org/10.5281/zenodo.5176030)
[![Join the chat at https://gitter.im/grassgis/community](https://badges.gitter.im/grassgis/community.svg)](https://gitter.im/grassgis/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

## Description

GRASS GIS ([https://grass.osgeo.org/](https://grass.osgeo.org/)) is
a Geographic Information System used for geospatial data management and
analysis, image processing, graphics/map production, spatial modeling, and
visualization.

Launch this repository in Binder and experiment with GRASS's Python API in
Jupyter Notebooks by clicking the button below:

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/OSGeo/grass/main?labpath=doc%2Fexamples%2Fnotebooks%2Fjupyter_example.ipynb)

## Contributing

In general: you don't really need write access as you can simply open
a [pull request](https://github.com/OSGeo/grass/pulls) to contribute to
GRASS GIS. See [CONTRIBUTING file](CONTRIBUTING.md) for more details.

## How to compile GRASS

> See the INSTALL.md file.

Yes, you should really read [INSTALL.md](INSTALL.md). In addition, there are
detailed [compile instructions](https://grasswiki.osgeo.org/wiki/Compile_and_Install)
in the Wiki.

## Docker

Build a docker image using the downloaded source code (run this in the directory
containing the source code):

A. Docker image **without graphical user interface - wxGUI**.

```bash
docker build -t grassgis .
```

A test run (assuming you have the existing GRASS GIS test location; it can be
downloaded from
[here](https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.zip))

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

B. Docker image **with graphical user interface - wxGUI**.

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

## Further documents

- [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md)
- [Roadmap](https://grass.osgeo.org/about/roadmap/)

## Thanks to all contributors ❤

[![GRASS contributors](https://contrib.rocks/image?repo=OSGeo/grass "GRASS contributors")](https://github.com/OSGeo/grass/graphs/contributors)
