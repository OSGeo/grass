#!/bin/sh

#fetch the description stuff from HTML pages for 5.7

GRASS57=$HOME/grass57

########################

if [ $# -ne 2 ] ; then
 echo g51.extract_description.sh /path/to/grass53src htmlfile
 exit
fi

PATHGRASS50="$1"

FILE=$PATHGRASS50/html/html/$2.html

CUTLINE="`grep -ni '<H2>DESCRIPTION' $FILE | cut -d':' -f1`"
if [ "$CUTLINE" == "" ] ; then
  echo "ERROR: no <H2>DESCRIPTION</H2> present in html file"
  exit
fi

TOTALLINES=`wc -l $FILE  | awk '{print $1}'`
FROMBOTTOM=$(( $TOTALLINES - $CUTLINE ))

echo "<H2>DESCRIPTION</H2>" > description.html
echo "" >> description.html
tail -$FROMBOTTOM $FILE | grep -vi '</body>' | grep -vi '</html>' >> description.html
