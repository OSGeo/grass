#!/bin/bash

# Tests for r3.null, Trac ticket #2992 I
# replacing values by NULLs creates too many NULLs

set -e
set -x

# setup

# works in NC SPM

g.region -3 n=220180 s=219900 w=638360 e=638720 t=153 b=106 res=2 res3=2

r3.mapcalc expr="test_1 = float(col() + row() + depth())"

# test

# cols, rows numbered in rast algebra from 1,
# so there should be one cell with value 3

r3.null map=test_1 setnull=3
eval `r3.univar map=test_1 -g | grep null_cells`
reference=1

if (( $null_cells != $reference )) ; then
    >&2 echo "Wrong number of NULL cells ($null_cells != $reference)"
    exit 1
fi

# cleanup

g.remove -f type=rast3 name=test_1
