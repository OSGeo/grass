### Vector maps in general

A \"vector map\" is a data layer consisting of a number of sparse
features in geographic space. These might be data points (drill sites),
lines (roads), polygons (park boundary), volumes (3D CAD structure), or
some combination of all these. Typically each feature in the map will be
tied to a set of attribute layers stored in a database (road names, site
ID, geologic type, etc.). As a general rule these can exist in 2D or 3D
space and are independent of the GIS\'s computation region.

### Attribute management

The default database driver used by GRASS GIS 8 is SQLite. GRASS GIS
handles multiattribute vector data by default. The *db.\** set of
commands provides basic SQL support for attribute management, while the
*v.db.\** set of commands operates on vector maps.

Note: The list of available database drivers can vary in various binary
distributions of GRASS GIS, for details see [SQL support in GRASS
GIS](sql.html).

### Vector data import and export

The [v.in.ogr](v.in.ogr.html) module offers a common interface for many
different vector formats. Additionally, it offers options such as
on-the-fly creation of new projects or extension of the default region
to match the extent of the imported vector map. For special cases, other
import modules are available, e.g. [v.in.ascii](v.in.ascii.html) for
input from a text file containing coordinate and attribute data, and
[v.in.db](v.in.db.html) for input from a database containing coordinate
and attribute data. With [v.external](v.external.html) external maps can
be virtually linked into a mapset, only pseudo-topology is generated but
the vector geometry is not imported. The *v.out.\** set of commands
exports to various formats. To import and export only attribute tables,
use [db.in.ogr](db.in.ogr.html) and [db.out.ogr](db.out.ogr.html).

GRASS GIS vector map exchange between different projects (same
projection) can be done in a lossless way using the
[v.pack](v.pack.html) and [v.unpack](v.unpack.html) modules.

The naming convention for vector maps requires that map names start with
a character, not a number (map name scheme:
\[A-Za-z\]\[A-Za-z0-9\_\]\*).

### Metadata

The [v.info](v.info.html) display general information such as metadata
and attribute columns about a vector map including the history how it
was generated. Each map generating command stores the command history
into the metadata (query with [v.info -h mapname](v.info.html)).
Metadata such as map title, scale, organization etc. can be updated with
[v.support](v.support.html).

### Vector map operations

GRASS vector map processing is always performed on the full map. If this
is not desired, the input map has to be clipped to the current region
beforehand ([v.in.region](v.in.region.html),
[v.overlay](v.overlay.html), [v.select](v.select.html)).

### Vector model and topology

GRASS is a topological GIS. This means that adjacent geographic
components in a single vector map are related. For example in a
non-topological GIS if two areas shared a common border that border
would be digitized two times and also stored in duplicate. In a
topological GIS this border exists once and is shared between two areas.
Topological representation of vector data helps to produce and maintain
vector maps with clean geometry as well as enables certain analyses that
can not be conducted with non-topological or spaghetti data. In GRASS,
topological data are referred to as level 2 data and spaghetti data is
referred to as level 1.

Sometimes topology is not necessary and the additional memory and space
requirements are burdensome to a particular task. Therefore two modules
allow for working level 1 (non-topological) data within GRASS. The
[v.in.ascii](v.in.ascii.html) module allows users to input points
without building topology. This is very useful for large files where
memory restrictions may cause difficulties. The other module which works
with level 1 data is [v.surf.rst](v.surf.rst.html) which enables spatial
approximation and topographic analysis from a point or isoline file.

In GRASS, the following vector object types are defined:

-   point: a point;
-   line: a directed sequence of connected vertices with two endpoints
    called nodes;
-   boundary: the border line to describe an area;
-   centroid: a point within a closed ring of boundaries;
-   area: the topological composition of a closed ring of boundaries and
    a centroid;
-   face: a 3D area;
-   kernel: a 3D centroid in a volume (not yet implemented);
-   volume: a 3D corpus, the topological composition of faces and kernel
    (not yet implemented).

