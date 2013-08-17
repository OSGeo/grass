#!/bin/sh
# Here we test the limit of the number of layers
# @preprocess 
# The region setting should work for UTM and LL test locations

# temporary disabled test for performance reason
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

MAP_LIST="map_list.txt"
rm ${MAP_LIST}

NUM_MAPS=50000

count=1
while [ $count -lt ${NUM_MAPS} ]; do
    name="test_bench_${count}"
    r.mapcalc --quiet expr="${name} = ${count}"
    echo ${name} >> ${MAP_LIST}
    count=$((count + 1))
done

t.create type=strds temporaltype=absolute output=benchmark1 title="Benchmark1" descr="Benchmark1 dataset"
t.create type=strds temporaltype=absolute output=benchmark2 title="Benchmark2" descr="Benchmark2 dataset"

echo "### Register maps"
time t.register -i input=benchmark1  file=${MAP_LIST} start="2001-01-01 00:00:00" increment="1 hours"
time t.register -i input=benchmark2  file=${MAP_LIST} start="2001-01-01 00:00:00" increment="1 hours"

echo "### List maps"
time t.rast.list input=benchmark1 column=name,start_time > "/dev/null"
time t.rast.list input=benchmark2 column=name,start_time > "/dev/null"

echo "### Remove STRDS and maps"
time t.remove -rf type=strds input=benchmark1
echo "### Remove STRDS"
time t.remove type=strds input=benchmark2
