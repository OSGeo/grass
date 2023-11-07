#!/bin/sh
# Test for t.rast.mapcalc
export GRASS_OVERWRITE=1
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create several space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc -s expr="prec_1 = rand(0, 550)"
r.mapcalc -s expr="prec_2 = rand(0, 450)"
r.mapcalc -s expr="prec_3 = rand(0, 320)"
r.mapcalc -s expr="prec_4 = rand(0, 510)"
r.mapcalc -s expr="prec_5 = rand(0, 300)"
r.mapcalc -s expr="prec_6 = rand(0, 650)"

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"
t.register -i type=raster input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 months"
t.register type=raster input=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6

t.info precip_abs1
t.info precip_abs2

# The first @test
t.rast.mapcalc --v -sn inputs=precip_abs1,precip_abs2 output=precip_abs3 \
    expression="if(start_time() >= 0 && end_time() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) " base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

t.rast.mapcalc --v -sn inputs=precip_abs1,precip_abs2 output=precip_abs3 \
    expression="if(start_year()>=0&&start_month()>=0&&start_day()>=0&&start_hour()>=0&&start_minute()>=0&&start_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) " base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

t.rast.mapcalc --v -sn inputs=precip_abs1,precip_abs2 output=precip_abs3 \
    expression="if(end_year()>=0&&end_month()>=0&&end_day()>=0&&end_hour()>=0&&end_minute()>=0&&end_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) " base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

# The first @test
t.rast.mapcalc --v -sn inputs=precip_abs1,precip_abs2 output=precip_abs3 \
    expression="if(start_doy() >= 0 && start_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) " base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

# The first @test
t.rast.mapcalc --v -sn inputs=precip_abs1,precip_abs2 output=precip_abs3 \
    expression="if(end_doy() >= 0 && end_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) " base=new_prec \
           method=equal nprocs=5
t.info type=strds input=precip_abs3

t.rast.mapcalc --v -sn inputs=precip_abs1 output=precip_abs3 \
    expression="if(precip_abs1 == prec_1, prec_1, null())" base=new_prec


# @postprocess
t.remove -fr type=strds input=precip_abs1,precip_abs2,precip_abs3
