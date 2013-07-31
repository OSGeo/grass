#!/bin/sh

# Markus Neteler
# Test cases for 2D raster data

# Tests:
#   - generate 3x3 map, value 1/1.1
#   - calculate statistics
#   - compare with known results

#
# TODO
#   - how big EPSILON?

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program."
    exit 1
fi

#### check if we have awk
if [ ! -x "`which awk`" ] ; then
    echo "$PROG: awk required, please install first" 1>&2
    exit 1
fi

# setting environment, so that awk works properly in all languages
unset LC_ALL
export LC_NUMERIC=C

eval `g.gisenv`
: ${GISBASE?} ${GISDBASE?} ${LOCATION_NAME?} ${MAPSET?}
LOCATION=$GISDBASE/$LOCATION_NAME/$MAPSET

# some definitions
PIXEL=3
# how big EPSILON?
#    epsilon for doubles in IEEE is 2.220446e-16
EPSILON=22204460000000000
PID=$$
TMPNAME="`echo ${PID}_tmp_testmap | sed 's+\.+_+g'`"

# some functions - keep order here
cleanup()
{
 echo "Removing temporary map"
 g.remove rast=$TMPNAME > /dev/null
}

# check if a MASK is already present:
MASKTMP=mask.$TMPNAME
USERMASK=usermask_${MASKTMP}
if test -f $LOCATION/cell/MASK
then
 echo "A user raster mask (MASK) is present. Saving it..."
 g.rename MASK,$USERMASK > /dev/null
fi

finalcleanup()
{
 echo "Restoring user region"
 g.region region=$TMPNAME
 g.remove region=$TMPNAME > /dev/null
 #restore user mask if present:
 if test -f $LOCATION/cell/$USERMASK ; then
  echo "Restoring user MASK"
  g.remove rast=MASK > /dev/null
  g.rename $USERMASK,MASK > /dev/null
 fi
}

check_exit_status()
{
 if [ $1 -ne 0 ] ; then
  echo "An error occurred."
  cleanup ; finalcleanup
  exit 1
 fi
}

########## test function goes here
compare_result()
{
 EXPECTED=$1
 FOUND=$2
 VALUENAME=$3

 # test for NAN
 if [ "$FOUND" = "nan" ] ; then
  echo "ERROR. $VALUENAME: Expected=$EXPECTED | FOUND=$FOUND"
  cleanup ; finalcleanup
  exit 1
 fi

 # check for difference + 1
 DIFF=`echo $EXPECTED $FOUND $EPSILON | awk '{printf "%16f", ($1 - $2) * $3 }'`
 #make absolute value
 DIFF=`echo $DIFF | awk '{printf("%f", sqrt($1 * $1))}'`
 #round to integer
 DIFF=`echo $DIFF | awk '{printf("%20d", int($1+0.5))}'`

 # check if difference > 0
 if [ $DIFF -gt 0 ] ; then
  echo "ERROR. $VALUENAME: Expected=$EXPECTED | FOUND=$FOUND"
  cleanup ; finalcleanup
  exit 1
 fi
}

#check if a MASK is already present:
MASKTMP=mask.$TMPNAME
USERMASK=usermask_${MASKTMP}
if test -f $LOCATION/cell/MASK
then
 echo "A user raster mask (MASK) is present. Saving it..."
 g.rename MASK,$USERMASK > /dev/null
 check_exit_status $?
fi

echo "Saving current & setting test region."
g.region save=$TMPNAME
check_exit_status $?
g.region s=0 n=$PIXEL w=0 e=$PIXEL res=1 tbres=1
check_exit_status $?

########### 2D raster INT tests ###########
VALUE=1
echo "INT/CELL test."
r.mapcalc "$TMPNAME=1"
check_exit_status $?

echo "Univariate statistics of INT/CELL test."
eval `r.univar -g $TMPNAME`
check_exit_status $?
compare_result 9 $n n
compare_result $VALUE $min min
compare_result $VALUE $max max
compare_result 0 $range range
compare_result $VALUE $mean mean
compare_result 0 $stddev stddev
compare_result 0 $variance variance
compare_result 0 $coeff_var coeff_var
compare_result 9 $sum sum

cleanup
echo "INT/CELL univariate statistics test successful"
echo "##################################"

########### 2D raster FCELL tests ###########
VALUE=1.1
echo "FLOAT/FCELL test."
r.mapcalc "$TMPNAME=$VALUE"
check_exit_status $?

echo "Univariate statistics of FLOAT/FCELL test."
eval `r.univar -g $TMPNAME`
check_exit_status $?
compare_result 9 $n n
compare_result $VALUE $min min
compare_result $VALUE $max max
compare_result 0 $range range
compare_result $VALUE $mean mean
compare_result 0 $stddev stddev
compare_result 0 $variance variance
compare_result 0 $coeff_var coeff_var
compare_result 9.9 $sum sum

cleanup
echo "FLOAT/FCELL univariate statistics test successful"
echo "##################################"

###########
# if we arrive here...

finalcleanup
echo "All tests successful. Congrats."
exit 0

