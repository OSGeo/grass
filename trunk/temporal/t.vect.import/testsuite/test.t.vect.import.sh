#!/bin/sh
# Test the import of space time vector datasets

# We need to set a specific region in the
# @preprocess step of this test.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

mkdir test

v.random --o -z output=soil_1 n=100 zmin=0 zmax=100 column=height seed=1
v.random --o -z output=soil_2 n=100 zmin=0 zmax=100 column=height seed=2
v.random --o -z output=soil_3 n=100 zmin=0 zmax=100 column=height seed=3

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soil_1
soil_2
soil_3
EOF

t.create --o type=stvds temporaltype=absolute output=soil_abs1 title="A test" descr="A test"
t.register -i type=vector input=soil_abs1 file="${n1}" start='2001-01-01' increment="1 months"

# The first @test
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar.bz2 compression=bzip2 directory=/tmp
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar.gz compression=gzip directory=/tmp
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar compression=no directory=/tmp

t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar.bz2 compression=bzip2 directory=/tmp
t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar.gz compression=gzip directory=/tmp
t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar compression=no directory=/tmp

# Checking different flags
t.vect.import --o input=stvds_export_gml.tar.bz2 output=precip_abs1 directory=test\
          -oe title="A test" description="Description of a test"
t.vect.import --o input=stvds_export_gml.tar.bz2 output=precip_abs1 directory=test\
          -o title="A test" description="Description of a test"
t.vect.import --o input=stvds_export_gml.tar.bz2 output=precip_abs1 directory=test\
              title="A test" description="Description of a test"

# Import using different compression and formats
t.vect.import --o input=stvds_export_gml.tar.gz output=soil_abs2 directory=test\
              title="A test" description="Description of a test"
v.info soil_1
t.vect.import --o input=stvds_export_gml.tar output=soil_abs2 directory=test\
              title="A test" description="Description of a test"
v.info soil_1
t.vect.import --o input=stvds_export_pack.tar output=soil_abs2 directory=test\
              title="A test" description="Description of a test"
v.info soil_1
t.vect.import --o input=stvds_export_pack.tar.gz output=soil_abs2 directory=test\
              title="A test" description="Description of a test"
v.info soil_1
t.vect.import --o input=stvds_export_pack.tar.bz2 output=soil_abs2 directory=test\
              title="A test" description="Description of a test"
v.info soil_1

# Cleaning up
rm -rf test
g.remove -f type=vector name=soil_1,soil_2,soil_3
t.unregister type=vector file="${n1}"
t.remove type=stvds input=soil_abs1,soil_abs2
rm stvds_export_gml.tar.bz2
rm stvds_export_gml.tar.gz
rm stvds_export_gml.tar
rm stvds_export_pack.tar.bz2
rm stvds_export_pack.tar.gz
rm stvds_export_pack.tar

