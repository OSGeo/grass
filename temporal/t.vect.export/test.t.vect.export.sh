#!/bin/sh
# Export of space time vector datasets

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z output=soil_1 n=100 zmin=0 zmax=100 column=height  seed=1
v.random --o -z output=soil_2 n=100 zmin=0 zmax=100 column=height seed=2
v.random --o -z output=soil_3 n=100 zmin=0 zmax=100 column=height seed=3

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soil_1
soil_2
soil_3
EOF

t.create --o type=stvds temporaltype=absolute output=soil_abs1 title="A test" descr="A test"
t.register -i type=vect input=soil_abs1 file="${n1}" start='2001-01-01' increment="1 months"

# The first @test
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar.bz2 compression=bzip2 workdir=/tmp
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar.gz compression=gzip workdir=/tmp
t.vect.export format=GML input=soil_abs1 output=stvds_export_gml.tar compression=no workdir=/tmp

t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar.bz2 compression=bzip2 workdir=/tmp
t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar.gz compression=gzip workdir=/tmp
t.vect.export format=pack input=soil_abs1 output=stvds_export_pack.tar compression=no workdir=/tmp

t.unregister type=vect file="${n1}"
t.remove type=stvds input=soil_abs1
rm stvds_export_gml.tar.bz2
rm stvds_export_gml.tar.gz
rm stvds_export_gml.tar
rm stvds_export_pack.tar.bz2
rm stvds_export_pack.tar.gz
rm stvds_export_pack.tar
