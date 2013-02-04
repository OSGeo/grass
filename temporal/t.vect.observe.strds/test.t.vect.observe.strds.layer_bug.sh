#!/bin/sh
# Here we test the limit of the number of layers
# @preprocess 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

MAP_LIST="map_list.txt"
rm ${MAP_LIST}

count=1
while [ $count -lt 500 ]; do
    name="test_prec_${count}"
    r.mapcalc --o expr="${name} = ${count}"
    echo ${name} >> ${MAP_LIST}
    count=$((count + 1))
done

v.random --o output=prec n=5 seed=1

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i input=precip_abs1  file=${MAP_LIST} start="2001-01-01 00:00:00" increment="1 hours"

# The @test
t.vect.observe.strds input=prec strds=precip_abs1 output=prec_observer vector=prec_observer column="test_val"
v.info prec_observer
t.info type=stvds input=prec_observer
t.vect.list input=prec_observer
t.vect.db.select input=prec_observer

# @postprocess
t.unregister type=rast file=${MAP_LIST}
t.remove type=strds input=precip_abs1
t.remove type=stvds input=prec_observer

g.remove vect=prec_observer
g.mremove -f rast=test_prec_*
