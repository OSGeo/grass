## DESCRIPTION

*r.relief* creates a raster shaded relief map based on current
resolution settings and on sun altitude, azimuth, and z-exaggeration
values entered by the user.

The parameters controlling the shading are:

1. An **input** raster map to provide elevation values for the shaded
    relief map. Typically, this would be a map layer of elevation;
    however, any raster map layer can be named.
2. The **altitude** of the sun in degrees above the horizon (a value
    between 0 and 90 degrees).
3. The **azimuth** of the sun in degrees to the east of north (a value
    between 0 and 360 degrees; clockwise from north)
4. The scaling parameter, which compensates for a different horizontal
    **scale** than vertical scale. If **scale** is a number, then the
    ewres and nsres are multiplied by that scale to calculate the
    shading. (Default=1.0 for equivalent horizontal and vertical
    scales.)
5. The **zscale** exaggeration factor that changes the apparent relief
    for the shaded relief map. This can be any positive (or negative)
    floating point value. (default=1.0)
6. Horizontal distances are calculated in meters, using geodesic
    distances for a latitude-longitude projection. With an elevation map
    measured in feet, the **units** option can be set to automatically
    convert meters to international feet (0.3048 meters = 1 foot) or
    survey feet (1200 / 3937 meters = 1 foot). The **units** parameter
    overrides the **scale** parameter.

*r.relief* assigns a grey-scale color table to the new shaded relief
map.

## NOTES

To visually improve the result of shade maps from low resolution
elevation models, use *[r.resamp.interp](r.resamp.interp.md)* with
bilinear or bicubic method to resample the DEM at higher resolution.
*r.relief* is then run on the resampled DEM.

The current mask is ignored.

## EXAMPLES

### Shaded relief map

In this example, the aspect map in the North Carolina sample dataset is
used to hillshade the elevation map:

```sh
g.region raster=elevation -p
r.relief input=elevation output=elevation_shade
```

![GRASS r.relief result (subset)](r_relief.png)  
*r.relief: shaded elevation map (subset)*

### Colorizing a shaded relief map

Color can be added later using *[r.shade](r.shade.md)* or
*[d.shade](d.shade.md)*:

```sh
r.shade shade=elevation_shade color=elevation output=elevation_shaded
```

### Using the scale factor in Latitude-Longitude

In Latitude-Longitude coordinate reference systems (or other non-metric
systems), the *scale* factor has to be used:

```sh
# Latitude-Longitude example
g.region raster=srtm -p
r.relief input=srtm output=srtm_shaded scale=111120
```

### Exporting shaded relief maps to GeoTIFF

The data range of shaded relief maps usually does not permit exporting
the map to GeoTIFF format along with its associated color table due to
limitations in the GeoTIFF format.

The most simple way to export it while even reducing the file size is to
export as palette byte map. This requires a conversion done in
*[r.mapcalc](r.mapcalc.md)*, using the \# operator to convert map
category values to their grey scale equivalents:

```sh
# using the map created above

# create new map from map category values
r.mapcalc expression="elevation_shade_byte = #elevation_shade"

# verify data range
r.info elevation_shade_byte

# assign grey color table
r.colors elevation_shade_byte color=grey

# export (optionally: createopt="COMPRESS=DEFLATE,BIGTIFF=YES")
r.out.gdal input=elevation_shade_byte createopt="COMPRESS=DEFLATE" \
           output=elevation_shade.tif

# add overview images in GeoTIFF file for faster zooming
gdaladdo --config GDAL_CACHEMAX 2000 elevation_shade.tif 2 4 8 16
```

## SEE ALSO

*[d.shade](d.shade.md), [d.his](d.his.md), [g.region](g.region.md),
[r.shade](r.shade.md), [r.blend](r.blend.md), [r.colors](r.colors.md),
[r.mapcalc](r.mapcalc.md), [r.resamp.interp](r.resamp.interp.md)*

## AUTHORS

Jim Westervelt, U.S. Army Construction Engineering Research Laboratory  
Markus Metz: Enhanced fast C version of r.relief for GRASS GIS 7