Lines and boundaries can be composed of multiple vertices.

Area topology also holds information about isles. These isles are
located within that area, not touching the boundaries of the outer area.
Isles are holes inside the area, and can consist of one or more areas.
They are used internally to maintain correct topology for areas.

The [v.type](v.type.html) module can be used to convert between vector
types if possible. The [v.build](v.build.html) module is used to
generate topology. It optionally allows the user to extract erroneous
vector objects into a separate map. Topological errors can be corrected
either manually within [wxGUI vector digitizer](wxGUI.vdigit.html) or,
to some extent, automatically in [v.clean](v.clean.html). A dedicated
vector editing module is [v.edit](v.edit.html) which supports global and
local editing operations. Adjacent polygons can be found by
[v.to.db](v.to.db.html) (see \'sides\' option).

Many operations including extraction, queries, overlay, and export will
only act on features which have been assigned a category number.
Typically a centroid will hold the attribute data for the area with
which the centroid is associated. Boundaries are not typically given a
category ID as it would be ambiguous as to which area either side of it
the attribute data would belong to. An exception might be when the
boundary between two crop-fields is the center-line of a road, and the
category information is an index to the road name. For everyday use
boundaries and centroids can be treated as internal data types and the
user can work directly and more simply with the \"area\" type.

### Vector object categories and attribute management

GRASS vectors can be linked to one or many database management systems
(DBMS). The *db.\** set of commands provides basic SQL support for
attribute management, while the *v.db.\** set of commands operates on a
table linked to a vector map.

-   **Categories**\
    Categories are used to categorize vector objects and link
    attribute(s) to each category. Each vector object can have zero, one
    or several categories. Category numbers do not have to be unique for
    each vector object, several vector objects can share the same
    category.\
    Category numbers are stored both within the geometry file for each
    vector object and within the (optional) attribute table(s) (usually
    the \"cat\" column). It is not required that attribute table(s) hold
    an entry for each category, and attribute table(s) can hold
    information about categories not present in the vector geometry
    file. This means that e.g. an attribute table can be populated first
    and then vector objects can be added to the geometry file with
    category numbers. Using [v.category](v.category.html), category
    numbers can be printed or maintained.\
    \
-   **Layers**\
    Layers are a characteristic of the vector feature (geometries) file.
    As mentioned above, categories allow the user to give either a
    unique id to each feature or to group similar features by giving
    them all the same id. Layers allow several parallel, but different
    groupings of features in a same map. The practical benefit of this
    system is that it allows placement of thematically distinct but
    topologically related objects into a single map, or for linking time
    series attribute data to a series of locations that did not change
    over time.\
    For example, one can have roads with one layer containing the unique
    id of each road and another layer with ids for specific routes that
    one might take, combining several roads by assigning the same id.
    While this example can also be dealt with in an attribute table,
    another example of the use of layers that shows their specific
    advantage is the possibility to identify the same geometrical
    features as representing different thematic objects. For example, in
    a map with a series of polygons representing fields in which at the
    same time the boundaries of these fields have a meaning as linear
    features, e.g. as paths, one can give a unique id to each field as
    area in layer 1, and a unique id in layer 2 to those boundary lines
    that are paths. If the paths will always depend on the field
    boundaries (and might change if these boundaries change) then
    keeping them in the same map can be helpful. Trying to reproduce the
    same functionality through attributes is much more complicated.\
    If a vector object has zero categories in a layer, then it does not
    appear in that layer. In this fashion some vector objects may appear
    in some layers but not in others. Taking the example of the fields
    and paths, only some boundaries, but not all, might have a category
    value in layer 2. With *option=report*,
    [v.category](v.category.html) lists available categories in each
    layer.\
    Optionally, each layer can (but does not have to) be linked to an
    attribute table. The link is made by the category values of the
    features in that layer which have to have corresponding entries in
    the specified key column of the table. Using
    [v.db.connect](v.db.connect.html) connections between layers and
    attribute tables can be listed or maintained.\
    In most modules, the first layer (*layer=1*) is active by default.
    Using *layer=-1* one can access all layers.
-   **SQL support**\
    By default, SQLite is used as the attribute database. Also other
    supported DBMS backends (such as SQLite, PostgreSQL, MySQL etc.)
    provide full SQL support as the SQL statements are sent directly to
    GRASS\' database management interface (DBMI). Only the DBF driver
    provides just very limited SQL support (as DBF is not an SQL DB).
    SQL commands can be directly executed with
    [db.execute](db.execute.html), [db.select](db.select.html) and the
    other db.\* modules.

When creating vector maps from scratch, in general an attribute table
must be created and the table must be populated with one row per
category (using [v.to.db](v.to.db.html)). However, this can be performed
in a single step using [v.db.addtable](v.db.addtable.html) along with
the definition of table column types. Column adding and dropping can be
done with [v.db.addcolumn](v.db.addcolumn.html) and
[v.db.dropcolumn](v.db.dropcolumn.html). A table column can be renamed
with [v.db.renamecolumn](v.db.renamecolumn.html). To drop a table from a
map, use [v.db.droptable](v.db.droptable.html). Values in a table can be
updated with [v.db.update](v.db.update.html). Tables can be joined with
with [v.db.join](v.db.join.html).

### Editing vector attributes

To manually edit attributes of a table, the map has to be queried in
\'edit mode\' using [d.what.vect](d.what.vect.html). To bulk process
attributes, it is recommended to use SQL
([db.execute](db.execute.html)).

### Geometry operations

The module [v.in.region](v.in.region.html) saves the current region
extents as a vector area. Split vector lines can be converted to
polylines by [v.build.polylines](v.build.polylines.html). Long lines can
be split by [v.split](v.split.html) and [v.segment](v.segment.html).
Buffer and circles can be generated with [v.buffer](v.buffer.html) and
[v.parallel](v.parallel.html). [v.generalize](v.generalize.html) is
module for generalization of GRASS vector maps. 2D vector maps can be
changed to 3D using [v.drape](v.drape.html) or
[v.extrude](v.extrude.html). If needed, the spatial position of vector
points can be perturbed by [v.perturb](v.perturb.html). The
[v.type](v.type.html) command changes between vector types (see list
above). Projected vector maps can be reprojected with
[v.proj](v.proj.html). Unprojected maps can be geocoded with
[v.transform](v.transform.html). Based on the control points,
[v.rectify](v.rectify.html) rectifies a vector map by computing a
coordinate transformation for each vector object. Triangulation and
point-to-polygon conversions can be done with
[v.delaunay](v.delaunay.html), [v.hull](v.hull.html), and
[v.voronoi](v.voronoi.html). The [v.random](v.random.html) command
generated random points.

### Vector overlays and selections

Geometric overlay of vector maps is done with [v.patch](v.patch.html),
[v.overlay](v.overlay.html) and [v.select](v.select.html), depending on
the combination of vector types. Vectors can be extracted with
[v.extract](v.extract.html) and reclassified with
[v.reclass](v.reclass.html).

### Vector statistics

Statistics can be generated by [v.qcount](v.qcount.html),
[v.sample](v.sample.html), [v.normal](v.normal.html),
[v.univar](v.univar.html), and [v.vect.stats](v.vect.stats.html).
Distances between vector objects are calculated with
[v.distance](v.distance.html).

### Vector-Raster-DB conversion

The [v.to.db](v.to.db.html) transfers vector information into database
tables. With [v.to.points](v.to.points.html),
[v.to.rast](v.to.rast.html) and [v.to.rast3](v.to.rast3.html)
conversions are performed. Note that a raster mask (\"MASK\") will not
be respected since it is only applied when *reading* an existing GRASS
raster map.

### Vector queries

Vector maps can be queried with [v.what](v.what.html) and
[v.what.vect](v.what.vect.html).

### Vector-Raster queries

Raster values can be transferred to vector maps with
[v.what.rast](v.what.rast.html) and [v.rast.stats](v.rast.stats).

### Vector network analysis

GRASS provides support for vector network analysis. The following
algorithms are implemented:

-   Network preparation and maintenance: [v.net](v.net.html)
-   Shortest path: [d.path](d.path.html) and
    [v.net.path](v.net.path.html)
-   Shortest path between all pairs of nodes
    [v.net.allpairs](v.net.allpairs.html)
-   Allocation of sources (create subnetworks, e.g. police station
    zones): [v.net.alloc](v.net.alloc.html)
-   Iso-distances (from centers): [v.net.iso](v.net.iso.html)
-   Computes bridges and articulation points:
    [v.net.bridge](v.net.bridge.html)
-   Computes degree, centrality, betweeness, closeness and eigenvector
    centrality measures: [v.net.centrality](v.net.centrality.html)
-   Computes strongly and weakly connected components:
    [v.net.components](v.net.components.html)
-   Computes vertex connectivity between two sets of nodes:
    [v.net.connectivity](v.net.connectivity.html)
-   Computes shortest distance via the network between the given sets of
    features: [v.net.distance](v.net.distance.html)
-   Computes the maximum flow between two sets of nodes:
    [v.net.flow](v.net.flow.html)
-   Computes minimum spanning
    tree:[v.net.spanningtree](v.net.spanningtree.html)
-   Minimum Steiner trees (star-like connections, e.g. broadband cable
    connections): [v.net.steiner](v.net.steiner.html)
-   Finds shortest path using timetables:
    [v.net.timetable](v.net.timetable.html)
-   Traveling salesman (round trip):
    [v.net.salesman](v.net.salesman.html)

Vector directions are defined by the digitizing direction (a\--\>\--b).
Both directions are supported, most network modules provide parameters
to assign attribute columns to the forward and backward direction.

### Vector networks: Linear referencing system (LRS)

LRS uses linear features and distance measured along those features to
positionate objects. There are the commands
[v.lrs.create](v.lrs.create.html) to create a linear reference system,
[v.lrs.label](v.lrs.label.html) to create stationing on the LRS,
[v.lrs.segment](v.lrs.segment.html) to create points/segments on LRS,
and [v.lrs.where](v.lrs.where.html) to find line id and real km+offset
for given points in vector map using linear reference system.

The [LRS tutorial](lrs.html) explains further details.

### Interpolation and approximation

Some of the vector modules deal with spatial or volumetric approximation
(also called interpolation): [v.kernel](v.kernel.html),
[v.surf.idw](v.surf.idw.html), [v.surf.rst](v.surf.rst.html), and
[v.vol.rst](v.vol.rst.html).

### Lidar data processing

Lidar point clouds (first and last return) are imported from text files
with [v.in.ascii](v.in.ascii.html) or from LAS files with
[v.in.lidar](v.in.lidar.html). Both modules recognize the -b flag to not
build topology. Outlier detection is done with
[v.outlier](v.outlier.html) on both first and last return data. Then,
with [v.lidar.edgedetection](v.lidar.edgedetection.html), edges are
detected from last return data. The building are generated by
[v.lidar.growing](v.lidar.growing.html) from detected edges. The
resulting data are post-processed with
[v.lidar.correction](v.lidar.correction.html). Finally, the DTM and DSM
are generated with [v.surf.bspline](v.surf.bspline.html) (DTM: uses the
\'v.lidar.correction\' output; DSM: uses last return output from outlier
detection).\
In addition, [v.decimate](v.decimate.html) can be used to decimates a
point cloud.

### See also

-   [Introduction to raster data processing](rasterintro.html)
-   [Introduction to 3D raster data (voxel)
    processing](raster3dintro.html)
-   [Introduction to image processing](imageryintro.html)
-   [Introduction into temporal data processing](temporalintro.html)
-   [Introduction to database management](databaseintro.html)
-   [Projections and spatial transformations](projectionintro.html)
