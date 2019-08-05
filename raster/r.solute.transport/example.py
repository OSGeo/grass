#!/usr/bin/env python3
# This is an example script how groundwater flow and solute transport are
# computed within grass
import sys
import os
import grass.script as grass

# Overwrite existing maps
grass.run_command("g.gisenv", set="OVERWRITE=1")

grass.message("Set the region")

# The area is 200m x 100m with a cell size of 1m x 1m
grass.run_command("g.region", res=1, res3=1, t=10, b=0, n=100, s=0, w=0, e=200)
grass.run_command("r.mapcalc", expression="phead=if(col() == 1 , 50, 40)")
grass.run_command("r.mapcalc", expression="phead=if(col() ==200  , 45 + row()/40, phead)")
grass.run_command("r.mapcalc", expression="status=if(col() == 1 || col() == 200 , 2, 1)")
grass.run_command("r.mapcalc", expression="well=if((row() == 50 && col() == 175) || (row() == 10 && col() == 135) , -0.001, 0)")
grass.run_command("r.mapcalc", expression="hydcond=0.00005")
grass.run_command("r.mapcalc", expression="recharge=0")
grass.run_command("r.mapcalc", expression="top_conf=20")
grass.run_command("r.mapcalc", expression="bottom=0")
grass.run_command("r.mapcalc", expression="poros=0.17")
grass.run_command("r.mapcalc", expression="syield=0.0001")
grass.run_command("r.mapcalc", expression="null=0.0")
#
grass.message(_("Compute a steady state groundwater flow"))

grass.run_command("r.gwflow", solver="cg", top="top_conf", bottom="bottom", phead="phead",\
  status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="syield",\
  recharge="recharge", output="gwresult_conf", dt=8640000000000, type="confined")

grass.message(_("generate the transport data"))
grass.run_command("r.mapcalc", expression="c=if(col() == 15 && row() == 75 , 500.0, 0.0)")
grass.run_command("r.mapcalc", expression="cs=if(col() == 15 && row() == 75 , 0.0, 0.0)")
grass.run_command("r.mapcalc", expression="tstatus=if(col() == 1 || col() == 200 , 3, 1)")
grass.run_command("r.mapcalc", expression="diff=0.0000001")
grass.run_command("r.mapcalc", expression="R=1.0")

# Compute the initial state
grass.run_command("r.solute.transport", solver="bicgstab", top="top_conf",\
  bottom="bottom", phead="gwresult_conf", status="tstatus", hc_x="hydcond", hc_y="hydcond",\
  rd="R", cs="cs", q="well", nf="poros", output="stresult_conf_0", dt=3600, diff_x="diff",\
  diff_y="diff", c="c", al=0.1, at=0.01)

# Compute the solute transport for 300 days in 10 day steps
for dt in range(30):
    grass.run_command("r.solute.transport", solver="bicgstab", top="top_conf",\
    bottom="bottom", phead="gwresult_conf", status="tstatus", hc_x="hydcond", hc_y="hydcond",\
    rd="R", cs="cs", q="well", nf="poros", output="stresult_conf_" + str(dt + 1), dt=864000, diff_x="diff",\
    diff_y="diff", c="stresult_conf_" + str(dt), al=0.1, at=0.01, vx="vx", vy="vy")

