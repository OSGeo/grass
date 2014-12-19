#!/bin/sh
# Univariate statitsics for space time raster3d datasets

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

# Generate data
r3.mapcalc expr="prec_1 = rand(0, 550)" -s
r3.mapcalc expr="prec_2 = rand(0, 450)" -s
r3.mapcalc expr="prec_3 = rand(0, 320)" -s
r3.mapcalc expr="prec_4 = rand(0, 510)" -s
r3.mapcalc expr="prec_5 = rand(0, 300)" -s
r3.mapcalc expr="prec_6 = rand(0, 650)" -s

t.create type=str3ds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register type=raster3d --v -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-15 12:05:45" increment="14 days"
t.info type=str3ds input=precip_abs1

# The first @test
t.rast3d.univar -e input=precip_abs1 

t.rast3d.univar -s input=precip_abs1 

t.remove -rf type=str3ds input=precip_abs1

