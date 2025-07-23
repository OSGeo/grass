## DESCRIPTION

*i.modis.qc* extracts Requested Quality Assessment flags from the
following MODIS products: MOD09A1, MOD09Q1, MOD11A1, MOD11A2, MOD13A2,
MOD13Q1, MCD43B2. This does include MOD09A1 QA_state_500m layer (see
Notes).  
Added MOD09GA support in 2016, it follows MOD09A1 and its StateQA, but
does not have BRDF State QA, instead has Salt Pan State QA.

### MOD09A1 and MOD09Q1

```sh
MOD09A1/Q1: MODLAND QA Bits. bits=[0-1]
```

- \[00\]= class 0: Corrected product produced at ideal quality -- all
  bands
- \[01\]= class 1: Corrected product produced at less than ideal quality
  -- some or all bands
- \[10\]= class 2: Corrected product NOT produced due to cloud effect --
  all bands
- \[11\]= class 3: Corrected product NOT produced due to other reasons
  -- some or all bands maybe be fill value (Note that a value of \[11\]
  overrides a value of \[01\])

```sh
MOD09Q1: Cloud State. bits=[2-3] 
```

- \[00\]= class 0: Clear -- No clouds
- \[01\]= class 1: Cloudy
- \[10\]= class 2: Mixed
- \[11\]= class 3: Not Set ; Assumed Clear

```sh
MOD09Q1: Band-wise Data Quality 250m bits=[4-7][8-11]
MOD09A1: Band-wise Data Quality 500m bits=[2-5][6-9][10-13][14-17][18-21][22-25][26-29]
```

- \[0000\]= class 0: highest quality
- \[0111\]= class 1: noisy detector
- \[1000\]= class 2: dead detector; data interpolated in L1B
- \[1001\]= class 3: solar zenith ≥ 86 degrees
- \[1010\]= class 4: solar zenith ≥ 85 and \< 86 degrees
- \[1011\]= class 5: missing input
- \[1100\]= class 6: internal constant used in place of climatological
  data for at least one atmospheric constant
- \[1101\]= class 7: correction out of bounds, pixel constrained to
  extreme allowable value
- \[1110\]= class 8: L1B data faulty
- \[1111\]= class 9: not processed due to deep ocean or cloud
- Class 10-15: Combination of bits unused

```sh
MOD09A1/Q1: Atmospheric correction bit=[12]/[30]
```

- \[0\]= class 0: Not Corrected product
- \[1\]= class 1: Corrected product

```sh
MOD09A1/Q1: Adjacency correction bit=[13]/[31]
```

- \[0\]= class 0: Not Corrected product
- \[1\]= class 1: Corrected product

```sh
MOD09Q1: Different orbit from 500m product, bit=[14]
```

- \[0\]= class 0: same orbit as 500m
- \[1\]= class 1: different orbit from 500m

```sh
MOD09A1s: Cloud State bits=[0-1]
```

- \[00\]= class 0: clear
- \[01\]= class 1: cloudy
- \[10\]= class 2: mixed
- \[11\]= class 3: not set, assumed clear

```sh
MOD09A1s: Cloud shadow bits=[2]
```

- \[0\]= class 0: no
- \[1\]= class 1: yes

```sh
MOD09A1s: Land/Water Flag bits=[3-5]
```

- \[000\]= class 0: Shallow ocean
- \[001\]= class 1: Land
- \[010\]= class 2: Ocean coastlines and lake shorelines
- \[011\]= class 3: Shallow inland water
- \[100\]= class 4: Ephemeral water
- \[101\]= class 5: Deep inland water
- \[110\]= class 6: Continental/moderate ocean
- \[111\]= class 7: Deep ocean

```sh
MOD09A1s: Aerosol Quantity bits=[6-7]
```

- \[00\]= class 0: Climatology
- \[01\]= class 1: Low
- \[10\]= class 2: Average
- \[11\]= class 3: High

```sh
MOD09A1s: Cirrus detected bits=[8-9]
```

- \[00\]= class 0: None
- \[01\]= class 1: Small
- \[10\]= class 2: Average
- \[11\]= class 3: High

```sh
MOD09A1s: Internal Cloud Algorithm Flag bits=[10]
```

- \[0\]= class 0: No cloud
- \[1\]= class 1: Cloud

```sh
MOD09A1s: Internal Fire Algorithm Flag bits=[11]
```

- \[0\]= class 0: No fire
- \[1\]= class 1: Fire

```sh
MOD09A1s: MOD35 snow/ice flag bits=[12]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

```sh
MOD09A1s: Pixel adjacent to cloud bits=[13]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

