## DESCRIPTION

*r.in.gdal* allows a user to create a GRASS GIS raster map layer, or
imagery group, from any GDAL supported raster map format, with an
optional title. The imported file may also be used to create a new
project (previously called location).

### GDAL supported raster formats

Full details on all GDAL supported formats are available at:

<https://gdal.org/en/stable/drivers/raster/>

Selected formats out of the more than 140 supported formats:

```sh
Long Format Name                              Code           Creation  Georeferencing Maximum file size
---------------------------------------------+-------------+----------+--------------+-----------------
ADRG/ARC Digitilized Raster Graphics          ADRG              Yes      Yes          --
Arc/Info ASCII Grid                           AAIGrid           Yes      Yes          2GB
Arc/Info Binary Grid (.adf)                   AIG               No       Yes          --
Arc/Info Export E00 GRID                      E00GRID           No       Yes          --
ArcSDE Raster                                 SDE               No       Yes          --
ASCII Gridded XYZ                             XYZ               Yes      Yes          --
BSB Nautical Chart Format (.kap)              BSB               No       Yes          --
CEOS (Spot for instance)                      CEOS              No       No           --
DB2                                           DB2               Yes      Yes          No limits
DODS / OPeNDAP                                DODS              No       Yes          --
EarthWatch/DigitalGlobe .TIL                  TIL               No       No           --
ENVI .hdr Labelled Raster                     ENVI              Yes      Yes          No limits
Envisat Image Product (.n1)                   ESAT              No       No           --
EOSAT FAST Format                             FAST              No       Yes          --
Epsilon - Wavelet compressed images           EPSILON           Yes      No           --
Erdas 7.x .LAN and .GIS                       LAN               No       Yes          2GB
ERDAS Compressed Wavelets (.ecw)              ECW               Yes      Yes
Erdas Imagine (.img)                          HFA               Yes      Yes          No limits
Erdas Imagine Raw                             EIR               No       Yes          --
ERMapper (.ers)                               ERS               Yes      Yes
ESRI .hdr Labelled                            EHdr              Yes      Yes          No limits
EUMETSAT Archive native (.nat)                MSGN              No       Yes
FIT                                           FIT               Yes      No           --
FITS (.fits)                                  FITS              Yes      No           --
Fuji BAS Scanner Image                        FujiBAS           No       No           --
GDAL Virtual (.vrt)                           VRT               Yes      Yes          --
Generic Binary (.hdr Labelled)                GENBIN            No       No           --
GeoPackage                                    GPKG              Yes      Yes          No limits
Geospatial PDF                                PDF               Yes      Yes          --
GMT Compatible netCDF                         GMT               Yes      Yes          2GB
Golden Software Surfer 7 Binary Grid          GS7BG             Yes      Yes          4GiB
Graphics Interchange Format (.gif)            GIF               Yes      No           2GB
GRASS Raster Format                           GRASS             No       Yes          --
GSat File Format                              GFF               No       No           --
Hierarchical Data Format Release 4 (HDF4)     HDF4              Yes      Yes          2GiB
Hierarchical Data Format Release 5 (HDF5)     HDF5              No       Yes          2GiB
Idrisi Raster                                 RST               Yes      Yes          No limits
ILWIS Raster Map (.mpr,.mpl)                  ILWIS             Yes      Yes          --
Image Display and Analysis (WinDisp)          IDA               Yes      Yes          2GB
In Memory Raster                              MEM               Yes      Yes
Intergraph Raster                             INGR              Yes      Yes          2GiB
IRIS                                          IRIS              No       Yes          --
Japanese DEM (.mem)                           JDEM              No       Yes          --
JAXA PALSAR Product Reader (Level 1.1/1.5)    JAXAPALSAR        No       No           --
JPEG2000 (.jp2, .j2k)                         JP2OpenJPEG       Yes      Yes
JPEG JFIF (.jpg)                              JPEG              Yes      Yes          4GiB
KMLSUPEROVERLAY                               KMLSUPEROVERLAY   Yes      Yes
MBTiles                                       MBTiles           Yes      Yes          --
Meta Raster Format                            MRF               Yes      Yes          --
Meteosat Second Generation                    MSG               No       Yes
MG4 Encoded Lidar                             MG4Lidar          No       Yes          --
Microsoft Windows Device Independent Bitmap   BMP               Yes      Yes          4GiB
Military Elevation Data (.dt0, .dt1, .dt2)    DTED              Yes      Yes          --
Multi-resolution Seamless Image Database      MrSID             No       Yes          --
NASA Planetary Data System                    PDS               No       Yes          --
NetCDF                                        netCDF            Yes      Yes          2GB
Netpbm (.ppm,.pgm)                            PNM               Yes      No           No limits
NITF                                          NITF              Yes      Yes          10GB
NLAPS Data Format                             NDF               No       Yes          No limits
NOAA NGS Geoid Height Grids                   NGSGEOID          No       Yes
NOAA Polar Orbiter Level 1b Data Set (AVHRR)  L1B               No       Yes          --
OGC Web Coverage Service                      WCS               No       Yes          --
OGC Web Map Service, and TMS, WorldWind, On EaWMS               No       Yes          --
OGC Web Map Tile Service                      WMTS              No       Yes          --
OGDI Bridge                                   OGDI              No       Yes          --
Oracle Spatial GeoRaster                      GEORASTER         Yes      Yes          No limits
OziExplorer .MAP                              MAP               No       Yes          --
OZI OZF2/OZFX3                                OZI               No       Yes          --
PCI Geomatics Database File                   PCIDSK            Yes      Yes          No limits
PCRaster                                      PCRaster          Yes      Yes
Planet Labs Mosaics API                       PLMosaic          No       Yes          --
Portable Network Graphics (.png)              PNG               Yes      No
PostGIS Raster (previously WKTRaster)         PostGISRaster     No       Yes          --
RadarSat2 XML (product.xml)                   RS2               No       Yes          4GB
Rasdaman                                      RASDAMAN          No       No           No limits
Rasterlite - Rasters in SQLite DB             Rasterlite        Yes      Yes          --
Raster Product Format/RPF (CADRG, CIB)        RPFTOC            No       Yes          --
R Object Data Store                           R                 Yes      No           --
ROI_PAC Raster                                ROI_PAC           Yes      Yes          --
R Raster (.grd)                               RRASTER           No       Yes          --
SAGA GIS Binary format                        SAGA              Yes      Yes          --
SAR CEOS                                      SAR_CEOS          No       Yes          --
Sentinel 1 SAR SAFE (manifest.safe)           SAFE              No       Yes          No limits
Sentinel 2                                    SENTINEL2         No       Yes          No limits
SGI Image Format                              SGI               Yes      Yes          --
SRTM HGT Format                               SRTMHGT           Yes      Yes          --
TerraSAR-X Complex SAR Data Product           COSAR             No       No           --
TerraSAR-X Product                            TSX               Yes      No           --
TIFF / BigTIFF / GeoTIFF (.tif)               GTiff             Yes      Yes          4GiB/None for BigTIFF
USGS ASCII DEM / CDED (.dem)                  USGSDEM           Yes      Yes          --
USGS Astrogeology ISIS cube (Version 3)       ISIS3             No       Yes          --
USGS SDTS DEM (*CATD.DDF)                     SDTS              No       Yes          --
Vexcel MFF                                    MFF               Yes      Yes          No limits
VICAR                                         VICAR             No       Yes          --
VTP Binary Terrain Format (.bt)               BT                Yes      Yes          --
WEBP                                          WEBP              Yes      No           --
WMO GRIB1/GRIB2 (.grb)                        GRIB              No       Yes          2GB
```

