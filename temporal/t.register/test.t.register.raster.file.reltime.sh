#!/bin/sh
# This is a test to register and unregister raster maps in
# space time raster input.
# The raster maps will be registered in different space time raster
# inputs

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster inputs
# with relative
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
prec_1|1
prec_2|2
prec_3|3
prec_4|4
prec_5|5
prec_6|6
EOF
cat "${n2}"

cat > "${n3}" << EOF
prec_1|1|4
prec_2|4|7
prec_3|7|10
prec_4|10|11
prec_5|11|14
prec_6|14|17
EOF
cat "${n3}"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval
t.create --o type=strds temporaltype=relative output=precip_abs8 title="A test with input files" descr="A test with input files"

# Test with input files
# File 1, and 3 without a space time raster dataset
t.register --o -i file="${n1}" start=0 increment=7 unit=days
t.unregister --v type=raster file="${n1}"
t.register --o file="${n2}" unit=minutes
t.unregister --v type=raster file="${n1}"
t.register --o -i file="${n3}" unit=seconds
t.unregister --v type=raster file="${n1}"
# File 1
t.register --o -i input=precip_abs8 file="${n1}" start=0 increment=7 unit=days
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 1
t.unregister --v type=raster file="${n1}"
t.register --o input=precip_abs8 file="${n1}" start=20 unit=years
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2
t.unregister --v type=raster file="${n1}"
t.register --o input=precip_abs8 file="${n2}" unit=minutes
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2 ERROR ERROR -- Increment computation needs to be fixed
t.unregister --v type=raster file="${n1}"
t.register --o input=precip_abs8 file="${n2}" increment=14 unit=days
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2 ERROR ERROR -- Increment computation needs to be fixed
t.unregister --v type=raster file="${n1}"
t.register --o -i input=precip_abs8 file="${n2}" increment=14 unit=days
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8

# File 3
t.unregister --v type=raster file="${n1}"
t.register --o -i input=precip_abs8 file="${n3}" unit=seconds
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8

t.unregister --v type=raster file="${n1}"

# @test of correct @failure handling
t.register --o -i input=precip_abs8 maps=preac_1,prec_2 file="${n3}" # Maps and file set
t.register --o -i input=precip_abs8 file="${n3}" # No relative unit set

t.remove --v type=strds input=precip_abs8
