#!/usr/bin/env python
# Shellscript to verify r.gwflow calculation, this calculation is based on
# the example at page 167 of the following book:
#	author = "Kinzelbach, W. and Rausch, R.",
#	title = "Grundwassermodellierung",
#	publisher = "Gebr{\"u}der Borntraeger (Berlin, Stuttgart)",
#	year = "1995"
#
import sys
import os
import grass.script as grass

# Overwrite existing maps
grass.run_command("g.gisenv", set="OVERWRITE=1")

grass.message(_("Set the region"))

# The area is 2000m x 1000m with a cell size of 25m x 25m
grass.run_command("g.region", res=50, n=950, s=0, w=0, e=2000)

grass.run_command("r.mapcalc", expression="phead= if(row() == 19, 5, 3)")
grass.run_command("r.mapcalc", expression="status=if((col() == 1 && row() == 13) ||\
                     (col() == 1 && row() == 14) ||\
		     (col() == 2 && row() == 13) ||\
		     (col() == 2 && row() == 14) ||\
		     (row() == 19), 2, 1)")

grass.run_command("r.mapcalc", expression="hydcond=0.001")
grass.run_command("r.mapcalc", expression="recharge=0.000000006")
grass.run_command("r.mapcalc", expression="top=20")
grass.run_command("r.mapcalc", expression="bottom=0")
grass.run_command("r.mapcalc", expression="poros=0.1")
grass.run_command("r.mapcalc", expression="null=0.0")

#compute a steady state groundwater flow
grass.run_command("r.gwflow", "f", solver="cholesky", top="top", bottom="bottom", phead="phead", \
 status="status", hc_x="hydcond", hc_y="hydcond", s="poros", \
 recharge="recharge", output="gwresult", dt=864000000000, type="unconfined", budget="water_budget")
