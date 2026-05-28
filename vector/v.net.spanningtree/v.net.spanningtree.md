## DESCRIPTION

*v.net.spanningtree* finds the minimum spanning tree in a network.

## NOTES

A spanning tree is a minimum cost subnetwork connecting all nodes in an
undirected network (same forward and backward costs). If a network is
disconnected then the module computes the minimum spanning tree for each
(weakly) connected component. So, strictly speaking,
*v.net.spanningtree* does not compute spanning tree but a spanning
forest. As the name suggests, a spanning tree is a tree. That is, it
contains no cycles and if a component has N nodes then the tree has N-1
edges connecting all nodes. **Accol** is used to specify the costs of
the edges. The **output** consists of the edges in the spanning tree.

## EXAMPLES

Find cheapest set of pipelines connecting all nodes.

```sh
v.net.spanningtree input=projected_pipelines output=spanningtree accol=cost
```

## SEE ALSO

*[v.net](v.net.md), [v.net.steiner](v.net.steiner.md)*

## AUTHORS

Daniel Bundala, Google Summer of Code 2009, Student  
Wolf Bergenheim, Mentor
