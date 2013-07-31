#!/bin/sh
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create several space time raster datasets
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

# The first @test
t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register --o -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 seconds"
t.remove -rf type=strds input=precip_abs1
# This will produce an error
t.remove -r type=strds input=precip_abs1
