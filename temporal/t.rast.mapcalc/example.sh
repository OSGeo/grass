#!/bin/sh
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Lets create the raster maps
r.mapcalc --o expr="a3 = 3"
r.mapcalc --o expr="a4 = 4"
r.mapcalc --o expr="a5 = 5"
r.mapcalc --o expr="a6 = 6"
r.mapcalc --o expr="a7 = 7"
r.mapcalc --o expr="a8 = 8"

r.mapcalc --o expr="b1 = 1"
r.mapcalc --o expr="b2 = 2"
r.mapcalc --o expr="b3 = 3"
r.mapcalc --o expr="b4 = 4"
r.mapcalc --o expr="b5 = 5"
r.mapcalc --o expr="b6 = 6"
r.mapcalc --o expr="b7 = 7"
r.mapcalc --o expr="b8 = 8"
r.mapcalc --o expr="b9 = 8"
r.mapcalc --o expr="b10 = 10"
r.mapcalc --o expr="b11 = 11"
r.mapcalc --o expr="b12 = 12"

t.create --o type=strds temporaltype=absolute \
    output=A title="Dataset A" descr="Dataset A"
t.create --o type=strds temporaltype=absolute \
    output=B title="Dataset B" descr="Dataset B"
t.register --o -i type=rast input=A maps=a3,a4,a5,a6,a7,a8 \
    start="2001-03-01" increment="1 months"
t.register --o -i type=rast input=B maps=b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12 \
    start="2001-01-01" increment="1 months"

t.rast.mapcalc --o --v input=A,B output=C base=c method=equal \
    expr="if(start_month() == 5 || start_month() == 6, (A + B), (A * B))"

t.info type=strds input=C
t.rast.list -h input=C columns=name,start_time,min,max

