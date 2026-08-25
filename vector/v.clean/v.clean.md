## DESCRIPTION

*v.clean* allows the user to automatically fix topology of vector maps.
Several tools may be listed to be executed sequentially. In this case,
also the threshold parameter requires several values to be listed
accordingly. An error map is optionally written which stores the
erroneous geometries.

### Break lines/boundaries

Setting *tool=break* breaks lines/boundaries at intersections and it also
breaks lines/boundaries forming a collapsed loop. For example,
0.0;1.0;0.0 is broken at 1.0.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

Hint: Breaking lines should be followed by removing duplicates, e.g.
*v.clean ... tool=break,rmdupl*. If the *-c* flag is used with *v.clean
... tool=break*, duplicates are automatically removed.

### Remove duplicate geometry features

Setting *tool=rmdupl* removes geometry features with identical coordinates.
Categories are merged. If a point and a centroid have identical
coordinates, one of them will be removed if both points and centroids
are selected with *v.clean ... type=point,centroid*. The same applies
for lines and boundaries.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

The *rmdupl* tool should be used after breaking lines and breaking
polygons.

### Remove dangles or change boundary dangles to type line

A line/boundary is considered to be a dangle if no other line of given
*type* is on at least one end node. If a dangle is formed by several
lines, such a string of lines is taken as one dangle and line lengths
are summarized. Setting *tool=rmdangle* deletes a dangle if the (combined)
length is shorter than *thresh* or *thresh* \< 0. If the combined length
is larger than *thresh*, nothing is deleted.

Threshold has to be given as maximum line/boundary length in map units;
for latitude-longitude projects in degrees. Dangles shorter than
*thresh* are removed sequentially. All dangles will be removed if
*thresh* \< 0.

With *thresh* \< 0, only closed loops and lines connecting loops will
remain. This is useful to remove all incorrect boundaries after other
cleaning operations with *thres* is \< 0. Areas can then be successfully
built.

To preferentially remove shortest dangles first, a first pass with a
small *thresh* value can be followed by subsequent passes with higher
*thresh* values. This can be done as one *v.clean* job by listing the
tool several times and by defining a list of increasing *thresh* values.

Setting *tool=chdangle* is similar to the *rmdangle* tool, but works only on
boundaries and changes dangling boundaries to lines instead of removing
them.

### Remove or change bridges connecting an area and an island or two islands

A bridge is an area type connection of an island (polygon in a polygon)
to the outer polygon. This is topologically incorrect (but OGC Simple
Features allow it). Setting *tool=rmbridge* removes bridges and setting
*tool=chbridge* changes bridges to type line:

```sh
    +-------------+             +-------------+   +-------------+
    |            P|  P: polygon |            P|   |            P|
    |    +---+    |  I: island  |    +---+    |   |    +---+    |
    |    | I |    |  B: bridge  |    | I |    |   |    | I |    |
    |    |   |    |  L: line    |    |   |    |   |    |   |    |
    |    +-+-+    |             |    +---+    |   |    +-.-+    |
    |      |      |             |             |   |      .      |
    |      | B    |             |             |   |      . L    |
    |      |      |             |             |   |      .      |
    +------+------+             +-------------+   +-------------+
```

Islands and areas must be already clean, i.e. without dangles or small
angles, e.g. *v.clean ... type=boundary
tool=rmdangle,rmsa,break,rmdupl,rmbridge thresh=-1,0,0,0,0*.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

### Snap lines to vertex in threshold

Setting *tool=snap* snaps vertices to another vertex not farther away than
*thresh*. If there is no other vertex within *thresh*, no snapping will
be done. The *type* option can have a strong influence on the result. A
too large threshold and *type=boundary* can severely damage area
topology, beyond repair.

Threshold gives maximum distance to another vertex in map units, for
latitude-longitude projects in degrees.

Snapped boundaries may need to be cleaned with *break,rmdupl,rmsa*. If
the *-c* flag is used with *v.clean tool=snap*, the sequence of
*break,rmdupl,rmsa* is automatically repeated after snapping until no
more small angles a left. Additional cleaning with e.g.
*tool=rmdangle*may be necessary.

### Remove duplicate area centroids

Setting *tool=rmdac* removes duplicate area centroids that can result from
deleting boundaries.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

### Break (topologically clean) areas (imported from a non topological format)

Setting *tool=bpol* breaks boundaries on each point shared between 2 and
more areas where angles of boundary segments are different and on all
boundary nodes (start and end points of each boundary). The *bpol* tool
behaves similar to *break* for boundaries, but does not break collapsed
loops. The *bpol* tool is faster than the *break* tool but needs more
memory.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

The *bpol* tool should be followed by *rmdupl*. If the *-c* flag is used
with *v.clean ... tool=bpol*, duplicates are automatically removed.

### Remove vertices in threshold from lines and boundaries

