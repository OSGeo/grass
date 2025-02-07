## DESCRIPTION

*i.landsat.toar* is used to transform the calibrated digital number of
Landsat imagery products to top-of-atmosphere radiance or
top-of-atmosphere reflectance and temperature (band 6 of the sensors TM
and ETM+). Optionally, it can be used to calculate the at-surface
radiance or reflectance with atmospheric correction (DOS method).

Usually, to do so the production date, the acquisition date, and the
solar elevation are needed. Moreover, for Landsat-7 ETM+ it is also
needed the gain (high or low) of the nine respective bands.

Optionally (recommended), the data can be read from metadata file (.met
or MTL.txt) for all Landsat MSS, TM, ETM+ and OLI/TIRS. However, if the
solar elevation is given the value of the metadata file is overwritten.
This is necessary when the data in the .met file is incorrect or not
accurate. Also, if acquisition or production dates are not found in the
metadata file then the command line values are used.

**Attention**: Any null value or smaller than QCALmin in the input
raster is set to null in the output raster and it is not included in the
equations.

**Attention**: This module does **not** respect the current region
settings, in order to have the largest possible sample of pixels from
where to get the darkest one of the scene and perform the DOS
correction. To limit the results to a custom region, the user is advised
to clip the results (with
[r.clip](https://grass.osgeo.org/grass-stable/manuals/addons/r.clip.html),
for instance) or to define the region first, import the images with
region cropping, and then running the module.

## Uncorrected at-sensor values (method=uncorrected, default)

The standard geometric and radiometric corrections result in a
calibrated digital number (QCAL = DN) images. To further standardize the
impact of illumination geometry, the QCAL images are first converted
first to at-sensor radiance and then to at-sensor reflectance. The
thermal band is first converted from QCAL to at-sensor radiance, and
then to effective at-sensor temperature in Kelvin degrees.

Radiometric calibration converts QCAL to **at-sensor radiance**, a
radiometric quantity measured in W/(m² \* sr \* µm) using the equations:

- gain = (Lmax - Lmin) / (QCALmax - QCALmin)
- bias = Lmin - gain \* QCALmin
- radiance = gain \* QCAL + bias

where, *Lmax* and *Lmin* are the calibration constants, and *QCALmax*
and *QCALmin* are the highest and the lowest points of the range of
rescaled radiance in QCAL.

Then, to calculate **at-sensor reflectance** the equations are:

- sun_radiance = \[Esun \* sin(e)\] / (PI \* d^2)
- reflectance = radiance / sun_radiance

where, *d* is the earth-sun distance in astronomical units, *e* is the
solar elevation angle, and *Esun* is the mean solar exoatmospheric
irradiance in W/(m² \* µm).

## Simplified at-surface values (method=dos\[1-4\])

Atmospheric correction and reflectance calibration remove the path
radiance, i.e. the stray light from the atmosphere, and the spectral
effect of solar illumination. To output these simple **at-surface
radiance** and **at-surface reflectance**, the equations are (not for
thermal bands):

- sun_radiance = TAUv \* \[Esun \* sin(e) \* TAUz + Esky\] / (PI \* d^2)
- radiance_path = radiance_dark - percent \* sun_radiance
- radiance = (at-sensor_radiance - radiance_path)
- reflectance = radiance / sun_radiance

where, *percent* is a value between 0.0 and 1.0 (usually 0.01), *Esky*
is the diffuse sky irradiance, *TAUz* is the atmospheric transmittance
along the path from the sun to the ground surface, and *TAUv* is the
atmospheric transmittance along the path from the ground surface to the
sensor. *radiance_dark* is the at-sensor radiance calculated from the
darkest object, i.e. DN with a least 'dark_parameter' (usually 1000)
pixels for the entire image. The values are,

- DOS1: TAUv = 1.0, TAUz = 1.0 and Esky = 0.0
- DOS2: TAUv = 1.0, Esky = 0.0, and TAUz = sin(e) for all bands with
  maximum wave length less than 1. (i.e. bands 4-6 MSS, 1-4 TM, and 1-4
  ETM+) other bands TAUz = 1.0
- DOS3: TAUv = exp\[-t/cos(sat_zenith)\], TAUz = exp\[-t/sin(e)\], Esky
  = rayleigh
- DOS4: TAUv = exp\[-t/cos(sat_zenith)\], TAUz = exp\[-t/sin(e)\], Esky
  = PI \* radiance_dark

**Attention**: Output radiance remain untouched (i.e. no set to 0.0 when
it is negative) then they are possible negative values. However, output
reflectance is set to 0.0 when is obtained a negative value.

## NOTES

The output raster cell values can be rescaled with the **scale**
parameter (e.g., with 100 in case of using reflectance output in
*i.gensigset*).

### On Landsat-8 metadata file

NASA reports a structure of the L1G Metadata file
([LDCM-DFCB-004.pdf](http://landsat.usgs.gov/documents/LDCM-DFCB-004.pdf))
for Landsat Data Continuity Mission (i.e. Landsat-8).

NASA retains in MIN_MAX_RADIANCE group the necessary information to
transform Digital Numbers (DN) in radiance values. Then,
*i.landsat.toar* replaces the possible standard values with the metadata
values. The results match with the values reported by the metada file in
RADIOMETRIC_RESCALING group.

Also, NASA reports the same values of reflectance for all bands in
max-min values and in gain-bias values. This is strange that all bands
have the same range of reflectance. Also, they wrote in the web page as
to calculate reflectance directly from DN, first with
RADIOMETRIC_RESCALING values and second divided by sin(sun_elevation).

This is a simple rescaling

- reflectance = radiance / sun_radiance = (DN \* RADIANCE_MULT +
  RADIANCE_ADD) / sun_radiance
- now reflectance = DN \* REFLECTANCE_MULT + REFLECTANCE_ADD
- then REFLECTANCE_MULT = RADIANCE_MULT / sun_radiance
- and REFLECTANCE_ADD = RADIANCE_ADD / sun_radiance

The problem arises when we need ESUN values (not provided) to compute
sun_radiance and DOS. We assume that REFLECTANCE_MAXIMUM corresponds to
the RADIANCE_MAXIMUM, then

- REFLECTANCE_MAXIMUM / sin(e) = RADIANCE_MAXIMUM / sun_radiance
- Esun = (PI \* d^2) \* RADIANCE_MAXIMUM / REFLECTANCE_MAXIMUM

where *d* is the earth-sun distance provided by metadata file or
computed inside the program.

The *i.landsat.toar* reverts back the NASA rescaling to continue using
Lmax, Lmin, and Esun values to compute the constant to convert DN to
radiance and radiance to reflectance with the "traditional" equations
and simple atmospheric corrections. **Attention**: When MAXIMUM values
are not provided, *i.landsat.toar* tries to calculate Lmax, Lmin, and
Esun from RADIOMETRIC_RESCALING (in tests the results were the same).

### Calibration constants

In verbose mode (flag **--verbose**), the program write basic satellite
data and the parameters used in the transformations.

Production date is not an exact value but it is necessary to apply
correct calibration constants, which were changed in the dates:

- Landsat-1 MSS: never
- Landsat-2 MSS: July 16, 1975
- Landsat-3 MSS: June 1, 1978
- Landsat-4 MSS: August 26, 1982 and April 1, 1983
- Landsat-4 TM: August 1, 1983 and January 15, 1984
- Landsat-5 MSS: April 6, 1984 and November 9, 1984
- Landsat-5 TM: May 4, 2003 and April, 2 2007
- Landsat-7 ETM+: July 1, 2000
- Landsat-8 OLI/TIRS: launched in 2013

## EXAMPLES

### Metadata file examples

Transform digital numbers of Landsat-7 ETM+ in band rasters 203_30.1,
203_30.2 \[...\] to uncorrected at-sensor reflectance in output files
203_30.1_toar, 203_30.2_toar \[...\] and at-sensor temperature in output
files 293_39.61_toar and 293_39.62_toar:

```sh
i.landsat.toar input=203_30. output=_toar \
  metfile=p203r030_7x20010620.met
```

or

```sh
i.landsat.toar input=L5121060_06020060714. \
  output=L5121060_06020060714_toar \
  metfile=L5121060_06020060714_MTL.txt
```

or

```sh
i.landsat.toar input=LC80160352013134LGN03_B output=toar \
  metfile=LC80160352013134LGN03_MTL.txt sensor=oli8 date=2013-05-14
```

### DOS1 example

DN to reflectance using DOS1:

```sh
# rename channels or make a copy to match i.landsat.toar's input scheme:
g.copy raster=lsat7_2002_10,lsat7_2002.1
g.copy raster=lsat7_2002_20,lsat7_2002.2
g.copy raster=lsat7_2002_30,lsat7_2002.3
g.copy raster=lsat7_2002_40,lsat7_2002.4
g.copy raster=lsat7_2002_50,lsat7_2002.5
g.copy raster=lsat7_2002_61,lsat7_2002.61
g.copy raster=lsat7_2002_62,lsat7_2002.62
g.copy raster=lsat7_2002_70,lsat7_2002.7
g.copy raster=lsat7_2002_80,lsat7_2002.8
```

Calculation of reflectance values from DN using DOS1 (metadata obtained
from
[p016r035_7x20020524.met.gz](https://grassbook.org/wp-content/uploads/ncexternal/landsat/2002/p016r035_7x20020524.met.gz)):

```sh
i.landsat.toar input=lsat7_2002. output=lsat7_2002_toar. sensor=tm7 \
  method=dos1 date=2002-05-24 sun_elevation=64.7730999 \
  product_date=2004-02-12 gain=HHHLHLHHL
```

The resulting Landsat channels are named
`lsat7_2002_toar.1 .. lsat7_2002_toar.8`.

## REFERENCES

- Chander G., B.L. Markham and D.L. Helder, 2009: Remote Sensing of
  Environment, vol. 113
- Chander G.H. and B. Markham, 2003: IEEE Transactions On Geoscience And
  Remote Sensing, vol. 41, no. 11.
- Chavez P.S., jr. 1996: Image-based atmospheric corrections - Revisited
  and Improved. Photogrammetric Engineering and Remote Sensing 62(9):
  1025-1036.
- Huang et al: At-Satellite Reflectance, 2002: A First Order
  Normalization Of Landsat 7 ETM+ Images.
- R. Irish: [Landsat 7. Science Data Users
  Handbook](http://landsathandbook.gsfc.nasa.gov/orbit_coverage/).
  February 17, 2007; 15 May 2011.
- Markham B.L. and J.L. Barker, 1986: Landsat MSS and TM
  Post-Calibration Dynamic Ranges, Exoatmospheric Reflectances and
  At-Satellite Temperatures. EOSAT Landsat Technical Notes, No. 1.
- Moran M.S., R.D. Jackson, P.N. Slater and P.M. Teillet, 1992: Remote
  Sensing of Environment, vol. 41.
- Song et al, 2001: Classification and Change Detection Using Landsat TM
  Data, When and How to Correct Atmospheric Effects? Remote Sensing of
  Environment, vol. 75.

## SEE ALSO

*[i.atcorr](i.atcorr.md), [i.colors.enhance](i.colors.enhance.md),
[r.mapcalc](r.mapcalc.md), [r.in.gdal](r.in.gdal.md)*

[Landsat Data
Dictionary](https://lta.cr.usgs.gov/DD/landsat_dictionary.html) by USGS

## AUTHOR

E. Jorge Tizado (ej.tizado unileon es), Dept. Biodiversity and
Environmental Management, University of León, Spain
