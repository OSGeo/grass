## DESCRIPTION

*v.net.centrality* computes degree, closeness, betweenness and
eigenvector centrality measures.

## NOTES

The module computes various centrality measures for each node and stores
them in the given columns of an attribute table, which is created and
linked to the output map. For the description of these, please check the
following [wikipedia article](https://en.wikipedia.org/wiki/Centrality).
If the column name is not given for a measure then that measure is not
computed. If **-a** flag is set then points are added on nodes without
points. Also, the points for which the output is computed can be
specified by **cats**, **layer** and **where** parameters. However, if
any of these parameters is present then **-a** flag is ignored and no
new points are added.  
Betweenness measure is not normalised. In order to get the normalised
values (between 0 and 1), each number needs to be divided by *N choose
2=N\*(N-1)/2* where N is the number of nodes in the connected component.
Computation of eigenvector measure terminates if the given number of
iterations is reached or the cumulative *squared* error between the
successive iterations is less than **error**.

## EXAMPLES

Compute closeness and betweenness centrality measures for each node and
produce a map containing not only points already present in the input
map but a map with point on every node.

```sh
v.net.centrality input=roads output=roads_cent closeness=closeness \
      betweenness=betweenness -a
```

## SEE ALSO

*[v.net](v.net.md), [v.generalize](v.generalize.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor
