#!/usr/bin/env bash

# Test parallel calls of r.import in Bash.
# To test the reprojection, it needs to run in a location other than
# WGS84 (that's the CRS of imported file used) or r.import may (should)
# skip the reprojection part of the code.
# Imported raster map presence or absence based on a search pattern is
# used to evaluate if the expected result is created.

# This test likely overwhelms the system for a while because all the
# processes will start more or less at once, but this is exactly the
# point of the test because we want increase a chance of a potential
# race condition or name conflict.

# Undefined variables are errors.
set -u
# More set commands later on.

if [ $# -eq 0 ]
then
    NUM_SERIALS=3
    NUM_PARALLELS=50
elif [ $# -eq 2 ]
then
    # Allow user to set a large number of processes.
    NUM_SERIALS=$3
    NUM_PARALLELS=$4
else
    >&2 echo "Usage:"
    >&2 echo "  $0"
    >&2 echo "  $0 <nproc serial> <nproc parallel>"
    >&2 echo "Example:"
    >&2 echo "  $0 5 300"
    >&2 echo "Use zero or two parameters, not $#."
    exit 1
fi

# Remove maps at exit.
cleanup () {
    EXIT_CODE=$?
    g.remove type=raster pattern="test_parallel_ser_*" -f --quiet
    g.remove type=raster pattern="test_parallel_par_*" -f --quiet
    exit $EXIT_CODE
}

trap cleanup EXIT

DATA="data/data2.asc"

# Fail fast and show commands
set -e
set -x

# Serial
# The parallel loop version won't fail even if command returns non-zero,
# so we need to check the command ahead of time.
# Since this is useful mostly for making sure the command works for this
# script, so the command should be exactly the same.

for i in `seq 1 $NUM_SERIALS`
do
    r.import input="$DATA" output="test_parallel_ser_$i"
done

# Parallel

for i in `seq 1 $NUM_PARALLELS`
do
    r.import input="$DATA" output="test_parallel_par_$i" &
done

wait

EXPECTED=$NUM_PARALLELS
NUM=$(g.list type=raster pattern='test_parallel_par_*' mapset=. | wc -l)

if [ ${NUM} -ne ${EXPECTED} ]
then
    echo "Parallel test: Got ${NUM} but expected ${EXPECTED} maps"
    exit 1
fi
