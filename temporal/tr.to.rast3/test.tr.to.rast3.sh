# This is a test to register and unregister raster maps in
# space time raster input.
# The raster maps will be registered in different space time raster
# inputs

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster inputs
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=1 res=10 res3=1 -p3

r.mapcalc --o expr="prec_1 = 100"
r.mapcalc --o expr="prec_2 = 200"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500"
r.mapcalc --o expr="prec_6 = 600"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval

t.create --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"

tr.register -i input=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 months"

tr.to.rast3 --o input=precip_abs output=prec_sum

t.remove type=rast input=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs

r3.info prec_sum
g.region s=0 n=80 w=0 e=120 b=0 t=1 res=10 res3=1
