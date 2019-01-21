#!/bin/bash
############################################################################
#
# MODULE:       r.drain
# AUTHOR(S):    Markus Neteler, Sören Gebbert, Vaclav Petras
# PURPOSE:      Test GRASS GIS using the test framework
#               Documentation:
#                 https://trac.osgeo.org/grass/wiki/GSoC/2014/TestingFrameworkForGRASS
#                 https://grass.osgeo.org/grass76/manuals/libpython/gunittest_running_tests.html#example-bash-script-to-run-be-used-as-a-cron-job
#
#               Data:
#                 Since we use the full NC dataset (nc_spm_08_grass7.tar.gz) here, we need to generate
#                 some simplified names as used in NC basic for some test cases. This happens automatically below.
#
# COPYRIGHT:    (C) 2019 by Markus Neteler, and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
############################################################################

### CONFIGURATION


source test_framework_GRASS_GIS_with_NC.conf

######### nothing to change below

set -e  # fail fast

# here we suppose default compilation settings of GRASS GIS and no make install
GRASSBIN="$GRASSSRC/bin.${ARCH}/${GRASSBIN}"
GRASSDIST="$GRASSSRC/dist.${ARCH}"

# necessary hardcoded GRASS paths
GRASSDIST_PYTHON="$GRASSDIST/etc/python"
GRASS_MULTI_RUNNER="$GRASSSRC/lib/python/gunittest/multirunner.py"
GRASS_MULTI_REPORTER="$GRASSSRC/lib/python/gunittest/multireport.py"

DATE_FLAGS="--utc +%Y-%m-%d-%H-%M"
NOW=$(date $DATE_FLAGS)

# get number of processors of current machine
MYNPROC=`getconf _NPROCESSORS_ONLN`
# leave one PROC free for other tasks
GCCTHREADS=`expr $MYNPROC - 1`

# contains last executed command stdout and stderr
# here were rely on reports being absolute
OUTPUT_LOGFILE="$REPORTS/output-$NOW.txt"

# these are relative to REPORTS
CURRENT_REPORT_BASENAME="reports_for_date-"
FINAL_REPORT_DIR="summary_report"
CURRENT_REPORTS_DIR="$CURRENT_REPORT_BASENAME$NOW"
LOGFILE="$REPORTS/runs.log"

mkdir -p $REPORTS/$CURRENT_REPORTS_DIR
mkdir -p $GRASSDATA

# fetch sample data
SAMPLEDATA=nc_spm_08_grass7.tar.gz
(cd $GRASSDATA ; wget -c https://grass.osgeo.org/sampledata/north_carolina/$SAMPLEDATA ; tar xfz $SAMPLEDATA )

echo "Nightly GRASS GIS test started: $NOW" >> ${LOGFILE}

# Preparation: Since we use the full NC dataset, we need to generate some simplified names as used in NC basic for some test cases
echo "
g.copy raster=basin_50K,basin
g.copy raster=boundary_county_500m,boundary
g.copy raster=landcover_1m,landcover
g.copy raster=geology_30m,geology
g.copy raster=landuse96_28m,landuse
g.copy raster=soilsID,soils
g.copy vector=zipcodes_wake,zipcodes
g.copy vector=schools_wake,schools
" > $GRASSDATA/tmp_rename.sh
$GRASSBIN $GRASSDATA/nc_spm_08_grass7/PERMANENT --exec sh $GRASSDATA/tmp_rename.sh
rm -f $GRASSDATA/tmp_rename.sh

if [ "$COMPILE" = "yes" ] ; then
   ## compile current source code from scratch
   cd $GRASSSRC
   make distclean -j$GCCTHREADS
   svn update
   ./$CONFIGURE ...  # configure meta script containing all the compiler flags
   make -j$GCCTHREADS
fi

# run tests for the current source code
cd $REPORTS/$CURRENT_REPORTS_DIR
$PYTHON $GRASS_MULTI_RUNNER \
    --grassbin $GRASSBIN \
    --grasssrc $GRASSSRC \
    --grassdata $GRASSDATA \
    --location nc_spm_08_grass7 --location-type nc \
    --location other_location --location-type other_type

# create overall report of all so far executed tests
# the script depends on GRASS but just Python part is enough
export PYTHONPATH="$GRASSDIST_PYTHON:$PYTHONPATH"
$PYTHON $GRASS_MULTI_REPORTER --output $FINAL_REPORT_DIR \
    $CURRENT_REPORT_BASENAME*/*

# publish on Web site
if [ "$PUBLISH" = "yes" ] ; then
   ## although we cannot be sure the tests were executed was successfully
   ## so publish or archive results
   rsync -rtvu --delete $REPORTS/ $SERVERDIR
fi

echo "Nightly ($NOW) GRASS GIS test finished: $(date $DATE_FLAGS)" >> ${LOGFILE}

exit 0

