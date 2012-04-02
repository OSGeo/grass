#!/bin/sh

# Script to test i.topo.corr with a synthetic map
#
# Use North Carolina location to test:
#   grass64 ~/grassdata/nc_spm_08/user1

if test "$GISBASE" = ""; then
 echo "You must be in GRASS to run this program."
 exit
fi

export GRASS_OVERWRITE=1

# we use the NC location, time zone UTM-5
g.region n=308500 s=215000 w=630000 e=704800 nsres=250 ewres=250 -pa

# note: no daylight saving time in summer 1954!
DATETIME=`echo "2 Aug 1954 09:34:53 -0500"`

DAY=`echo $DATETIME | cut -d' ' -f1 | awk '{printf "%d", $1}'`
# need to translate word month to numeric month for r.sunmask
MON=`echo $DATETIME | cut -d' ' -f2 | sed 's+Jan+1+g' | sed 's+Feb+2+g' | sed 's+Mar+3+g' | sed 's+Apr+4+g' | sed 's+May+5+g' | sed 's+Jun+6+g' | sed 's+Jul+7+g' | sed 's+Aug+8+g' |sed 's+Sep+9+g' | sed 's+Oct+10+g' | sed 's+Nov+11+g' | sed 's+Dec+12+g'`
YEAR=`echo $DATETIME | cut -d' ' -f3 | awk '{printf "%d", $1}'`
TMPTIME=`echo $DATETIME | cut -d' ' -f4 | awk '{printf "%d", $1}'`
HOUR=`echo $TMPTIME | cut -d':' -f1 | awk '{printf "%d", $1}'`
MIN=`echo $TMPTIME | cut -d':' -f2 | awk '{printf "%d", $1}'`
SEC=`echo $TMPTIME | cut -d':' -f3 | awk '{printf "%d", $1}'`
TIMEZ=`echo $DATETIME | cut -d' ' -f5 | awk '{printf "%d", $1/100}'`
unset TMPTIME

# create synthetic DEM (kind of roof)
r.plane --o myplane0 dip=45 az=0 east=637500 north=221750 elev=1000 type=float
r.plane --o myplane90 dip=45 az=90 east=684800 north=221750 elev=1000 type=float
r.plane --o myplane180 dip=45 az=180 east=684800 north=260250 elev=1000 type=float
r.plane --o myplane270 dip=45 az=270 east=684800 north=221750 elev=1000 type=float
r.mapcalc "myplane_pyr = double(min(myplane90,myplane270,myplane0,myplane180)/10. + 8600.)"

# nviz
# nviz myplane_pyr

# get sun position
eval `r.sunmask -s -g output=dummy elev=myplane_pyr year=$YEAR month=8 day=$DAY hour=$HOUR minute=$MIN second=$SEC timezone=$TIMEZ`

solarzenith=`echo $sunangleabovehorizon | awk '{printf "%f", 90. - $1}'`
echo "Sun position ($DATETIME): solarzenith: $solarzenith, sunazimuth: $sunazimuth"

# shade relief
r.shaded.relief input=myplane_pyr output=myplane_pyr_shaded altitude=$sunangleabovehorizon azimuth=$sunazimuth
d.mon stop=wx0 2> /dev/null
d.mon wx0
d.rast myplane_pyr_shaded

# pre-run: illumination map
i.topo.corr -i input=myplane_pyr output=myplane_pyr_illumination \
	    basemap=myplane_pyr zenith=$solarzenith azimuth=$sunazimuth 
r.colors myplane_pyr_illumination color=gyr

# show original
d.erase -f
r.colors myplane_pyr color=grey
d.rast.leg myplane_pyr
echo "Original" | d.text color=black

# nviz with illumination draped over
# nviz myplane_pyr color=myplane_pyr_illumination

# making the 'band' reflectance file from the shade map
r.mapcalc "myplane_pyr_band = double((myplane_pyr_shaded - 60.)/18.)"
r.colors myplane_pyr_band color=gyr
d.mon stop=wx0 2> /dev/null
d.mon wx0
d.rast myplane_pyr_band
d.legend myplane_pyr_band
echo "Band reflectance" | d.text color=black

## test it:
# percent
METHOD=percent
i.topo.corr input=myplane_pyr_band output=myplane_pyr_topocorr_${METHOD} basemap=myplane_pyr_illumination zenith=$solarzenith method=$METHOD
d.mon stop=wx1 2> /dev/null
d.mon wx1
d.rast.leg myplane_pyr_topocorr_${METHOD}.myplane_pyr_band
echo "METHOD=percent" | d.text color=black

# minnaert
METHOD=minnaert
i.topo.corr input=myplane_pyr_band output=myplane_pyr_topocorr_${METHOD} basemap=myplane_pyr_illumination zenith=$solarzenith method=$METHOD
d.mon stop=wx2 2> /dev/null
d.mon wx2
d.rast.leg myplane_pyr_topocorr_${METHOD}.myplane_pyr_band
echo "METHOD=minnaert" | d.text color=black

# c-factor
METHOD=c-factor
i.topo.corr input=myplane_pyr_band output=myplane_pyr_topocorr_${METHOD} basemap=myplane_pyr_illumination zenith=$solarzenith method=$METHOD
d.mon stop=wx3 2> /dev/null
d.mon wx3
d.rast.leg myplane_pyr_topocorr_${METHOD}.myplane_pyr_band
echo "METHOD=c-factor" | d.text color=black

# cosine
METHOD=cosine
i.topo.corr input=myplane_pyr_band output=myplane_pyr_topocorr_${METHOD} basemap=myplane_pyr_illumination zenith=$solarzenith method=$METHOD
d.mon stop=wx4 2> /dev/null
d.mon wx4
d.rast.leg myplane_pyr_topocorr_${METHOD}.myplane_pyr_band
echo "METHOD=cosine" | d.text color=black
