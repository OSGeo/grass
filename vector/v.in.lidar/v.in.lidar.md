## DESCRIPTION

*v.in.lidar* converts LiDAR point clouds in LAS format to a GRASS
vector, using the [libLAS](https://liblas.org) library. The created
vector is true 3D with x, y, z coordinates.

For larger datasets, it is recommended to not build topology (-b flag).
Also, creating a table with attributes can take some time for larger
datasets.

The optional **spatial** parameter defines spatial query extents. This
parameter allows the user to restrict the region to a spatial subset
while importing the data. All LiDAR points falling into this rectangle
subregion are imported. The **-r** current region flag is identical, but
uses the current region settings as the spatial bounds (see
*[g.region](g.region.md)*).

A LiDAR pulse can have multiple returns. The first return values can be
used to obtain a digital surface model (DSM) where e.g. canopy cover is
represented. The last return values can be used to obtain a digital
terrain model (DTM) where e.g. the forest floor instead of canopy cover
is represented. The **return_filter** option allows selecting one of
first, mid, or last returns.

LiDAR points can be already classified into standardized classes. For
example, class number 2 represents ground (for other classes see LAS
format specification in references). The **class_filter** option allows
selecting one or more classes, as numbers (integers) separated by comma.

Note that proper filtering of the input points in not only critical for
the analysis itself but it can also speed up the processing
significantly.

### Decimation

Table with selected percentages of points to keep with corresponding
decimation parameters:

| percentage | parameters    |
|------------|---------------|
| 0.1%       | preserve=1000 |
| 1%         | preserve=100  |
| 5%         | preserve=20   |
| 10%        | preserve=10   |
| 20%        | preserve=5    |
| 25%        | preserve=4    |
| 50%        | skip=2        |
| 75%        | skip=4        |
| 80%        | skip=5        |
| 90%        | skip=10       |

Table with selected fractions of points to keep with corresponding
decimation parameters:

| ratio | parameters |
|-------|------------|
| 1/3   | preserve=3 |
| 1/4   | preserve=4 |
| 1/5   | preserve=5 |
| 1/6   | preserve=6 |

Table with selected fractions of points to throw away with corresponding
decimation parameters:

| ratio | parameters |
|-------|------------|
| 1/3   | skip=3     |
| 1/4   | skip=4     |
| 1/5   | skip=5     |
| 1/6   | skip=6     |

## Project Creation

*v.in.lidar* attempts to preserve coordinate reference system (CRS)
information when importing datasets if the source format includes such
information, and if the LAS driver supports it. If the source dataset
CRS does not match the CRS of the current project (previously called
location) *v.in.lidar* will report an error message
("`Coordinate reference system of dataset does not appear to match current project`")
and then report the PROJ_INFO parameters of the source dataset.

If the user wishes to ignore the difference between the coordinate
system of the source data and the current project, they may pass the
**-o** flag to override the CRS check.

If the user wishes to import the data with the full CRS definition, it
is possible to have *v.in.lidar* automatically create a new project
based on the CRS and extents of the file being read. This is
accomplished by passing the name to be used for the new project via the
**project** parameter. Upon completion of the command, a new project
will have been created (with only a PERMANENT mapset), and the vector
map will have been imported with the indicated **output** name into the
PERMANENT mapset.

## NOTES

The typical file extensions for the LAS format are .las and .laz
(compressed). The compressed LAS (.laz) format can be imported only if
libLAS has been compiled with laszip support. It is also recommended to
compile libLAS with GDAL, needed to test for matching CRSs.

## EXAMPLE

The sample LAS data are in the file "Serpent Mound Model LAS Data.laz",
available at [Serpent Mound Model LAS
Data.laz](https://github.com/PDAL/data/raw/4ee9ee43b195268a59113555908c1c0cdf955bd4/liblas/Serpent-Mound-Model-LAS-Data.laz)

```sh
# print LAS file info
v.in.lidar -p input="Serpent Mound Model LAS Data.laz"

# create a project with CRS information of the LAS data
v.in.lidar -i input="Serpent Mound Model LAS Data.laz" project=Serpent_Mound

# quit and restart GRASS in the newly created project "Serpent_Mound"
# real import of LiDAR LAS data, without topology and without attribute table
v.in.lidar -tb input="Serpent Mound Model LAS Data.laz" output=Serpent_Mound_Model_pts
```

## REFERENCES

[ASPRS LAS
format](https://www.asprs.org/committee-general/laser-las-file-format-exchange-activities.html)  
[LAS library](https://liblas.org/)  
[LAS library C API](https://liblas.org/doxygen/liblas_8h.html)
documentation

## SEE ALSO

*[r.in.lidar](r.in.lidar.md), [r3.in.lidar](r3.in.lidar.md),
[g.region](g.region.md), [v.vect.stats](v.vect.stats.md),
[v.in.ogr](v.in.ogr.md)*

## AUTHORS

Markus Metz  
Vaclav Petras, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/) (decimation, cats, areas,
zrange)  
based on *v.in.ogr* by various authors
