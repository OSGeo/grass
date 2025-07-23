## DESCRIPTION

*v.net.flow* computes the maximum flow and minimum cut between two sets
of nodes.

## NOTES

The two sets of nodes are called *sources* and *sink* and *v.net.flow*
finds the maximum flow from the former to the latter ones. Edge
capacities can be specified by **arc_column** for forward direction and
**arc_backward_column** for backward direction. If the latter parameter
is omitted then the same capacity is used in either direction. The sets
are given by the respective **cats**, **layer** and **where**
parameters. Maximum flow corresponds to the maximum amount of water
possibly flowing through the network preserving the capacity constraints
and minimum cut to the set of edges of minimum total capacity completely
separating sources from sinks. The cut produced by this module
corresponds to the first fully saturated edges from sources to sinks. An
attribute table containing the flow information is linked to the
**output** map. The table consists of two columns: *cat* and *flow* and
stores the flow along each line. Negative flow means that "water" is
flowing in the backward direction. **Cut** map contains the edges in the
minimum cut.  
A famous
[result](https://en.wikipedia.org/wiki/Max-flow_min-cut_theorem) says
that the total amount of water flowing is equal to the minimum cut.

## EXAMPLES

Find maximum flow from factories to stores using SPEED for the
capacities.

```sh
v.net.flow input=roads output=roads_flow cut=roads_cut arc_column=SPEED \
           source_where="type=factory" sink_where="type=store"
```

If all the capacties are one then the minimum cut corresponds to the
minimum number of edges separating sources from sinks.

```sh
v.net.flow input=network output=flow cut=cut arc_column=ones \
           source_cats=1-10 sink_cats=100-100
```

## SEE ALSO

*[v.net](v.net.md), [v.net.connectivity](v.net.connectivity.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor
