#!/bin/sh
# This scripts runs all tests available in the temporal module directory
# Logs a written to "run.log" 

LOG_FILE="/tmp/run.log"
echo "Logfile\n\n" > $LOG_FILE

# For each directory
for mydir in `ls -d t*` ; do
    if [ -d $mydir ] ; then
        echo $mydir
        cd $mydir
        for myfile in `ls test.* | grep -v '~'` ; do
            sh $myfile >> $LOG_FILE 2>&1
        done
        cd ..
    fi
done
mv $LOG_FILE .
