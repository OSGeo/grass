#!/bin/sh
# Here we test the limit of the number of layers
# @preprocess 
# The region setting should work for UTM and LL test locations

# temporary disabled test for performance reason
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

MAP_LIST="map_list.txt"
rm ${MAP_LIST}

NUM_MAPS=10000

echo "### Generate raster maps"

count=1
while [ $count -lt ${NUM_MAPS} ]; do
    name="test_bench_${count}"
    r.mapcalc --quiet expr="${name} = ${count}"
    echo ${name} >> ${MAP_LIST}
    count=$((count + 1))
done

echo "### Create space time datasets"

time t.create type=strds temporaltype=absolute output=bench1 title="Bench1" descr="Bench1"
time t.create type=strds temporaltype=absolute output=bench2 title="Bench2" descr="Bench2"
time t.create type=strds temporaltype=absolute output=bench3 title="Bench3" descr="Bench3"
time t.create type=strds temporaltype=absolute output=bench4 title="Bench4" descr="Bench4"

echo "### Register maps"
time t.register -i input=bench1  file=${MAP_LIST} start="2001-01-01 00:00:00" increment="1 day"
echo "### Register maps again"
time t.register input=bench2  file=${MAP_LIST}
echo "### Register maps again"
time t.register input=bench3  file=${MAP_LIST}
echo "### Register maps again"
time t.register input=bench4  file=${MAP_LIST}

echo "### List maps"
time t.rast.list input=bench1 column=name,start_time > "/dev/null"
time t.rast.list input=bench2 column=name,start_time > "/dev/null"
time t.rast.list input=bench3 column=name,start_time,end_time \
    where="start_time > '2001-01-01'" > "/dev/null"
time t.rast.list input=bench4 column=name,start_time,end_time,min,max \
    where="start_time > '2001-01-01'" > "/dev/null"

echo "### STRDS Infos"
t.info bench1
t.info bench2
t.info bench3
t.info bench4

echo "### Remove STRDS and maps"
time t.remove type=strds input=bench1
time t.remove type=strds input=bench2
time t.remove type=strds input=bench3
time t.remove -rf type=strds input=bench4

