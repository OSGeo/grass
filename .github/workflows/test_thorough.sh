#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

grass79 --tmp-location XY --exec \
    g.extension g.download.location
grass79 --tmp-location XY --exec \
    g.download.location url=http://fatra.cnr.ncsu.edu/data/nc_spm_full_v2alpha2.tar.gz dbase=$HOME
grass79 $HOME/nc_spm_full_v2alpha2/PERMANENT --exec \
    t.upgrade

grass79 --tmp-location XY --exec \
    python3 -m grass.gunittest.main \
    --grassdata $HOME --location nc_spm_full_v2alpha2 --location-type nc \
    --min-success 80
