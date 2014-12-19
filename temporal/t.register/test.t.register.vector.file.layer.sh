#!/bin/sh
# This is a test to register and unregister vector maps with layer support in
# space time vector input.

# We need to set a specific region in the
# @preprocess step of this test. We generate
# vector with v.random and create several space time vector inputs
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z output=lidar_abs_1 n=5 zmin=0 zmax=100 

n1=`g.tempfile pid=4 -d` # Only map names and layer
n2=`g.tempfile pid=5 -d` # Map names, layer and start time 
n3=`g.tempfile pid=6 -d` # Map names, layer, start time and increment
# The vector map
v.random --o -z seed=1 output=lidar_abs_orig n=100 zmin=0 zmax=100 column=sand
# Adding new layer with categories
v.category input=lidar_abs_orig out=lidar_abs_1 option=transfer layer=1,2,3,4,5,6 --o

cat > "${n1}" << EOF
lidar_abs_1:1
lidar_abs_1:2
lidar_abs_1:3
lidar_abs_1:4
lidar_abs_1:5
lidar_abs_1:6
EOF
cat "${n1}"

cat > "${n2}" << EOF
lidar_abs_1:1|2001-01-01
lidar_abs_1:2|2001-02-01
lidar_abs_1:3|2001-03-01
lidar_abs_1:4|2001-04-01
lidar_abs_1:5|2001-05-01
lidar_abs_1:6|2001-06-01
EOF
cat "${n2}"

cat > "${n3}" << EOF
lidar_abs_1:1|2001-01-01|2001-04-01
lidar_abs_1:2|2001-04-01|2001-07-01
lidar_abs_1:3|2001-07-01|2001-10-01
lidar_abs_1:4|2001-10-01|2002-01-01
lidar_abs_1:5|2002-01-01|2002-04-01
lidar_abs_1:6|2002-04-01|2002-07-01
EOF
cat "${n3}"

t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds1 title="A test" descr="A test"

# The first @test
# Test with input files
# File 1
t.register --o --v type=vector input=lidar_abs_ds1 file="${n1}" start="2001-01-01" increment="1 months"
t.list type=vector columns=id,name,start_time,end_time where="name='lidar_abs_1'"
# File 1
t.register --o --v type=vector input=lidar_abs_ds1 file="${n1}" start="2001-01-01"
t.list type=vector columns=id,name,start_time,end_time where="name='lidar_abs_1'"
# File 2
t.register --o --v type=vector input=lidar_abs_ds1 file="${n2}" 
t.list type=vector columns=id,name,start_time,end_time where="name='lidar_abs_1'"
# File 2
t.register --o --v type=vector input=lidar_abs_ds1 -i file="${n2}" start=file increment="1 months"
t.list type=vector columns=id,name,start_time,end_time where="name='lidar_abs_1'"
# File 3
t.register --o --v type=vector input=lidar_abs_ds1 file="${n3}" start=file 
t.list type=vector columns=id,name,start_time,end_time where="name='lidar_abs_1'"

t.unregister --v type=vector maps=lidar_abs_1:1,lidar_abs_1:2,lidar_abs_1:3,lidar_abs_1:4,lidar_abs_1:5,lidar_abs_1:6
t.remove type=stvds input=lidar_abs_ds1
g.remove -f type=vector name=lidar_abs_1,lidar_abs_orig
