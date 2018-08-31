#!/bin/sh
# Test the extraction of a subset of a space time raster3d input

# We need to set a specific region in the
# @preprocess step of this test.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r3.mapcalc --o expr="prec_1 = rand(0, 550)" -s
r3.mapcalc --o expr="prec_2 = rand(0, 450)" -s
r3.mapcalc --o expr="prec_3 = rand(0, 320)" -s
r3.mapcalc --o expr="prec_4 = rand(0, 510)" -s
r3.mapcalc --o expr="prec_5 = rand(0, 300)" -s
r3.mapcalc --o expr="prec_6 = rand(0, 650)" -s

t.create --o type=str3ds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i type=raster_3d input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 months"

# The @test
t.rast3d.extract --o input=precip_abs1 output=precip_abs2 where="start_time > '2001-06-01'" expression=" if(precip_abs1 > 400, precip_abs1, null())" base=new_prec

t.info type=str3ds input=precip_abs2

t.unregister type=raster_3d maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=str3ds input=precip_abs1,precip_abs2