### Project Creation

*r.in.gdal* attempts to preserve coordinate reference system (CRS)
information when importing datasets if the source format includes CRS
information, and if the GDAL driver supports it. If the CRS of the
source dataset does not match the CRS of the current project *r.in.gdal*
will report an error message
(`Coordinate reference system of dataset does not appear to match current project`)
and then report the PROJ_INFO parameters of the source dataset.

If the user wishes to ignore the difference between the apparent
coordinate system of the source data and the current project, they may
pass the **-o** flag to override the CRS check.

If the user wishes to import the data with the full CRS definition, it
is possible to have r.in.gdal automatically create a new project based
on the CRS and extents of the file being read. This is accomplished by
passing the name to be used for the new project via the **project**
parameter. Upon completion of the command, a new project will have been
created (with only a PERMANENT mapset), and the raster will have been
imported with the indicated **output** name into the PERMANENT mapset.

### Support for Ground Control Points

In case the image contains Ground Control Points (GCP) they are written
to a POINTS file within an imagery group. They can directly be used for
[i.rectify](i.rectify.md).

The **target** option allows you to automatically re-project the GCPs
from their own CRS into another CRS read from the PROJ_INFO file of the
project name **target**.

If the **target** project does not exist, a new project will be created
matching the CRS definition of the GCPs. The target of the output group
will be set to the new project, and [i.rectify](i.rectify.md) can now be
used without any further preparation.

