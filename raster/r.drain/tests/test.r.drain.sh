#!/bin/sh

# to be run in the North Carolina location

export GRASS_OVERWRITE=1

r.in.ascii testascii_nc.asc out=testascii
g.region rast=testascii -p

d.mon wx0
sleep 2
d.rast.num testascii

r.drain testascii out=drain coord=638442.5,220548.5
d.rast -o drain
