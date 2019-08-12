Running the test framework of GRASS GIS
=======================================

This is an advanced guide to running tests of GRASS GIS using GRASS
testing framework (`gunittest`). For introduction to this topic,
go to :ref:`test-general`.


.. _running-tests-report:

Running tests and creating report
---------------------------------

To test before commit, run all tests using testing framework.
First start GRASS GIS session and go to the root directory of your
GRASS GIS source code copy::

    cd my/grass/source/code/root

Then execute::

    python -m grass.gunittest.main --location locname --location-type nc

where ``locname`` is a name of location in current GRASS GIS data(base) directory
(GISDBASE) and ``nc`` is a location specified by individual test files
(the later is not yet fully implemented, so just put there ``nc`` every time).

``grass.gunittest.main`` writes a text summary to standard output and
it creates an HTML report from all tests in all ``testsuite`` directories inside
the directory tree. The report is placed in ``testreport`` by default.
Open file ``testreport/index.html`` in you web browser to inspect it.

To execute just part of the tests when fixing something, ``cd`` into some
subdirectory, e.g. ``lib`` and execute the same command as above. 
gain, it will execute all tests in all ``testsuite`` subdirectories and
create a report.

For changing GRASS GIS data(base) directory and for other parameters, see
help for ``grass.gunittest.main`` module::

    python -m grass.gunittest.main --help


Running individual test files
-----------------------------

To run a single test file, start GRASS session in the Location and Mapset
suitable for testing and go to the directory where the test file is.
Then run the file as a Python script::

    python test_something.py

If the file is a ``gunittest``-based or ``unittest``-based test,
you will receive a textual output with failed individual tests (test methods).
If the file is a general Python scriptyou need to examine the output carefully
as well as source code itself to see what is expected behavior.

The same as for general Python scripts, applies also to Shell scripts,
so you should examine the output carefully.
You should execute scripts using::

    sh -e -x test_topology_vgeneralize.sh

The ``-x`` is just to see which commands are executed but the ``-e`` flag
is crucial because this is how the GRASS testing framework runs the Shell
scripts. The flag causes execution to stop once some command gives a non-zero
return code.


Running tests and creating report
---------------------------------

Currently there is full support only for running all the tests in
the small (basic) version of GRASS GIS sample Location for North Carolina
(see `GRASS GIS sample data`).

.. _GRASS GIS sample data: https://grass.osgeo.org/download/sample-data


Example Bash script to run be used as a cron job
------------------------------------------------

.. code-block:: bash

    #!/bin/bash

    set -e  # fail fast

    REPORTS=".../testreports"
    GRASSSRC=".../grass-src"
    # here we suppose default compilation settings of GRASS and no make install
    GRASSBIN="$GRASSSRC/bin.../grass78"
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
    git pull
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
using ``crontab -e`` and adding a line similar to the following one::

    0 4 * * 1 .../grasstests/test_grass_gis.sh

Which will perform the tests every Monday at 4:00 in the morning (local time).

Particular script and frequency depends on what you want to test and
how many resources you want to use.
