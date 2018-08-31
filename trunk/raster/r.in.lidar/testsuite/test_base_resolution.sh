#!/usr/bin/env bash

set -e
set -x

basename="test_rinlidar_"

g.region n=20 s=10 e=30 w=20 res=2.5
r.in.xyz data/points.txt output=${basename}base x=1 y=2 z=3 separator=comma

echo "With base raster resolution matching current region"

r.in.lidar input=data/points.las output=${basename}with_region \
    base_raster=${basename}base method=min -o
echo "Almost all in the following r.univar output should be zero"
r.univar ${basename}with_region
echo "Automatic test if there are only allowed values..."
r.univar ${basename}with_region -g \
    | grep -ve "=0$" | grep -ve "=-nan$" | grep -e "=[^2-9][^12345789]$"

echo "With base raster resolution different from current region"

g.region res=5

r.in.lidar input=data/points.las output=${basename}with_base \
    base_raster=${basename}base method=min -o -d
echo "Almost all in the following r.univar output should be zero"
r.univar ${basename}with_base
echo "Automatic test if there are only allowed values..."
r.univar ${basename}with_base -g \
    | grep -ve "=0$" | grep -ve "=-nan$" | grep -e "=[^12356789]$"

echo "Test successful
When running manually maps can be now removed with:
  g.remove type=rast pattern='test_rinlidar_*' -f
However, the region was changed to whatever the test needed."
