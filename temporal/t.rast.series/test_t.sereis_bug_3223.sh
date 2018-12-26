#/bin/bash
# Test bug #3223
export GRASS_OVERWRITE=1
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc expression="prec_1 = 100" --o
r.mapcalc expression="prec_2 = 200" --o
r.mapcalc expression="prec_3 = 300" --o
r.mapcalc expression="prec_4 = 400" --o
r.mapcalc expression="prec_5 = 500" --o
r.mapcalc expression="prec_6 = 600" --o

t.create type=strds temporaltype=absolute \
    output=precip_abs title="Example" \
    descr="Example" --o

t.register -i type=raster input=precip_abs \
    maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01" increment="1 months" --o

t.rast.series input=precip_abs output=precip_abs_average

r.info precip_abs_average

t.rast.series input=precip_abs output=precip_abs_average_3months where="start_time >= '2001-04-01'"

r.info precip_abs_average_3months

