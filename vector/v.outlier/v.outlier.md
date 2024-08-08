## DESCRIPTION

*v.outlier* removes outliers in a 3D point cloud. By default, the
outlier identification is done by a bicubic spline interpolation of the
observation with a high regularization parameter and a low resolution in
south-north and east-west directions. Those points that differ in an
absolute value more than the given threshold from a fixed value,
reckoned from its surroundings by the interpolation, are considered as
an outlier, and hence are removed.

The *filter* option specifies if all outliers will be removed (default),
or only positive or only negative outliers. Filtering out only positive
outliers can be useful to filter out vegetation returns (e.g. from
forest canopies) from LIDAR point clouds, in order to extract Digital
Terrain Models. Filtering out only negative outliers can be useful to
estimate vegetation height.

There is a flag to create a vector that can be visualizated by qgis.
That means that topology is build and the z coordinate is considered as
a category.

## EXAMPLES

### Basic outlier removal

```
v.outlier input=vector_map output=vector_output outlier=vector_outlier thres_O=25
```

In this case, a basic outlier removal is done with a threshold of 25 m.

### Basic outlier removal

```
v.outlier input=vector_map output=vector_output outlier=vector_outlier qgis=vector_qgis
```

Now, the outlier removal uses the default threshold and there is also an
output vector available for visualizaton in QGIS
(<http://www.qgis.org>).

### North Carolina dataset example

```
v.outlier input=elev_lid792_bepts output=elev_lid792_bepts_nooutliers \
  outlier=elev_lid792_bepts_outliers ew_step=5 ns_step=5 thres_o=0.1
```

## NOTES

This module is designed to work with LIDAR data, so not topology is
built but in the QGIS output.

## SEE ALSO

*[v.surf.bspline](v.surf.bspline.html)*

## AUTHORS

Original version of the program in GRASS 5.4:\
Maria Antonia Brovelli, Massimiliano Cannata, Ulisse Longoni and Mirko
Reguzzoni\
\
Updates for GRASS 6:\
Roberto Antolin
