#!/usr/bin/env bash

# Test usage of temporary region in Python in parallel and when nested.
# Even with the nesting the parallelization happens only on the first
# level the nested processes don't create only one child.
# Calls a Python script to do the actual job.
# Script can be executed without parameters which is meant primarily for
# automated testing.
# See the command line parameters handling below for further details.

# Undefined variables are errors.
set -u
# More set commands later on.

if [ $# -eq 0 ]
then
    # No arguments supplied, use default which is small enough to run
    # well on small machines and does not overwhelm a normal system.
    MAP_PARALLEL=parallel_tmp_region_parallel
    MAP_NEST=parallel_tmp_region_nest
    NUM_SEQUNTIALS=10
    NUM_PARALLELS=50
    SIMPLE_SIZE=150
    NEST_SIZES=100,200
    NUM_NEST=2
elif [ $# -eq 7 ]
then
    # Allow user to set a large number of processes.
    MAP_PARALLEL=$1
    MAP_NEST=$2
    NUM_SEQUNTIALS=$3
    NUM_PARALLELS=$4
    SIMPLE_SIZE=$5
    NEST_SIZES=$6
    NUM_NEST=$7
else
    >&2 echo "Usage:"
    >&2 echo "  $0"
    >&2 echo "  $0 <par name> <nest name> <nproc seq> <nproc par> <simple size> <nest sizes> <num nest>"
    >&2 echo "Example:"
    >&2 echo "  $0 parallel nest 5 50 250 100,200,300,400 4"
    >&2 echo "Notes:"
    >&2 echo "  Runs are done in these combinations and order:"
    >&2 echo "    Serial and simple (i.e., sequence, not nested)"
    >&2 echo "    Parallel and simple (i.e., not nested)"
    >&2 echo "    Serial and nested"
    >&2 echo "    Parallel and nested"
    >&2 echo "  The <num nest> should be number of elements in <nest sizes>."
    >&2 echo "  Nesting numbering in output raster maps starts with 0."
    >&2 echo "  Return codes are checked only for serial runs."
    >&2 echo "  Raster maps are produced only for parallel runs."
    >&2 echo "Use zero or seven parameters, not $#."
    exit 1
fi

MAP_PARALLEL_PATTERN="${MAP_PARALLEL}_*_size_*_nesting_*"
MAP_NEST_PATTERN="${MAP_NEST}_*_size_*_nesting_*"

# Remove maps at exit.
cleanup () {
    EXIT_CODE=$?
    g.remove type=raster pattern="${MAP_PARALLEL_PATTERN}" -f --quiet
    g.remove type=raster pattern="${MAP_NEST_PATTERN}" -f --quiet
    exit $EXIT_CODE
}

trap cleanup EXIT

SCRIPT="./data/script_using_temporary_region.py"

# First, test if the script is in its place.
# (This needs to be tested explicitly because command_not_found_handle
# implementations may return 0 when command/file is not found.)
command -v "${SCRIPT}"
if [ $? -ne 0 ]
then
    >&2 echo "Script ${SCRIPT} not found"
    exit 1
fi

# Fail fast and show commands
set -e
set -x

# Serial/Sequential

for i in `seq 1 $NUM_SEQUNTIALS`
do
    "${SCRIPT}" "${SIMPLE_SIZE}" 0
done

# Parallel
# Because it is much harder to check return codes of parallel processes,
# we go a step further and generate data and check their presence.

for i in `seq 1 $NUM_PARALLELS`
do
    "${SCRIPT}" "${SIMPLE_SIZE}" 0 "${MAP_PARALLEL}_$i" &
done

wait

EXPECTED=$NUM_PARALLELS
NUM=$(g.list type=raster pattern="${MAP_PARALLEL_PATTERN}" mapset=. | wc -l)

if [ ${NUM} -ne ${EXPECTED} ]
then
    echo "Pure parallel test: Got ${NUM} but expected ${EXPECTED} maps"
    exit 1
fi

# With nesting

for i in `seq 1 $NUM_SEQUNTIALS`
do
    "${SCRIPT}" "${NEST_SIZES}" 0
done

for i in `seq 1 $NUM_PARALLELS`
do
    "${SCRIPT}" "${NEST_SIZES}" 0 "${MAP_NEST}_$i" &
done

wait

EXPECTED=$(( $NUM_PARALLELS * $NUM_NEST ))
NUM=$(g.list type=raster pattern="${MAP_NEST_PATTERN}" mapset=. | wc -l)

if [ ${NUM} -ne ${EXPECTED} ]
then
    echo "Parallel with nesting: Got ${NUM} but expected ${EXPECTED} maps"
    exit 1
fi
