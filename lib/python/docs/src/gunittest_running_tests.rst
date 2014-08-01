Running the tests of GRASS GIS
==============================

This is an advanced guide to running tests of GRASS GIS using GRASS
testing framework (`gunittest`).

Example Bash script to run be used as a cron job
------------------------------------------------

.. code-block:: bash

    #!/bin/bash

    set -e  # fail fast

    REPORTS=".../testreports"
    GRASSSRC=".../grass-src"
    # here we suppose default compilation settings of GRASS and no make install
    GRASSBIN="$GRASSSRC/bin.../grass71"
    GRASSDIST="$GRASSSRC/dist..."
    
    # necessary hardcoded GRASS paths
    GRASSDIST_PYTHON="$GRASSDIST/etc/python"
    GRASS_MULTI_RUNNER="$GRASSSRC/lib/python/gunittest/multirunner.py"
    GRASS_MULTI_REPORTER="$GRASSSRC/lib/python/gunittest/multireport.py"

    DATE_FLAGS="--utc +%Y-%m-%d-%H-%M"
    NOW=$(date $DATE_FLAGS)

    # contains last executed command stdout and stderr
    # here were rely on reports being absolute
    OUTPUT_LOGFILE="$REPORTS/output-$NOW.txt"

    # these are relative to REPORTS
    CURRENT_REPORT_BASENAME="reports_for_date-"
    FINAL_REPORT_DIR="summary_report"
    CURRENT_REPORTS_DIR="$CURRENT_REPORT_BASENAME$NOW"
    LOGFILE="$REPORTS/runs.log"

    GRASSDATA="/grassdata/tests-grassdata"

    echo "Nightly GRASS GIS test started: $NOW" >> $LOGFILE

    # compile current source code from scratch
    cd $GRASSSRC
    make distclean -j4
    svn up
    ./configure ...  # or a script containing all the flags
    make -j4

    # run tests for the current source code
    cd $REPORTS
    mkdir $CURRENT_REPORTS_DIR
    cd $CURRENT_REPORTS_DIR
    python $GRASS_MULTI_RUNNER \
        --grassbin $GRASSBIN \
        --grasssrc $GRASSSRC \
        --grassdata $GRASSDATA \
        --location nc_spm_08_grass7 --location-type nc \
        --location other_location --location-type other_type

    # create overall report of all so far executed tests
    # the script depends on GRASS but just Python part is enough
    export PYTHONPATH="$GRASSDIST_PYTHON:$PYTHONPATH"
    python $GRASS_MULTI_REPORTER --output $FINAL_REPORT_DIR \
        $CURRENT_REPORT_BASENAME*/*

    # although we cannot be sure the tests were executed was successfully
    # so publish or archive results
    rsync -rtvu --delete $REPORTS/ "/var/www/html/grassgistestreports"

    echo "Nightly ($NOW) GRASS GIS test finished: $(date $DATE_FLAGS)" >> $LOGFILE

A script similar to this one can be used as a cron job, on most Linux systems
using ``crontab -e`` and adding the following line::

    0 4 * * 1 /home/vpetras/grasstests/test_grass_gis.sh

Which will perform the tests every Monday at 4 in the morning.
