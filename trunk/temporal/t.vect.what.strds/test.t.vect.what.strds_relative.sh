#!/bin/sh
# Test the temporal and spatial sampling of raster maps by vector point maps
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100.0"
r.mapcalc --o expr="prec_2 = 200.0"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500.0"
r.mapcalc --o expr="prec_6 = 600.0"

v.random --o -z output=soil_1 npoints=5 zmin=0 zmax=100 column=heigh seed=1
v.random --o -z output=soil_2 npoints=5 zmin=0 zmax=100 column=height seed=2
v.random --o -z output=soil_3 npoints=5 zmin=0 zmax=100 column=height seed=3

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soil_1|0|10
soil_2|12|17
soil_3|20|30
EOF

t.create --o type=stvds temporaltype=relative output=soil_rel1 title="A test" descr="A test"
t.register type=vector input=soil_rel1 file="${n1}" unit=minutes
t.info type=stvds input=soil_rel1

t.create --o type=strds temporaltype=relative output=precip_rel1 title="A test" descr="A test"
t.register type=raster input=precip_rel1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start=0 increment=5 unit=minutes

# The @test

t.vect.what.strds --v input=soil_rel1 strds=precip_rel1 sampling=start,during column=map_vals method=maximum
v.db.select map=soil_1
v.db.select map=soil_2
v.db.select map=soil_3

t.vect.what.strds --v input=soil_rel1 strds=precip_rel1 sampling=during column=map_vals method=average
v.db.select map=soil_1
v.db.select map=soil_2
v.db.select map=soil_3

t.vect.what.strds --v input=soil_rel1 strds=precip_rel1 sampling=start,during
v.db.select map=soil_1
v.db.select map=soil_2
v.db.select map=soil_3

# @postprocess
t.unregister type=vector maps=soil_1,soil_2,soil_3
t.remove type=stvds input=soil_rel1

t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_rel1

g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
g.remove -f type=vector name=soil_1,soil_2,soil_3
