#!/bin/sh
# Test the extraction of a subset of a space time vector input with
# time stamped layer

# We need to set a specific region in the
# @preprocess step of this test.
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
t.vect.observe.strds input=prec strds=precip_abs1 output=prec_observer vector=prec_observer column=prec
v.info prec_observer
t.info type=stvds input=prec_observer
t.vect.list input=prec_observer

t.vect.extract --o input=prec_observer output=test_extract_1 base=test_1
t.info type=stvds input=test_extract_1
t.vect.univar input=test_extract_1 column=prec 
t.vect.extract --o input=prec_observer expr="prec > 400" output=test_extract_2 base=test_2
t.info type=stvds input=test_extract_2
t.vect.univar input=test_extract_2 column=prec 
t.vect.extract --o input=prec_observer where="start_time >= '2001-05-01'" expr="prec > 400" output=test_extract_3 base=test_3
t.info type=stvds input=test_extract_3
t.vect.univar input=test_extract_3 column=prec 



# @postprocess
t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1
t.remove type=stvds input=prec_observer,test_extract_1,test_extract_2,test_extract_3
t.unregister type=vector maps=prec_observer:1,prec_observer:2,prec_observer:3,prec_observer:4,prec_observer:5,prec_observer:6

g.remove -f type=vector name=prec_observer
g.remove -f type=vector pattern=test_*
g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
