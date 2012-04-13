#!/usr/bin/env bash
# This scripts runs all tests available in the temporal module directory
# Logs a written to "run.log" 

LOG_FILE="/tmp/run.log"
echo "Logfile\n\n" > $LOG_FILE

# For each directory
for dir in `ls -d t*` ; do
    if [ -d $dir ] ; then
        echo $dir
        cd $dir
        for file in `ls test.* | grep -v '~'` ; do
            bash $file >> $LOG_FILE 2>&1
        done
        cd -
    fi
done
cp $LOG_FILE .
