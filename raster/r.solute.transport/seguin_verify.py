#!/usr/bin/env python
# Shellscript to verify r.solute.transport calculation, this calculation is based on
# the example 1.1 and 1.2 at page 175 of the following book:
#	title = "Str{\"o}mungs und Transportmodellierung",
#	author = "Lege, T. and Kolditz, O. and Zielke W.",
#	publisher = "Springer (Berlin; Heidelberg; New York; Barcelona; Hongkong; London; Mailand; Paris; Singapur; Tokio)",
#	edition = "2. Auflage",
#	year = "1996",
#	series = "Handbuch zur Erkundung des Untergrundes von Deponien und Altlasten"

import sys
import os
import grass.script as grass

# Overwrite existing maps
grass.run_command("g.gisenv", set="OVERWRITE=1")

grass.message(_("Set the region"))

# The area is 2000m x 1000m with a cell size of 25m x 25m
grass.run_command("g.region", res=25, res3=25, t=25, b=0, n=1000, s=0, w=0, e=2000)

grass.message(_("Create all the input maps needed for groundwater flow computation"))

# Initial condition of the piezometric head, we have a huge gradient from 280m to 50m
# over a distance of 2000m
grass.run_command("r.mapcalc", expression="phead_1=if(col() == 1 , 200, 50)")
# Set the active cells and the dirichlet boundary condition
grass.run_command("r.mapcalc", expression="status_1=if(col() == 1 || col() == 80 , 2, 1)")
# We have a no wells
grass.run_command("r.mapcalc", expression="well_1=0")
# The hydraulic conductivity
grass.run_command("r.mapcalc", expression="hydcond_1=0.0001")
# The recharge, well we have no recharge at all
grass.run_command("r.mapcalc", expression="recharge_1=0")
# We have a confined aquifer with a height of 25m
grass.run_command("r.mapcalc", expression="top_conf_1=25")
# Bottom of the aquifer starts at 0m
grass.run_command("r.mapcalc", expression="bottom_1=0")
# This porosity of sand aquifer
grass.run_command("r.mapcalc", expression="poros_1=0.17")
# The specific yield of a confined aquifer
grass.run_command("r.mapcalc", expression="syield_1=0.0001")
grass.run_command("r.mapcalc", expression="null_1=0.0")

grass.message("First compute a steady state groundwater flow")

# Compute the steady state groundwater flow
grass.run_command("r.gwflow", "s", solver="cg", top="top_conf_1", bottom="bottom_1", phead="phead_1",\
 status="status_1", hc_x="hydcond_1", hc_y="hydcond_1", \
 q="well_1", s="syield_1", r="recharge_1", output="gwresult_conf_1",\
 dt=8640000000000, type="confined")

grass.message(_("generate the transport data"))

# The initial concentration is zero
grass.run_command("r.mapcalc", expression="c_1=if(col() == 20 && row() == 20 , 0.0, 0.0)")
# One inner sources (result of chemical reaction)
grass.run_command("r.mapcalc", expression="cs_1=if(col() == 20 && row() == 20 , 0.0013888, 0.0)")
# No pollution by well
grass.run_command("r.mapcalc", expression="cin_1=0.0")
# We have a transfer boundary condition
grass.run_command("r.mapcalc", expression="tstatus_1=if(col() == 1 || col() == 80 , 3, 1)")
# No diffusion coefficient known for the solution
grass.run_command("r.mapcalc", expression="diff_1=0.0")
# Normal retardation
grass.run_command("r.mapcalc", expression="R_1=1.0")

# Longitudinal and transversal dispersivity length
AL=100
AT=10

# Compute the solute transport using the above defined dispersivity coefficients for a timestep of 1000d
grass.run_command("r.solute.transport", "c", "s", error=0.000000000000001, maxit=1000, solver="bicgstab",\
  top="top_conf_1", bottom="bottom_1", phead="gwresult_conf_1", status="tstatus_1", hc_x="hydcond_1",\
  hc_y="hydcond_1", r="R_1", cs="cs_1", q="well_1", nf="poros_1", output="stresult_conf_1", dt=86400000,\
  diff_x="diff_1", diff_y="diff_1", cin="cin_1", c="c_1", al=AL, at=AT, vx="stresult_conf_vel_1_x", vy="stresult_conf_vel_1_y")

# The second computation uses different porosity for higher groundwater velocity

# Used to compute a lower velocity
grass.run_command("r.mapcalc", expression="poros_2=1")

# Compute the solute transport using the above defined dispersivity coefficients for a timestep of 1000d
grass.run_command("r.solute.transport", "c", "s", error=0.000000000000001, maxit=1000, solver="bicgstab",\
  top="top_conf_1", bottom="bottom_1", phead="gwresult_conf_1", status="tstatus_1", hc_x="hydcond_1",\
  hc_y="hydcond_1", r="R_1", cs="cs_1", q="well_1", nf="poros_2", output="stresult_conf_2", dt=86400000,\
  diff_x="diff_1", diff_y="diff_1", cin="cin_1", c="c_1", al=AL, at=AT, vx="stresult_conf_vel_2_x", vy="stresult_conf_vel_2_y")
