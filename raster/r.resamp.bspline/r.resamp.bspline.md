## DESCRIPTION

*r.resamp.bspline* performs a bilinear/bicubic spline interpolation with
Tykhonov regularization. The input is a raster surface map, e.g.
elevation, temperature, precipitation etc. Output is a raster map.
Optionally, only input NULL cells are interpolated, useful to fill NULL
cells, an alternative to *[r.fillnulls](r.fillnulls.md)*. Using the
**-n** flag to only interpolate NULL cells will considerably speed up
the module.

The input raster map is read at its native resolution, the output raster
map will be produced for the current computational region set with
*[g.region](g.region.md)*. A raster mask, if present, will be respected.
Masked values will be treated like other NULL cells in both the input
and output maps.

Spline step values **ew_step** for the east-west direction and
**ns_step** for the north-south direction should not be smaller than the
east-west and north-south resolutions of the input map. For a raster map
without NULL cells, 1 \* resolution can be used, but check for
undershoots and overshoots. For very large areas with missing values
(NULL cells), larger spline step values may be required, but most of the
time the defaults (1.5 x resolution) should be fine.

The Tykhonov regularization parameter (**lambda**) acts to smooth the
interpolation. With a small **lambda**, the interpolated surface closely
follows observation points; a larger value will produce a smoother
interpolation. Reasonable values are 0.0001, 0.001, 0.005, 0.01, 0.02,
0.05, 0.1 (needs more testing). For seamless NULL cell interpolation, a
small value is required. The default **lambda** value is set to 0.01.

From a theoretical perspective, the interpolating procedure takes place
in two parts: the first is an estimate of the linear coefficients of a
spline function; these are derived from the observation points using a
least squares regression; the second is the computation of the
interpolated surface (or interpolated vector points). As used here, the
splines are 2D piece-wise non-zero polynomial functions calculated
within a limited 2D area. The length of each spline step is defined by
**ew_step** for the east-west direction and **ns_step** for the
north-south direction. For optimal performance, the spline step values
should be no less than the east-west and north-south resolutions of the
input map. Each non-NULL cell observation is modeled as a linear
function of the non-zero splines in the area around the observation. The
least squares regression predicts the coefficients of these linear
functions. Regularization avoids the need to have one one observation
and one coefficient for each spline (in order to avoid instability).

A cross validation "leave-one-out" analysis is available to help to
determine the optimal **lambda** value that produces an interpolation
that best fits the original observation data. The more points used for
cross-validation, the longer the time needed for computation. Empirical
testing indicates a threshold of a maximum of 100 points is recommended.
Note that cross validation can run very slowly if more than 100
observations are used. The cross-validation output reports *mean* and
*rms* of the residuals from the true point value and the estimated from
the interpolation for a fixed series of **lambda** values. No vector nor
raster output will be created when cross-validation is selected.

## EXAMPLES

### Basic interpolation

```sh
r.resamp.bspline input=raster_surface output=interpolated_surface method=bicubic
```

A bicubic spline interpolation will be done and a raster map with
estimated (i.e., interpolated) values will be created.

### Interpolation of NULL cells and patching

General procedure:

```sh
# set region to area with NULL cells, align region to input map
g.region n=north s=south e=east w=west align=input -p
# interpolate NULL cells
r.resamp.bspline -n input=input_raster output=interpolated_nulls method=bicubic
# set region to area with NULL cells, align region to input map
g.region raster=input -p
# patch original map and interpolated NULLs
r.patch input=input_raster,interpolated_nulls output=input_raster_gapfilled
```

### Interpolation of NULL cells and patching (NC data)

In this example, the SRTM elevation map in the North Carolina sample
dataset is filtered for outlier elevation values; missing pixels are
then re-interpolated to obtain a complete elevation map:

```sh
g.region raster=elev_srtm_30m -p
d.mon wx0
d.histogram elev_srtm_30m

r.univar -e elev_srtm_30m

# remove too low elevations (esp. lakes)
# Threshold: thresh = Q1 - 1.5 * (Q3 - Q1)
r.mapcalc "elev_srtm_30m_filt = if(elev_srtm_30m < 50.0, null(), elev_srtm_30m)"

# verify
d.histogram elev_srtm_30m_filt
d.erase
d.rast elev_srtm_30m_filt

r.resamp.bspline -n input=elev_srtm_30m_filt output=elev_srtm_30m_complete \
  method=bicubic

d.histogram elev_srtm_30m_complete
d.rast elev_srtm_30m_complete
```

### Estimation of **lambda** parameter with a cross validation process

A random sample of points should be generated first with
*[r.random](r.random.md)*, and the current region should not include
more than 100 non-NULL random cells.

```sh
r.resamp.bspline -c input=input_raster
```

## REFERENCES

- Brovelli M. A., Cannata M., and Longoni U.M., 2004, LIDAR Data
  Filtering and DTM Interpolation Within GRASS, Transactions in GIS,
  April 2004, vol. 8, iss. 2, pp. 155-174(20), Blackwell Publishing Ltd
- Brovelli M. A. and Cannata M., 2004, Digital Terrain model
  reconstruction in urban areas from airborne laser scanning data: the
  method and an example for Pavia (Northern Italy). Computers and
  Geosciences 30, pp.325-331
- Brovelli M. A e Longoni U.M., 2003, Software per il filtraggio di dati
  LIDAR, Rivista dell'Agenzia del Territorio, n. 3-2003, pp. 11-22 (ISSN
  1593-2192)
- Antolin R. and Brovelli M.A., 2007, LiDAR data Filtering with GRASS
  GIS for the Determination of Digital Terrain Models. Proceedings of
  Jornadas de SIG Libre, Girona, España. CD ISBN: 978-84-690-3886-9

## SEE ALSO

*[r.fillnulls](r.fillnulls.md), [r.resamp.rst](r.resamp.rst.md),
[r.resamp.interp](r.resamp.interp.md),
[v.surf.bspline](v.surf.bspline.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHORS

Markus Metz  
  
based on *[v.surf.bspline](v.surf.bspline.md)* by  
Maria Antonia Brovelli, Massimiliano Cannata, Ulisse Longoni, Mirko
Reguzzoni, Roberto Antolin