```sh
MOD09A1s: BRDF correction performed bits=[14]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

```sh
MOD09A1s: Internal Snow Mask bits=[15]
```

- \[0\]= class 0: No snow
- \[1\]= class 1: Snow

### MOD11A1

```sh
MOD11A1: Mandatory QA Flags bits=[0-1]
```

- \[00\]= class 0: LST produced, good quality, not necessary to examine
  more detailed QA
- \[01\]= class 1: LST produced, other quality, recommend examination of
  more detailed QA
- \[10\]= class 2: LST not produced due to cloud effects
- \[11\]= class 3: LST not produced primarily due to reasons other than
  cloud

```sh
MOD11A1: Data Quality Flag bits=[2-3]
```

- \[00\]= class 0: Good data quality of L1B in bands 31 and 32
- \[01\]= class 1: Other quality data
- \[10\]= class 2: TBD
- \[11\]= class 3: TBD

```sh
MOD11A1: Emis Error Flag bits=[4-5]
```

- \[00\]= class 0: Average emissivity error ≤ 0.01
- \[01\]= class 1: Average emissivity error ≤ 0.02
- \[10\]= class 2: Average emissivity error ≤ 0.04
- \[11\]= class 3: Average emissivity error \> 0.04

```sh
MOD11A1: LST Error Flag bits=[6-7]
```

- \[00\]= class 0: Average LST error ≤ 1
- \[01\]= class 1: Average LST error ≤ 2
- \[10\]= class 2: Average LST error ≤ 3
- \[11\]= class 3: Average LST error \> 3

### MOD11A2

```sh
MOD11A2: Mandatory QA Flags bits=[0-1]
```

- \[00\]= class 0: LST produced, good quality, not necessary to examine
  more detailed QA
- \[01\]= class 1: LST produced, other quality, recommend examination of
  more detailed QA
- \[10\]= class 2: LST not produced due to cloud effects
- \[11\]= class 3: LST not produced primarily due to reasons other than
  cloud

```sh
MOD11A2: Data Quality Flag bits=[2-3]
```

- \[00\]= class 0: Good data quality of L1B in 7 TIR bands
- \[01\]= class 1: Other quality data
- \[10\]= class 2: TBD
- \[11\]= class 3: TBD

```sh
MOD11A2: Emis Error Flag bits=[4-5]
```

- \[00\]= class 0: Average emissivity error ≤ 0.01
- \[01\]= class 1: Average emissivity error ≤ 0.02
- \[10\]= class 2: Average emissivity error ≤ 0.04
- \[11\]= class 3: Average emissivity error \> 0.04

```sh
MOD11A2: LST Error Flag bits=[6-7]
```

- \[00\]= class 0: Average LST error ≤ 1
- \[01\]= class 1: Average LST error ≤ 2
- \[10\]= class 2: Average LST error ≤ 3
- \[11\]= class 3: Average LST error \> 3

### MOD13A2

```sh
MOD13A2: Mandatory QA Flags 1km bits[0-1]
```

- \[00\]= class 0: VI produced, good quality
- \[01\]= class 1: VI produced, but check other QA
- \[10\]= class 2: Pixel produced, but most probably cloud
- \[11\]= class 3: Pixel not produced due to other reasons than clouds

```sh
MOD13A2: VI Usefulness Flag bits[2-5]
```

- \[0000\]= class 0: Highest quality
- \[0001\]= class 1: Lower quality
- \[0010\]= class 2: Decreasing quality
- \[0100\]= class 3: Decreasing quality
- \[1000\]= class 4: Decreasing quality
- \[1001\]= class 5: Decreasing quality
- \[1010\]= class 6: Decreasing quality
- \[1100\]= class 7: Lowest quality
- \[1101\]= class 8: Quality so low that it is not useful
- \[1110\]= class 9: L1B data faulty
- \[1111\]= class 10: Not useful for any other reason/not processed

```sh
MOD13A2: Aerosol quantity Flags 1km bits[6-7]
```

- \[00\]= class 0: Climatology
- \[01\]= class 1: Low
- \[10\]= class 2: Average
- \[11\]= class 3: High

```sh
MOD13A2: Adjacent cloud detected 1km bit[8]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13A2: Atmosphere BRDF correction performed 1km bit[9]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13A2: Mixed clouds 1km bit[10]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13A2: Land/Water Flags 1km bits[11-13]
```

- \[000\]= class 0: Shallow Ocean
- \[001\]= class 1: Land (Nothing else but land)
- \[010\]= class 2: Ocean Coastlines and lake shorelines
- \[011\]= class 3: Shallow inland water
- \[100\]= class 4: Ephemeral water
- \[101\]= class 5: Deep inland water
- \[110\]= class 6: Moderate or continental ocean
- \[111\]= class 7: Deep ocean

```sh
MOD13A2: Possible Snow/Ice 1km bits[14]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

