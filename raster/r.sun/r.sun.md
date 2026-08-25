## DESCRIPTION

**r.sun** computes beam (direct), diffuse and ground reflected solar
irradiation raster maps for given day, latitude, surface and atmospheric
conditions. Solar parameters (e.g. time of sunrise and sunset,
declination, extraterrestrial irradiance, daylight length) are stored in
the resultant maps' history files. Alternatively, the local time can be
specified to compute solar incidence angle and/or irradiance raster
maps. The shadowing effect of the topography is incorporated by default.
This can be done either internally by calculation of the shadowing
effect directly from the digital elevation model or by specifying raster
maps of the horizon height which is much faster. These horizon raster
maps can be calculated using [r.horizon](r.horizon.md).

For latitude-longitude coordinates it requires that the elevation map is
in meters. The rules are:

- lat/lon coordinates: elevation in meters;
- Other coordinates: elevation in the same unit as the easting-northing
  coordinates.

The solar geometry of the model is based on the works of Krcho (1990),
later improved by Jenco (1992). The equations describing Sun -- Earth
position as well as an interaction of the solar radiation with
atmosphere were originally based on the formulas suggested by Kitler and
Mikler (1986). This component was considerably updated by the results
and suggestions of the working group co-ordinated by Scharmer and Greif
(2000) (this algorithm might be replaced by SOLPOS algorithm-library
included in GRASS within [r.sunmask](r.sunmask.md) command). The model
computes all three components of global radiation (beam, diffuse and
reflected) for the clear sky conditions, i.e. not taking into
consideration the spatial and temporal variation of clouds. The extent
and spatial resolution of the modelled area, as well as integration over
time, are limited only by the memory and data storage resources. The
model is built to fulfil user needs in various fields of science
(hydrology, climatology, ecology and environmental sciences,
photovoltaics, engineering, etc.) for continental, regional up to the
landscape scales.

The model considers a shadowing effect of the local topography unless
switched off with the *-p* flag. **r.sun** works in two modes: In the
first mode it calculates for the set local time a solar incidence angle
\[degrees\] and solar irradiance values \[W.m-2\]. In the second mode
daily sums of solar radiation \[Wh.m-2.day-1\] are computed within a set
day. By a scripting the two modes can be used separately or in a
combination to provide estimates for any desired time interval. The
model accounts for sky obstruction by local relief features. Several
solar parameters are saved in the resultant maps' history files, which
may be viewed with the [r.info](r.info.md) command.

The solar incidence angle raster map *incidout* is computed specifying
elevation raster map *elevation*, aspect raster map *aspect*, slope
steepness raster map *slope,* given the day *day* and local time *time*.
There is no need to define latitude for projects with known and defined
coordinate system (check it with the [g.proj](g.proj.md) command). If
you have undefined projection, (x,y) system, etc. then the latitude can
be defined explicitly for large areas by input raster map *lat_in* with
interpolated latitude values. All input raster maps must be floating
point (FCELL) raster maps. Null data in maps are excluded from the
computation (and also speeding-up the computation), so each output
raster map will contain null data in cells according to all input raster
maps. The user can use [r.null](r.null.md) command to create/reset null
file for your input raster maps.  
The specified day *day* is the number of the day of the general year
where January 1 is day no.1 and December 31 is 365. Time *time* must be
a local (solar) time (i.e. NOT a zone time, e.g. GMT, CET) in decimal
system, e.g. 7.5 (= 7h 30m A.M.), 16.1 = 4h 6m P.M..

The solar *declination* parameter is an option to override the value
computed by the internal routine for the day of the year. The value of
geographical latitude can be set as a constant for the whole computed
region or, as an option, a grid representing spatially distributed
values over a large region. The geographical latitude must be also in
decimal system with positive values for northern hemisphere and negative
for southern one. In similar principle the Linke turbidity factor
(*linke*, *lin* ) and ground albedo (*albedo*, *alb*) can be set.

Besides clear-sky radiations, the user can compute a real-sky radiation
(beam, diffuse) using *coeff_bh* and *coeff_dh* input raster maps
defining the fraction of the respective clear-sky radiations reduced by
atmospheric factors (e.g. cloudiness). The value is between 0-1. Usually
these coefficients can be obtained from a long-terms meteorological
measurements provided as raster maps with spatial distribution of these
coefficients separately for beam and diffuse radiation (see Suri and
Hofierka, 2004, section 3.2).

