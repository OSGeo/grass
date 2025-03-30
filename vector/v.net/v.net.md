## DESCRIPTION

*v.net* is used for network preparation and maintenance. Its main use is
to create a vector network from vector lines (*arcs* ) and points
(*nodes*) by creating nodes from intersections in a map of vector lines
(*node* operator), by connecting a vector lines map with a points map
(*connect* operator), and by creating new lines between pairs of vector
points (*arcs* operator).

A GIS network consists of topologically correct lines (arcs). That is,
the lines must be connected by shared vertices where real connections
exist. In GRASS GIS you also can add nodes to the network. These are
specially designated vertices used for analyzing network properties or
computing cost/distance measures. That is, **not all vertices are
treated as nodes by default**. Only *[v.net.path](v.net.path.md)* can
use a network without nodes, they are required for all the other network
modules. In GRASS, network arcs are stored in one data layer (normally
layer 1) and nodes are stored in a different data layer (normally layer
2).

*v.net* offers two ways to add nodes to a network of arcs and one method
to add arcs to a set of nodes:

1. Use the *connect* operation to create nodes from a vector points
    file and add these nodes to an existing vector network of arcs
    (i.e., lines/boundaries). This is useful when the goal is to analyze
    a set of places (points) in relation to a network--for example
    travel costs between places. Only points within the *thresh*
    (threshold) distance to a line/boundary will be connected as network
    nodes. There are two ways to connect nodes. By default, *v.net* will
    create new lines connecting each point to the closest line of the
    network. If you use the *-s* flag, however, the new nodes will be
    added on the closest line of the network at the point closest to the
    point you wish to add. When using the *connect* operation, some
    lines will share the same category. In order to assign unique costs
    to each line, a new layer needs to be created with  
    `v.category input=yourmap option=add cat=1 step=1 layer=3 output=newmap`  
    followed by  
    `v.db.addtable map=newmap layer=3 table=tablename`.
2. Create nodes and arcs from a vector line/boundary file using the
    *node* operation. This is useful if you are mostly interested in the
    network itself and thus you can use intersections of the network as
    start and end points. Nodes will be created at all intersections of
    two or more lines. For an *arc* that consists of several segments
    connected by vertices (the typical case), only the starting and
    ending vertices are treated as network nodes.
3. Create straight-line arcs between pairs of nodes with the *arcs*
    option. This produces networks like those of airline flights between
    airports. It is also similar to the kind of network created with
    social networking software, making it possible to create
    georeferenced social networks.

While the arcs created with v.net will retain any attribute information
associated with the input vector line/boundary file in data layer 1,
nodes created and stored in data layer 2 will not have any associated
attribute information.

