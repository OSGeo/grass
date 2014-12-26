#!/bin/sh
# Convert space time raster datasets into space time voxel cubes
# We need to set a specific region in the
# @preprocess step of this test.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=1 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100.0"
r.mapcalc --o expr="prec_2 = 200.0"
r.mapcalc --o expr="prec_3 = 300.0"
r.mapcalc --o expr="prec_4 = 400.0"
r.mapcalc --o expr="prec_5 = 500.0"
r.mapcalc --o expr="prec_6 = 600.0"

n1=`g.tempfile pid=1 -d`

cat > "${n1}" << EOF
prec_1|2001-01-01|2001-04-01
prec_2|2001-05-01|2001-07-01
prec_3|2001-08-01|2001-10-01
EOF

# @test
# We create the space time raster inputs and register the raster maps with absolute time interval

t.create --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"
t.create --o type=strds temporaltype=relative output=precip_rel title="A test" descr="A test"

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="3 years"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="2 months"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="8 days"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="6 hours"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="30 minutes"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs maps=prec_1,prec_2,prec_3 start="2001-01-01" increment="1 seconds"
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_abs file=${n1}
t.info type=strds input=precip_abs

t.rast.to.rast3 --o input=precip_abs output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.register --o --v -i type=raster input=precip_rel maps=prec_4,prec_5,prec_6 start=1000 increment=100 unit=years
t.info type=strds input=precip_rel

t.rast.to.rast3 --o input=precip_rel output=precipitation
t.info type=raster_3d input=precipitation
r3.info precipitation

t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs,precip_rel
