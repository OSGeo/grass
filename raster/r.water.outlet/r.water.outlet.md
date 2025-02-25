## DESCRIPTION

*r.water.outlet* generates a watershed basin from a drainage direction
map and a set of coordinates representing the outlet point of watershed.

Input drainage direction map indicates the "aspect" for each cell.
Multiplying positive values by 45 will give the direction in degrees
that the surface runoff will travel from that cell. The value -1
indicates that the cell is a depression area. Other negative values
indicate that surface runoff is leaving the boundaries of the current
geographic region. The absolute value of these negative cells indicates
the direction of flow. This raster map is generated from
*[r.watershed](r.watershed.md)*.

Output raster map values of one (1) indicate the watershed basin. Values
of zero (0) are not in the watershed basin.

## NOTES

In the context of this program, a watershed basin is the region upstream
of an outlet point. Thus, if the user chooses an outlet point on a hill
slope, the resulting map will be a thin silver of land representing the
overland slope uphill of the point.

## EXAMPLE

A watershed in the [North Carolina sample
dataset](https://grass.osgeo.org/download/data/) region:

```sh
g.region raster=elev_lid792_1m -p
# the watershed outlet position should be placed on a stream (from
# accumulation map):
r.watershed elev_lid792_1m threshold=5000 accumulation=accum_5K drainage=draindir_5K basin=basin_5K
r.water.outlet input=draindir_5K output=basin_A30 coordinates=638740.423248,220271.519225

d.mon wx0
d.rast map=accum_5K
d.rast map=basin_A30

# overlay with transparency
r.colors map=basin_A30 color=grey
d.his h=accum_5K i=basin_A30

# report outlet size in ha
r.report map=basin_A30 units=h
```

![Figure: Watershed draped over flow accumulation](r_water_outlet.png)  
*Figure: Watershed draped over flow accumulation*

## SEE ALSO

*[r.wateroutlet.lessmem](https://grass.osgeo.org/grass8/manuals/addons/r.wateroutlet.lessmem.html)
(addon), [d.where](d.where.md), [r.basins.fill](r.basins.fill.md),
[r.watershed](r.watershed.md), [r.topidx](r.topidx.md)*

## AUTHOR

Charles Ehlschlaeger, U.S. Army Construction Engineering Research
Laboratory