For nodes created using the *connect* and *arcs* operations (methods 1
and 3 above), the nodes can be reconnected to the attribute table of the
input vector points file using the attribute table manager ("manage
layers" tab) or by running *[v.db.connect](v.db.connect.md)*.

For nodes created using the *nodes* operation (method 2 above), it is
possible to create an attribute table for the new nodes in layer 2 using
the attribute table manager and connect it to layer 2 ("manage layers"
tab) or to create a table with *[v.db.addtable](v.db.addtable.md)*,
connect it to layer 2 with *[v.db.connect](v.db.connect.md)*, and update
the new table with cat values with *[v.to.db](v.to.db.md)*.

The *turntable* operation creates a turntable with the costs for every
possible turn on every possible node (intersection, crossroad) in given
layer (arc_layer). U-turns are taken in account too. Turntable is
created in **turn_layer** and **turn_cat_layer**. Building the turntable
allows you to model e.g. traffic code, where some turns may be
prohibited. If features in analyzed network are changed, the turntable
must be created again (e.g. it includes v.net connect operation).
Turntable name consists of output vector map name + "\_turntable\_" +
"t" + "\_" + turn_layer + "\_" + "tuc" + "\_" + turn_cat_layer + "\_" +
"a" + "\_" + arc_layer e. g. roads_turntable_t_3_tuc_4_a_1

These modules are able to work with the turntable:
*[v.net.alloc](v.net.alloc.md), [v.net.iso](v.net.iso.md),
[v.net.path](v.net.path.md), [v.net.salesman](v.net.salesman.md)* For
more information about turns in the vector network analyses see the
"turns" [wiki
page](https://grasswiki.osgeo.org/wiki/Turns_in_the_vector_network_analysis).

Once a vector network has been created, it can be analyzed in a number
of powerful ways using the suite of *v.net*.\* modules. The shortest
route between two nodes, following arcs, can be computed
(*[v.net.path](v.net.path.md)*), as can the shortest route that will
pass through a set of nodes and return to the starting node
(*[v.net.salesman](v.net.salesman.md)*). Least cost routes through the
network can be calculated on the basis of distance only or on the basis
of distance weighted by an attribute associated with each arc (for
example, travel speed along a network segment). A network can be divided
into concentric zones of equal travel cost around one or more nodes
(*[v.net.iso](v.net.iso.md)*) or subdivided so that each node is
surrounded by a zone in which all arcs can be reached with the same
travel costs as all arcs surrounding each other node
(*[v.net.alloc](v.net.alloc.md)*). In addition to the modules listed
above, the GRASS vector networking suite includes numerous other modules
for analysis of network costs and connectivity. These include:
*[v.net.allpairs](v.net.allpairs.md), [v.net.bridge](v.net.bridge.md),
[v.net.centrality](v.net.centrality.md),
[v.net.components](v.net.components.md),
[v.net.distance](v.net.distance.md), [v.net.flow](v.net.flow.md),
[v.net.spanningtree](v.net.spanningtree.md),
[v.net.steiner](v.net.steiner.md),
[v.net.timetable](v.net.timetable.md),
[v.net.visibility](v.net.visibility.md)*

## NOTES

For a vector map prepared for network analysis in GRASS, nodes are
represented by the grass-internal geometry type *node* and arcs by the
geometry type *line*. If vector editing is required to modify the graph,
*[g.gui.vdigit](g.gui.vdigit.md)* or *[v.edit](v.edit.md)* can be used.
See also the [Linear Referencing System](lrs.md) available in GRASS GIS.

## EXAMPLES

The examples are [North Carolina
dataset](https://grassbook.org/datasets/datasets-3rd-edition/) based.

### Create nodes globally for all line ends and intersections

```sh
v.net input=streets_wake output=streets_node operation=nodes
# verify result
v.category streets_node option=report
```

### Merge in nodes from a separate map within given threshold

```sh
v.net input=streets_wake points=firestations out=streets_net \
      operation=connect threshold=500
# verify result
v.category streets_net option=report
```

The nodes are stored in layer 2 unless `node_layer=1` is used.

### Generating network for vector point map

For generating network for given vector point map an input file in the
following format is required:

```sh
[category of edge] [category of start node] [category of end node]
```

Option 1: Save the file (e.g. "points.txt") and generate the map:

```sh
v.net points=geodetic_swwake_pts output=geodetic_swwake_pts_net \
      operation=arcs file=points.txt
# verify result
v.category geodetic_swwake_pts_net option=report
```

Option 2: Read in from command line:

```sh
v.net points=geodetic_swwake_pts output=geodetic_swwake_pts_net \
      operation=arcs file=- << EOF
1 28000 28005
2 27945 27958
3 27886 27897
EOF

# verify result
v.category geodetic_swwake_pts_net option=report
```

### Generating network with turntable for vector point map

Following example generates a vector map with turntable:

```sh
v.net operation=turntable in=railroads out=railroads_ttb
```

## SEE ALSO

*[g.gui.vdigit](g.gui.vdigit.md), [v.edit](v.edit.md), [Vector Network
Analysis Tool](wxGUI.vnet.md)*

*[v.net.alloc](v.net.alloc.md), [v.net.allpairs](v.net.allpairs.md),
[v.net.bridge](v.net.bridge.md),
[v.net.centrality](v.net.centrality.md),
[v.net.components](v.net.components.md),
[v.net.connectivity](v.net.connectivity.md),
[v.net.distance](v.net.distance.md), [v.net.flow](v.net.flow.md),
[v.net.iso](v.net.iso.md), [v.net.path](v.net.path.md),
[v.net.salesman](v.net.salesman.md),
[v.net.spanningtree](v.net.spanningtree.md),
[v.net.steiner](v.net.steiner.md),
[v.net.timetable](v.net.timetable.md),
[v.net.visibility](v.net.visibility.md)*

## AUTHORS

Radim Blazek, ITC-irst, Trento, Italy  
Martin Landa, FBK-irst (formerly ITC-irst), Trento, Italy and CTU in
Prague, Czech Republic (operation 'connect' and 'arcs')  
Markus Metz: important fixes and improvements

### TURNS SUPPORT

The turns support was implemnented as part of GRASS GIS turns cost
project at Czech Technical University in Prague, Czech Republic. Eliska
Kyzlikova, Stepan Turek, Lukas Bocan and Viera Bejdova participated at
the project. Implementation: Stepan Turek Documentation: Lukas Bocan
Mentor: Martin Landa
