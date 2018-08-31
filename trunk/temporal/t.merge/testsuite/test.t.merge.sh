#!/bin/sh
# Tests the merging of space time datasets 

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r.mapcalc --o expr="prec_1 = rand(10, 150)" -s
r.mapcalc --o expr="prec_2 = rand(20, 250)" -s
r.mapcalc --o expr="prec_3 = rand(30, 350)" -s
r.mapcalc --o expr="prec_4 = rand(40, 450)" -s
r.mapcalc --o expr="prec_5 = rand(50, 550)" -s
r.mapcalc --o expr="prec_6 = rand(60, 650)" -s

# Register maps in temporal database
t.register -i --o maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01" increment="1 month"

# We need to create three space time dataset and register the maps
# in several space time datasets
t.create --o type=strds temporaltype=absolute output=precip_abs1 \
	title="Test" descr="This is the 1 test strds" semantictype=sum
t.register --o input=precip_abs1 maps=prec_1,prec_2,prec_3 

t.create --o type=strds temporaltype=absolute output=precip_abs2 \
	title="Test" descr="This is the 2 test strds" semantictype=sum
t.register --o input=precip_abs2 maps=prec_3,prec_4,prec_5

t.create --o type=strds temporaltype=absolute output=precip_abs3 \
	title="Test" descr="This is the 3 test strds" semantictype=sum
t.register --o input=precip_abs3 maps=prec_4,prec_5,prec_6


# @test to merge two and three space time datasets
t.merge inputs=precip_abs1 output=precip_abs4
t.info precip_abs4
t.rast.list precip_abs4

t.unregister type=raster maps=prec_1,prec_2,prec_3 input=precip_abs4
t.merge --o inputs=precip_abs1,precip_abs2,precip_abs3 output=precip_abs4
t.info precip_abs4
t.rast.list precip_abs4

t.merge --o inputs=precip_abs1,precip_abs2,precip_abs3,precip_abs4 output=precip_abs4
t.info precip_abs4
t.rast.list precip_abs4

t.remove type=strds input=precip_abs1,precip_abs2,precip_abs3,precip_abs4
t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
