#1/bin/sh
# This is a test to sample a space time raster dataset with a space time vector dataset

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

v.random --o -z output=pnts1 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts2 n=20 zmin=0 zmax=100 column=height

v.random --o -z output=pnts3 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts4 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts5 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts6 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts7 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=pnts8 n=20 zmin=0 zmax=100 column=height

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`

cat > "${n1}" << EOF
prec_1
prec_2
prec_3
prec_4
prec_5
prec_6
EOF

cat > "${n2}" << EOF
pnts1|2001-01-01|2001-03-01
pnts2|2001-05-01|2001-07-01
EOF

cat > "${n3}" << EOF
pnts3|2001-01-01|2001-01-05
pnts4|2001-01-05|2001-01-10
pnts5|2001-01-10|2001-01-15
pnts6|2001-01-15|2001-01-20
pnts7|2001-01-20|2001-01-25
pnts8|2001-01-25|2001-01-30
EOF


t.create --o type=strds temporaltype=absolute output=precip_abs0 \
	title="A test with raster input files" descr="A test with raster input files"
t.create --o type=stvds temporaltype=absolute output=pnts_abs0 \
	title="A test with vector input files" descr="A test with vector input files"
t.create --o type=stvds temporaltype=absolute output=pnts_abs1 \
	title="A test with vector input files" descr="A test with vector input files"

t.register --o type=rast -i input=precip_abs0 file="${n1}" start="2001-01-01" increment="1 months"
t.rast.list precip_abs0 -h
t.register --o type=vect    input=pnts_abs0 file="${n2}"
t.vect.list pnts_abs0 -h
t.register --o type=vect    input=pnts_abs1 file="${n3}"
t.vect.list pnts_abs1 -h

# The @test
t.sample method=start    input=precip_abs0,precip_abs0,precip_abs0,precip_abs0 samtype=stvds sample=pnts_abs0 
t.sample method=equal    input=precip_abs0,precip_abs0,precip_abs0,precip_abs0 samtype=stvds sample=pnts_abs0
t.sample method=contain  input=precip_abs0,precip_abs0,precip_abs0 samtype=stvds sample=pnts_abs0 -c
t.sample method=overlap  input=precip_abs0,precip_abs0 samtype=stvds sample=pnts_abs0 -cs
t.sample method=during   input=precip_abs0 samtype=stvds sample=pnts_abs0 -c
t.sample method=precedes input=precip_abs0 samtype=stvds sample=pnts_abs0 -c
t.sample method=follows  input=precip_abs0 samtype=stvds sample=pnts_abs0 -c
t.sample method=precedes,follows input=precip_abs0 samtype=stvds sample=pnts_abs0 -c
t.sample input=precip_abs0 samtype=strds sample=precip_abs0 -cs


# Test with temporal point data
t.register --o type=rast input=precip_abs0 file="${n1}" start="2001-01-01" increment="1 months"
t.rast.list precip_abs0 -h
t.sample input=precip_abs0 samtype=stvds sample=pnts_abs0 -cs
t.sample input=precip_abs0 samtype=stvds sample=pnts_abs1 -cs

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.unregister type=vect maps=pnts1,pnts2,pnts3,pnts4,pnts5,pnts6,pnts7,pnts8
t.remove type=strds input=precip_abs0
t.remove type=stvds input=pnts_abs0,pnts_abs1
g.remove rast=prec_1,prec_2,prec_3,,prec_4,prec_5,prec_6
g.remove vect=pnts1,pnts2,pnts3,pnts4,pnts5,pnts6,pnts7,pnts8
