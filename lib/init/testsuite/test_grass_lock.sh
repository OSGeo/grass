#!/usr/bin/env bash

set -e
set -x

GRASS="grass"
LOCATION="test_tmp_mapset_xy"
MAPSET_PATH="${LOCATION}/PERMANENT"

cleanup () {
    rm -r "${LOCATION}"
}

trap cleanup EXIT

# Setup

"${GRASS}" -e -c XY "${LOCATION}"

"${GRASS}" "${MAPSET_PATH}" --exec g.region rows=200 cols=200

# Test using sleep
# This shows that --no-lock works.

"${GRASS}" "${MAPSET_PATH}" --no-lock --exec sleep 10 &

for i in `seq 1 5`
do
    "${GRASS}" "${MAPSET_PATH}" --no-lock --exec sleep 10 &
done

wait

# Test with computation
# When this works, there should be no warnings about "No such file..."
# Increasing number of processes and size of the region increases chance
# of warnings and errors, but it is too much to have it in test
# (1000 processes and rows=10000 cols=10000).

for i in `seq 1 50`
do
    "${GRASS}" "${MAPSET_PATH}" --no-lock --no-clean --exec \
        r.mapcalc "a_$i = sqrt(sin(rand(0, 100)))" -s &
done

wait

# Evaluate the computation
# See how many raster maps are in the mapset

EXPECTED=50
NUM=$("${GRASS}" "${MAPSET_PATH}" --exec g.list type=raster pattern="*" | wc -l)

if [ "$NUM" -ne "$EXPECTED" ]
then
    echo "Got $NUM but expected $EXPECTED maps"
    exit 1
fi
