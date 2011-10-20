# This is a test to list raster maps of a space time raster dataset

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`

cat > $n1 << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF

cat > $n2 << EOF
prec_1|2001-01-01
prec_2|2001-02-01
prec_3|2001-03-01
prec_4|2001-04-01
prec_5|2001-05-01
prec_6|2001-06-01
EOF

cat > $n3 << EOF
prec_1|2001-01-01|2001-04-01
prec_2|2001-05-01|2001-07-01
prec_3|2001-08-01|2001-10-01
prec_4|2001-11-01|2002-01-01
prec_5|2002-02-01|2002-04-01
prec_6|2002-05-01|2002-07-01
EOF

cat > $n4 << EOF
prec_1|2001-01-01|2001-07-01
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-04-01
prec_4|2001-04-01|2001-06-01
prec_5|2001-05-01|2001-06-01
prec_6|2001-06-01|2001-07-01
EOF

cat > $n5 << EOF
prec_1|2001-01-01|2001-03-11
prec_2|2001-02-01|2001-04-01
prec_3|2001-03-01|2001-06-02
prec_4|2001-04-01|2001-04-01
prec_5|2001-05-01|2001-05-01
prec_6|2001-06-01|2001-07-01
EOF

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs2 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs3 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs4 title="A test with input files" descr="A test with input files"
t.create --o type=strds temporaltype=absolute output=precip_abs5 title="A test with input files" descr="A test with input files"

# The first @test
tr.register    input=precip_abs1 file=$n1 start="2001-01-01" increment="1 months"
tr.list    fs=" | " method=comma     input=precip_abs1
tr.list -h input=precip_abs1
tr.list -h fs=" | " method=cols      input=precip_abs1
tr.list -h fs=" | " method=delta     input=precip_abs1
tr.list -h fs=" | " method=deltagaps input=precip_abs1

tr.register -i input=precip_abs2 file=$n2 start=file
tr.list    fs=" | " method=comma     input=precip_abs2
tr.list -h input=precip_abs2
tr.list -h fs=" | " method=cols      input=precip_abs2
tr.list -h fs=" | " method=delta     input=precip_abs2
tr.list -h fs=" | " method=deltagaps input=precip_abs2

tr.register -i input=precip_abs3 file=$n3 start=file end=file
tr.list    fs=" | " method=comma     input=precip_abs3
tr.list -h fs=" | " method=delta     input=precip_abs3
tr.list -h fs=" | " method=deltagaps input=precip_abs3

tr.register -i input=precip_abs4 file=$n4 start=file end=file
tr.list    fs=" | " method=comma     input=precip_abs4
tr.list -h fs=" | " method=delta     input=precip_abs4
tr.list -h fs=" | " method=deltagaps input=precip_abs4

tr.register -i input=precip_abs5 file=$n5 start=file end=file
tr.list    fs=" | " method=comma     input=precip_abs5
tr.list -h input=precip_abs5
tr.list -h fs=" | " method=cols      input=precip_abs5
tr.list -h fs=" | " method=delta     input=precip_abs5
tr.list -h fs=" | " method=deltagaps input=precip_abs5

t.remove type=rast input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1,precip_abs2,precip_abs3,precip_abs4,precip_abs5
