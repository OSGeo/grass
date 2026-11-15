## DESCRIPTION

*v.net.salesman* calculates the optimal route to visit nodes on a vector
network.

Costs may be either line lengths or attributes saved in a database
table. These attribute values are taken as costs of whole segments, not
as costs to traverse a length unit (e.g. meter) of the segment. For
example, if the speed limit is 100 km / h, the cost to traverse a 10 km
long road segment must be calculated as  
length / speed = 10 km / (100 km/h) = 0.1 h.  
Supported are cost assignments for arcs, and also different costs for
both directions of a vector line. For areas, costs will be calculated
along boundary lines.

The input vector needs to be prepared with *v.net operation=connect* in
order to connect points representing center nodes to the network.

Points specified by category must be exactly on network nodes, and the
input vector map needs to be prepared with *v.net operation=connect*.

The flag **-t** enables turntable support. This flag requires additional
parameters, **turn_layer** and **turn_cat_layer**, that are otherwise
ignored. The turntable allows to model e.g. traffic code, where some
turns may be prohibited. This means that the input layer is expanded by
turntable with costs of every possible turn on any possible node
(intersection) in both directions. Turntable can be created by the
*[v.net](v.net.md)* module. For more information about turns in the
vector network analyses see [wiki
page](https://grasswiki.osgeo.org/wiki/Turns_in_the_vector_network_analysis).

## NOTES

Arcs can be closed using cost = -1. Turns support: The costs of turns on
visiting nodes are not taken in account.

## EXAMPLE

Traveling salesman for 6 digitized nodes (Spearfish):

Shortest path, along unimproved roads:

![v.net.salesman example using distance](vnetsalesman.png)

Fastest path, along highways:

![v.net.salesman example using time](vnetsalesmantime.png)

Searching for the shortest path using distance and the fastest path
using traveling time according to the speed limits of different road
types:

```sh
# Spearfish

g.copy vect=roads,myroads

# we have 6 locations to visit on our trip
echo "1|601653.5|4922869.2|a
2|608284|4923776.6|b
3|601845|4914981.9|c
4|596270|4917456.3|d
5|593330.8|4924096.6|e
6|598005.5|4921439.2|f" | v.in.ascii in=- cat=1 x=2 y=3 out=centers col="cat integer, \
                         east double precision, north double precision, label varchar(43)"

# verify data preparation
v.db.select centers
v.category centers op=report
# type       count        min        max
# point          6          1          6


# create lines map connecting points to network (on layer 2)
v.net myroads points=centers out=myroads_net op=connect thresh=500
v.category myroads_net op=report
# Layer / table: 1 / myroads_net
# type       count        min        max
# line         837          1          5
#
# Layer: 2
# type       count        min        max
# point          6          1          5

# find the shortest path
v.net.salesman myroads_net center_cats=1-6 out=mysalesman_distance

# set up costs as traveling time

# create unique categories for each road in layer 3
v.category in=myroads_net out=myroads_net_time opt=add cat=1 layer=3 type=line

# add new table for layer 3
v.db.addtable myroads_net_time layer=3 col="cat integer,label varchar(43),length double precision,speed double precision,cost double precision,bcost double precision"

# copy road type to layer 3
v.to.db myroads_net_time layer=3 qlayer=1 opt=query qcolumn=label columns=label

# upload road length in miles
v.to.db myroads_net_time layer=3 type=line option=length col=length unit=miles

# set speed limits in miles / hour
v.db.update myroads_net_time layer=3 col=speed val="5.0"
v.db.update myroads_net_time layer=3 col=speed val="75.0" where="label='interstate'"
v.db.update myroads_net_time layer=3 col=speed val="75.0" where="label='primary highway, hard surface'"
v.db.update myroads_net_time layer=3 col=speed val="50.0" where="label='secondary highway, hard surface'"
v.db.update myroads_net_time layer=3 col=speed val="25.0" where="label='light-duty road, improved surface'"
v.db.update myroads_net_time layer=3 col=speed val="5.0" where="label='unimproved road'"

# define traveling costs as traveling time in minutes:

# set forward costs
v.db.update myroads_net_time layer=3 col=cost val="length / speed * 60"
# set backward costs
v.db.update myroads_net_time layer=3 col=bcost val="length / speed * 60"

# find the fastest path
v.net.salesman myroads_net_time arc_layer=3 node_layer=2 arc_column=cost arc_backward_column=bcost center_cats=1-6 out=mysalesman_time
```

To display the result, run for example:

```sh
# Display the results
g.region vector=myroads_net

# shortest path
d.mon x0
d.vect myroads_net
d.vect centers -c icon=basic/triangle
d.vect mysalesman_distance col=green width=2
d.font Vera
d.vect centers col=red disp=attr attrcol=label lsize=12

# fastest path
d.mon x1
d.vect myroads_net
d.vect centers -c icon=basic/triangle
d.vect mysalesman_time col=green width=2
d.font Vera
d.vect centers col=red disp=attr attrcol=label lsize=12
```

## SEE ALSO

*[d.path](d.path.md), [v.net](v.net.md), [v.net.alloc](v.net.alloc.md),
[v.net.iso](v.net.iso.md), [v.net.path](v.net.path.md),
[v.net.steiner](v.net.steiner.md)*

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
Markus Metz  
Documentation: Markus Neteler, Markus Metz

### TURNS SUPPORT

The turns support was implemented as part of GRASS GIS turns cost
project at Czech Technical University in Prague, Czech Republic.  
Eliska Kyzlikova, Stepan Turek, Lukas Bocan and Viera Bejdova
participated in the project.

Implementation: Stepan Turek  
Documentation: Lukas Bocan  
Mentor: Martin Landa
