#!/bin/sh
# Test for t.rast.mapcalc2

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create several space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc expr="a1 = rand(0, 550)"
r.mapcalc expr="a2 = rand(0, 450)"
r.mapcalc expr="a3 = rand(0, 320)"
r.mapcalc expr="a4 = rand(0, 510)"
r.mapcalc expr="a5 = rand(0, 300)"
r.mapcalc expr="a6 = rand(0, 650)"

t.create type=strds temporaltype=absolute output=A1 title="A test" descr="A test"
t.create type=strds temporaltype=absolute output=A2 title="A test" descr="A test"
t.register -i type=rast input=A1 maps=a1,a2,a3,a4,a5,a6 start="2001-01-01" increment="3 months"
t.register type=rast input=A2 maps=a1,a2,a3,a4,a5,a6

t.info A1
t.info A2

# The first @test
t.rast.mapcalc2 --v expression="B = A1 + A2" base=b nprocs=5
t.info type=strds input=B

t.rast.mapcalc2 --v expression="B = if(start_year()>=0&&start_month()>=0&&start_day()>=0&&start_hour()>=0&&start_minute()>=0&&start_second()>=0, A1 + A2) " base=b nprocs=5
t.info type=strds input=B

t.rast.mapcalc2 --v expression="B = if(end_year()>=0&&end_month()>=0&&end_day()>=0&&end_hour()>=0&&end_minute()>=0&&end_second()>=0, A1 + A2) " base=b nprocs=5
t.info type=strds input=B

t.rast.mapcalc2 --v expression="B = if(start_doy() >= 0 && start_dow() >= 0, A1 + A2) " base=b nprocs=5
t.info type=strds input=B

t.rast.mapcalc2 --v expression="B = if(end_doy() >= 0 && end_dow() >= 0, A1 + A2) " base=b nprocs=5
t.info type=strds input=B

t.rast.mapcalc2 --v expression="B = A1[-1] + A2[1] " base=b nprocs=5
t.info type=strds input=B

# @postprocess
t.remove -rf type=strds input=A1,A2,B
