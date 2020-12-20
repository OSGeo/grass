#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

grass79 --tmp-project XY --exec \
    g.extension g.download.project
grass79 --tmp-project XY --exec \
    g.download.project url=http://fatra.cnr.ncsu.edu/data/nc_spm_full_v2alpha2.tar.gz dbase=$HOME

grass79 --tmp-project XY --exec \
    python3 -m grass.gunittest.main \
        --grassdata $HOME --project nc_spm_full_v2alpha2 --project-type nc \
        --min-success 80
