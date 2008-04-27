#!/bin/sh

############################################################################
#
# MODULE:	    v.univar.sh
# AUTHOR(S):	Michael Barton, Arizona State University
# PURPOSE:	    Calculates univariate statistics from a GRASS vector map.
#                Based on r.univar.sh by Markus Neteler
# COPYRIGHT:	(C) 2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#% description: Calculates univariate statistics on selected table column for a GRASS vector map.
#% keywords: vector, statistics
#%End
#%flag
#%  key: e
#%  description: Extended statistics (quartiles and 90th percentile)
#%END
#%option
#% key: table
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of data table
#% required : yes
#%End
#%option
#% key: column
#% type: string
#% description: Column on which to calculate statistics (must be numeric)
#% required : yes
#%end
#%option
#% key: database
#% type: string
#% description: Database/directory for table
#% required : no
#%end
#%option
#% key: driver
#% type: string
#% description: Database driver
#% required : no
#%end
#%option
#% key: where
#% type: string
#% description: WHERE conditions of SQL statement without 'where' keyword
#% required : no
#%end

if  [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
 exit 1
fi   

if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

g.message -w "This module is superseded and will be removed in future \
  versions of GRASS. Use the v.univar instead."

PROG=`basename $0`

#### check if we have awk
if [ ! -x "`which awk`" ] ; then
    g.message -e "awk required, please install awk/gawk first"
    exit 1
fi

# setting environment, so that awk works properly in all languages
unset LC_ALL
LC_NUMERIC=C
export LC_NUMERIC


TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
    g.message -e "Unable to create temporary files"
    exit 1
fi

cleanup()
{
   \rm -f "$TMP" "$TMP.sort"
}

# what to do in case of user break:
exitprocedure()
{
 g.message -e 'User break!'
 cleanup
 exit 1
}
# shell check for user break (signal list: trap -l)
trap "exitprocedure" 2 3 15


g.message "Calculation for column $GIS_OPT_COLUMN of table $GIS_OPT_TABLE..."
g.message "Reading column values..."

if [ -z "$GIS_OPT_DATABASE" ] ; then
    db=""
else
    db="database=$GIS_OPT_DATABASE"
fi

if [ -z "$GIS_OPT_DRIVER" ] ; then
    drv=""
else
    drv="driver=$GIS_OPT_DRIVER"
fi

if [ -z "$GIS_OPT_WHERE" ] ; then
   db.select table="$GIS_OPT_TABLE" $db $drv sql="select $GIS_OPT_COLUMN from $GIS_OPT_TABLE" -c > "$TMP"
else
   db.select table="$GIS_OPT_TABLE" $db $drv sql="select $GIS_OPT_COLUMN from $GIS_OPT_TABLE WHERE $GIS_OPT_WHERE" -c > "$TMP"
fi


# debug:
#echo "database = $GIS_OPT_DATABASE"
#echo "db = $db"
#echo ""
#echo "drv = $drv"
#echo "driver = $GIS_OPT_DRV"
#echo ""

#check if map contains only NULL's in current region
LINES=`wc -l "$TMP" | awk '{print $1}'`
if [ "$LINES" -eq 0 ] ; then
 g.message -e "Table $GIS_OPT_TABLE contains no data." 
 cleanup
 exit 1
fi

# calculate statistics
g.message "Calculating statistics..."
cat "$TMP" | awk 'BEGIN {sum = 0.0 ; sum2 = 0.0; min = 10e10 ; max = -min}
function abs(x){return x < 0 ? -x : x}
(NF>0) {
	sum += $1 ; sum2 += $1 * $1 ; sum3 += abs($1) ; N++;
        if ($1 > max) {max = $1}
        if ($1 < min) {min = $1}
       }
END{
    if(N>0){
	print ""
	print "Number of values:",N
	print "Minimum:",min
	print "Maximum:",max
	print "Range:",max-min
	print "-----"
	print "Mean:",sum/N
	print "Arithmetic mean of absolute values:",sum3/N
	print "Variance:",(sum2 - sum*sum/N)/N
	print "Standard deviation:",sqrt((sum2 - sum*sum/N)/N)
	print "Coefficient of variation:",(sqrt((sum2 - sum*sum/N)/N))/(sqrt(sum*sum)/N)
	print "-----"
    }else{
	print "ERROR: No non-null values found"
    }
}'

if [ $GIS_FLAG_E -eq 1 ] ; then
  #preparations:
  cat "$TMP" | sort -n > "$TMP.sort"
  NUMBER=`cat "$TMP.sort" | wc -l | awk '{print $1}'`
  ODDEVEN=`echo "$NUMBER" | awk '{print $1%2}'`

  # 0.25 quartile
  QUARTILE=0.25
  QPOS=`echo $NUMBER $QUARTILE | awk '{printf "%d", $1 * $2 + 0.5}'`
  QELEMENT=`head -n $QPOS "$TMP.sort" | tail -n 1`
  echo "1st Quartile: $QELEMENT"

  #Calculate median
  if [ $ODDEVEN -eq 0 ]
  then
   # even
   EVENMEDIANNUMBER=`expr $NUMBER / 2`
   EVENMEDIANNUMBERPLUSONE=`expr $EVENMEDIANNUMBER + 1`
   # select two numbers
   SELECTEDNUMBERS=`cat "$TMP.sort" | head -n $EVENMEDIANNUMBERPLUSONE | tail -n 2`
   RESULTEVENMEDIAN=`echo $SELECTEDNUMBERS | awk '{printf "%f", ($1 + $2)/2.0}'`
   echo "Median (even N): $RESULTEVENMEDIAN"
  else
   # odd
   ODDMEDIANNUMBER=`echo $NUMBER | awk '{printf "%d", int($1/2+.5)}'`
   RESULTODDMEDIAN=`cat "$TMP.sort" | head -n $ODDMEDIANNUMBER | tail -n 1 | awk '{printf "%f", $1}'`
   echo "Median (odd N): $RESULTODDMEDIAN"
  fi


  # 0.75 quartile
  QUARTILE=0.75
  QPOS=`echo $NUMBER $QUARTILE | awk '{printf "%d", $1 * $2 + 0.5}'`
  QELEMENT=`head -n $QPOS "$TMP.sort" | tail -n 1`
  echo "3rd Quartile: $QELEMENT"

  # 0.90 percentile
  QUARTILE=0.9
  QPOS=`echo $NUMBER $QUARTILE | awk '{printf "%d", $1 * $2 + 0.5}'`
  QELEMENT=`head -n $QPOS "$TMP.sort" | tail -n 1`
  echo "90th Percentile: $QELEMENT"

fi

cleanup
