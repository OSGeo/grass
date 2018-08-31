#!/usr/bin/env bash

# this is testtest for #2309
# Add a select all flag to v.edit
# https://trac.osgeo.org/grass/ticket/2309

set -e
set -x

v.in.ascii input=- output=vedit_test separator=comma <<EOF
913240.0,250614.0
913250.0,250628.0
EOF
v.edit -r map=vedit_test tool=catadd layer=2 cats=10

expected="10
10"
out=$(v.category option=print layer=2 input=vedit_test)

if [[ ${out} != "${expected}" ]]; then
    echo "FAIL: Expected '${expected}' not equals to output '${out}'"
    exit 1
fi

# test also the original categories
expected="1
2"
out=$(v.category option=print layer=1 input=vedit_test)

if [[ ${out} != "${expected}" ]]; then
    echo "FAIL: Expected '${expected}' not equals to output '${out}'"
    exit 1
fi

g.remove -f type=vector name=vedit_test
