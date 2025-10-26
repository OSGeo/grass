## DESCRIPTION

*v.lrs.where* identifies line id and real milepost+offset for points in
vector map using linear reference system.

## EXAMPLE

This example is written for the Spearfish dataset (it continues the
example from *v.lrs.create*).

In this example, the 'route_lrs' shall be queried for unknown positions
(points, stored in the map *newpoints*) along the LRS:

```sh
# generate query points
echo "590866.15|4926737.0
590933|4927133" | v.in.ascii out=newpoints

v.lrs.where lines=route_lrs points=newpoints rstable=route_lrs
pcat|lid|mpost|offset
pcat|lid|mpost|offset
1|22|4.000000+212.091461
2|22|6.000000+188.112093

# verification
g.region vector=route_lrs n=n+100 s=s-100 -p
d.erase
d.vect route_lrs
d.vect busstops disp=attr attr=cat size=10 bg=white lcol=blue yref=bottom
d.vect busstops icon=basic/circle fcol=blue
d.vect newpoints col=red

# measure distance to previous bus stop:
# use measuring tool in graphical user interface
```

## SEE ALSO

*[v.lrs.where](v.lrs.create.md), [v.lrs.segment](v.lrs.segment.md),
[v.lrs.label](v.lrs.label.md)*

*[LRS tutorial](lrs.md),  
[Introducing the Linear Reference System in
GRASS](https://foss4g.asia/2004/Full-Paper_PDF/Introducing-the-Linear-Reference-System-in-GRASS.pdf)*

## AUTHORS

Radim Blazek, ITC-irst/MPA Solutions  
Documentation update (based on above journal article and available
fragments): Markus Neteler
