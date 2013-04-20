#!/bin/sh
# Space time raster dataset temporal snapping with absolute time
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
prec_1|2001-01-01|2001-07-01
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-04-01
prec_4|2001-04-01|2001-06-01
prec_5|2001-05-01|2001-06-01
prec_6|2001-06-01|2001-07-01
EOF

t.create --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"
t.register --o type=rast input=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01 12:00:00" increment="14 days"

# The first @test
t.info type=strds input=precip_abs
t.rast.list input=precip_abs

t.snap --o input=precip_abs 

t.info type=strds input=precip_abs
t.rast.list input=precip_abs

# The second @test
# Creating a valid topology
t.register --o type=rast input=precip_abs file=${n1}

t.rast.list input=precip_abs
t.topology input=precip_abs

t.snap --o input=precip_abs 

t.rast.list input=precip_abs
t.topology input=precip_abs

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs
