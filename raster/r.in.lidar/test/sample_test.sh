# GNU GPL
# Vaclav Petras
# this is an idea for a test
export GRASS_OVERWRITE=1
r.in.lidar in=points.las out=test_pure
r.in.lidar in=points.las out=test_hag base=elevation
r.mapcalc "test_difference = (elevation + test_hag) - test_pure"
r.univar test_difference


g.region -p -a raster=test_difference@manual res=0.1 zoom=test_difference@manual
