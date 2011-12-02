#!/bin/sh

# Test script for r.viewshed based on a synthetic DEM
# RUN THIS IS ANY GRASS LOCATION, e.g. NC or Spearfish

# create first hemisphere
g.region n=1000 s=0 w=0 e=1000 -p res=1
r.mapcalc 'disk.15031=if(sqrt((col() - 500)^2 + (500 - row())^2)<500,sqrt((col() - 500)^2 + (500 - row())^2),null())'
r.mapcalc 'hemisphere1=500 * sin(acos (disk.15031/500))'

# create second hemisphere
g.region n=500 s=0 w=0 e=500 -p res=1
r.mapcalc 'disk.14947=if(sqrt((col() - 500)^2 + (500 - row())^2)<500,sqrt((col() - 500)^2 + (500 - row())^2),null())'
r.mapcalc 'hemisphere2=500 * sin(acos (disk.14947/500))'
g.remove --q rast=disk.14947,disk.15031
# merge both
r.mapcalc "hemisphere=hemisphere1 + hemisphere2"

d.mon x0
d.rast hemisphere

# run r.viewshed
r.viewshed hemisphere out=hemisphere_viewshed coord=250,250 max=1000000 obs=100 mem=2000 --o
r.shaded.relief hemisphere --o
d.his h=hemisphere_viewshed i=hemisphere.shade

# compare to r.los
r.los hemisphere out=hemisphere_los coord=250,250 max=1000000 obs=100 --o
d.mon x1
d.his h=hemisphere_los i=hemisphere.shade

r.mapcalc "hemisphere_diff = hemisphere_viewshed - hemisphere_los"
r.colors hemisphere_diff color=differences
d.mon x2
d.rast.leg pos=80 map=hemisphere_diff

nviz hemisphere col=hemisphere_viewshed

