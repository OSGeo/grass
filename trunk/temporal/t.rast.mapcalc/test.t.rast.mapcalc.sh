#!/bin/sh
# Test for t.rast.mapcalc 

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create several space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r.mapcalc expr="prec_1 = rand(0, 550)" -s
r.mapcalc expr="prec_2 = rand(0, 450)" -s
r.mapcalc expr="prec_3 = rand(0, 320)" -s
r.mapcalc expr="prec_4 = rand(0, 510)" -s
r.mapcalc expr="prec_5 = rand(0, 300)" -s
r.mapcalc expr="prec_6 = rand(0, 650)" -s

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"
t.register -i type=raster input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 months"
t.register type=raster input=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6

t.info precip_abs1
t.info precip_abs2

# The first @test
t.rast.mapcalc -n inputs=precip_abs1,precip_abs2 output=precip_abs3 \
           expression=" precip_abs1 + precip_abs2" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

t.rast.mapcalc -s inputs=precip_abs1,precip_abs2,precip_abs3 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) / precip_abs2" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

t.rast.mapcalc -s inputs=precip_abs1,precip_abs2 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) * null()" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

t.rast.mapcalc -sn inputs=precip_abs1,precip_abs2 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) * null()" base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs4

# Let the test fail
g.remove -f type=raster name=prec_1
t.rast.mapcalc -sn inputs=precip_abs1,precip_abs2 output=precip_abs4 \
           expression=" (precip_abs1 + precip_abs2) * null()" base=new_prec \
           method=equal nprocs=5

# @postprocess
t.remove -rf type=strds input=precip_abs1,precip_abs2,precip_abs3,precip_abs4
