# This is a test to absolute time for vector maps with layer support
# We need to set a specific region in the
# @preprocess step of this test. We generate
# vector with v.random and create several space time vector inputs
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z output=lidar_abs_1 n=5 zmin=0 zmax=100 

n1=`g.tempfile pid=4 -d` # Only map names and layer
n2=`g.tempfile pid=5 -d` # Map names, layer and start time 
n3=`g.tempfile pid=6 -d` # Map names, layer start time and increment
v.random --o -z seed=1 output=lidar_abs_orig n=20 zmin=0 zmax=100 column=sand
# Adding new layer with categories
v.category input=lidar_abs_orig out=lidar_abs option=transfer layer=1,1 --o
v.category input=lidar_abs out=lidar_abs_orig option=transfer layer=1,2 --o
v.category input=lidar_abs_orig out=lidar_abs option=transfer layer=1,3 --o
v.category input=lidar_abs out=lidar_abs_orig option=transfer layer=1,4 --o
v.category input=lidar_abs_orig out=lidar_abs option=transfer layer=1,5 --o
v.category input=lidar_abs out=lidar_abs_orig option=transfer layer=1,6 --o
g.copy --o vect=lidar_abs_orig,lidar_abs_1

cat > "${n1}" << EOF
lidar_abs_1|1
lidar_abs_1|2
lidar_abs_1|3
lidar_abs_1|4
lidar_abs_1|5
lidar_abs_1|6
EOF
cat "${n1}"

cat > "${n2}" << EOF
lidar_abs_1|1|2001-01-01
lidar_abs_1|2|2001-02-01
lidar_abs_1|3|2001-03-01
lidar_abs_1|4|2001-04-01
lidar_abs_1|5|2001-05-01
lidar_abs_1|6|2001-06-01
EOF
cat "${n2}"

cat > "${n3}" << EOF
lidar_abs_1|1|2001-01-01|2001-04-01
lidar_abs_1|2|2001-04-01|2001-07-01
lidar_abs_1|3|2001-07-01|2001-10-01
lidar_abs_1|4|2001-10-01|2002-01-01
lidar_abs_1|5|2002-01-01|2002-04-01
lidar_abs_1|6|2002-04-01|2002-07-01
EOF
cat "${n3}"

# The first @test
# Test with input files
# File 1
t.time.abs --v type=vect file="${n1}" start="2001-01-01" increment="1 months" layer=file
t.list type=vect columns=id,name,start_time,end_time where='name = "lidar_abs_1"'
# File 1
t.time.abs --v type=vect file="${n1}" start="2001-01-01"  layer=file
t.list type=vect columns=id,name,start_time,end_time where='name = "lidar_abs_1"'
# File 2
t.time.abs --v type=vect file="${n2}" start=file  layer=file
t.list type=vect columns=id,name,start_time,end_time where='name = "lidar_abs_1"'
# File 2
t.time.abs --v type=vect -i file="${n2}" start=file increment="1 months"  layer=file
t.list type=vect columns=id,name,start_time,end_time where='name = "lidar_abs_1"'
# File 3
t.time.abs --v type=vect file="${n3}" start=file end=file  layer=file
t.list type=vect columns=id,name,start_time,end_time where='name = "lidar_abs_1"'

t.remove --v type=vect input=lidar_abs_1:1,lidar_abs_1:2,lidar_abs_1:3,lidar_abs_1:4,lidar_abs_1:5,lidar_abs_1:6
g.remove vect=lidar_abs_1,lidar_abs_orig
