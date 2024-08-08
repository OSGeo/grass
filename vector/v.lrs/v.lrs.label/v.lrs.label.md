## DESCRIPTION

*v.lrs.label* generates LRS labels for pretty-printing of a LRS. This
example is written for the Spearfish dataset (it continues the example
from *v.lrs.create*).

```
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

*[v.lrs.create](v.lrs.create.html), [v.lrs.segment](v.lrs.segment.html),
[v.lrs.where](v.lrs.where.html), [d.labels](d.labels.html),
[v.label](v.label.html)*

*[LRS tutorial](lrs.html),\
[Introducing the Linear Reference System in
GRASS](http://gisws.media.osaka-cu.ac.jp/grass04/viewpaper.php?id=50)*

## AUTHORS

Radim Blazek, ITC-irst/MPA Solutions\
Documentation update (based on above journal article and available
fragments): Markus Neteler
