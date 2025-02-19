## DESCRIPTION

*r.buildvrt* builds a virtual raster (VRT) that is a mosaic of the list
of input raster maps. The purpose of such a VRT is to provide fast
access to small subsets of the VRT, also with multiple simultaneous read
requests.

## NOTES

*r.buildvrt* creates a list of raster maps that can be located in
different mapsets. The output is a read-only link to the original raster
maps which is only valid if the original raster maps remain in the
originally indicated mapset. A VRT can also be built from raster maps
registered with *[r.external](r.external.md)*. However, GRASS VRTs built
from external registered data (see below) are known to have performance
issues.

Reading the whole VRT is slower than reading the equivalent single
raster map. Only reading small parts of the VRT provides a performance
benefit.

A GRASS virtual raster can be regarded as a simplified version of GDAL's
[virtual raster
format](https://gdal.org/en/stable/drivers/raster/vrt.html). The GRASS
equivalent is simpler because issues like nodata, projection,
resolution, resampling, masking are already handled by native GRASS
raster routines.

## EXAMPLES

### VRT from a DEM in the North Carolina sample dataset

In this exercise a low resolution DEM is resampled to a high resolution
DEM. This is subsequently cut into small tiles and from that a virtual
tile mosaik is created:

```sh
# set the computational region to elevation map
g.region raster=elev_state_500m -p
# enforce higher resolution
g.region res=50 -p
# resample the 500 meter DEM to 50 meter resolution
r.resamp.interp input=elev_state_500m output=elev_state_50m method=bilinear
# create tiles from resulting large 50 m elevation map
r.tile input=elev_state_50m output=elev_state_50m_tile_ width=1000 height=1000 overlap=0
# for convenience, dump list of tile names to a file
g.list type=raster pattern=elev_state_50m_tile_* output=tilelist.csv
# build a mosaik as VRT from tile list
r.buildvrt file=tilelist.csv output=elev_state_50m_vrt
```

## KNOWN ISSUES

Users may experience significant performance degradation with virtual
rasters built with *r.buildvrt* over GDAL-linked (*r.external*) raster
maps, especially on slower file systems with latency like NFS.
Performance degradation may also occur on local file systems, but is
usually less severe. For such use cases consider using the GRASS GIS
addon
[r.buildvrt.gdal](https://grass.osgeo.org/grass-stable/manuals/addons/r.buildvrt.gdal.html)
or building GDAL VRTs, e.g. with *gdalbuildvrt*.

## SEE ALSO

*[r.tile](r.tile.md), [r.patch](r.patch.md), [r.external](r.external.md)
[r.buildvrt.gdal](https://grass.osgeo.org/grass-stable/manuals/addons/r.buildvrt.gdal.html)*

The equivalent GDAL utility
*[gdalbuildvrt](https://gdal.org/en/stable/programs/gdalbuildvrt.html)*

## AUTHOR

Markus Metz  
Sponsored by [mundialis](https://www.mundialis.de)
