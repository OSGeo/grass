#!/bin/sh
# Test the temporal and spatial sampling/observation of raster maps by vector point maps
# using relative time
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = 100.0"
r.mapcalc --o expr="prec_2 = 200.0"
r.mapcalc --o expr="prec_3 = 300"
r.mapcalc --o expr="prec_4 = 400"
r.mapcalc --o expr="prec_5 = 500.0"
r.mapcalc --o expr="prec_6 = 600.0"

v.random --o -z output=prec n=5 seed=1

t.create --o type=strds temporaltype=relative output=precip_abs1 title="A test" descr="A test"
t.register -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start=0 increment=1 unit=months

# The @test
t.vect.observe.strds input=prec strds=precip_abs1 output=prec_observer vector=prec_observer column=observation
v.info prec_observer
t.info type=stvds input=prec_observer
t.vect.list input=prec_observer
t.vect.db.select input=prec_observer

# @postprocess
t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1
t.remove type=stvds input=prec_observer
t.unregister type=vector maps=prec_observer:1,prec_observer:2,prec_observer:3,prec_observer:4,prec_observer:5,prec_observer:6

#g.remove -f type=vector name=prec_observer,test_extract
#g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
