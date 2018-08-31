#!/usr/bin/env bash

# this is test for #2618
# v.category option=transfer does not copy multiple cat values
# https://trac.osgeo.org/grass/ticket/2618

set -e
set -x

echo "100|100|1" | v.in.ascii output=vcategory_test_1 input=-
fid=$(v.edit --quiet layer=1 map=vcategory_test_1 tool=select cat=1)
v.edit map=vcategory_test_1 layer=1 tool=catadd ids=${fid} cats=2
v.category option=transfer layer=1,2 input=vcategory_test_1 output=vcategory_test_2

expected="1/2"
out=$(v.category option=print layer=1 input=vcategory_test_2)

if [[ ${out} != ${expected} ]]; then
    echo "FAIL: Expected '${expected}' not equals to output ${out}"
    exit 1
fi

out=$(v.category option=print layer=2 input=vcategory_test_2)

if [[ ${out} != ${expected} ]]; then
    echo "FAIL: Expected '${expected}' not equals to output ${out}"
    exit 1
fi

g.remove -f type=vector name=vcategory_test_1,vcategory_test_2
