## DESCRIPTION

*r.sunmask* creates an output map layer based on an input elevation
raster map layer and the position of the sun. The output map layer
contains the cast shadow areas resulting from sunlight and elevation.
The user can either specify the sun position directly or the module
calculates it from given location and date/time parameters using the
SOLPOS (Solar and Moon Position Algorithm) developed by the [National
Renewable Energy Laboratory](https://www.nrel.gov/) (NREL). SOLPOS
operates in two modes, either

- \(A\) parameters to specify the exact known position of the sun, or
- \(B\) parameters to specify the date/time for the sun position
  calculation by *r.sunmask* itself

must be used.

The module performs sunset/sunrise checks and refraction correction for
sun position calculation. Local coordinate systems are internally
transformed to latitude/longitude for the SOLPOS algorithm. Elevation is
not taken into account for sunset/sunrise calculations.

The solar zenith angle ("sun angle above horizon") is defined as the
angle between the horizon and the vertical (directly overhead or
zenith). Its values can range from 90°, when the sun is directly
overhead, to 0°, when the sun is on the horizon. Values lower than 0°
indicate that the sun is below the horizon.

The solar azimuth angle ("sun azimuth") defines the direction of the
sun. It is the angle between north and the projection of the sun's rays
onto the horizontal plane. This angle is measured in a clockwise
direction and can vary between 0° and 360°. Specifically, an azimuth of
0° means the sun is in the north, 90° in the east, 180° in the south and
270° in the west.

## NOTES

*r.sunmask* and daylight saving time: Instead of converting the local
time to GMT, the SOLPOS algorithm uses what is known as Local Standard
Time, which is generally defined as an offset from GMT. So the key is
the offset from GMT, which is the solpos Time Zone parameter. If the
user specifies clock time (different for winter and summer), s/he would
have to change the Time Zone parameter in *r.sunmask* (**timezone**
parameter) seasonally. See also [Daylight saving time by region and
country](https://en.wikipedia.org/wiki/Daylight_saving_time_by_country).

Note: In latitude/longitude projects the position coordinates pair
(east/west) has to be specified in decimal degree (not DD:MM:SS). If not
specified, the map center's coordinates will be used. Also *g.region -l*
displays the map center's coordinates in latitude/longitude (or
*g.region -c* in the actual coordinate system).

Note for module usage with the *-g* flag, when performing calculations
close to sunset/sunrise:

```sh
 [...]
 sunangleabovehorizont=0.434240
 sunrise=07:59:19
 sunset=16:25:17
 Time (07:59:02) is before sunrise (07:59:19)!
 WARNING: Nothing to calculate. Please verify settings.
 No map calculation requested. Finished.
```

In above calculation it appears to be a mistake as the program indicates
that we are before sunrise while the *sun angle above horizon* is
already positive. The reason is that *sun angle above horizon* is
calculated with correction for atmosphere refraction while *sunrise* and
*sunset* are calculated **without** correction for atmosphere
refraction. The output without *-g* flag contains related indications.

## EXAMPLE

Example for North Carolina sample data set for the calculation of sun
position angles and more:

```sh
# set the region to a place near Raleigh (NC)
g.region raster=elev_lid792_1m -p

# compute only sun position and no output map
r.sunmask -s elev_lid792_1m year=2012 month=2 \
          day=22 hour=10 minute=30 timezone=-5
Using map center coordinates: 638650.000000 220375.000000
Calculating sun position... (using solpos (V. 11 April 2001) from NREL)
2012/02/22, daynum: 53, time: 10:30:00 (decimal time: 10.500000)
long: -78.678856, lat: 35.736160, timezone: -5.000000
Solar position: sun azimuth: 143.006409, sun angle above horz. (refraction corrected): 36.233879
Sunrise time (without refraction): 06:58:11
Sunset time  (without refraction): 17:58:47

# with -g flag, useful for eval() shell function
r.sunmask -s -g elev_lid792_1m  year=2012 month=2 \
          day=22 hour=10 minute=30 timezone=-5
Using map center coordinates: 638650.000000 220375.000000
Calculating sun position... (using solpos (V. 11 April 2001) from NREL)
date=2012/02/22
daynum=53
time=10:30:00
decimaltime=10.500000
longitudine=-78.678856
latitude=35.736160
timezone=-5.000000
sunazimuth=143.006409
sunangleabovehorizon=36.233879
sunrise=06:58:11
sunset=17:58:47
```

## Acknowledgements

Acknowledgements: National Renewable Energy Laboratory for their [SOLPOS
2.0](https://www.nrel.gov/grid/solar-resource/solpos.html) sun position
algorithm.

## SEE ALSO

*[g.region](g.region.md), [r.sun](r.sun.md),
[r.sunhours](r.sunhours.md), [r.slope.aspect](r.slope.aspect.md)*

## AUTHORS

Janne Soimasuo, Finland, 1994  
update to FP by Huidae Cho, 2001  
SOLPOS algorithm feature added by Markus Neteler, 2001
