## DESCRIPTION

*i.landsat.acca* implements the **Automated Cloud-Cover Assessment**
(ACCA) Algorithm from Irish (2000) with the constant values for pass
filter one from Irish et al. (2006). To do this, it needs Landsat band
numbers 2, 3, 4, 5, and 6 (or band 61 for Landsat-7 ETM+) which have
already been processed from DN into reflectance and band-6 temperature
with *[i.landsat.toar](i.landsat.toar.md)*).

The ACCA algorithm gives good results over most of the planet with the
exception of ice sheets because ACCA operates on the premise that clouds
are colder than the land surface they cover. The algorithm was designed
for Landsat-7 ETM+ but because reflectance is used it is also useful for
Landsat-4/5 TM.

## NOTES

*i.landsat.acca* works in the current region settings.

## EXAMPLES

Run the standard ACCA algorithm with filling of small cloud holes (the
**-f** flag): With per-band reflectance raster maps named
`226_62.toar.1, 226_62.toar.2, ...` and LANDSAT-7 thermal band
`226_62.toar.61`, outputting to a new raster map named `226_62.acca`:

```sh
i.landsat.toar sensor=7 gain=HHHLHLHHL date=2003-04-07 \
  product_date=2008-11-27 band_prefix=226_62 solar_elevation=49.51654

i.landsat.acca -f band_prefix=226_62.toar output=226_62.acca
```

## REFERENCES

- Irish R.R., Barker J.L., Goward S.N., and Arvidson T., 2006.
  Characterization of the Landsat-7 ETM+ Automated Cloud-Cover
  Assessment (ACCA) Algorithm. Photogrammetric Engineering and Remote
  Sensing vol. 72(10): 1179-1188.
- Irish, R.R., 2000. Landsat 7 Automatic Cloud Cover Assessment. In S.S.
  Shen and M.R. Descour (Eds.): Algorithms for Multispectral,
  Hyperspectral, and Ultraspectral Imagery VI. Proceedings of SPIE,
  4049: 348-355.

## SEE ALSO

*[i.atcorr](i.atcorr.md), [i.landsat.toar](i.landsat.toar.md)*

## AUTHOR

E. Jorge Tizado (ej.tizado unileon es), Dept. Biodiversity and
Environmental Management, University of León, Spain
