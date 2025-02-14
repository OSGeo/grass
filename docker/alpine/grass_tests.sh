#/bin/bash

# to be used in alpine Dockerfile

# add dependency
apk add --no-cache py3-scikit-learn

echo "Testing the GDAL-GRASS plugins:"
gdalinfo --formats | grep "GRASS Rasters" && \
ogrinfo --formats | grep "GRASS Vectors" || echo "...failed"

# Test grass-session
/usr/bin/python3 /scripts/test_grass_session.py
# Test PDAL
grass --tmp-project EPSG:25832 --exec r.in.pdal input="/tmp/simple.laz" output="count_1" method="n" resolution=1 -g

# Test GRASS GIS Python-addon installation
grass --tmp-project XY --exec g.extension extension=r.learn.ml2 operation=add && \
   grass --tmp-project XY --exec g.extension extension=r.learn.ml2 operation=remove -f

# Test GRASS GIS C-addon installation: raster and vector
grass --tmp-project XY --exec g.extension extension=r.gwr operation=add && \
   grass --tmp-project XY --exec g.extension extension=r.gwr operation=remove -f
grass --tmp-project XY --exec g.extension extension=v.centerpoint operation=add && \
   grass --tmp-project XY --exec g.extension extension=v.centerpoint operation=remove -f

# cleanup dependency
apk del py3-scikit-learn
