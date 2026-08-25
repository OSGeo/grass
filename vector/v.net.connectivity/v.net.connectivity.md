## DESCRIPTION

*v.net.connectivity* computes vertex connectivity between two sets, i.e.
the minimum number of vertices whose removal would separate two given
sets.

## NOTES

Two sets (*set1* and *set2*) are specified by respective **layer**,
**where** and **cats** parameters. Similarly to
[v.net.flow](v.net.flow.md) module, capacities of nodes can be given by
**node_column** option. *v.net.connectivity* finds the set of nodes of
minimum total capacitiy separating the two given sets and outputs map
containing points on the positions of these nodes. Default capacity,
which is used when no column is specified, is one.

## EXAMPLE

The following command finds the minimum number of intersections
separating roads on the left bank from roads on the right bank.

```sh
v.net.connectivity input=roads output=roads_conn set1_where="bank=left" \
      set2_where="bank=right"
```

```sh
v.net.connectivity input=airtraffic output=connectivity \
      set1_where="name=JFK" set2_where="name=Heathrow" node_column=capacity
```

## SEE ALSO

*[v.net](v.net.md), [v.net.flow](v.net.flow.md),
[v.net.bridge](v.net.bridge.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor
