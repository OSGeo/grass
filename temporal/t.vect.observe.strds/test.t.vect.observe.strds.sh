#!/bin/sh
# Test the temporal and spatial sampling/observation of raster maps by vector point maps
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

r.mapcalc  expr="prec_1 = 100.0"
r.mapcalc  expr="prec_2 = 200.0"
r.mapcalc  expr="prec_3 = 300"
r.mapcalc  expr="prec_4 = 400"
r.mapcalc  expr="prec_5 = 500.0"
r.mapcalc  expr="prec_6 = 600.0"

v.random output=prec n=5 seed=1
v.random -z output=test_1 column=test n=5 seed=1 

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-03-01 00:00:00" increment="1 months"

# The @test
t.vect.observe.strds input=prec strds=precip_abs1 output=prec_observer vector=prec_observer column="test_val"
v.info prec_observer
t.info type=stvds input=prec_observer
t.vect.list input=prec_observer
t.vect.db.select input=prec_observer

t.vect.observe.strds input=test_1 strds=precip_abs1 output=test_1_observer  vector=test_1_observer
v.info test_1_observer
t.info type=stvds input=test_1_observer
t.vect.list input=test_1_observer
t.vect.db.select input=test_1_observer


# @postprocess
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1
t.remove type=stvds input=prec_observer,test_1_observer
t.unregister type=vect maps=prec_observer:1,prec_observer:2,prec_observer:3,prec_observer:4,prec_observer:5,prec_observer:6
t.unregister type=vect maps=test_1_observer:1,test_1_observer:2,test_1_observer:3,test_1_observer:4,test_1_observer:5,test_1_observer:6

g.remove vect=prec_observer,test_1_observer
g.remove rast=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
