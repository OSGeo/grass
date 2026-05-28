## DESCRIPTION

*v.lrs.label* generates LRS labels for pretty-printing of a LRS. This
example is written for the Spearfish dataset (it continues the example
from *v.lrs.create*).

```sh
v.lrs.label route_lrs rstable=route_lrs output=route_lrs_labels \
            labels=labels col=red size=50 xoffset=100

g.region vector=route_lrs n=n+100 s=s-100 -p
d.erase
d.vect route_lrs
d.vect route_lrs_labels col=grey type=line
d.vect busstops disp=attr attr=cat size=10 bg=white lcol=green yref=bottom
d.vect busstops icon=basic/circle fcol=green
d.labels labels
```

## SEE ALSO

*[v.lrs.create](v.lrs.create.md), [v.lrs.segment](v.lrs.segment.md),
[v.lrs.where](v.lrs.where.md), [d.labels](d.labels.md),
[v.label](v.label.md)*

*[LRS tutorial](lrs.md),  
[Introducing the Linear Reference System in
GRASS](https://foss4g.asia/2004/Full-Paper_PDF/Introducing-the-Linear-Reference-System-in-GRASS.pdf)*

## AUTHORS

Radim Blazek, ITC-irst/MPA Solutions  
Documentation update (based on above journal article and available
fragments): Markus Neteler
