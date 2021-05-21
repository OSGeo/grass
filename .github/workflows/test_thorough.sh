#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

grass79 --tmp-location XY --exec \
    g.extension g.download.location
grass79 --tmp-location XY --exec \
    g.download.location url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz dbase=$HOME

grass79 --tmp-location XY --exec \
    python3 -m grass.gunittest.main \
    --grassdata $HOME --location nc_spm_full_v2alpha2 --location-type nc \
    --min-success 100
