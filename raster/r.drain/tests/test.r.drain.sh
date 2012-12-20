#!/bin/sh

# to be run in the North Carolina location

export GRASS_OVERWRITE=1

r.in.ascii testascii_nc.asc out=testascii
g.region rast=testascii -p

d.mon wx0
sleep 2
d.rast.num testascii

r.drain input=testascii output=drain coord=638442.5,220548.5
d.rast drain

d.erase
r.mapcalc "testascii_float = float(testascii)"
r.drain input=testascii_float output=drain coord=638442.5,220548.5
d.rast drain

# for statistical test results:
# - verify number of resulting pixels (r.univar -g)
# - verify minimum and maximum of result (r.univar -g)
