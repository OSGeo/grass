#!/bin/sh
# This is a test to register and unregister raster maps in
# space time raster input.
# The raster maps will be registered in different space time raster
# inputs

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster inputs
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)" -s
r.mapcalc --o expr="prec_2 = rand(0, 450)" -s
r.mapcalc --o expr="prec_3 = rand(0, 320)" -s
r.mapcalc --o expr="prec_4 = rand(0, 510)" -s
r.mapcalc --o expr="prec_5 = rand(0, 300)" -s
r.mapcalc --o expr="prec_6 = rand(0, 650)" -s

n1=`g.tempfile pid=1 -d` # Only map names
n2=`g.tempfile pid=2 -d` # Map names and start time
n3=`g.tempfile pid=3 -d` # Map names start time and increment

cat > "${n1}" << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF
cat "${n1}"

cat > "${n2}" << EOF
prec_1|2001-01-01
prec_2|2001-02-01
prec_3|2001-03-01
prec_4|2001-04-01
prec_5|2001-05-01
prec_6|2001-06-01
EOF
cat "${n2}"

cat > "${n3}" << EOF
prec_1|2001-01-01|2001-04-01
prec_2|2001-04-01|2001-07-01
prec_3|2001-07-01|2001-10-01
prec_4|2001-10-01|2002-01-01
prec_5|2002-01-01|2002-04-01
prec_6|2002-04-01|2002-07-01
EOF
cat "${n3}"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval
t.create --o type=strds temporaltype=absolute output=precip_abs8 title="A test with input files" descr="A test with input files"

# Test with input files
# File 1
t.register --o -i input=precip_abs8 file="${n1}" start="2001-01-01" increment="1 months"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 1
t.register --o input=precip_abs8 file="${n1}" start="2001-01-01" 
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2
t.register --o input=precip_abs8 file="${n2}"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2
t.register --o input=precip_abs8 file="${n2}" increment="1 months"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 3
t.register --o -i input=precip_abs8 file="${n3}"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8

t.remove --v type=strds input=precip_abs8
t.unregister --v type=raster file="${n1}"
