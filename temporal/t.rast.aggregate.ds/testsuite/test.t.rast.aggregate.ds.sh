#!/bin/sh
# Aggregate a dataset by a second one

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc expr="prec_1 = 100"
r.mapcalc expr="prec_2 = 150"
r.mapcalc expr="prec_3 = 250"
r.mapcalc expr="prec_4 = 250"
r.mapcalc expr="prec_5 = 150"
r.mapcalc expr="prec_6 = 100"

v.random -z output=soil_1 n=20 zmin=0 zmax=100 column=height
v.random -z output=soil_2 n=20 zmin=0 zmax=100 column=height
v.random -z output=soil_3 n=20 zmin=0 zmax=100 column=height

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soil_1|2001-01-01|2001-04-01
soil_2|2001-05-01|2001-07-01
soil_3|2001-08-01|2001-12-01
EOF

t.create type=stvds temporaltype=absolute output=soil_abs1 title="A test" descr="A test"
t.register type=vector input=soil_abs1 file="${n1}"

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i type=raster input=precip_abs1 \
    maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-03-01 00:00:00" increment="1 months"

# The @test

t.rast.aggregate.ds --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    type=stvds sample=soil_abs1 method=sum sampling=contains offset=33
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2 method=deltagap

t.rast.aggregate.ds --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    type=stvds sample=soil_abs1 method=sum sampling=contains -n
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2 method=deltagap

# @postprocess
#t.remove -rf type=stvds input=soil_abs1
#t.remove -rf type=strds input=precip_abs1,precip_abs2