Setting *tool=prune* simplifies lines and boundaries by removing vertices
according to threshold. This tool preserves area topology, areas are
never deleted and centroid attachment is never changed.
*[v.generalize](v.generalize.md)* offers much more functionality for
line simplification but does not preserve area topology.

### Remove small areas

Setting *tool=rmarea* removes all areas \<= *thresh*. The longest boundary
with an adjacent area is removed or all boundaries if there is no
adjacent area. Area categories are not combined when a small area is
merged with a larger area.

Threshold must always be in square meters, also for latitude-longitude
projects or projects with units other than meters.

### Remove all lines or boundaries of zero length

Setting *tool=rmline* removes all lines or boundaries of zero length that
may have resulted from other cleaning operations. Zero length boundaries
are redundant and do not influence area topology.

Threshold does not apply (it is ignored), use an arbitrary value (e.g.,
0) if *v.clean* is run with several tools.

### Remove small angles between lines at nodes

Setting *tool=rmsa* only concerns angles which are so small that the
calculated angle is 0. The following figure should help demonstrate what
the tool does.

Threshold does not apply, use dummy value if *v.clean* is run with
several tools.

![v_clean_rmsa](v_clean_rmsa.png)  
*tool=rmsa*

The *rmsa* tool should be followed by *break,rmdupl*. The *rmsa* tool
followed by *break,rmdupl* may need to be run more than once to remove
all small angles. If the *-c* flag is used with *v.clean ... tool=rmsa*,
the sequence of *rmsa,break,rmdupl* is automatically repeated until no
more small angles a left.

## NOTES

The user does **not** have to run *[v.build](v.build.md)* on the
*output* vector, unless the *-b* flag was used. The *-b* flag affects
**only** the *output* vector - topology is always built for *error*
vector.

## EXAMPLES

### Snapping lines to vertex in threshold

```sh
v.clean input=testmap output=cleanmap tool=snap threshold=1
```

### Inspecting the topological errors visually

Both *[v.build](v.build.md)* and *v.clean* can collect the topological
errors into a vector map:

```sh
v.build -e map=imported error=build_errors
v.clean -c input=imported output=clean error=cleaning_errors tool=snap,rmdangle,rmbridge,chbridge,bpol,prune threshold=5
```

The vector maps can be visualized together with the original data by the
following set of display commands:

```sh
d.vect map=imported color=26:26:26 fill_color=77:77:77 width=5
d.vect map=build_errors color=255:33:36 fill_color=none width=5 icon=basic/point size=30
d.vect map=cleaning_errors color=255:33:36 fill_color=none width=5 icon=basic/point size=30
```

![topological errors](v_clean.png)  
*Figure: Topological errors detected in the original data (left) and
cleaned data (right)*

### Cleaning OGR imported data (Simple Feature data) such as SHAPE file

The import of areas with *[v.in.ogr](v.in.ogr.md) -c* (no cleaning)
requires a subsequent run of *v.clean* to update the map to a
topologically valid structure (removal of duplicate collinear lines
etc). The tools used for that are *bpol* and *rmdupl*:

```sh
v.clean input=areamap output=areamap_clean tool=bpol,rmdupl type=boundary
```

### Extracting intersection points of vector lines

```sh
v.clean input=lines1 output=lines2 err=points tool=break type=line
```

Intersection points are written to 'points' map.

### Break lines

*v.clean* will break the lines where they cross, creating new node if
needed. Example:

```sh
v.in.ascii -n out=crossed_lines format=standard << EOF
L 2
 0 5
 10 5
L 2
 5 0
 5 10
EOF

v.clean in=crossed_lines out=crossed_lines_brk \
        error=intersection tool=break type=line
```

### Remove all lines of zero length

```sh
v.out.ascii zero format=standard
L  2 1
 -819832.09065589 -987825.2187231
 -806227.28362601 -971104.80702988
 1     1
L  2 1
 -799165.24638913 -972974.16982788
 -799165.24638913 -972974.16982788
 1     2

v.clean input=zero output=zero_clean tool=rmline type=line

v.out.ascii zero_clean format=standard
L  2 1
 -819832.09065589 -987825.2187231
 -806227.28362601 -971104.80702988
 1     1
```

v.clean type=boundary would remove nothing.

### Repeatedly remove dangling lines up to 50m length

```sh
v.clean input=testmap output=cleanmap type=line \
        tool=rmdangle,rmdangle,rmdangle,rmdangle threshold=5,10,20,50
```

## SEE ALSO

*[v.info](v.info.md), [v.build](v.build.md),
[g.gui.vdigit](g.gui.vdigit.md), [v.edit](v.edit.md),
[v.fill.holes](v.fill.holes.md), [v.generalize](v.generalize.md)*

## AUTHORS

David Gerdes, U.S. Army Construction Engineering Research Laboratory  
Radim Blazek, ITC-irst, Trento, Italy  
Martin Landa, FBK-irst (formerly ITC-irst), Trento, Italy
