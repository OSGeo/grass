#!/bin/sh
# This is a test to register and unregister raster maps in
# space time raster datasets.
# The raster maps will be registered in different space time raster
# datasets.

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

r.timestamp map=prec_1 date="1 jan 1995 / 23 sep 1995"
r.timestamp map=prec_2 date="14 jul 1996 / 15 jul 1996"
r.timestamp map=prec_3 date="1 mar 1998"
r.timestamp map=prec_4 date="17 mar 1950 / 28 apr 1960"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"

t.register --o -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4
t.info type=strds input=precip_abs1
t.info -g type=strds input=precip_abs1
r.info map=prec_1
t.rast.list input=precip_abs1
t.topology input=precip_abs1

# Test the warning message
t.remove -rf type=strds input=precip_abs1
