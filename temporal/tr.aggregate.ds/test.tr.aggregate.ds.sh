# Test the extraction of a subset of a space time raster input

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster inputs
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 150"
r.mapcalc --o expr="prec_3 = 250"
r.mapcalc --o expr="prec_4 = 250"
r.mapcalc --o expr="prec_5 = 150"
r.mapcalc --o expr="prec_6 = 100"

v.random --o -z output=soil_1 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=soil_2 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=soil_3 n=20 zmin=0 zmax=100 column=height

n1=`g.tempfile pid=1 -d` 

cat > $n1 << EOF
soil_1|2001-01-01|2001-04-01
soil_2|2001-05-01|2001-07-01
soil_3|2001-08-01|2001-12-01
EOF

t.create --o type=stvds temporaltype=absolute output=soil_abs1 title="A test" descr="A test"
tv.register input=soil_abs1 file=$n1 start=file end=file

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
tr.register -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-03-01 00:00:00" increment="1 months"

# The @test

tr.aggregate.ds --o input=precip_abs1 output=precip_abs2 base=prec_sum type=stvds dataset=soil_abs1 method=sum
t.info type=strds input=precip_abs2
tr.list input=precip_abs2 method=deltagap

# @postprocess
t.remove type=vect input=soil_1,soil_2,soil_3
t.remove type=stvds input=soil_abs1

t.remove type=rast input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1,precip_abs2
