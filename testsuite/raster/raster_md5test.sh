#!/bin/sh

# Markus Neteler
# Test cases for 2D raster data

# Tests:
#   - generate 3x3 map, value 1/1.1
#   - calculate md5sum
#   - compare with known results

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program."
    exit 1
fi

#### check if we have sed
if [ ! -x "`which sed`" ] ; then
    echo "$PROG: sed required, please install first" 1>&2
    exit 1
fi

#### check if we have md5sum
if [ ! -x "`which md5sum`" ] ; then
    echo "$PROG: md5sum required, please install first" 1>&2
    exit 1
fi

#### check if we have cut
if [ ! -x "`which cut`" ] ; then
    echo "$PROG: cut required, please install first" 1>&2
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
check_md5sum()
{
 EXPECTED="$1"
 FOUND="$2"

 # test for NAN
 if [ "$FOUND" = "nan" ] ; then
  echo "ERROR. $VALUENAME: Expected=$EXPECTED | FOUND=$FOUND"
  cleanup ; finalcleanup
  exit 1
 fi

 if [ "$EXPECTED" != "$FOUND" ] ; then
  echo "ERROR. The md5sum differs."
  cleanup ; finalcleanup
  exit 1
 fi
}

echo "Saving current & setting test region."
g.region save=$TMPNAME
check_exit_status $?
g.region s=0 n=$PIXEL w=0 e=$PIXEL res=1 tbres=1
check_exit_status $?

########### 2D raster INT tests ###########
VALUE=1
echo "INT/CELL md5sum test."
r.mapcalc "$TMPNAME=1"
check_exit_status $?

echo "MD5 checksum on output of INT/CELL test."
MD5="`r.out.ascii $TMPNAME dp=15 | md5sum | cut -d' ' -f1`"
check_md5sum "549e7dabe70df893803690571d2e1503" "$MD5"

cleanup
echo "INT/CELL md5sum test successful"
echo "##################################"

########### 2D raster FCELL tests ###########
VALUE=1.1
echo "FLOAT/FCELL md5sum test."
r.mapcalc "$TMPNAME=$VALUE"
check_exit_status $?

echo "MD5 checksum on output of FLOAT/FCELL test."
MD5="`r.out.ascii $TMPNAME dp=15 | md5sum | cut -d' ' -f1`"
check_md5sum "379f3d880b6d509051af6b4ccf470762" "$MD5"

cleanup
echo "FLOAT/FCELL md5sum test successful"
echo "##################################"

###########
# if we arrive here...

finalcleanup

echo "All tests successful. Congrats."
exit 0
