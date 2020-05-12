#!/usr/bin/env bash

if [[ $# -eq 0 ]]
then
    # No arguments supplied, use default which is is small enough to run
    # well on small machines and does not overwhelm a normal system.
    NPROC=50
    ROWS=200
    COLS=200
elif [[ $# -eq 3 ]]
then
    # Allow user to set a large number of processes.
    NPROC=$1
    ROWS=$2
    COLS=$3
else
    >&2 echo "Usage:"
    >&2 echo "  $0"
    >&2 echo "  $0 <number of processes> <rows> <cols>"
    >&2 echo "Example which takes a lot of resources:"
    >&2 echo "  $0 1000 500 500"
    >&2 echo "Use zero or three parameters, not $#"
    exit 1
fi

set -e  # fail fast
set -x  # repeat commands

GRASS="grass"
LOCATION="$PWD/test_tmp_mapset_xy"
MAPSET_PATH="${LOCATION}/PERMANENT"

cleanup () {
    rm -r "${LOCATION}"
    exit 1
}

trap cleanup EXIT

# Setup

"${GRASS}" -e -c XY "${LOCATION}"

"${GRASS}" "${MAPSET_PATH}" --exec g.region rows=$ROWS cols=$COLS

# Test using sleep
# This shows that --no-lock works.

PARAM="--no-lock"
# To check that it fails as expected, uncomment the following line.
# PARAM=""

# Sanity check.
"${GRASS}" "${MAPSET_PATH}" --exec sleep 1

# Specialized sanity check.
"${GRASS}" "${MAPSET_PATH}" $PARAM --exec sleep 10 &

for i in `seq 1 ${NPROC}`
do
    "${GRASS}" "${MAPSET_PATH}" $PARAM --exec sleep 10 &
done

wait

# Test with computation
# When this works, there should be no warnings about "No such file..."
# Increasing number of processes and size of the region increases chance
# of warnings and errors, but it is too much to have it in test
# (1000 processes and rows=10000 cols=10000).

PARAM="--no-clean"
# To check that it fails as expected, uncomment the following line.
# PARAM=""

for i in `seq 1 ${NPROC}`
do
    "${GRASS}" "${MAPSET_PATH}" --no-lock $PARAM --exec \
        r.mapcalc "a_$i = sqrt(sin(rand(0, 100)))" -s &
done

wait

"${GRASS}" "${MAPSET_PATH}" --clean-only

# Evaluate the computation
# See how many raster maps are in the mapset

EXPECTED="${NPROC}"
NUM=$("${GRASS}" "${MAPSET_PATH}" --exec g.list type=raster pattern="*" | wc -l)

if [ "$NUM" -ne "$EXPECTED" ]
then
    echo "Got $NUM but expected $EXPECTED maps"
    exit 1
fi
