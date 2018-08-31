#!/bin/sh
# v.in.garmin and v.in.gpsbabel test script
# by Hamish Bowman 18 May 2007 ; public domain
# assumes downloading from a garmin on port /dev/gps

for PGM in garmin gpsbabel ; do
  for DO_PTS in pts lines ; do
    for TASK in w t r ; do
      if [ $TASK = w ] && [ $DO_PTS = "lines" ] ; then
         continue
      fi
      if [ $DO_PTS = "pts" ] ; then
        PTFLAG="-p"
      else
        PTFLAG=""
      fi
      MAPNAME="test_${PGM}_${TASK}_${DO_PTS}_$$"
      echo "-- Running [v.in.$PGM] for [$TASK] download as [$DO_PTS] into <$MAPNAME> --"
      v.in.$PGM -$TASK $PTFLAG out="$MAPNAME"
      if [ $? -ne 0 ] ; then
         exit 1
      fi
      awk 'BEGIN {printf("\n\n\n\n")}'
    done
  done
done
