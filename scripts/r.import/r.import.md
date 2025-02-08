## DESCRIPTION

*r.import* imports a map or selected bands from a GDAL raster datasource
into the current project (previously called location) and mapset. If the
coordinate reference system (CRS) of the input does not match the CRS of
the project, the input is reprojected into the current project. If the
CRS of the input does match the CRS of the project, the input is
imported directly with [r.in.gdal](r.in.gdal.md).

## NOTES

*r.import* checks the CRS metadata of the dataset to be imported against
the current project's CRS. If not identical a related error message is
shown.  
To override this projection check (i.e. to use current project's CRS) by
assuming that the dataset has the same CRS as the current project the
**-o** flag can be used. This is also useful when geodata to be imported
do not contain any CRS metadata at all. The user must be sure that the
CRS is identical in order to avoid introducing data errors.

### Resolution

*r.import* reports the estimated target resolution for each input band.
The estimated resolution will usually be some floating point number,
e.g. 271.301. In case option **resolution** is set to *estimated*
(default), this floating point number will be used as target resolution.
Since the target resolution should be typically the rounded estimated
resolution, e.g. 250 or 300 instead of 271.301, flag **-e** can be used
first to obtain the estimate without importing the raster bands. Then
the desired resolution is set with option **resolution_value** and
option **resolution**=*value*. For latlong projects, the resolution
might be set to arc seconds, e.g. 1, 3, 7.5, 15, and 30 arc seconds are
commonly used resolutions.

### Resampling methods

When reprojecting a map to a new spatial reference system, the projected
data is resampled with one of four different methods: nearest neighbor,
bilinear, bicubic interpolation or lanczos.

In the following, common use cases are:

**nearest** is the simplest method and the only possible method for
categorical data.

**bilinear** does linear interpolation and provides smoother output than
**nearest**. **bilinear** is recommended when reprojecting a DEM for
hydrological analysis or for surfaces where overshoots must be avoided,
e.g. precipitation should not become negative.

**bicubic** produces smoother output than **bilinear**, at the cost of
overshoots. Here, valid pixels that are adjacent to NULL pixels or edge
pixels are set to NULL.

**lanczos** produces the smoothest output of all methods and preserves
contrast best. **lanczos** is recommended for imagery. Both **bicubic**
and **lanczos** preserve linear features. With **nearest** or
**bilinear**, linear features can become zigzag features after
reprojection.

In the bilinear, bicubic and lanczos methods, if any of the surrounding
cells used to interpolate the new cell value are NULL, the resulting
cell will be NULL, even if the nearest cell is not NULL. This will cause
some thinning along NULL borders, such as the coasts of land areas in a
DEM. The bilinear_f, bicubic_f and lanczos_f interpolation methods can
be used if thinning along NULL edges is not desired. These methods "fall
back" to simpler interpolation methods along NULL borders. That is, from
lanczos to bicubic to bilinear to nearest.

For explanation of the **-l** flag, please refer to the
[r.in.gdal](r.in.gdal.md) manual.

When importing whole-world maps the user should disable map-trimming
with the **-n** flag. For further explanations of **-n** flag, please
refer the to [r.proj](r.proj.md) manual.

## EXAMPLES

### Import of SRTM V3 global data at 1 arc-seconds resolution

The SRTM V3 1 arc-second global data (~30 meters resolution) are
available from EarthExplorer (<https://earthexplorer.usgs.gov/>). The
SRTM collections are located under the "Digital Elevation" category.

Example for North Carolina sample dataset (the tile name is
"n35_w079_1arc_v3.tif"):

```sh
# set computational region to e.g. 10m elevation model:
g.region raster=elevation -p

# Import with reprojection on the fly. Recommended parameters:
# resample   Resampling method to use for reprojection - bilinear
# extent     Output raster map extent - region: extent of current region
# resolution Resolution of output raster map
#  - region: current region resolution - limit to g.region setting from above
r.import input=n35_w079_1arc_v3.tif output=srtmv3_resamp10m resample=bilinear \
  extent=region resolution=region title="SRTM V3 resampled to 10m resolution"

# beautify colors:
r.colors srtmv3_resamp10m color=elevation
```

### Import of WorldClim data

Import of a subset from WorldClim [Bioclim data
set](https://www.worldclim.org/data/bioclim.html), to be reprojected to
current project CRS (North Carolina sample dataset). Different
resolutions are available, in this example we use the 2.5 arc-minutes
resolution data. During import, we spatially subset the world data to
the North Carolina region using the *extent* parameter:

```sh
# download selected Bioclim data (2.5 arc-minutes resolution)
# optionally tiles are available for the 30 arc-sec resolution
wget https://geodata.ucdavis.edu/climate/worldclim/1_4/grid/cur/bio_2-5m_bil.zip

# extract BIO1 from package (BIO1 = Annual Mean Temperature):
unzip bio_2-5m_bil.zip bio1.bil bio1.hdr

# prior to import, fix broken WorldClim extent using GDAL tool
gdal_translate -a_ullr -180 90 180 -60 bio1.bil bio1_fixed.tif

# set computational region to North Carolina, 4000 m target pixel resolution
g.region -d res=4000 -ap

# subset to current region and reproject on the fly to current project CRS,
# using -n since whole-world map is imported:
r.import input=bio1_fixed.tif output=bioclim01 resample=bilinear \
         extent=region resolution=region -n

# temperature data are in °C * 10
r.info bioclim01
r.univar -e bioclim01
```

## SEE ALSO

*[r.in.gdal](r.in.gdal.md), [r.proj](r.proj.md)*

## AUTHORS

Markus Metz  
Improvements: Martin Landa, Anna Petrasova
