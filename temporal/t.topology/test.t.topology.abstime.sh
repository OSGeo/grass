#!/bin/sh
# Tests the temporal topology module of space time datasets 

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"
r.mapcalc --o expr="A = 1"
r.mapcalc --o expr="B = 2"
r.mapcalc --o expr="C = 3"
r.mapcalc --o expr="D = 3"

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`
n6=`g.tempfile pid=6 -d`

cat > "${n1}" << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF

cat > "${n2}" << EOF
prec_1|2001-01-01
prec_2|2001-02-01
prec_3|2001-03-01
prec_4|2001-04-01
prec_5|2001-05-01
prec_6|2001-06-01
EOF

cat > "${n3}" << EOF
prec_1|2001-01-01|2001-04-01
prec_2|2001-05-01|2001-10-01
prec_3|2001-08-01|2001-10-01
prec_4|2001-11-01|2002-01-01
prec_5|2001-11-01|2002-04-01
prec_6|2002-05-01|2002-07-01
EOF

cat > "${n4}" << EOF
prec_1|2001-01-01|2001-07-01
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-04-01
prec_4|2001-04-01|2001-06-01
prec_5|2001-05-01|2001-06-01
prec_6|2001-06-01|2001-07-01
EOF

cat > "${n5}" << EOF
prec_1|2001-01-01|2001-03-11
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-06-02
prec_4|2001-04-01|2001-04-01
prec_5|2001-05-01|2001-05-01
prec_6|2001-06-01|2001-07-01
EOF

cat > "${n6}" << EOF
A|1990-06-01|1990-06-05
B|1990-06-05|1990-06-18
C|1990-06-10|1990-06-13
D|1990-06-16|1990-06-20
EOF



# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval
t.create --o type=strds temporaltype=absolute output=precip_abs title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=maps title="A test with maps in input files" descr="A test with maps in input files"

t.register --o type=rast -i input=precip_abs file="${n1}" start="2001-01-01" increment="1 months"
cat "${n1}"
t.topology    input=precip_abs
t.topology -m input=precip_abs

t.register --o type=rast -i input=precip_abs file="${n2}"
cat "${n2}"
t.topology    input=precip_abs
t.topology -m input=precip_abs

t.register --o type=rast -i input=precip_abs file="${n3}"
cat "${n3}"
t.topology    input=precip_abs
t.topology -m input=precip_abs

t.register --o type=rast -i input=precip_abs file="${n4}"
cat "${n4}"
t.topology    input=precip_abs
t.topology -m input=precip_abs

t.register --o type=rast -i input=precip_abs file="${n5}"
cat "${n5}"
t.topology    input=precip_abs
t.topology -m input=precip_abs

t.register --o type=rast -i input=maps file="${n6}"
cat "${n6}"
t.topology    input=maps
t.topology -m input=maps

t.remove type=strds input=precip_abs,maps
t.unregister type=rast file="${n1}"
t.unregister type=rast maps=A,B,C,D
