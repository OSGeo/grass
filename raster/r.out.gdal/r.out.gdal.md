## DESCRIPTION

*r.out.gdal* allows a user to export a GRASS raster map layer into any
GDAL supported raster map format. If a GRASS raster map is exported for
a particular application, the application's native format would be
preferable. GeoTIFF is supported by a wide range of applications (see
also **NOTES** on GeoTIFF below).

To specify multiple creation options use a comma separated list
(*createopt="TFW=YES,COMPRESS=DEFLATE"*).

For possible *createopt* and *metaopt* parameters please consult the
individual [supported
formats](https://gdal.org/en/stable/drivers/raster/) pages on the GDAL
website. The *createopt* parameter may be used to create TFW or World
files ("TFW=YES","WORLDFILE=ON").

*r.out.gdal* also supports the export of multiband rasters as a group,
when the imagery group's name is entered as input. (created imagery
groups with the *[i.group](i.group.md)* module)

As with most GRASS raster modules, the current region extents and region
resolution are used, and a raster mask is respected if present. Use
*[g.region](g.region.md)*'s "align=", or "raster=" options if you need
to realign the region settings to match the original map's before
export.

## SUPPORTED RASTER FORMATS

The set of [supported raster
formats](https://gdal.org/en/stable/drivers/raster/) written by
*r.out.gdal* depends on the local GDAL installation, printed with the
*-l* flag. Available may be (incomplete list):

```sh
  AAIGrid: Arc/Info ASCII Grid
  BMP: MS Windows Device Independent Bitmap
  BSB: Maptech BSB Nautical Charts
  DTED: DTED Elevation Raster
  ELAS: ELAS
  ENVI: ENVI .hdr Labelled
  FIT: FIT Image
  GIF: Graphics Interchange Format (.gif)
  GTiff: GeoTIFF
  HDF4Image: HDF4 Dataset
  HFA: Erdas Imagine Images (.img)
  JPEG2000: JPEG-2000 part 1 (ISO/IEC 15444-1)
  JPEG: JPEG JFIF
  MEM: In Memory Raster
  MFF2: Atlantis MFF2 (HKV) Raster
  MFF: Atlantis MFF Raster
  NITF: National Imagery Transmission Format
  PAux: PCI .aux Labelled
  PCIDSK: PCIDSK Database File
  PNG: Portable Network Graphics
  PNM: Portable Pixmap Format (netpbm)
  VRT: Virtual Raster
  XPM: X11 PixMap Format
```

## NOTES

Out of the GDAL data types, the closest match for GRASS CELL, FCELL and
DCELL rasters are respectively Int32, Float32 and Float64. These are not
exact equivalents, but they will preserve the maximum possible data
range and number of decimal places for each respective GRASS raster data
type. Please keep in mind that not all CELL rasters will require Int32 -
e.g., 0-255 CELL raster are covered by the Byte *type* as well.
Moreover, some GDAL-supported formats do not support all the data types
possible in GDAL and GRASS. Use *[r.info](r.info.md)* to check the data
type and range for your GRASS raster, refer to specific format
documentation (on the [GDAL website](https://gdal.org/)), format
vendor's documentation, and e.g. the Wikipedia article *[Typical
boundaries of primitive integral
types](https://en.wikipedia.org/wiki/C_syntax#Typical_boundaries_of_primitive_integral_types)*
for details.

### Ranges of GDAL data types

```sh
  GDAL data type           minimum      maximum

  Byte                   0          255
  UInt16                 0       65,535
  Int16, CInt16            -32,768       32,767
  UInt32                 0    4,294,967,295
  Int32, CInt32     -2,147,483,648    2,147,483,647
  Float32, CFloat32        -3.4E38       3.4E38
  Float64, CFloat64      -1.79E308         1.79E308
```

If there is a need to keep file sizes small, use the simplest data type
covering the data range of the raster(s) to be exported, e.g., if
suitable use Byte rather than UInt16; use Int16 rather than Int32; or
use Float32 rather than Float64. In addition, the COMPRESS **createopt**
used can have a very large impact on the size of the output file.

Some software may not recognize all of the compression methods available
for a given file format, and certain compression methods may only be
supported for certain data types (depends on vendor and version).

If the export settings are set such that data loss would occur in the
output file (i.e, due to the particular choice of data type and/or file
type), the normal behaviour of *r.out.gdal* in this case would be to
issue an error message describing the problem and exit without
exporting. The **-f** flag allows raster export even if some of the data
loss tests are not passed, and warnings are issued instead of errors.

*r.out.gdal* exports may appear all black or gray on initial display in
other GIS software. This is not a bug of *r.out.gdal*, but often caused
by the default color table assigned by that software. The default color
table may be grayscale covering the whole range of possible values which
is very large for e.g. Int32 or Float32. E.g. stretching the color table
to actual min/max would help (sometimes under symbology).

### Adding overviews to speed up map display in other software

Adding overviews with [`gdaladdo`](https://www.gdal.org/gdaladdo.html)
after exporting can speed up display. The overviews are created
internally within the exported file. The amount of levels (power-of-two
factors) are controlled with the **overviews** parameter. The higher the
overview level defined by the user the more lower resolution internal
overviews are added Note that other software might create their own
overviews, ignoring existing overviews.

### GeoTIFF caveats

GeoTIFF exports can only be displayed by standard image viewers if the
GDAL data type was set to Byte and the GeoTIFF contains either one or
three bands. All other data types and numbers of bands can be properly
read with GIS software only. Although GeoTIFF files usually have a .tif
extension, these files are not necessarily images but first of all
spatial raster datasets, e.g. land cover or elevation.

When writing out multi-band GeoTIFF images for users of ESRI software or
ImageMagick, the interleaving mode should be set to "pixel" using
*createopt="INTERLEAVE=PIXEL"*. BAND interleaving is slightly more
efficient, but not supported by some applications. This issue only
arises when writing out multi-band imagery groups.

Classic TIFF format supports only files with up to 4GB. Files that
exceed this limit (in compressed or uncompressed form) need to be
exported with *createopt="BIGTIFF=YES"*. BIGTIFF is available if GDAL is
built with libtiff &= 4.0.

### Improving GeoTIFF compatibility

To create a GeoTIFF that is highly compatible with various other GIS
software packages, it is recommended to keep the GeoTIFF file as simple
as possible. You will have to experiment with which options your
software is compatible with, as this varies widely between vendors and
versions. Long term, the less metadata you have to remove the more
self-documenting (and useful) the dataset will be.

Here are some things to try:

- Create a World file with `createopt="TFW=YES"`.
- Do not use GeoTIFF internal compression. Other GIS software often
  supports only a subset of the available compression methods with the
  supported methods differing between GIS software packages.
  Unfortunately this means the output image can be rather huge, but the
  file can be compressed with software like `zip`, `gnuzip`, or `bzip2`.
- Skip exporting the color table. Color tables are not always properly
  rendered, particularly for type UInt16, and the GeoTIFF file can
  appear completely black. If you are lucky the problematic software
  package has a method to reset the color table and assign a new color
  table (sometimes called symbology).
- Keep metadata simple with `createopt="PROFILE=GeoTIFF"` or
  `createopt="PROFILE=BASELINE"`. With BASELINE no GDAL or GeoTIFF tags
  will be written and a World file is required (*createopt="TFW=YES"*).

### Offset/scale parameters

Offset is only relevant if not zero. Scale is only relevant if not 1.

## EXAMPLES

### Export the integer raster basin_50K map to GeoTIFF format

See also [GeoTIFF format
description](https://gdal.org/en/stable/drivers/raster/gtiff.html)
(GDAL):

```sh
g.region raster=basin_50K -p
r.out.gdal input=basin_50K output=basin_50K.tif
```

### Export the integer raster landclass96 map to Cloud Optimized GeoTIFF format

See also [Cloud Optimized GeoTIFF (COG) format
description](https://gdal.org/en/stable/drivers/raster/cog.html) (GDAL):

```sh
g.region -p raster=landclass96
r.out.gdal -fmt input=landclass96 output=landclass96.tif format=COG overviews=4
```

### Export a DCELL raster map in GeoTIFF format suitable for ESRI software

```sh
g.region raster=elevation -p
r.out.gdal in=elevation output=elevation.tif createopt="PROFILE=GeoTIFF,TFW=YES"
```

### Export a raster map in "Deflate" compressed GeoTIFF format

```sh
g.region raster=elevation -p
r.out.gdal in=elevation output=elevation.tif createopt="COMPRESS=DEFLATE"
```

### Export a large raster map in LZW compressed (Big) GeoTIFF format

```sh
# integer map export
g.region raster=zipcodes -p
# Using PREDICTOR 2 for integer maps can further reduce file size
r.out.gdal in=zipcodes output=zipcodes.tif createopt="COMPRESS=LZW,PREDICTOR=2,BIGTIFF=YES"

# floating point map export
g.region raster=elevation -p
# Using PREDICTOR 3 for floating point data can further reduce file size
r.out.gdal in=elevation output=elevation.tif createopt="COMPRESS=LZW,PREDICTOR=3,BIGTIFF=YES"
```

### Export a raster map with internal overview in "Deflate" compressed GeoTIFF format

```sh
g.region raster=elevation -p
# overviews=5 corresponds to 'gdaladdo ... 2 4 8 16 32'
r.out.gdal in=elevation output=elevation.tif createopt="COMPRESS=DEFLATE" overviews=5
```

### Export R,G,B imagery bands in GeoTIFF format suitable for ESRI software

```sh
i.group group=nc_landsat_rgb input=lsat7_2002_30,lsat7_2002_20,lsat7_2002_10
g.region raster=lsat7_2002_30 -p
r.out.gdal in=nc_landsat_rgb output=nc_landsat_rgb.tif type=Byte \
  createopt="PROFILE=GeoTIFF,INTERLEAVE=PIXEL,TFW=YES"
```

### Export group of image maps as multi-band file

```sh
g.list group
i.group group=tm7 subgroup=tm7 input=tm7_10,tm7_20,tm7_30,tm7_40,tm7_50,tm7_60,tm7_70
i.group -l tm7
g.region raster=tm7_10 -p
r.out.gdal tm7 output=lsat_multiband.tif
gdalinfo lsat_multiband.tif
```

### Export RGB with alpha channel that encodes NULL cells

When exporting exporting RGB data rather than GIS data for Web
applications or generally the scope of visualization, the alpha channel
is of use. Here the export type is commonly the Byte data type.

When exporting data with *r.out.gdal*, assigning a **nodata** value
(specific parameter of the module) means that any band values equal to
this nodata value will be interpreted as nodata. Using an additional
alpha channel means that all pixels with an alpha value of 0 are
transparent. The alpha channel thus represents per-pixel encoding of
nodata, just like the GRASS raster mask or per-raster null file. That
means when using an alpha channel, you do not need to "free up" any
particular value, but you can use any value you like to replace NULL
cells, as long as the value can be represented by the Byte data type. It
does not matter if that value is already present in any of the input
bands.

Hence for "visual-only" RGB data export it is needed to create an
additional alpha channel that encodes all NULL cells and in the RGB
bands to be exported replace NULL cells with some value in the range
0-255. For example:

```sh
# for simplicity variables are used
RMAP="lsat7_2000_30"
GMAP="lsat7_2000_20"
BMAP="lsat7_2000_10"

OUTNAME="lsat7_2000_RGBA.tif"

# extract alpha
r.mapcalc "out_a = if(isnull($RMAP) || isnull($GMAP) || isnull($BMAP), 0, 255)"

# replace NULL cells with a valid value, extract colors

# exporting 8 bit RGB data, not GIS data, therefore the `#` operator:
r.mapcalc "out_r = if(isnull($RMAP), 0, #$RMAP)"
r.mapcalc "out_g = if(isnull($GMAP), 0, #$GMAP)"
r.mapcalc "out_b = if(isnull($BMAP), 0, #$BMAP)"

# create group for export
i.group group=out_rgba input=out_r,out_g,out_b,out_a

# remove any mask because this works only if there are
# no NULL cells in the bands to be exported
r.mask -r

# export the group:
# add PROFILE=BASELINE to createopt to produce a standard TIFF file
# without any GTiff extensions
r.out.gdal input=out_rgba output=$OUTNAME -cm createopt="PHOTOMETRIC=RGB,ALPHA=YES"
gdalinfo $OUTNAME
```

The resulting GeoTIFF file can be used e.g. for Web server applications.

### Export the floating point raster elevation map to ERDAS/IMG format

See also [Erdas Imagine .img format
description](https://gdal.org/en/stable/drivers/raster/hfa.html) (GDAL):

```sh
g.region raster=elevation -p
r.out.gdal input=elevation output=elelevation.img format=HFA type=Float32
```

### Export raster map with offset/scale assigned

An offset value can be assigned to output raster data by **offset**
parameter. Similarly a scale value can be assigned by **scale**
parameter.

```sh
# produce raster map
g.region n=100 s=0 e=100 w=0 res=1
r.random.cells output=random distance=1.0
r.info -r random

# export raster data with offset/scale assigned
r.out.gdal input=random output=random.tif offset=100 scale=0.01
```

Check result by *gdalinfo* command line tool:

```sh
gdalinfo random.tif
...
Band 1 Block=100x40 Type=UInt16, ColorInterp=Palette
  Description = random
    Computed Min/Max=1.000,3661.000
  NoData Value=65535
  Offset: 100,   Scale:0.01
...
```

## GDAL RELATED ERROR MESSAGES

- "ERROR 6: SetColorInterpretation() not supported for this dataset.":
  This *may* indicate that the color table was not written properly. But
  usually it will be correct and the message can be ignored.
- "ERROR 6: SetNoDataValue() not supported for this dataset.": The
  selected output format does not support "no data". It is recommended
  to use a different output format if your data contains NULLs.
- "Warning 1: Lost metadata writing to GeoTIFF ... too large to fit in
  tag.": The color table metadata may be too large. It is recommended to
  simplify or not write the color table, or use a different output
  format or the flags **-c** and **-m**.

## REFERENCES

GDAL Pages: <https://gdal.org>

## SEE ALSO

[GDAL supported raster
formats](https://gdal.org/en/stable/drivers/raster/index.html) and [GDAL
supported vector
formats](https://gdal.org/en/stable/drivers/vector/index.html)

*[r.out.ascii](r.out.ascii.md), [r.out.bin](r.out.bin.md),
[r.out.mat](r.out.mat.md), [r.out.png](r.out.png.md),
[r.out.ppm](r.out.ppm.md), [r.pack](r.pack.md)*

## AUTHORS

Vytautas Vebra (oliver4grass at gmail.com)  
Markus Metz (improved nodata logic)
