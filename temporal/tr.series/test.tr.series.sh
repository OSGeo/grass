# This is a test to register and unregister raster maps in
# space time raster dataset.
# The raster maps will be registered in different space time raster
# datasets

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster datasets
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 200"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500"
r.mapcalc --o expr="prec_6 = 600"

# The first @test
# We create the space time raster datasets and register the raster maps with absolute time interval

t.create --v --o type=strds temporaltype=absolute dataset=precip_abs gran="1 months" title="A test" descr="A test"

tr.register --v dataset=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 months"

tr.series --o input=precip_abs method=average output=prec_average where="start_time > '2001-03-01'"
tr.series --o -t input=precip_abs method=maximum output=prec_max sort=start_time
tr.series --o -t input=precip_abs method=sum output=prec_sum

t.remove --v type=raster dataset=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove --v type=strds dataset=precip_abs

r.info prec_average
t.info type=raster dataset=prec_max
t.info type=raster dataset=prec_sum
