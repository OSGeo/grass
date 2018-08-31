#!/bin/sh
# This scripts runs all tests available in the temporal module directory
# Logs a written to "run.log" 

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

LOG_FILE="/tmp/run.log"
echo "Logfile\n\n" > $LOG_FILE

export GRASS_MESSAGE_FORMAT=plain

# For each directory
for mydir in `ls -d t*` ; do
    if [ -d "${mydir}" ] ; then
        echo "${mydir}"
        cd "${mydir}"
        for myfile in `ls test.* | grep -v '~'` ; do
	    echo "####### TEST ####### ${myfile}" >> "${LOG_FILE}" 2>&1
            sh "${myfile}" >> "${LOG_FILE}" 2>&1
        done
        cd ..
    fi
done
mv "${LOG_FILE}" .
