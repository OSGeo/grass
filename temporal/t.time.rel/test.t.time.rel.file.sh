# This is a test to absolute time for raster maps
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster inputs
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d` # Only map names
n2=`g.tempfile pid=2 -d` # Map names and start time
n3=`g.tempfile pid=3 -d` # Map names start time and increment

cat > $n1 << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF
cat $n1

cat > $n2 << EOF
prec_1|1
prec_2|2
prec_3|3
prec_4|4
prec_5|5
prec_6|6
EOF
cat $n2

cat > $n3 << EOF
prec_1|1|4
prec_2|4|7
prec_3|7|10
prec_4|10|11
prec_5|11|14
prec_6|14|17
EOF
cat $n3

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval
t.create --o type=strds temporaltype=relative output=precip_rel title="A test with input files" descr="A test with input files"

# Test with input files
# File 1
t.time.rel -i file=$n1 start=20 increment=5

tr.register input=precip_rel file=$n1
t.info type=strds input=precip_rel
tr.list input=precip_rel

# File 1
t.time.rel file=$n1 start=20
t.info type=strds input=precip_rel
tr.list input=precip_rel
# File 2
t.time.rel file=$n2 start=file
t.info type=strds input=precip_rel
tr.list input=precip_rel
# File 2
t.time.rel -i file=$n2 start=file increment=5
t.info type=strds input=precip_rel
tr.list input=precip_rel
# File 3
t.time.rel file=$n3 start=file end=file
t.info type=strds input=precip_rel
tr.list input=precip_rel

t.remove --v type=strds input=precip_rel
t.remove --v type=rast file=$n1