```sh
MOD13A2: Possible Shadow 1km bits[15]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

### MOD13Q1

```sh
MOD13Q1: Mandatory QA Flags 250m bits[0-1]
```

- \[00\]= class 0: VI produced, good quality
- \[01\]= class 1: VI produced, but check other QA
- \[10\]= class 2: Pixel produced, but most probably cloud
- \[11\]= class 3: Pixel not produced due to other reasons than clouds

```sh
MOD13Q1: VI Usefulness Flag 250m bits[2-5]
```

- \[0000\]= class 0: Highest quality
- \[0001\]= class 1: Lower quality
- \[0010\]= class 2: Decreasing quality
- \[0100\]= class 3: Decreasing quality
- \[1000\]= class 4: Decreasing quality
- \[1001\]= class 5: Decreasing quality
- \[1010\]= class 6: Decreasing quality
- \[1100\]= class 7: Lowest quality
- \[1101\]= class 8: Quality so low that it is not useful
- \[1110\]= class 9: L1B data faulty
- \[1111\]= class 10: Not useful for any other reason/not processed

```sh
MOD13Q1: Aerosol quantity Flags 250m bits[6-7]
```

- \[00\]= class 0: Climatology
- \[01\]= class 1: Low
- \[10\]= class 2: Average
- \[11\]= class 3: High

```sh
MOD13Q1: Adjacent cloud detected 250m bit[8]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13Q1: Atmosphere BRDF correction performed 250m bit[9]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13Q1: Mixed clouds 250m bit[10]
```

- \[00\]= class 0: No
- \[01\]= class 1: Yes

```sh
MOD13Q1: Land/Water Flags 250m bits[11-13]
```

- \[000\]= class 0: Shallow Ocean
- \[001\]= class 1: Land (Nothing else but land)
- \[010\]= class 2: Ocean Coastlines and lake shorelines
- \[011\]= class 3: Shallow inland water
- \[100\]= class 4: Ephemeral water
- \[101\]= class 5: Deep inland water
- \[110\]= class 6: Moderate or continental ocean
- \[111\]= class 7: Deep ocean

```sh
MOD13Q1: Possible Snow/Ice 250m bits[14]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

```sh
MOD13Q1: Possible Shadow 250m bits[15]
```

- \[0\]= class 0: No
- \[1\]= class 1: Yes

### MCD43B2

```sh
MCD43B2: Albedo Quality Ancillary Platform Data 1km bits[0-3]
SDS: BRDF_Albedo_Ancillary
```

- \[0000\]= class 0: Satellite Platform: Terra
- \[0001\]= class 1: Satellite Platform: Terrra/Aqua
- \[0010\]= class 2: Satellite Platform: Aqua
- \[1111\]= class 15: Fill Value
- Classes 3-14: Not used

```sh
MCD43B2: Albedo Quality Ancillary Land/Water Data 1km bits[4-7]
SDS: BRDF_Albedo_Ancillary
```

- \[0000\] class 0: Shallow Ocean
- \[0001\] class 1: Land (Nothing else but land)
- \[0010\] class 2: Ocean and lake shorelines
- \[0011\] class 3: Shallow inland water
- \[0100\] class 4: Ephemeral water
- \[0101\] class 5: Deep inland water
- \[0110\] class 6: Moderate or continental ocean
- \[0111\] class 7: Deep ocean
- \[1111\] class 15: Fill Value
- Classes 8-14: Not used

```sh
MCD43B2: Albedo Quality Ancillary Sun Zenith Angle at Local Solar Noon Data 1km bits[8-14]
SDS: BRDF_Albedo_Ancillary
```

```sh
MCD43B2: Band-wise Albedo Quality Data 1km
SDS: BRDF_Albedo_Band_Quality
```

bits\[0-3\]\[4-7\]\[8-11\]\[12-15\]\[16-19\]\[20-23\]\[24-27\]  

- \[0000\]= class 0: best quality, 75% or more with best full inversions
- \[0001\]= class 1: good quality, 75% or more with full inversions
- \[0010\]= class 2: Mixed, 50% or less full inversions and 25% or less
  fill values
- \[0011\]= class 3: All magnitude inversions or 50% or less fill values
- \[0100\]= class 4: 75% or more fill values
- Classes 5-14: Not Used
- \[1111\]= class 15: Fill Value

## NOTES

In MOD09A1: It seems that cloud related info is not filled properly in
the standard QC (MOD09A1 in this module) since version 3, State-QA 500m
images (MOD09A1s in this module) should be used (see Vermote et al.,
2008). MOD11A2 quality control (QC) bands do not have a FillValue
(No-data) according to [MODIS Land Products
site](https://lpdaac.usgs.gov/dataset_discovery/modis/modis_products_table/mod11a2_v006).
However, the metadata of the QC bands (i.e.: `gdalinfo QC_band`) shows
`No-data=0`. This value is then transformed into GRASS NULLs when data
is imported through [r.in.gdal](r.in.gdal.md). Applying *i.modis.qc* on
those QC bands will not give the expected range of values in the
different QC bits. Therefore, before using *i.modis.qc*, the user needs
to set the NULL value in QC bands back to zero (i.e.:
`r.null map=QC_band null=0`) or just edit the metadata with GDAL
utilities before importing into GRASS GIS. This is a known issue for
MOD11A2 (8-day LST product), but other MODIS products might be affected
as well.

## TODO

Add more daily products.

## REFERENCES

- [MODIS
  Products](https://lpdaac.usgs.gov/dataset_discovery/modis/modis_products_table)
- Vermote E.F., Kotchenova S.Y., Ray J.P. MODIS Surface Reflectance
  User's Guide. Version 1.2. June 2008. MODIS Land Surface Reflectance
  Science Computing Facility. [Homepage](http://modis-sr.ltdri.org)

## SEE ALSO

*[i.vi](i.vi.md)*

## AUTHOR

Yann Chemin
