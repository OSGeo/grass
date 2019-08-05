#!/usr/bin/env python3
# Shellscript to verify r.solute.transport calculation, this calculation is based on
# the example 2.1 and 2.2 at page 181 of the following book:
#	title = "Str{\"o}mungs und Transportmodellierung",
#	author = "Lege, T. and Kolditz, O. and Zielke W.",
#	publisher = "Springer (Berlin; Heidelberg; New York; Barcelona; Hongkong; London; Mailand; Paris; Singapur; Tokio)",
#	edition = "2. Auflage",
#	year = "1996",
#	series = "Handbuch zur Erkundung des Untergrundes von Deponien und Altlasten"
#

import grass.script as grass

# Overwrite existing maps
grass.run_command("g.gisenv", set="OVERWRITE=1")

grass.message(_("Set the region"))

# The area is 2000m x 1000m with a cell size of 50m x 50m
grass.run_command("g.region", res=50, res3=50, t=25, b=0, n=1000, s=0, w=0, e=2000)

grass.message(_("Create all the input maps needed for groundwater flow computation"))

# Initial condition of the piezometric head, we have a huge gradient from 275m to 50m
# over a distance of 2000m
grass.run_command("r.mapcalc", expression="phead_1=if(col() == 1 , 275, 50)")
# Set the active cells and the dirichlet boundary condition
grass.run_command("r.mapcalc", expression="status_1=if(col() == 1 || col() == 40 , 2, 1)")
# We have a single well with a small influent pumping rate
grass.run_command("r.mapcalc", expression="well_1=if((row() == 10 && col() == 10), 0.000002818287, 0)")
# The hydraulic conductivity
grass.run_command("r.mapcalc", expression="hydcond_1=0.0001")
# The recharge, well we have no recharge at all
grass.run_command("r.mapcalc", expression="recharge_1=0")
# We have a confined aquifer with a height of 25m
grass.run_command("r.mapcalc", expression="top_conf_1=25")
# Bottom of the aquifer starts at 0m
grass.run_command("r.mapcalc", expression="bottom_1=0")
# This is the standard porosity of sand aquifer
grass.run_command("r.mapcalc", expression="poros_1=0.17")
# The specific yield of a confined aquifer
grass.run_command("r.mapcalc", expression="syield_1=0.0001")
grass.run_command("r.mapcalc", expression="null_1=0.0")

grass.message("First compute a steady state groundwater flow")

# Compute the steady state groundwater flow
grass.run_command("r.gwflow", solver="cg", top="top_conf_1", bottom="bottom_1", phead="phead_1",\
 status="status_1", hc_x="hydcond_1", hc_y="hydcond_1", \
 q="well_1", s="syield_1", recharge="recharge_1", output="gwresult_conf_1",\
 dt=8640000000000, type="confined")

grass.message(_("generate the transport data"))

# The initial concentration is zero
grass.run_command("r.mapcalc", expression="c_1=if(col() == 10 && row() == 10 , 0.0, 0.0)")
# No inner sources or sinks (result of chemical reaction)
grass.run_command("r.mapcalc", expression="cs_1=0.0")
# The pollution is inserted by a well
grass.run_command("r.mapcalc", expression="cin_1=if(col() == 10 && row() == 10 ," + str(0.2/86400) + ", 0.0)")
# We have a transfer boundary condition
grass.run_command("r.mapcalc", expression="tstatus_1=if(col() == 1 || col() == 40 , 3, 1)")
# No diffusion coefficient known for the solution
grass.run_command("r.mapcalc", expression="diff_1=0.0")
# Normal retardation
grass.run_command("r.mapcalc", expression="R_1=1.0")

# Longitudinal and transversal dispersivity length
AL=50
AT=5

# Compute the solute transport using the above defined dispersivity coefficients for a timestep of 250d
grass.run_command("r.solute.transport", "c", error=0.000000000000001, maxit=1000, solver="bicgstab",\
  top="top_conf_1", bottom="bottom_1", phead="gwresult_conf_1", status="tstatus_1", hc_x="hydcond_1",\
  hc_y="hydcond_1", rd="R_1", cs="cs_1", q="well_1", nf="poros_1", output="stresult_conf_1", dt=21600000,\
  diff_x="diff_1", diff_y="diff_1", cin="cin_1", c="c_1", al=AL, at=AT)

# Get the maximum concentration
range = grass.parse_command("r.info", "r", map="stresult_conf_1")
# Normalize the result
grass.run_command("r.mapcalc", expression="stresult_conf_1_norm = stresult_conf_1/" + str(range["max"]))
#Create contour lines range from 0.7 to 0.1 in 0.1 steps
grass.run_command("r.contour", input="stresult_conf_1_norm", output="stresult_conf_1_norm", minlevel=0.1, maxlevel=0.7, step=0.1)

# The second computation uses different dispersivity lengths
AL=10
AT=1

# Compute the solute transport using the above defined dispersivity coefficients for a timestep of 250d
grass.run_command("r.solute.transport", "c", error=0.000000000000001, maxit=1000, solver="bicgstab",\
  top="top_conf_1", bottom="bottom_1", phead="gwresult_conf_1", status="tstatus_1", hc_x="hydcond_1",\
  hc_y="hydcond_1", rd="R_1", cs="cs_1", q="well_1", nf="poros_1", output="stresult_conf_2", dt=21600000,\
  diff_x="diff_1", diff_y="diff_1", cin="cin_1", c="c_1", al=AL, at=AT)


# Get the maximum concentration
range = grass.parse_command("r.info", "r", map="stresult_conf_2")
# Normalize the result
grass.run_command("r.mapcalc", expression="stresult_conf_2_norm = stresult_conf_2/" + str(range["max"]))
#Create contour lines range from 0.7 to 0.1 in 0.1 steps
grass.run_command("r.contour", input="stresult_conf_2_norm", output="stresult_conf_2_norm", minlevel=0.1, maxlevel=0.7, step=0.1)
