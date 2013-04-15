#!/bin/sh
# This is a test to export space time raster datasets as VTK time series data

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 res=0.5 -p

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"
r.mapcalc --o expr="elevation = sin(row() + col()) * 10"

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
prec_1|2001-01-01|2001-02-01
prec_2|2001-02-01|2001-03-01
prec_3|2001-03-01|2001-04-01
prec_4|2001-05-01|2001-06-01
prec_5|2001-07-01|2001-08-01
prec_6|2001-08-01|2001-09-01
EOF

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs2 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs3 title="A test with input files" descr="A test with input files"

# The first @test
mkdir /tmp/test1
t.register -i --o type=rast input=precip_abs1 file="${n1}"
t.rast.out.vtk input=precip_abs1 expdir=/tmp/test1 
ls -al /tmp/test1 

mkdir /tmp/test2
t.register -i --o type=rast input=precip_abs2 file="${n1}" 
t.rast.out.vtk input=precip_abs2 expdir=/tmp/test2 elevation=elevation
ls -al /tmp/test2 

mkdir /tmp/test3
t.register -i --o type=rast input=precip_abs3 file="${n1}"
t.rast.out.vtk -g input=precip_abs3 expdir=/tmp/test3 elevation=elevation
ls -al /tmp/test3 

rm -rf /tmp/test1
rm -rf /tmp/test2
rm -rf /tmp/test3

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1,precip_abs2,precip_abs3
