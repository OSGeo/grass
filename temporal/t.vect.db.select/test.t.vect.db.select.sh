#!/bin/sh
# Test the database output of vector maps in a space time vector dataset
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

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-03-01 00:00:00" increment="1 months"

# The @test
# We need to create a nice space time vector dataset with layers for testing
t.vect.observe.strds input=prec strds=precip_abs1 output=prec_observer vector=prec_observer column=observation
t.vect.list input=prec_observer
t.vect.db.select input=prec_observer
t.vect.db.select input=prec_observer where="cat = 1" separator="  |  "
t.vect.db.select input=prec_observer where="cat = 1" separator="  |  " columns=observation

# Time instances
t.register --o input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2004-01-01 00:00:00" increment="3 months"

t.vect.observe.strds --o input=prec strds=precip_abs1 output=prec_observer vector=prec_observer 
t.vect.list input=prec_observer
t.vect.db.select input=prec_observer
t.vect.db.select input=prec_observer where="cat = 1" separator="  |  "

# @postprocess
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1
t.remove type=stvds input=prec_observer
t.unregister type=vect maps=prec_observer:1,prec_observer:2,prec_observer:3,prec_observer:4,prec_observer:5,prec_observer:6

g.remove vect=prec_observer,test_extract
g.remove rast=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6

