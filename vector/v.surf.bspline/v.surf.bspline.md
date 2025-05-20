## DESCRIPTION

*v.surf.bspline* performs a bilinear/bicubic spline interpolation with
Tykhonov regularization. The **input** is a 2D or 3D vector *point*
layer. Values to interpolate can be the z values of 3D points or the
values in a user-specified attribute column in a 2D or 3D vector layer.
Output can be a raster (**raster_output**) or vector (**output**) layer.
Optionally, a "sparse point" vector layer can be input which indicates
the location of **output** vector points.

## NOTES

From a theoretical perspective, the interpolating procedure takes place
in two parts: the first is an estimate of the linear coefficients of a
spline function, which is derived from the observation points using a
least squares regression. The second is the computation of the
interpolated surface or interpolated vector points. As used here, the
splines are 2D piece-wise non-zero polynomial functions calculated
within a limited, 2D area. The length, in mapping units, of each spline
step is defined by **ew_step** for the east-west direction and
**ns_step** for the north-south direction. For optimal performance, the
length of spline step should be no less than the distance between
observation points. Each vector point observation is modeled as a linear
function of the non-zero splines in the area around the observation. The
least squares regression predicts the coefficients of these linear
functions. Regularization avoids the need to have one observation and
one coefficient for each spline (in order to avoid instability).

With regularly distributed data points, a spline step corresponding to
the maximum distance between two points in both the east and north
directions is sufficient. However, data points are often not regularly
distributed and require statistical regularization or estimation. In
such cases, v.surf.bspline will attempt to minimize the gradient of
bilinear splines or the curvature of bicubic splines in areas lacking
point observations. As a general rule, the spline step length should be
greater than the mean distance between observation points (twice the
distance between points is a good starting point). Separate east-west
and north-south spline step length arguments allows the user to account
for some degree of anisotropy in the distribution of observation points.
Short spline step lengths, especially spline step lengths that are less
than the distance between observation points, can greatly increase the
processing time.

The maximum number of splines for each direction at each time is fixed,
regardless of the spline step length. As the total number of splines
increases (i.e., with small spline step lengths), the region is
automatically split into subregions for interpolation. Each subregion
can contain no more than 150x150 splines. To avoid subregion boundary
problems, subregions are created to partially overlap each other. A
weighted mean of observations, based on point locations, is calculated
within each subregion.

The Tykhonov regularization parameter, **lambda_i**, acts to smooth the
interpolation. With a small **lambda_i**, the interpolated surface
closely follows observation points; a larger value will produce a
smoother interpolation.

The input can be a 2D or 3D point vector layer. If input is 3D and
**column** is not given than z-coordinates are used for interpolation.
Parameter **column** is required when input is 2D vector layer.

*v.surf.bspline* can produce raster (**raster_output**) OR vector
**output** but NOT simultaneously. Note that topology is not built for
output point vector layer. If required, the topology can be built using
*[v.build](v.build.md)*.

If output is a point vector layer and **sparse** is not specified, the
output vector layer will contain points at the same locations as
observation points in the input layer but the values of the output
points will be interpolated values. If a **sparse** point vector layer
is specified, the output vector layer will contain points at the same
locations as the sparse vector layer points. The values will be those of
the interpolated raster surface at those points.

A cross validation "leave-one-out" analysis is available to help to
determine the optimal **lambda_i** value that produces an interpolation
that best fits the original observation data. The more points used for
cross-validation, the longer the time needed for computation. Empirical
testing indicates a threshold of a maximum of 100 points is recommended.
Note that cross validation can run very slowly if more than 100
observations are used. The cross-validation output reports *mean* and
*rms* of the residuals from the true point value and the estimated from
the interpolation for a fixed series of **lambda_i** values. No vector
nor raster output will be created when cross-validation is selected.

## EXAMPLES

### Basic interpolation

```sh
v.surf.bspline input=point_vector output=interpolate_surface method=bicubic
```

A bicubic spline interpolation will be done and a point vector layer
with estimated (i.e., interpolated) values will be created.

### Basic interpolation and raster output with a longer spline step

```sh
v.surf.bspline input=point_vector raster=interpolate_surface ew_step=25 ns_step=25
```

A bilinear spline interpolation will be done with a spline step length
of 25 map units. An interpolated raster layer will be created at the
current region resolution.

### Estimation of lambda_i parameter with a cross validation process

```sh
v.surf.bspline -c input=point_vector
```

### Estimation on sparse points

```sh
v.surf.bspline input=point_vector sparse=sparse_points output=interpolate_surface
```

An output layer of vector points will be created, corresponding to the
sparse vector layer, with interpolated values.

### Using attribute values instead z-coordinates

```sh
v.surf.bspline input=point_vector raster=interpolate_surface layer=1 \
  column=attrib_column
```

The interpolation will be done using the values in *attrib_column*, in
the table associated with layer 1.

### North Carolina dataset example using z-coordinates for interpolation

```sh
g.region region=rural_1m res=2 -p
v.surf.bspline input=elev_lid792_bepts raster=elev_lid792_rast \
  ew_step=5 ns_step=5 method=bicubic lambda_i=0.1
```

## KNOWN ISSUES

In order to avoid RAM memory problems, an auxiliary table is needed for
recording some intermediate calculations. This requires the *GROUP BY*
SQL function is used. This function is not supported by the DBF driver.
For this reason, vector output (**output**) is not permitted with the
DBF driver. There are no problems with the raster map output from the
DBF driver.

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

*[v.surf.idw](v.surf.idw.md), [v.surf.rst](v.surf.rst.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHORS

Original version (s.bspline.reg) in GRASS 5.4: Maria Antonia Brovelli,
Massimiliano Cannata, Ulisse Longoni, Mirko Reguzzoni  
Update for GRASS 6 and improvements: Roberto Antolin