The solar irradiation or irradiance raster maps *beam_rad*, *diff_rad*,
*refl_rad* are computed for a given day *day,* latitude *lat_in*,
elevation *elevation*, slope *slope* and aspect *aspect* raster maps.
For convenience, the output raster given as *glob_rad* will output the
sum of the three radiation components. The program uses the Linke
atmosphere turbidity factor and ground albedo coefficient. A default,
single value of Linke factor is *lin*=3.0 and is near the annual average
for rural-city areas. The Linke factor for an absolutely clear
atmosphere is *lin*=1.0. See notes below to learn more about this
factor. The incidence solar angle is the angle between horizon and solar
beam vector.

The solar radiation maps for a given day are computed by integrating the
relevant irradiance between sunrise and sunset times for that day. The
user can set a finer or coarser time step used for all-day radiation
calculations with the *step* option. The default value of *step* is 0.5
hour. Larger steps (e.g. 1.0-2.0) can speed-up calculations but produce
less reliable (and more jagged) results. As the sun moves through
approx. 15° of the sky in an hour, the default *step* of half an hour
will produce 7.5° steps in the data. For relatively smooth output with
the sun placed for every degree of movement in the sky you should set
the *step* to 4 minutes or less. *step*`=0.05` is equivalent to every 3
minutes. Of course setting the time step to be very fine proportionally
increases the module's running time.

The output units are in Wh per squared meter per given day
\[Wh/(m\*m)/day\]. The incidence angle and irradiance/irradiation maps
are computed with the shadowing influence of relief by default. It is
also possible for them to be computed without this influence using the
planar flag (*-p*). In mountainous areas this can lead to very different
results! The user should be aware that taking into account the shadowing
effect of relief can slow down the speed of computation, especially when
the sun altitude is low.

When considering the shadowing effect, speed and precision of
computation can be controlled by the *distance_step* parameter, which
defines the sampling density at which the visibility of a grid cell is
computed in the direction of a path of the solar flow. It also defines
the method by which the obstacle's altitude is computed. When choosing a
*distance_step* less than 1.0 (i.e. sampling points will be computed at
*distance_step* \* cellsize distance), *r.sun* takes the altitude from
the nearest grid point. Values above 1.0 will use the maximum altitude
value found in the nearest 4 surrounding grid points. The default value
*distance_step*=1.0 should give reasonable results for most cases (e.g.
on DEM). The *distance_step* value defines a multiplying coefficient for
sampling distance. This basic sampling distance equals to the arithmetic
average of both cell sizes. The reasonable values are in the range
0.5-1.5. The values below 0.5 will decrease and values above 1.0 will
increase the computing speed. Values greater than 2.0 may produce
estimates with lower accuracy in highly dissected relief. The fully
shadowed areas are written to the output maps as zero values. Areas with
NULL data are considered as no barrier with shadowing effect.

The maps' history files are generated containing the following listed
parameters used in the computation:  

- Solar constant used W.m-2  
- Extraterrestrial irradiance on a plane perpendicular to the solar beam
\[W.m-2\]  
- Day of the year  
- Declination \[radians\]  
- Decimal hour (Alternative 1 only)  
- Sunrise and sunset (min-max) over a horizontal plane  
- Daylight lengths  
- Geographical latitude (min-max)  
- Linke turbidity factor (min-max)  
- Ground albedo (min-max)

The user can use a nice shellcript with variable day to compute
radiation for some time interval within the year (e.g. vegetation or
winter period). Elevation, aspect and slope input values should not be
reclassified into coarser categories. This could lead to incorrect
results.

## OPTIONS

Currently, there are two modes of r.sun. In the first mode it calculates
solar incidence angle and solar irradiance raster maps using the set
local time. In the second mode daily sums of solar irradiation
\[Wh.m-2.day-1\] are computed for a specified day.

## NOTES

Solar energy is an important input parameter in different models
concerning energy industry, landscape, vegetation, evapotranspiration,
snowmelt or remote sensing. Solar rays incidence angle maps can be
effectively used in radiometric and topographic corrections in
mountainous and hilly terrain where very accurate investigations should
be performed.

