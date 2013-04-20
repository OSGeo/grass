#!/bin/sh
# Space time raster dataset temporal shifting
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=absolute output=precip_abs title="A test" descr="A test"
t.register -i --o type=rast input=precip_abs maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01 12:00:00" increment="14 days"

t.rast.list input=precip_abs

# The first @test
t.shift --o input=precip_abs granularity="3 years"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
t.shift --o input=precip_abs granularity="12 months"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
t.shift --o input=precip_abs granularity="2 days"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
t.shift --o input=precip_abs granularity="3 hours"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
t.shift --o input=precip_abs granularity="30 minutes"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
t.shift --o input=precip_abs granularity="5 seconds"
t.info type=strds input=precip_abs
t.rast.list input=precip_abs
# This should give an error because of the monthly increment 
# that will result in wrong number of days in the next month
t.shift --o input=precip_abs granularity="1 month"

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs
