## DESCRIPTION

*v.lrs.segment* creates points/segments from input lines, linear
reference system and positions read from `standard in` or a file.

The format is as follows:  

```sh
P <point_id> <line_id> <milepost>+<offset> [<side offset>]
L <segment_id> <line_id> <milepost>+<offset> <milepost>+<offset> [<side offset>]
```

## NOTES

For more information and examples see the help page for
*v.lrs.segment*'s sister module, *[v.segment](v.segment.md)*.

## EXAMPLE

This example is written for the Spearfish dataset (it continues the
example from *v.lrs.create*).

In this example, the 'route_lrs' shall be extended for a new position
(point) along the LRS after bus stop 4:

```sh
# new point on LRS
echo "P 7 22 4+180" | v.lrs.segment route_lrs out=route_lrs_new rstable=route_lrs

g.region vector=route_lrs n=n+100 s=s-100 -p
d.erase
# existing LRS
d.vect route_lrs
d.vect busstops disp=attr attr=cat size=10 bg=white lcol=blue yref=bottom
d.vect busstops icon=basic/circle fcol=blue
db.select table=route_lrs

# show modified map
d.vect route_lrs_new col=red
```

## TODO

Figure out how to merge result into existing LRS map and table.

## SEE ALSO

*[v.lrs.create](v.lrs.create.md), [v.lrs.where](v.lrs.where.md),
[v.lrs.label](v.lrs.label.md), [v.segment](v.segment.md)*

*[LRS tutorial](lrs.md),  
[Introducing the Linear Reference System in
GRASS](https://foss4g.asia/2004/Full-Paper_PDF/Introducing-the-Linear-Reference-System-in-GRASS.pdf)*

## AUTHOR

Radim Blazek