Some satellite images (e.g. NOAA/AVHRR, ENVISAT) can contain hundreds or
thousands of GCPs. In these cases thin plate spline coordinate
transformation is recommended, either before import with **gdalwarp
-tps** or after import with **i.rectify -t**.

### Map names: Management of offset and leading zeros

The **offset** parameter allows adding an offset to band number(s) which
is convenient in case of the import of e.g. a continuous time series
split across different input files.

The **num_digits** parameter allows defining the number of leading zeros
(zero padding) in case of band numbers (e.g., to turn `band.1` into
`band.001`).

## NOTES

Import of large files can be significantly faster when setting
**memory** to the size of the input file.

The *r.in.gdal* command does support the following features, as long as
the underlying format driver supports it:

### Color Table

Bands with associated colortables will have the color tables
transferred. Note that if the source has no colormap, r.in.gdal in GRASS
5.0 will emit no colormap. Use r.colors map=... color=grey to assign a
greyscale colormap. In a future version of GRASS r.in.gdal will likely
be upgraded to automatically emit greyscale colormaps.  

### Data Types

Most GDAL data types are supported. Float32 and Float64 type bands are
translated as GRASS floating point cells (but not double precision ...
this could be added if needed), and most other types are translated as
GRASS integer cells. This includes 16bit integer data sources. Complex
(some SAR signal data formats) data bands are translated to two floating
point cell layers (\*.real and \*.imaginary).  

### Georeferencing

If the dataset has affine georeferencing information, this will be used
to set the north, south, east and west edges. Rotational coefficients
will be ignored, resulting in incorrect positioning for rotated
datasets.  

#### Coordinate reference system

The dataset's CRS will be used to compare to the current project or to
define a new project. Internally GDAL represents CRS in OpenGIS Well
Known Text format. A large subset of the total set of GRASS CRSs are
supported.  

### Null Values

Raster bands for which a null value is recognised by GDAL will have the
null pixels transformed into GRASS style nulls during import. Many
generic formats (and formats poorly supported by GDAL) do not have a way
of recognising null pixels in which case r.null should be used after the
import.  

### GCPs

Datasets that have Ground Control Points will have them imported as a
POINTS file associated with the imagery group. Datasets with only one
band that would otherwise have been translated as a simple raster map
will also have an associated imagery group if there are ground control
points. The coordinate system of the ground control points is reported
by r.in.gdal but not preserved. It is up to the user to ensure that the
project established with i.target has a compatible coordinate system
before using the points with i.rectify.  

### Raster Attribute Tables

*r.in.gdal* can write out raster attribute tables as CSV files.
Moreover, information in raster attribute tables is automatically
imported as long as the field definitions contain information about how
to use a field, e.g. for color information or for labels.  

Planned improvements to *r.in.gdal* in the future include support for
reporting everything known about a dataset if the **output** parameter
is not set.

### Error Messages

