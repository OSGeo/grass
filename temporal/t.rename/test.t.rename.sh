#!/bin/sh
# Tests the rename module of space time datasets 

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"

# We need to create three space time dataset
t.create --v --o type=strds temporaltype=absolute output=precip_abs1 \
	title="Test" descr="This is the 1 test strds" semantictype=sum
t.register -i --o input=precip_abs1 maps=prec_1,prec_2 \
	start="2001-01-01" increment="1 seconds"

t.create --v --o type=strds temporaltype=absolute output=precip_abs2 \
	title="Test" descr="This is the 2 test strds" semantictype=sum
t.register --o input=precip_abs2 maps=prec_1,prec_2

t.create --v --o type=strds temporaltype=absolute output=precip_abs3 \
	title="Test" descr="This is the 3 test strds" semantictype=sum
t.register --o input=precip_abs3 maps=prec_1,prec_2


t.info precip_abs1
t.info precip_abs2
t.info precip_abs3

# @test Rename the space time raster dataset by overwritung an old one
t.rename --o type=strds input=precip_abs1 output=precip_abs2
t.info precip_abs2

t.info type=rast input=prec_1
t.info type=rast input=prec_2

t.rename --o type=strds input=precip_abs2 output=precip_abs4
t.info precip_abs4

t.info type=rast input=prec_1
t.info type=rast input=prec_2

# Error checking, new dataset has the wrong mapset
t.rename type=strds input=precip_abs4 output=precip_abs3@BLABLA
# Error checking, no overwrite flag set
t.rename type=strds input=precip_abs4 output=precip_abs3

t.remove --v type=strds input=precip_abs3,precip_abs4
t.unregister type=rast maps=prec_1,prec_2
g.remove rast=prec_1,prec_2
