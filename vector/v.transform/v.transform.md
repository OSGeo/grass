## DESCRIPTION

*v.transform* performs an affine transformation (translate and rotate)
of a vector map. An affine transform includes one or several linear
transformations (scaling, rotation) and translation (shifting). Several
linear transformations can be combined in a single operation. The
command can be used to georeference unreferenced vector maps or to
modify existing geocoded maps.

## NOTES

Coordinate transformation based on Ground Control Points (GCPs) is done
by *[v.rectify](v.rectify.md)* and not supported by *v.transform*.

Transformation parameters (i.e. *xshift*, *yshift*, etc.) can be fetched
from attribute table connected to the vector map. In this case vector
objects can be transformed with different parameters based on their
category number. If the parameter cannot be fetched from the table,
default value is used instead.

Note that the transformation matrix can be printed by
*[m.transform](m.transform.md)*.

## EXAMPLE

### DXF/DWG drawings

Most DXF/DWG drawings are done within XY coordinate space. To transform
them to a national grid, we can use *v.transform* together with
*v.rectify* and a first-order transformation.

```sh
v.transform -t in=watertowerXY out=watertower_z zscale=0.04 zshift=1320
v.rectify in=watertower_z out=watertowerUTM points=wt.points order=1
```

### Extrude 2D vector points to 3D based on attribute column values

Spearfish example with manual table editing for vertical shift:

```sh
# work on own map copy:
g.copy vect=archsites@PERMANENT,myarchsites
# add new 'zs' column to later store height of each site:
v.db.addcolumn myarchsites col="zs double precision"
v.db.update myarchsites layer=1 column=zs value="cat * 1000"

# perform z transformation:
v.transform -t input=archsites output=myarchsites3d column="zshift:zs" table="archsites_t"
# drop table containing transformation parameters:
echo "drop table archsites_t" | db.execute
```

The resulting map is a 3D vector map.

### Extrude 2D vector points to 3D with automated elevation extraction

Spearfish example with automated elevation extraction for vertical
shift:

```sh
# work on own map copy:
g.copy vect=archsites@PERMANENT,myarchsites
# add new 'zs' column to later store height of each site:
v.db.addcolumn myarchsites col="zs double precision"

# set region to elevation map and fetch individual heights:
g.region raster=elevation.10m -p
v.what.rast myarchsites rast=elevation.10m col=zs
# verify:
v.db.select myarchsites

# perform transformation to 3D
v.transform -t myarchsites output=myarchsites3d column="zshift:zs" layer=1
# drop table containing transformation parameters
v.db.dropcolumn myarchsites3d col=zs
```

The resulting map is a 3D vector map.

## SEE ALSO

*[m.transform](m.transform.md), [i.rectify](i.rectify.md),
[v.rectify](v.rectify.md), [r.region](r.region.md)*

## AUTHORS

Radim Blazek, ITC-irst, Trento, Italy,  
Column support added by Martin Landa, FBK-irst (formerly ITC-irst),
Trento, Italy (2007/09)
