# This is a test to list raster maps of a space time raster dataset

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r3.mapcalc --o expr="vol_1 = rand(0, 550)"
r3.mapcalc --o expr="vol_2 = rand(0, 450)"
r3.mapcalc --o expr="vol_3 = rand(0, 320)"
r3.mapcalc --o expr="vol_4 = rand(0, 510)"
r3.mapcalc --o expr="vol_5 = rand(0, 300)"
r3.mapcalc --o expr="vol_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`

cat > $n1 << EOF
vol_1
vol_2
vol_3
vol_4
vol_5
vol_6
EOF

cat > $n2 << EOF
vol_1|2001-01-01
vol_2|2001-02-01
vol_3|2001-03-01
vol_4|2001-04-01
vol_5|2001-05-01
vol_6|2001-06-01
EOF

cat > $n3 << EOF
vol_1|2001-01-01|2001-04-01
vol_2|2001-05-01|2001-07-01
vol_3|2001-08-01|2001-10-01
vol_4|2001-11-01|2002-01-01
vol_5|2002-02-01|2002-04-01
vol_6|2002-05-01|2002-07-01
EOF

cat > $n4 << EOF
vol_1|2001-01-01|2001-07-01
vol_2|2001-02-01|2001-04-01
vol_3|2001-03-01|2001-04-01
vol_4|2001-04-01|2001-06-01
vol_5|2001-05-01|2001-06-01
vol_6|2001-06-01|2001-07-01
EOF

cat > $n5 << EOF
vol_1|2001-01-01|2001-03-11
vol_2|2001-02-01|2001-04-01
vol_3|2001-03-01|2001-06-02
vol_4|2001-04-01|2001-04-01
vol_5|2001-05-01|2001-05-01
vol_6|2001-06-01|2001-07-01
EOF

t.create --o type=str3ds temporaltype=absolute output=volume_abs1 title="A test with input files" descr="A test with input files"
t.create --o type=str3ds temporaltype=absolute output=volume_abs2 title="A test with input files" descr="A test with input files"
t.create --o type=str3ds temporaltype=absolute output=volume_abs3 title="A test with input files" descr="A test with input files"
t.create --o type=str3ds temporaltype=absolute output=volume_abs4 title="A test with input files" descr="A test with input files"
t.create --o type=str3ds temporaltype=absolute output=volume_abs5 title="A test with input files" descr="A test with input files"

# The first @test
tr3.register    input=volume_abs1 file=$n1 start="2001-01-01" increment="1 months"
tr3.list    fs=" | " method=comma     input=volume_abs1
tr3.list -h input=volume_abs1
tr3.list -h fs=" | " method=cols      input=volume_abs1
tr3.list -h fs=" | " method=delta     input=volume_abs1
tr3.list -h fs=" | " method=deltagaps input=volume_abs1

tr3.register -i input=volume_abs2 file=$n2 start=file
tr3.list    fs=" | " method=comma     input=volume_abs2
tr3.list -h input=volume_abs2
tr3.list -h fs=" | " method=cols      input=volume_abs2
tr3.list -h fs=" | " method=delta     input=volume_abs2
tr3.list -h fs=" | " method=deltagaps input=volume_abs2

tr3.register -i input=volume_abs3 file=$n3 start=file end=file
tr3.list    fs=" | " method=comma     input=volume_abs3
tr3.list -h fs=" | " method=delta     input=volume_abs3
tr3.list -h fs=" | " method=deltagaps input=volume_abs3

tr3.register -i input=volume_abs4 file=$n4 start=file end=file
tr3.list    fs=" | " method=comma     input=volume_abs4
tr3.list -h fs=" | " method=delta     input=volume_abs4
tr3.list -h fs=" | " method=deltagaps input=volume_abs4

tr3.register -i input=volume_abs5 file=$n5 start=file end=file
tr3.list    fs=" | " method=comma     input=volume_abs5
tr3.list -h input=volume_abs5
tr3.list -h fs=" | " method=cols      input=volume_abs5
tr3.list -h fs=" | " method=delta     input=volume_abs5
tr3.list -h fs=" | " method=deltagaps input=volume_abs5

t.remove type=rast3d input=vol_1,vol_2,vol_3,vol_4,vol_5,vol_6
t.remove type=str3ds input=volume_abs1,volume_abs2,volume_abs3,volume_abs4,volume_abs5