*"ERROR: Input map is rotated - cannot import."*  
In this case the image must be first externally rotated, applying the
rotation info stored in the metadata field of the raster image file. For
example, the
[gdalwarp](https://gdal.org/en/stable/programs/gdalwarp.html) software
can be used to transform the map to North-up (note, there are several
gdalwarp parameters to select the resampling algorithm):

```sh
gdalwarp rotated.tif northup.tif
```

*"ERROR: Coordinate reference system of dataset does not appear to match
the current project."*  
You need to create a project whose CRS matches the data you wish to
import. Try using **project** parameter to create a new project based
upon the CRS information in the file. If desired, you can then reproject
it to another project with *r.proj*. Alternatively you can override this
error by using the **-o** flag.

*"WARNING: G_set_window(): Illegal latitude for North"*  
Latitude/Longitude projects in GRASS can not have regions which exceed
90° North or South. Non-georeferenced imagery will have coordinates
based on the images's number of pixels: 0,0 in the bottom left;
cols,rows in the top right. Typically imagery will be much more than 90
pixels tall and so the GIS refuses to import it. If you are sure that
the data is appropriate for your Lat/Lon project and intend to reset the
map's bounds with the *r.region* module directly after import you may
use the **-l** flag to constrain the map coordinates to legal values.
While the resulting bounds and resolution will likely be wrong for your
map the map's data will be unaltered and safe. After resetting to known
bounds with *r.region* you should double check them with *r.info*,
paying special attention to the map resolution. In most cases you will
want to import into the datafile's native CRS, or into a simple XY
project and use the Georectifaction tools (*i.rectify* et al.) to
re-project into the target project. The **-l** flag should *only* be
used if you know the CRS is correct but the internal georeferencing has
gotten lost, and you know the what the map's bounds and resolution
should be beforehand.

## EXAMPLES

### ECAD Data

The [European Climate Assessment and Dataset (ECAD)
project](https://www.ecad.eu/) provides climate data for Europe ranging
from 1950 - 2015 or later. To import the different chunks of data
provided by the project as netCDF files, the offset parameter can be
used to properly assign numbers to the series of daily raster maps from
1st Jan 1950 (in case of importing the ECAD data split into multi-annual
chunks). The ECAD data must be imported into a LatLong project.

By using the *num_digits* parameter leading zeros are added to the map
name numbers, allowing for chronological numbering of the imported
raster map layers, so that *g.list* lists them in the correct order.
Here, use *num_digits=5* to have a 5 digit suffix with leading zeros
(00001 - 99999).

```sh
# Import of ECAD data split into chunks
# Import precipitation data
r.in.gdal -o input=rr_0.25deg_reg_1950-1964_v12.0.nc output=precipitation num_digits=5 offset=0
r.in.gdal -o input=rr_0.25deg_reg_1965-1979_v12.0.nc output=precipitation num_digits=5 offset=5479
r.in.gdal -o input=rr_0.25deg_reg_1980-1994_v12.0.nc output=precipitation num_digits=5 offset=10957
r.in.gdal -o input=rr_0.25deg_reg_1995-2015_v12.0.nc output=precipitation num_digits=5 offset=16436

# Import air pressure data
r.in.gdal -o input=pp_0.25deg_reg_1950-1964_v12.0.nc output=air_pressure num_digits=5 offset=0
r.in.gdal -o input=pp_0.25deg_reg_1965-1979_v12.0.nc output=air_pressure num_digits=5 offset=5479
r.in.gdal -o input=pp_0.25deg_reg_1980-1994_v12.0.nc output=air_pressure num_digits=5 offset=10957
r.in.gdal -o input=pp_0.25deg_reg_1995-2015_v12.0.nc output=air_pressure num_digits=5 offset=16436

# Import min temperature data
r.in.gdal -o input=tn_0.25deg_reg_1950-1964_v12.0.nc output=temperatur_min num_digits=5 offset=0
r.in.gdal -o input=tn_0.25deg_reg_1965-1979_v12.0.nc output=temperatur_min num_digits=5 offset=5479
r.in.gdal -o input=tn_0.25deg_reg_1980-1994_v12.0.nc output=temperatur_min num_digits=5 offset=10957
r.in.gdal -o input=tn_0.25deg_reg_1995-2015_v12.0.nc output=temperatur_min num_digits=5 offset=16436

# Import max temperature data
r.in.gdal -o input=tx_0.25deg_reg_1950-1964_v12.0.nc output=temperatur_max num_digits=5 offset=0
r.in.gdal -o input=tx_0.25deg_reg_1965-1979_v12.0.nc output=temperatur_max num_digits=5 offset=5479
r.in.gdal -o input=tx_0.25deg_reg_1980-1994_v12.0.nc output=temperatur_max num_digits=5 offset=10957
r.in.gdal -o input=tx_0.25deg_reg_1995-2015_v12.0.nc output=temperatur_max num_digits=5 offset=16436

# Import mean temperature data
r.in.gdal -o input=tg_0.25deg_reg_1950-1964_v12.0.nc output=temperatur_mean num_digits=5 offset=0
r.in.gdal -o input=tg_0.25deg_reg_1965-1979_v12.0.nc output=temperatur_mean num_digits=5 offset=5479
r.in.gdal -o input=tg_0.25deg_reg_1980-1994_v12.0.nc output=temperatur_mean num_digits=5 offset=10957
r.in.gdal -o input=tg_0.25deg_reg_1995-2015_v12.0.nc output=temperatur_mean num_digits=5 offset=16436
```

### GTOPO30 DEM

To avoid the GTOPO30 data being read incorrectly, you can add a new line
"PIXELTYPE SIGNEDINT" in the .HDR to force interpretation of the file as
signed rather than unsigned integers. Then the .DEM file can be
imported. Finally, e.g. the 'terrain' color table can be assigned to the
imported map with *r.colors*.

### GLOBE DEM

To import [GLOBE DEM tiles](http://www.ngdc.noaa.gov/mgg/topo/elev/)
(approx 1km resolution, better than GTOPO30 DEM data), the user has to
download additionally the related [HDR
file(s)](http://www.ngdc.noaa.gov/mgg/topo/elev/esri/hdr/). Finally,
e.g. the 'terrain' color table can be assigned to the imported map with
*r.colors*. See also their [DEM
portal](http://www.ngdc.noaa.gov/mgg/dem/demportal.html).

### Raster file import over network

Since GDAL 2.x it is possible to import raster data over the network
(see [GDAL Virtual File
Systems](https://gdal.org/en/stable/user/virtual_file_systems.html))
including [Cloud Optimized GeoTIFF](https://cogeo.org/), i.e. access
uncompressed and compressed raster data via a http(s) or ftp connection.
As an example the import of the global SRTMGL1 V003 tiles at 1 arc
second (about 30 meters) resolution, void-filled:

```sh
r.in.gdal /vsicurl/https://www.datenatlas.de/geodata/public/srtmgl1/srtmgl1.003.tif output=srtmgl1_v003_30m memory=2000
g.region raster=srtmgl1_v003_30m -p
r.colors srtmgl1_v003_30m color=srtm_plus
```

### HDF

The import of HDF bands requires the specification of the individual
bands as seen by GDAL:

```sh
# Example MODIS FPAR
gdalinfo MOD15A2.A2003153.h18v04.004.2003171141042.hdf
...
Subdatasets:
  SUBDATASET_1_NAME=HDF4_EOS:EOS_GRID:"MOD15A2.A2003153.h18v04.004.2003171141042.hdf":MOD_Grid_MOD15A2:Fpar_1km
  SUBDATASET_1_DESC=[1200x1200] Fpar_1km MOD_Grid_MOD15A2 (8-bit unsigned integer)
  SUBDATASET_2_NAME=HDF4_EOS:EOS_GRID:"MOD15A2.A2003153.h18v04.004.2003171141042.hdf":MOD_Grid_MOD15A2:Lai_1km
  SUBDATASET_2_DESC=[1200x1200] Lai_1km MOD_Grid_MOD15A2 (8-bit unsigned integer)
...

# import of first band, here FPAR 1km:
r.in.gdal HDF4_EOS:EOS_GRID:"MOD15A2.A2003153.h18v04.004.2003171141042.hdf":MOD_Grid_MOD15A2:Fpar_1km \
          out=fpar_1km_2003_06_02
# ... likewise for other HDF bands in the file.
```

## REFERENCES

GDAL Pages: <https://gdal.org>

## SEE ALSO

*[r.colors](r.colors.md), [r.import](r.import.md),
[r.in.ascii](r.in.ascii.md), [r.in.bin](r.in.bin.md),
[r.null](r.null.md), [t.register](t.register.md)*

GRASS GIS Wiki page: Import of [Global
datasets](https://grasswiki.osgeo.org/wiki/Global_datasets)

## AUTHOR

[Frank Warmerdam](https://wiki.osgeo.org/wiki/User:Warmerda)
([email](mailto:warmerdam-AT-pobox-dot-com)).