The clear-sky solar radiation model applied in the r.sun is based on the
work undertaken for development of European Solar Radiation Atlas
(Scharmer and Greif 2000, Page et al. 2001, Rigollier 2001). The clear
sky model estimates the global radiation from the sum of its beam,
diffuse and reflected components. The main difference between solar
radiation models for inclined surfaces in Europe is the treatment of the
diffuse component. In the European climate this component is often the
largest source of estimation error. Taking into consideration the
existing models and their limitation the European Solar Radiation Atlas
team selected the Muneer (1990) model as it has a sound theoretical
basis and thus more potential for later improvement.

Details of underlying equations used in this program can be found in the
reference literature cited below or book published by Neteler and
Mitasova: Open Source GIS: A GRASS GIS Approach (published in Kluwer
Academic Publishers in 2002).

Average monthly values of the Linke turbidity coefficient for a mild
climate in the northern hemisphere (see reference literature for your
study area):

| Month      | Jan | Feb | Mar | Apr | May | Jun | Jul | Aug | Sep | Oct | Nov | Dec | annual |
|------------|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|--------|
| mountains  | 1.5 | 1.6 | 1.8 | 1.9 | 2.0 | 2.3 | 2.3 | 2.3 | 2.1 | 1.8 | 1.6 | 1.5 | 1.90   |
| rural      | 2.1 | 2.2 | 2.5 | 2.9 | 3.2 | 3.4 | 3.5 | 3.3 | 2.9 | 2.6 | 2.3 | 2.2 | 2.75   |
| city       | 3.1 | 3.2 | 3.5 | 4.0 | 4.2 | 4.3 | 4.4 | 4.3 | 4.0 | 3.6 | 3.3 | 3.1 | 3.75   |
| industrial | 4.1 | 4.3 | 4.7 | 5.3 | 5.5 | 5.7 | 5.8 | 5.7 | 5.3 | 4.9 | 4.5 | 4.2 | 5.00   |

Planned improvements include the use of the SOLPOS algorithm for solar
geometry calculations and internal computation of aspect and slope.

### Solar time

By default r.sun calculates times as true solar time, whereby solar noon
is always exactly 12 o'clock everywhere in the current region. Depending
on where the zone of interest is located in the related time zone, this
may cause differences of up to an hour, in some cases (like Western
Spain) even more. On top of this, the offset varies during the year
according to the Equation of Time.

To overcome this problem, the user can use the option
*civil_time=\<timezone_offset\>* in r.sun to make it use real-world
(wall clock) time. For example, for Central Europe the timezone offset
is +1, +2 when daylight saving time is in effect.

### Extraction of shadow maps

A map of shadows can be extracted from the solar incidence angle map
(incidout). Areas with NULL values are shadowed. This will not work if
the *-p* flag has been used.

### Large maps and out of memory problems

With a large number or columns and rows, **r.sun** can consume
significant amount of memory. While output raster maps are not
partitionable, the input raster maps are using the *npartitions*
parameter. In case of out of memory error
(`ERROR: G_malloc: out of memory`), the *npartitions* parameter can be
used to run a segmented calculation which consumes less memory during
the computations. The amount of memory by **r.sun** is estimated as
follows:

```sh
# without input raster map partitioning:
#  memory requirements: 4 bytes per raster cell
#  rows,cols: rows and columns of current region (find out with g.region)
#  IR: number of input raster maps without horizon maps
#  OR: number of output raster maps
memory_bytes = rows*cols*(IR*4 + horizon_steps + OR*4)

# with input raster map partitioning:
memory_bytes = rows*cols*((IR*4+horizon_steps)/npartitions  + OR*4)
```

## EXAMPLES

North Carolina example (considering also cast shadows):

```sh
g.region raster=elevation -p

# calculate horizon angles (to speed up the subsequent r.sun calculation)
r.horizon elevation=elevation step=30 bufferzone=200 output=horangle \
    maxdistance=5000

# slope + aspect
r.slope.aspect elevation=elevation aspect=aspect.dem slope=slope.dem

# calculate global radiation for day 180 at 2p.m., using r.horizon output
r.sun elevation=elevation horizon_basename=horangle horizon_step=30 \
      aspect=aspect.dem slope=slope.dem glob_rad=global_rad day=180 time=14
# result: output global (total) irradiance/irradiation [W.m-2] for given day/time
r.univar global_rad
```

