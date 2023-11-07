#!/bin/sh
# Test the import of space time raster datasets

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

mkdir test

# Generate data
r.mapcalc expr="prec_1 = rand(0, 550)" -s
r.mapcalc expr="prec_2 = rand(0, 450)" -s
r.mapcalc expr="prec_3 = rand(0, 320)" -s
r.mapcalc expr="prec_4 = rand(0, 510)" -s
r.mapcalc expr="prec_5 = rand(0, 300)" -s
r.mapcalc expr="prec_6 = rand(0, 650)" -s

n1=`g.tempfile pid=1 -d`

cat > "${n1}" << EOF
prec_1|2001-01-01|2001-07-01
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-04-01
prec_4|2001-04-01|2001-06-01
prec_5|2001-05-01|2001-06-01
prec_6|2001-06-01|2001-07-01
EOF

eval `g.gisenv`

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test with input files" descr="A test with input files"

# The first @test
t.register -i type=raster input=precip_abs1 file="${n1}"

t.rast.export input=precip_abs1 output=strds_export.tar.bz2 compression=bzip2 format=AAIGrid directory=test
t.rast.export input=precip_abs1 output=strds_export.tar.gz compression=gzip format=GTiff directory=test
t.rast.export input=precip_abs1 output=strds_export.tar compression=no format=GTiff directory=test
t.rast.export input=precip_abs1 output=strds_export_pack.tar compression=no format=pack directory=test
t.rast.export input=precip_abs1 output=strds_export_pack.tar.gz compression=gzip format=pack directory=test
t.rast.export input=precip_abs1 output=strds_export_pack.tar.bz2 compression=bzip2 format=pack directory=test

# Checking different flags
t.rast.import --o input=strds_export.tar.bz2 output=precip_abs1 directory=test\
          -oe title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_abs1 directory=test\
          -loe title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
t.rast.import --o input=strds_export.tar.bz2 output=precip_abs1 directory=test\
          -l  title="A test" description="Description of a test"

# Import using different compression and formats
t.rast.import --o input=strds_export.tar.gz output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export.tar output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar.gz output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
r.info prec_1
t.rast.import --o input=strds_export_pack.tar.bz2 output=precip_abs1 directory=test\
              title="A test" description="Description of a test"
r.info prec_1

# Cleaning up
t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1
g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
rm -rf test
rm strds_export.tar.bz2
rm strds_export.tar.gz
rm strds_export.tar
rm strds_export_pack.tar
rm strds_export_pack.tar.gz
rm strds_export_pack.tar.bz2
