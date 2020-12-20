#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

grass79 --tmp-project EPSG:4326 --exec g.region res=0.1 -p
