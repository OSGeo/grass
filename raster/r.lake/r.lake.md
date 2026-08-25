## DESCRIPTION

*r.lake* fills a lake to a target water level from a given start point.
The user can think of it as *[r.grow](r.grow.md)* with additional checks
for elevation. The resulting raster map contains cells with values
representing lake depth and NULL for all other cells beyond the lake.
Lake depth is reported relative to specified water level (specified
level = 0 depth).

This module uses a 3x3 moving window approach to find all cells that
match three criteria and to define the lake:

- cells are below the specified elevation (i.e., water level);
- cells are connected with an initial cell (seed or coordinates pair
  value);
- cells are not NULL or masked.

The water level must be in DEM units.

## NOTES

The seed (starting) point can be a raster map with at least one cell
value greater than zero, or a seed point can be specified as an E, N
coordinate pair. If the seed is specified as a coordinate pair, an
additional check is done to make sure that the target water level is
above the level of the DEM. When a raster map is used as a seed,
however, no such checks are done. Specifying a target water level below
surface represented by DEM will result in an empty map. Note: a raster
lake map created in a previous run can also be used as a seed map for a
new run to simulate rising water levels.

The module will create a new map (**lake=foo**) or can be set to replace
the input (**seed=bar**) map if the **-o** flag is used. The user can
use **-o** flag to create animations of rising water level without
producing a separate map for each frame. An initial seed map must be
created to start the sequence, and will be overwritten during subsequent
runs with resulting water levels maps (i.e., a single file serves for
both input and output).

Negative output (the **-n** flag) is useful for visualisations in NVIZ.
It equals the mapcalc's expression *"negative = 0 - positive"*.

### r.mapcalc equivalent - for GRASS hackers

This module was initially created as a script using
*[r.mapcalc](r.mapcalc.md)*. This had some limitations - it was slow and
no checks where done to find out required iteration count. The shell
script code (using *[r.mapcalc](r.mapcalc.md)*) used in the original
script is shown below:

```sh
${seedmap} = if( ${dem}, \
if( if( isnull(${seedmap}),0,${seedmap} > 0), ${wlevel}-${dem}, \
 if( \
  if(isnull(${seedmap}[-1,0]),0, ${seedmap}[-1,0] > 0 && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[-1,1]),0, ${seedmap}[-1,1] > 0 && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[0,1]), 0, ${seedmap}[0,1] > 0  && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[1,1]), 0, ${seedmap}[1,1] > 0  && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[1,0]), 0, ${seedmap}[1,0] > 0  && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[1,-1]),0, ${seedmap}[1,-1] > 0 && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[0,-1]),0, ${seedmap}[0,-1] > 0 && ${wlevel} > ${dem}) ||\
  if(isnull(${seedmap}[-1,-1]),0, ${seedmap}[-1,-1] > 0 && ${wlevel} > ${dem}),\
 ${wlevel}-${dem}, null() )))
```

The `${seedmap}` variable is replaced by seed map names, `${dem}` with
DEM map name, and `${wlevel}` with target water level. To get single
water level, this code block is called with same level numerous times
(in a loop) as the lake grows by single cells during single run.

## KNOWN ISSUES

- The entire map is loaded into RAM.
- A completely negative seed map will not work! At least one cell must
  have a value \> 0. Output from `r.lake -n` *cannot* be used as input
  in the next run.

## EXAMPLE

Example of small flooding along a street (North Carolina sample
dataset):

```sh
g.region raster=elev_lid792_1m -p

# water accumulation next to street dam
r.lake elev_lid792_1m coordinates=638759.3,220264.1 water_level=113.4 lake=flooding

# draw resulting lake map over shaded terrain map
r.relief input=elev_lid792_1m output=elev_lid792_1m_shade
d.rast elev_lid792_1m_shade
d.rast flooding
d.vect streets_wake
```

![Small flooding along a street](r_lake_lidar_dem.jpg)  
*Small flooding along a street (*r.lake*, using Lidar 1m DEM)*

## SEE ALSO

*[r.mapcalc](r.mapcalc.md), [r.grow](r.grow.md), [r.plane](r.plane.md)*

## AUTHOR

Maris Nartiss (maris.nartiss gmail.com)
