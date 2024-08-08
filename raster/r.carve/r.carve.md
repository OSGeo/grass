## DESCRIPTION

*r.carve* accepts vector stream data as input, transforms them to
raster, and subtracts a default-depth + additional-depth from a DEM. If
the given width is more than 1 cell, it will carve the stream with the
given width. With the **-n** flag it should eliminate all flat cells
within the stream, so when and if the water gets into the stream it will
flow. The *points* option generates x,y,z for points which define the
stream with the z-value of the bottom of the carved-in stream. These
points can then be combined with contours to interpolate a new DEM with
better representation of valleys.

## NOTES

*r.carve* does not create a depressionless DEM because many depressions
are in flat areas and not in the streams.

## EXAMPLE

North Carolina sample dataset:

```
# set computational region
g.region raster=elev_lid792_1m -p

# digitize a ditch for the farm pond
echo "L  3 1
 638692.93595422 220198.90026383
 638737.42270627 220149.74706926
 638984.43306379 220148.19158842
 1     1" | v.in.ascii -n input=- output=ditch format=standard

# visualize original data
d.mon wx0
d.rast elev_lid792_1m
d.vect ditch

# carve
r.carve raster=elev_lid792_1m vector=ditch output=carved_dem width=3 depth=0.5

# visualize resulting carved DEM map
d.rast carved_dem

# visualize
r.relief input=elev_lid792_1m output=elev_lid792_1m_shaded
r.relief input=carved_dem output=carved_dem_shaded
d.rast elev_lid792_1m_shaded
d.erase
d.rast carved_dem_shaded

# flow accumulation
r.watershed elevation=elev_lid792_1m accumulation=elev_lid792_1m_accum
r.watershed elevation=carved_dem accumulation=carved_dem_accum
d.rast elev_lid792_1m_accum
d.erase
d.rast carved_dem_accum

# differences
r.mapcalc "accum_diff = elev_lid792_1m_accum - carved_dem_accum"
r.colors accum_diff color=differences
d.erase
d.rast accum_diff
```

  ------------------------------------------------------ ---------------------------------------------------------
               [![r.carve example: original                          [![r.carve example: original DEM
   DEM](r_carve_dem_orig.png){width="300" height="321"       shaded](r_carve_dem_orig_shaded.png){width="300"
           border="0"}](r_carve_dem_orig.png)\            height="321" border="0"}](r_carve_dem_orig_shaded.png)\
  *Fig: Original 1m LiDAR based DEM with vector streams      *Fig: Original 1m LiDAR based DEM shown as shaded
                       map on top*                                               terrain*

                [![r.carve example: carved                            [![r.carve example: carved DEM
  DEM](r_carve_dem_carved.png){width="300" height="321"     shaded](r_carve_dem_carved_shaded.png){width="300"
          border="0"}](r_carve_dem_carved.png)\          height="321" border="0"}](r_carve_dem_carved_shaded.png)\
             *Fig: Carved 1m LiDAR based DEM*            *Fig: Carved 1m LiDAR based DEM shown as shaded terrain*

          [![r.carve example: original DEM flow                     [![r.carve example: carved DEM flow
   accumulated](r_carve_dem_orig_accum.png){width="300"   accumulation](r_carve_dem_carved_accum.png){width="300"
  height="321" border="0"}](r_carve_dem_orig_accum.png)\ height="321" border="0"}](r_carve_dem_carved_accum.png)\
    *Fig: Flow accumulation in original 1m LiDAR based     *Fig: Flow accumulation in carved 1m LiDAR based DEM*
                           DEM*                          
  ------------------------------------------------------ ---------------------------------------------------------

## KNOWN ISSUES

The module does not operate yet in latitude-longitude coordinate
reference system. It has not been thoroughly tested, so not all options
may work properly - but this was the intention.

## REFERENCES

[Terrain modeling and Soil Erosion Simulations for Fort Hood and Fort
Polk test
areas](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/reports/cerl99/rep99.html),
by Helena Mitasova, Lubos Mitas, William M. Brown, Douglas M. Johnston,
GMSL (Report for CERL 1999)

## SEE ALSO

*[r.flow](r.flow.html), [r.fill.dir](r.fill.dir.html),
[r.watershed](r.watershed.html)*

## AUTHORS

Bill Brown (GMSL)\
GRASS 6 update: Brad Douglas
