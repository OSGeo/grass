#!/usr/bin/env bash

# TODO: because of bg process, this does not return non-zero on error
# This will need to be in Python (rather than complicating the Bash)
# or the script will need to produce raster maps (good idea anyway)
# to have some measurable output (these would be best tested from
# Python too).

set -e
set -x

NUM_SEQUNTIALS=10
NUM_PARALLELS=100

# Sequential

for i in `seq 1 $NUM_SEQUNTIALS`
do
    ./script_using_temporary_region.py 100 0
done

# Parallel

for i in `seq 1 $NUM_PARALLELS`
do
    ./script_using_temporary_region.py 100 0 &
done

time wait

# With nesting

for i in `seq 1 $NUM_SEQUNTIALS`
do
    ./script_using_temporary_region.py 100,200 0
done

for i in `seq 1 $NUM_PARALLELS`
do
    ./script_using_temporary_region.py 100,200 0 &
done

time wait
