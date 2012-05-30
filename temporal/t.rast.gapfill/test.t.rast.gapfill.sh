#!/bin/sh
# Fill the gaps in a space time raster dataset
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 300"
r.mapcalc --o expr="prec_3 = 500"

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
prec_1|2001-01-01|2001-02-01
prec_2|2001-03-01|2001-04-01
prec_3|2001-05-01|2001-06-01
EOF

t.create --v --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"
t.register --v type=rast input=precip_abs file="${n1}"

# @test
t.rast.gapfill input=precip_abs base="prec" nprocs=2
t.info precip_abs

t.info type=rast input=prec_6
t.info type=rast input=prec_7

# @postprocess

t.unregister --v type=rast maps=prec_1,prec_2,prec_3,prec_6,prec_7
t.remove --v type=strds input=precip_abs
g.remove rast=prec_1,prec_2,prec_3,prec_6,prec_7
