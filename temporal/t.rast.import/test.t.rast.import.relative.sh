#!/bin/sh
# Test the import of space time raster datasets with relative time

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

mkdir test

r.mapcalc expr="prec_1 = rand(0, 550)"
r.mapcalc expr="prec_2 = rand(0, 450)"
r.mapcalc expr="prec_3 = rand(0, 320)"
r.mapcalc expr="prec_4 = rand(0, 510)"
r.mapcalc expr="prec_5 = rand(0, 300)"
r.mapcalc expr="prec_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
prec_1|1|2
prec_2|2|3
prec_3|3|4
prec_4|4|5
prec_5|5|6
prec_6|6|7
EOF

eval `g.gisenv`

t.create type=strds temporaltype=relative output=precip_rel \
    title="A test with input files" descr="A test with input files"

# The first @test
t.register -i type=rast input=precip_rel file="${n1}"  unit="years" 

t.rast.export input=precip_rel output=strds_export.tar.bz2 compression=bzip2 format=GTiff workdir=test
t.rast.export input=precip_rel output=strds_export.tar.gz compression=gzip format=GTiff workdir=test
t.rast.export input=precip_rel output=strds_export.tar compression=no format=GTiff workdir=test
t.rast.export input=precip_rel output=strds_export_pack.tar compression=no format=pack workdir=test
t.rast.export input=precip_rel output=strds_export_pack.tar.gz compression=gzip format=pack workdir=test
t.rast.export input=precip_rel output=strds_export_pack.tar.bz2 compression=bzip2 format=pack workdir=test

# Checking different flags
t.rast.import --o input=strds_export.tar.bz2 output=precip_rel extrdir=test\
          -oe title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_rel extrdir=test\
          -loe title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_rel extrdir=test\
          -l  title="A test" description="Description of a test"

# Import using different compression and formats
t.rast.import --o input=strds_export.tar.gz output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export.tar output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar.gz output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar.bz2 output=precip_rel extrdir=test\
              title="A test" description="Description of a test"
r.info prec_1

# Cleaning up
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_rel
g.remove rast=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
rm -rf test
rm strds_export.tar.bz2
rm strds_export.tar.gz
rm strds_export.tar
rm strds_export_pack.tar  
rm strds_export_pack.tar.gz
rm strds_export_pack.tar.bz2
