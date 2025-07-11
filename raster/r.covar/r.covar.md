## DESCRIPTION

*r.covar* outputs a covariance/correlation matrix for user-specified
raster map layer(s). The output can be printed, or saved by redirecting
output into a file.

The output is an N x N symmetric covariance (correlation) matrix, where
N is the number of raster map layers specified on the command line.

## NOTES

This module can be used as the first step of a principle components
transformation. The covariance matrix would be input into a system which
determines eigen values and eigen vectors. An NxN covariance matrix
would result in N real eigen values and N eigen vectors (each composed
of N real numbers).

The module
*[m.eigensystem](https://grass.osgeo.org/grass-stable/manuals/addons/m.eigensystem.html)*
in [GRASS GIS Addons](https://grass.osgeo.org/download/addons/) can be
installed and used to generate the eigenvalues and vectors.

## EXAMPLE

For example,

```sh
g.region raster=layer.1 -p
r.covar -r map=layer.1,layer.2,layer.3
```

would produce a 3x3 matrix (values are example only):

```sh
     1.000000  0.914922  0.889581
     0.914922  1.000000  0.939452
     0.889581  0.939452  1.000000
```

In the above example, the eigen values and corresponding eigen vectors
for the covariance matrix are:

```sh
component   eigen value               eigen vector
    1       1159.745202   <0.691002  0.720528  0.480511>
    2          5.970541   <0.711939 -0.635820 -0.070394>
    3        146.503197   <0.226584  0.347470 -0.846873>
```

The component corresponding to each vector can be produced using
*[r.mapcalc](r.mapcalc.md)* as follows:

```sh
r.mapcalc "pc.1 = 0.691002*layer.1 + 0.720528*layer.2 + 0.480511*layer.3"
r.mapcalc "pc.2 = 0.711939*layer.1 - 0.635820*layer.2 - 0.070394*layer.3"
r.mapcalc "pc.3 = 0.226584*layer.1 + 0.347470*layer.2 - 0.846873*layer.3"
```

Note that based on the relative sizes of the eigen values, *pc.1* will
contain about 88% of the variance in the data set, *pc.2* will contain
about 1% of the variance in the data set, and *pc.3* will contain about
11% of the variance in the data set. Also, note that the range of values
produced in *pc.1*, *pc.2*, and *pc.3* will not (in general) be the same
as those for *layer.1*, *layer.2*, and *layer.3*. It may be necessary to
rescale *pc.1*, *pc.2* and *pc.3* to the desired range (e.g. 0-255).
This can be done with *[r.rescale](r.rescale.md)*.

## SEE ALSO

*[i.pca](i.pca.md),
[m.eigensystem](https://grass.osgeo.org/grass-stable/manuals/addons/m.eigensystem.html)
(Addon), [r.mapcalc](r.mapcalc.md), [r.rescale](r.rescale.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
