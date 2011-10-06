#!/usr/bin/env python
# Python script to verify r.gwflow calculation, this calculation is based on
# the example at page 133 of the following book:
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
grass.run_command("g.region", res=100, n=700, s=0, w=0, e=700)

grass.run_command("r.mapcalc", expression="phead=50")
grass.run_command("r.mapcalc", expression="status=if(col() == 1 || col() == 7 , 2, 1)")
grass.run_command("r.mapcalc", expression="well=if((row() == 4 && col() == 4), -0.1, 0)")
grass.run_command("r.mapcalc", expression="hydcond=0.0005")
grass.run_command("r.mapcalc", expression="recharge=0")
grass.run_command("r.mapcalc", expression="top_conf=20")
grass.run_command("r.mapcalc", expression="bottom=0")
grass.run_command("r.mapcalc", expression="s=0.0001")
grass.run_command("r.mapcalc", expression="null=0.0")

#First compute the groundwater flow
grass.run_command("r.gwflow", "f", solver="cholesky", top="top_conf", bottom="bottom", phead="phead",\
 status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="s",\
 recharge="recharge", output="gwresult_conf", dt=500, type="confined", budget="water_budget")

count=500
# loop over the timesteps
for i in range(20):
    grass.run_command("r.gwflow", "f", solver="cholesky", top="top_conf", bottom="bottom", phead="gwresult_conf",\
     status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="s",\
     recharge="recharge", output="gwresult_conf", dt=500, type="confined", budget="water_budget")
    count += 500

