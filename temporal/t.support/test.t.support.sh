#!/bin/sh
# Tests the support module of space time datasets 

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
# We create several space time raster datasets

# @test Register the maps in two space time datasets
t.create --v --o type=strds temporaltype=absolute output=precip_abs1 title="Test" descr="This is the 1 test strds" semantictype=sum
t.register -i --o input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 seconds"

t.create --v --o type=strds temporaltype=absolute output=precip_abs2 title="Test" descr="This is the 2 test strds" semantictype=sum
t.register -i --o input=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6

t.create --v --o type=strds temporaltype=relative output=precip_rel1 title="Test" descr="This is the 1 test strds" semantictype=min

# Check metadata update
t.info type=strds input=precip_rel1
t.support --v type=strds input=precip_rel1 title="Test support" descr="This is the support test strds" semantictype=max
t.info type=strds input=precip_rel1

# Check metadata update
t.info type=strds input=precip_abs1
t.support --v type=strds input=precip_abs1 title="Test support" descr="This is the support test strds" semantictype=mean
t.info type=strds input=precip_abs1


# @test the map update function
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=20 res3=20
r.mapcalc --o expr="prec_1 = rand(0, 55)"
r.mapcalc --o expr="prec_2 = rand(0, 45)"
r.mapcalc --o expr="prec_3 = rand(0, 32)"
r.mapcalc --o expr="prec_4 = rand(0, 51)"
r.mapcalc --o expr="prec_5 = rand(0, 30)"
r.mapcalc --o expr="prec_6 = rand(0, 65)"

# The map dependent metadata should have been updated
t.support --v -m type=strds input=precip_abs1
t.info type=strds input=precip_abs1
t.support --v -m type=strds input=precip_abs2
t.info type=strds input=precip_abs2

# Remove three maps
g.remove rast=prec_1,prec_2,prec_3

# Booth space time datasets should be updated and 3 maps must have been unregistered
t.support --v -m type=strds input=precip_abs1
t.info type=strds input=precip_abs1
t.info type=strds input=precip_abs2

t.remove --v type=strds input=precip_abs1,precip_rel1
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
g.remove rast=prec_4,prec_5,prec_6