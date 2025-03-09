## DESCRIPTION

*r.to.vect* scans the named **input** raster map layer, extracts points,
lines or area edge features from it, converts data to GRASS vector
format.

### Point conversion

The *r.to.vect* program extracts data from a GRASS raster map layer and
stores output in a new GRASS *vector* file.

### Line conversion

*r.to.vect* assumes that the *input* map has been thinned using
*[r.thin](r.thin.md)*.

*r.to.vect* extracts vectors (aka, "arcs") from a raster map. These arcs
may represent linear features (like roads or streams), or may represent
area edge features (like political boundaries, or soil mapping units).

*[r.thin](r.thin.md)* and *r.to.vect* may create excessive nodes at
every junction, and may create small spurs or "dangling lines" during
the thinning and vectorization process. These excessive nodes and spurs
may be removed using *[v.clean](v.clean.md)*.

### Area conversion

*r.to.vect* first traces the perimeter of each unique area in the raster
map layer and creates vector data to represent it. The cell category
values for the raster map layer will be used to create attribute
information for the resultant vector area edge data.

A true vector tracing of the area edges might appear blocky, since the
vectors outline the edges of raster data that are stored in rectangular
cells. To produce a better-looking vector map, *r.to.vect* smoothes the
corners of the vector data as they are being extracted. At each change
in direction (i.e., each corner), the two midpoints of the corner cell
(half the cell's height and width) are taken, and the line segment
connecting them is used to outline this corner in the resultant vector
map. (The cell's cornermost node is ignored.) Because vectors are
smoothed by this program, the resulting vector map will not be "true" to
the raster map from which it was created. The user should check the
resolution of the geographic region (and the original data) to estimate
the possible error introduced by smoothing.

*r.to.vect* extracts only area edges from the named raster input file.
If the raster map contains other data (i.e., line edges, or point data)
the output may be wrong.

By default, area centroids are often located close to boundaries and not
in the middle of an area. Centroids can be more centrally located with
the *-c* flag.

## EXAMPLES

The examples are based on the North Carolina sample dataset:

**Conversion of raster points to vector points:**

Random sampling of points:

```sh
g.region raster=elevation -p
# random sampling of points (note that r.random also writes vector points)
r.random elevation raster_output=elevrand1000 n=1000
r.to.vect input=elevrand1000 output=elevrand1000 type=point
# univariate statistics of sample points
v.univar elevrand1000 column=value type=point
# compare to univariate statistics on original full raster map
r.univar elevation
```

**Conversion of raster lines to vector lines:**

Vectorization of streams in watershed basins map:

```sh
g.region raster=elevation -p
r.watershed elev=elevation stream=elev.streams thresh=50000
r.to.vect -s input=elev.streams output=elev_streams type=line
# drop "label" column which is superfluous in this example
v.db.dropcolumn map=elev_streams column=label
v.db.renamecolumn map=elev_streams column=value,basin_id
# report length per basin ID
v.report map=elev_streams option=length units=meters sort=asc
```

**Conversion of raster polygons to vector polygons:**

Vectorization of simplified landuse class map:

```sh
g.region raster=landclass96 -p
# we smooth corners of area features
r.to.vect -s input=landclass96 output=my_landclass96 type=area
v.colors my_landclass96 color=random
```

## KNOWN ISSUES

For type=line the input raster map MUST be thinned by
*[r.thin](r.thin.md)*; if not, *r.to.vect* may crash.

## SEE ALSO

*[g.region](g.region.md), [r.thin](r.thin.md), [v.clean](v.clean.md)*

## AUTHORS

**Point support**  
Bill Brown  
  
**Line support**  
Mike Baba  
DBA Systems, Inc.  
10560 Arrowhead Drive  
Fairfax, Virginia 22030  
  
**Area support**  
*Original* version of *r.poly*:  
Jean Ezell and Andrew Heekin,  
U.S. Army Construction Engineering Research Laboratory

*Modified* program for smoothed lines:  
David Satnik, Central Washington University  
Updated 2001 by Andrea Aime, Modena, Italy  
  
**Update**  
Original r.to.sites, r.line and r.poly merged and updated to 5.7 by
Radim Blazek
