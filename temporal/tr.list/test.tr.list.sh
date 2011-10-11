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
t.create --o type=strds temporaltype=absolute dataset=precip_abs1 gran="1 senconds" title="A test" descr="A test"

tr.register -i dataset=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="2 months"
t.info type=strds dataset=precip_abs1

# The first @test
tr.list -c input=precip_abs1

t.remove type=strds dataset=precip_abs1
t.remove type=raster dataset=prec_1,prec_2,prec_3
t.remove type=raster dataset=prec_4,prec_5,prec_6
