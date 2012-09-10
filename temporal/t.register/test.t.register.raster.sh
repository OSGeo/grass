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
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs3 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs4 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs5 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs6 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs7 title="A test" descr="A test"

t.register --o -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 seconds"
t.info type=strds input=precip_abs1
t.info -g type=strds input=precip_abs1
r.info map=prec_1
t.rast.list input=precip_abs1
t.topology input=precip_abs1

t.register --o -i input=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="20 seconds, 5 minutes"
t.info type=strds input=precip_abs2
t.info -g type=strds input=precip_abs2
r.info map=prec_1
t.rast.list input=precip_abs2
t.topology input=precip_abs2

t.register --o -i input=precip_abs3 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="8 hours"
t.info -g type=strds input=precip_abs3
r.info map=prec_1
t.rast.list input=precip_abs3
t.topology input=precip_abs3

t.register --o input=precip_abs4 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 days"
t.info -g type=strds input=precip_abs4
r.info map=prec_1
t.rast.list input=precip_abs4
t.topology input=precip_abs4

t.register --o input=precip_abs5 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="4 weeks"
t.info -g type=strds input=precip_abs5
r.info map=prec_1
t.rast.list input=precip_abs5
t.topology input=precip_abs5

t.register --o input=precip_abs6 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-08-01" increment="2 months"
t.info -g type=strds input=precip_abs6
r.info map=prec_1
t.rast.list input=precip_abs6
t.topology input=precip_abs6

t.register --o input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
t.info -g type=strds input=precip_abs7
r.info map=prec_1
t.rast.list input=precip_abs7
t.topology input=precip_abs7
# Register with different valid time again
t.register --o input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info -g type=strds input=precip_abs7
r.info map=prec_1
t.rast.list input=precip_abs7
t.topology input=precip_abs7
# Register with different valid time again creating an interval
t.register --o -i input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info -g type=strds input=precip_abs7
r.info map=prec_1
t.rast.list input=precip_abs7
t.topology input=precip_abs7

t.register --o input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" end="2002-01-01"
t.info -g type=strds input=precip_abs7
r.info map=prec_1
t.rast.list input=precip_abs7
t.topology input=precip_abs7

# Check for correct errors
# Increment format error
t.register -i --o input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="months"


t.unregister type=rast maps=prec_1,prec_2,prec_3
# Test the warning message
t.unregister type=rast maps=prec_1,prec_2,prec_3
t.remove type=strds input=precip_abs1,precip_abs2,precip_abs3,precip_abs4,precip_abs5,precip_abs6,precip_abs7
t.unregister type=rast maps=prec_4,prec_5,prec_6
r.info map=prec_1
