#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

SAMPLE_DATA_URL=${SAMPLE_DATA_URL:-"https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2beta1.tar.gz"}

grass --tmp-project XY --exec \
    g.download.project url=$SAMPLE_DATA_URL path=$HOME

grass --tmp-project XY --exec \
    python3 -m grass.gunittest.main \
    --grassdata $HOME --location nc_spm_full_v2beta1 --location-type nc \
    --min-success 100 $@
