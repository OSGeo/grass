#!/bin/sh

set -e

# to be used in Dockerfile

# Display environment
printf "\n############\nPrinting defined environment variables:\n############\n"
printenv

# run simple LAZ test
cp docker/testdata/simple.laz /tmp/
cp docker/testdata/test_grass_session.py /tmp/
cp docker/testdata/test_grass_python.py /tmp/
cp -r demolocation /tmp/

# Test gdal-grass-plugin
printf "\n############\nTesting the gdal_grass plugin:\n############\n"
gdalinfo --formats | grep "GRASS -raster-"

# Test grass-session
printf "\n############\nTesting grass_session:\n############\n"
/usr/bin/python3 /tmp/test_grass_session.py

# Test grass-setup
printf "\n############\nTesting grass script setup:\n############\n"
export DEMOLOCATION=/tmp/demolocation/PERMANENT
/usr/bin/python3 /tmp/test_grass_python.py

# Test PDAL
printf "\n############\nTesting PDAL with laz:\n############\n"
grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

# Test GRASS GIS Python-addon installation
# add dependency
printf "\n############\nTesting GRASS GIS Python-addon installation:\n############\n"
/usr/bin/python3 -m pip install --no-cache-dir scikit-learn

grass --tmp-project XY --exec g.extension extension=r.learn.ml2 operation=add && \
	   grass --tmp-project XY --exec g.extension extension=r.learn.ml2 operation=remove -f

# cleanup dependency
/usr/bin/python3 -m pip uninstall -y scikit-learn

# Test GRASS GIS C-addon installation: raster and vector
printf "\n############\nTesting GRASS GIS C-addon installation:\n############\n"
grass --tmp-project XY --exec g.extension extension=r.gwr operation=add && \
	   grass --tmp-project XY --exec g.extension extension=r.gwr operation=remove -f
grass --tmp-project XY --exec g.extension extension=v.centerpoint operation=add && \
	   grass --tmp-project XY --exec g.extension extension=v.centerpoint operation=remove -f

# show GRASS GIS, PROJ, GDAL etc versions
printf "\n############\nPrinting GRASS, PDAL and Python versions:\n############\n"
grass --tmp-project EPSG:4326 --exec g.version -rge && \
    pdal --version && \
    python3 --version

# Test presence of central python packages
printf "\n############\nPrinting versions of central python packages:\n############\n"
python3 -c "import psycopg2;import numpy as np;print(psycopg2.__version__);print(np.__version__)"

# Run testsuite
if [ $TESTSUITE ] ; then
  printf "\n############\nRunning the testsuite:\n############\n"
  bash /grassdb/.github/workflows/test_thorough.sh
fi
