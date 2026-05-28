## DESCRIPTION

*i.aster.toar* calculates the Top Of Atmosphere (TOA) reflectance for
Terra-ASTER L1B in the visible, NIR and SWIR bands (9+1 bands) and
brightness temperature for the TIR bands (5 bands), all from L1B DN
values. It is useful to apply after import of original ASTER imagery
that is generally in standard DN values range.

The order of input bands is

- VNIR: 1,2,3N,3B
- SWIR: 4,5,6,7,8,9
- TIR: 10,11,12,13,14

in one comma-separated list.

## NOTES

Internally, a gain code is defined to modify gains according to spectral
bands following the GeoSystems GmbH ATCOR Ver. 2.0 Calibration Files.
The function is defined in gain_aster.c file.

```sh
/*Gain Code*/
    /*0 - High (Not Applicable for band 10-14: TIR)*/
    /*1 - Normal*/
    /*2 - Low 1(Not Applicable for band 10-14: TIR)*/
    /*3 - Low 2(Not Applicable for Band 1-3N/B and 10-14)*/
```

## SEE ALSO

*[i.landsat.toar](i.landsat.toar.md), [r.in.aster](r.in.aster.md)*

ASTER sensor data download: [ASTER: Advanced Spaceborne Thermal Emission
and Reflection Radiometer](https://asterweb.jpl.nasa.gov/)

## AUTHOR

Yann Chemin, CSU, Australia
