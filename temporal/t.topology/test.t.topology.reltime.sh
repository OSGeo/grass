#!/bin/sh
# Tests the temporal topology module of space time datasets 

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)" -s
r.mapcalc --o expr="prec_2 = rand(0, 450)" -s
r.mapcalc --o expr="prec_3 = rand(0, 320)" -s
r.mapcalc --o expr="prec_4 = rand(0, 510)" -s
r.mapcalc --o expr="prec_5 = rand(0, 300)" -s
r.mapcalc --o expr="prec_6 = rand(0, 650)" -s

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`

cat > "${n1}" << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF

cat > "${n2}" << EOF
prec_1|1
prec_2|2
prec_3|3
prec_4|4
prec_5|5
prec_6|6
EOF

cat > "${n3}" << EOF
prec_1|1|4
prec_2|5|7
prec_3|8|10
prec_4|11|12
prec_5|13|14
prec_6|15|25
EOF

cat > "${n4}" << EOF
prec_1|2001|2007
prec_2|2002|2004
prec_3|2003|2004
prec_4|2004|2006
prec_5|2005|2006
prec_6|2006|2007
EOF

cat > "${n5}" << EOF
prec_1|2001|2003
prec_2|2002|2004
prec_3|2003|2006
prec_4|2004|2004
prec_5|2005|2005
prec_6|2006|2007
EOF


# The first @test
# We create the space time raster inputs and register the raster maps with relolute time interval
t.create --o type=strds temporaltype=relative output=precip_rel_d title="A test with input files of unti days" descr="A test with input files"
t.create --o type=strds temporaltype=relative output=precip_rel_y title="A test with input files of unit years" descr="A test with input files"

t.register --o type=raster -i --v input=precip_rel_d file="${n1}" start=0 increment=1 unit=days
cat "${n1}"
t.topology    input=precip_rel_d
t.topology -m input=precip_rel_d

t.register --o type=raster -i input=precip_rel_d file="${n2}" unit=days
cat "${n2}"
t.topology    input=precip_rel_d
t.topology -m input=precip_rel_d

t.register --o type=raster -i input=precip_rel_d file="${n3}" unit=days
cat "${n3}"
t.topology    input=precip_rel_d
t.topology -m input=precip_rel_d

t.register --o type=raster -i input=precip_rel_y file="${n4}" unit=years
cat "${n4}"
t.topology    input=precip_rel_y
t.topology -m input=precip_rel_y

t.register --o type=raster -i input=precip_rel_y file="${n5}" unit=years
cat "${n5}"
t.topology    input=precip_rel_y
t.topology -m input=precip_rel_y

t.remove type=strds input=precip_rel_d
t.remove type=strds input=precip_rel_y
t.unregister type=raster file="${n1}"
