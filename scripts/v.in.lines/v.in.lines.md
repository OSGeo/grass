## DESCRIPTION

Imports a stream of ASCII x,y\[,z\] coordinates as a line or series of
lines.

## NOTES

Input ASCII coordinates are simply a series of "x y" data points. Lines
are separated by a row containing "`NaN NaN`".

The user can import 3D lines by providing 3 columns of data in the input
stream and using the **-z** flag.

This script is a simple wrapper around the *v.in.mapgen* module.

## EXAMPLE

```sh
v.in.lines in=- out=two_lines separator=, <<EOF
167.846717,-46.516653
167.846663,-46.516645
167.846656,-46.516644
167.846649,-46.516644
167.846642,-46.516643
NaN,NaN
167.846520,-46.516457
167.846528,-46.516461
167.846537,-46.516464
167.846535,-46.516486
167.846544,-46.516489
167.846552,-46.516493
EOF
```

## SEE ALSO

*[d.graph](d.graph.md), [v.centroids](v.centroids.md),
[v.in.ascii](v.in.ascii.md), [v.in.mapgen](v.in.mapgen.md),
[v.in.region](v.in.region.md), [v.out.ascii](v.out.ascii.md),
[r.in.poly](r.in.poly.md)*

## AUTHOR

Hamish Bowman  
Dunedin, New Zealand
