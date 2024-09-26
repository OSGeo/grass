#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

# Test execution of binary command
grass --tmp-project EPSG:4326 --exec g.region res=0.1 -p
# Test if python modules without extension are found
grass --tmp-project EPSG:4326 --exec which t.create
# Test if python modules can be called
grass --tmp-project EPSG:4326 --exec t.create --help
