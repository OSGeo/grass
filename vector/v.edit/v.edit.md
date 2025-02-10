## DESCRIPTION

The module *v.edit* allows the user to edit a vector map via command
line interface.

*v.edit* supports only "simple" vector features: points, centroids,
lines and boundaries. Currently, only 2D vector features (except of
**tool=zbulk**) are supported.

Provides editing features' geometry. Attribute data connected to the
vector map are not modified at all.

Vector features can be selected either by internal **id**, category
number **cats**, coordinates **coords**, bounding box **bbox**,
**polygon**, **where** statement (attribute data) or by **query**.
Selecting features by coordinates is affected by the current 2D
resolution or by the threshold distance given by **threshold**. The
options are *orthogonal*, i.e. can be used in various combinations. For
example:

```sh
v.edit map=roads tool=select \
  coord=599505,4921010,603389.0625,4918292.1875 \
  threshold=10000 where="label='interstate'"
```

selects all features (and prints their id's to standard output) covered
by two bounding boxes (center at 599505,4921010 and
603389.0625,4918292.1875, size 2\*10000) with attribute
`label='interstate'`.

## NOTES

If no vector features are selected or the flag **-b** is used, topology
is not build at the end.

## USAGE

### Feature selection

Vector features can be selected in several ways:

- **ids** - using internal (unique) feature id's
- **cats** - using category numbers
- **coords** - using x,y coordinate pairs (center of bounding box, size
  defined by **threshold**)
- **bbox** - using bounding box
- **polygon** - using polygon (at least 3 coordinate pairs have to be
  set)
- **where** - using where statement (attribute data)
- **query** - special query (e.g. minimal vector line length)

Additional parameters for vector feature specification are:

- **layer** - layer number (currently used only with **cats** or
  **where** option)
- **threshold** - threshold distance used for selecting vector features
  by coordinates

### Tool description

- **create** - Create new (empty) vector map (see also
  *[v.in.ascii](v.in.ascii.md)*). Optionally vector features (in [GRASS
  ASCII vector format](vectorascii.md)) can be read from standard input
  (**input=-**) or from the text file given by the **input** option.
- **add** - Add new vector features (defined in [GRASS ASCII vector
  format](vectorascii.md)) to existing vector map. Features can be read
  from standard input or from the given text file (**input** option). If
  no header is given, the **-n** flag must be used. Added features can
  be snapped (defined by **snap** parameter) to nodes or vertices based
  on threshold distance **threshold**.
- **delete** - Delete selected vector features from existing vector map.
- **copy** - Make identical copy of selected vector features. If
  background map **bgmap** is given copy features from background map,
  not from currently modified vector map.
- **move** - Move selected features of existing vector map relatively to
  their current location. This tool requires **move** option. The option
  defines coordinates of the movement direction. Moved features can be
  snapped (defined by **snap** parameter) to nodes or vertices based on
  threshold distance **threshold**.
- **flip** - Flip direction of selected vector lines (lines or
  boundaries).
- **catadd** - Add new layer category(ies) to selected vector
  feature(s). Category can be later used for new database entry.
- **catdel** - Delete layer category(ies) of selected vector feature(s).
- **merge** - Merge (at least two) selected vector lines or boundaries.
  The geometry of the merged vector lines can be changed. If the second
  line from two selected lines is in opposite direction to the first, it
  will be flipped. See also module
  *[v.build.polylines](v.build.polylines.md)*.
- **break** - Split given vector line or boundary into two lines on
  location given by **coords**. If **coords** not given, breaks all
  selected lines at each intersection (based on *[v.clean](v.clean.md)*,
  `tool=break`).
- **snap** - Snap vector features in given threshold. See also module
  *[v.clean](v.clean.md)*. Note that this tool supports only snapping to
  nodes. Parameters **snap** and **bgmap** are ignored.
- **connect** - Connect selected lines or boundaries, the first given
  line is connected to the second one. The second line is broken if
  necessary. The lines are connected only if distance between them is
  not greater than snapping threshold distance **threshold**.
- **extend** - Extend selected lines or boundaries without changing the
  current shape. Similar to **connect**, but the first and second lines
  are both extended until they intersect. The second line is broken if
  necessary. The lines are extended only if distance between them is not
  greater than snapping threshold distance **threshold**. If the first
  and second lines are parallel and do not intersect, no lines are
  extended. Use the **-p** flag to extend the first line across the
  parallel gap.
- **extendstart** - Similar to **extend**, but extend at start nodes
  only. Start nodes are used to select the second line and the end node
  of that line can also be extended if it is within the snapping
  threshold distance given by **threshold**.
- **extendend** - Similar to **extend**, but extend at end nodes only.
- **chtype** - Change feature type of selected geometry objects. Points
  are converted to centroids, centroids to points, lines to boundaries
  and boundaries to lines.
- **vertexadd** - Add vertex(ces) to the given vector lines or
  boundaries. Location of the new vertex is given by **coord** option.
  If -1 is given only first found line or boundary in bounding box is
  modified.
- **vertexdel** - Remove vertex(ces) specified by **coords** option. If
  -1 is given only first found line or boundary in bounding box is
  modified.
- **vertexmove** - Move vertex(ces) specified by **coords** option.
  Direction of the movement is specified by the **move** option. If -1
  is given only first found line or boundary in bounding box is
  modified. Moved vertex can be snapped (defined **snap**) to nodes or
  vertices based on threshold distance **threshold**.
- **zbulk** - Assign z coordinate to 3D vector lines in given bounding
  box. The first found line will get z coordinate based on value given
  by **zbulk** parameter. Z coordinate of other selected lines will be
  increased by step given by **zbulk** parameter. This tool strictly
  requires **bbox** and **zbulk** parameter. Also input vector map must
  be 3D.
- **select** - Print comma separated list of selected line id's. No
  editing is done.

## EXAMPLES

### Create new vector map

Create new (empty) vector map:

```sh
v.edit tool=create map=vectmap
```

Create new vector map and read data from file 'roads.txt':

```sh
v.out.ascii in=roads format=standard > roads.txt;
v.edit tool=create map=vectmap input=roads.txt
```

or alternatively

```sh
cat roads.txt | v.edit tool=create map=vectmap input=-
```

### Add new features to existing vector map

Add a new point to the vector map (without header):

```sh
echo "P 1 1
 640794 214874
 1 1" | v.edit -n tool=add map=vectmap input=-

# insert new row for each category in attribute table if doesn't exist yet
v.to.db map=vectmap option=cat
```

The input must be in [GRASS ASCII vector format](vectorascii.md).

Add new features read from standard input:

```sh
v.out.ascii in=railroads format=standard | v.edit tool=add map=vectmap input=-
```

### Delete selected features from vector map layer

Remove all vector features with category number 1 or 2:

```sh
v.edit tool=delete map=roads cats=1,2
```

Remove all vector features except of those with category number 1 or 2
(reverse selection):

```sh
v.edit -r tool=delete map=roads cats=1,2
```

Remove features with category 1 or 2 located on coordinates
600952.625,4926107 (bounding box based on the current 2D resolution):

```sh
g.region -d;
v.edit tool=delete map=roads cats=1,2 coords=600952.625,4926107
```

Remove all features with category 1 and 2 covered by two bounding boxes
(center coordinates 592542.892,4924766.996 and 603389.062,4918292.187,
size 1000 map units):

```sh
v.edit map=roads tool=delete \
  coord=592542.892,4924766.996,603389.062,4918292.187 \
  threshold=1000 cat=1,2
```

### Copy selected features from background map

Copy all features with category number 1 from background map:

```sh
v.edit map=roads tool=copy bgmap=archsites cat=1
```

### Move features

Move feature (vector point) located on coordinates 602580,4918480 to
coordinates 603580,4919480:

```sh
v.edit tool=move map=archsites coord=602580,4918480 th=1e-2 move=1000,1000
```

Move all features with category 1 1000 map units to the west and 1000
map units to the south. Moved features snap to nodes in threshold
distance 10 map units:

```sh
v.edit tool=move map=roads cat=1 move=1000,-1000 snap=node threshold=-1,10
```

Move all features defined by bounding box 601530,4921560,602520,4922310
(W,S,E,N) 1000 map units to the east and 1000 map units to the north:

```sh
v.edit tool=move map=roads bbox=601530,4921560,602520,4922310 move=-1000,1000
```

### Flip direction of vector lines

Flip direction of all vector lines:

```sh
v.edit tool=flip map=streams cats=1-9999 type=line
```

### Add / delete layer category number

Add new layer/category 2/1, 2/3, 2/4, 2/5 to features covered by given
polygon:

```sh
v.edit tool=catadd map=roads \
  polygon=599877.75,4925088.375,597164.812,4922524.5,601338.562,4920914.625 \
  layer=2 cat=1,3-5
```

Delete layer/category 1/1, line id 1:

```sh
v.edit tool=catdel map=roads id=1 cats=5
```

### Merge lines

Merge two lines with given category number:

```sh
v.edit map=roads tool=merge cat=4
```

### Split line on given point

Split line id 810 on coordinates 604268,4923570 in threshold 50 map
units:

```sh
v.edit map=roads tool=break coords=604268,4923570 id=810 threshold=50
```

### Break selected lines at each intersection

Break selected lines (with category number 1) at each intersection:

```sh
v.edit map=roads tool=break cat=1
```

### Snap lines

Snap all lines using threshold distance 20 map units:

```sh
v.edit map=roads id=1-9999 tool=snap threshold=-1,20 type=line
```

### Connect lines

Connect line id 48 to line id 565:

```sh
v.edit map=roads tool=connect id=48,565
```

Connect line id 48 to line id 565; line id 60 to line id 50. Maximum
threshold distance is 700 map units:

```sh
v.edit map=roads tool=connect id=48,565,60,50 threshold=-1,700
```

### Add vertex

Add new vertex to the line located at 600952,4926107, threshold is set
to 1 map unit:

```sh
v.edit tool=vertexadd map=roads coords=600952,4926107 threshold=1
```

### Delete vertices

Delete vertex located at 593191.608,4925684.849 (threshold set to 0.1
map units). Modify only lines with category 1:

```sh
v.edit tool=vertexdel map=roads coord=593191.608,4925684.849 \
  threshold=1-e1 cats=1
```

### Move vertices

Move vertices located at 604441,4921088 (threshold set to 100 map
units). Modify only lines with categories 1-10:

```sh
v.edit tool=vertexmove map=roads cats=1-10 coord=604441,4921088 \
  threshold=100 move=1000,1000
```

### Select features and print their id's

Print id's of selected features, e.g.:

```sh
v.edit map=soils@PERMANENT tool=select \
  bbox=595733.8125,4919781.75,598536.1875,4917396.75 --q
```

Example with *[d.vect](d.vect.md)*:

```sh
d.erase;
d.vect roads;
d.vect -i map=roads cats=`v.edit map=roads tool=select \
  coord=592542.89243878,4924766.99622811,603389.0625,4918292.1875 \
  threshold=1000 --q` col=red
```

Select all lines shorter (or equal) than 10 map units:

```sh
v.edit map=roads tool=select query=length threshold=-1,0,-10
```

Select from given bounding box all lines longer then 200 map units:

```sh
v.edit map=roads tool=select bbox=598260,4919730,605100,4926240 query=length threshold=-1,0,200
```

### Fix height of contours

Input vector map contains 2D lines representing contours. Height can be
assign to the contours using **tool=zbulk**. First of all 2D lines need
to be converted to 3D lines:

```sh
v.extrude input=line2 output=line3 height=0 type=line
```

All lines which intersect with the line given by coordinates will be
modified. First found line will get height 1000 map units, height of
other selected lines will be increased by 10 map units.

```sh
v.edit a2 tool=zbulk bbox=586121.25049368,4911970.21547109,603092.60466035,4927071.25713776 \
   zbulk=1000,10
```

## SEE ALSO

*[v.in.ascii](v.in.ascii.md), [v.info](v.info.md),
[v.build](v.build.md), [v.build.polylines](v.build.polylines.md),
[v.clean](v.clean.md), [v.extrude](v.extrude.md), [v.split](v.split.md)*

See also *[wxGUI vector digitizer](wxGUI.vdigit.md)*.

## AUTHORS

Original author: Wolf Bergenheim - independent developer  
Initial updates: Jachym Cepicky, Mendel University of Agriculture and
Forestry in Brno, Czech Republic  
Major update by Martin Landa, FBK-irst (formerly ITC-irst), Trento,
Italy  
Extend tools by Huidae Cho