Calculation of the integrated daily irradiation for a region in
North-Carolina for a given day of the year at 30m resolution. Here day
172 (i.e., 21 June in non-leap years):

```sh
g.region raster=elev_ned_30m -p

# considering cast shadows
r.sun elevation=elev_ned_30m linke_value=2.5 albedo_value=0.2 day=172 \
      beam_rad=b172 diff_rad=d172 \
      refl_rad=r172 insol_time=it172

d.mon wx0
# show irradiation raster map [Wh.m-2.day-1]
d.rast.leg b172
# show insolation time raster map [h]
d.rast.leg it172
```

We can compute the day of year from a specific date in Python:

```python
>>> import datetime
>>> datetime.datetime(2014, 6, 21).timetuple().tm_yday
172
```

## SEE ALSO

*[r.horizon](r.horizon.md), [r.slope.aspect](r.slope.aspect.md),
[r.sunhours](r.sunhours.md), [r.sunmask](r.sunmask.md),
[g.proj](g.proj.md), [r.null](r.null.md), [v.surf.rst](v.surf.rst.md)*

## REFERENCES

- Hofierka, J., Suri, M. (2002): The solar radiation model for Open
  source GIS: implementation and applications. International GRASS users
  conference in Trento, Italy, September 2002.
  ([PDF](http://skagit.meas.ncsu.edu/~jaroslav/trento/Hofierka_Jaroslav.pdf))
- Hofierka, J. (1997). Direct solar radiation modelling within an open
  GIS environment. Proceedings of JEC-GI'97 conference in Vienna,
  Austria, IOS Press Amsterdam, 575-584.
- Jenco, M. (1992). Distribution of direct solar radiation on georelief
  and its modelling by means of complex digital model of terrain (in
  Slovak). Geograficky casopis, 44, 342-355.
- Kasten, F. (1996). The Linke turbidity factor based on improved values
  of the integral Rayleigh optical thickness. Solar Energy, 56 (3),
  239-244.
- Kasten, F., Young, A. T. (1989). Revised optical air mass tables and
  approximation formula. Applied Optics, 28, 4735-4738.
- Kittler, R., Mikler, J. (1986): Basis of the utilization of solar
  radiation (in Slovak). VEDA, Bratislava, p. 150.
- Krcho, J. (1990). Morfometrická analza a digitálne modely georeliéfu
  (Morphometric analysis and digital models of georelief, in Slovak).
  VEDA, Bratislava.
- Muneer, T. (1990). Solar radiation model for Europe. Building services
  engineering research and technology, 11, 4, 153-163.
- Neteler, M., Mitasova, H. (2002): Open Source GIS: A GRASS GIS
  Approach, Kluwer Academic Publishers. (Appendix explains formula;
  [r.sun script download](https://grassbook.org/))
- Page, J. ed. (1986). Prediction of solar radiation on inclined
  surfaces. Solar energy R&D in the European Community, series F - Solar
  radiation data, Dordrecht (D. Reidel), 3, 71, 81-83.
- Page, J., Albuisson, M., Wald, L. (2001). The European solar radiation
  atlas: a valuable digital tool. Solar Energy, 71, 81-83.
- Rigollier, Ch., Bauer, O., Wald, L. (2000). On the clear sky model of
  the ESRA - European Solar radiation Atlas - with respect to the
  Heliosat method. Solar energy, 68, 33-48.
- Scharmer, K., Greif, J., eds., (2000). The European solar radiation
  atlas, Vol. 2: Database and exploitation software. Paris (Les Presses
  de l'École des Mines).
- Joint Research Centre: [GIS solar radiation database for
  Europe](http://re.jrc.ec.europa.eu/pvgis/) and [Solar radiation and
  GIS](http://re.jrc.ec.europa.eu/pvgis/solres/solmod3.htm)

## AUTHORS

Jaroslav Hofierka, GeoModel, s.r.o. Bratislava, Slovakia  
Marcel Suri, GeoModel, s.r.o. Bratislava, Slovakia  
Thomas Huld, JRC, Italy  
© 2007, Jaroslav Hofierka, Marcel Suri. This program is free software
under the GNU General Public License (\>=v2)

[hofierka@geomodel.sk](MAILTO:hofierka@geomodel.sk)
[suri@geomodel.sk](MAILTO:suri@geomodel.sk)
