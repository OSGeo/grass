#!/bin/sh
# Shellscript to verify r.solute.transport calculation, this calculation is based on
# the example 2.1 and 2.2 at page 181 of the following book:
#	title = "Str{\"o}mungs und Transportmodellierung",
#	author = "Lege, T. and Kolditz, O. and Zielke W.",
#	publisher = "Springer (Berlin; Heidelberg; New York; Barcelona; Hongkong; London; Mailand; Paris; Singapur; Tokio)",
#	edition = "2. Auflage",
#	year = "1996",
#	series = "Handbuch zur Erkundung des Untergrundes von Deponien und Altlasten"
#
# set the region 
g.region res=50 res3=1 t=10 b=0 n=1000 s=0 w=0 e=2000
r.mapcalc "phead_1=if(col() == 1 , 200, 50)"
r.mapcalc "status_1=if(col() == 1 || col() == 40 , 2, 1)"
r.mapcalc "well_1=if((row() == 10 && col() == 10), 0.000002818287, 0)"
r.mapcalc "well_1=0.0"
r.mapcalc "hydcond_1=0.0001"
r.mapcalc "reacharge_1=0"
r.mapcalc "top_conf_1=25"
r.mapcalc "bottom_1=0"
r.mapcalc "poros_1=0.11209524"
r.mapcalc "syield_1=0.0001"
r.mapcalc "null_1=0.0"

#generate the transport data
r.mapcalc "c_1=if(col() == 10 && row() == 10 , 0.0, 0.0)"
r.mapcalc "cs_1=if(col() == 10 && row() == 10 , 0.000000000000000000001, 0.0)"
r.mapcalc "cin_1=if(col() == 10 && row() == 10 , 0.00000231481, 0.0)"
r.mapcalc "tstatus_1=if(col() == 1 || col() == 40 , 3, 1)"
r.mapcalc "diff_1=0.0"
r.mapcalc "R_1=1.0"

AL=50
AT=5

#First compute a steady state groundwater flow
r.gwflow --o -s solver=cg top=top_conf_1 bottom=bottom_1 phead=phead_1\
 status=status_1 hc_x=hydcond_1 hc_y=hydcond_1 \
 q=well_1 s=syield_1 r=reacharge_1 output=gwresult_conf_1\
 dt=8640000000000 type=confined 

# create contour lines
r.contour input=gwresult_conf_1 output=gwresult_conf_contour_1 step=10 --o

###
### compute with low velocity
###

r.solute.transport --o -cs error=0.000000000000001 maxit=1000 solver=bicgstab top=top_conf_1 bottom=bottom_1\
 phead=gwresult_conf_1 status=tstatus_1 hc_x=hydcond_1 hc_y=hydcond_1\
 R=R_1 cs=cs_1 q=well_1 nf=poros_1 output=stresult_conf_1 dt=21600000\
 diff_x=diff_1 diff_y=diff_1 cin=cin_1 c=c_1 al=$AL at=$AT velocity=stresult_conf_vel_1

eval `r.univar stresult_conf_1 -g`

#export as postscript and convert into pdf
export GRASS_TRUECOLOR=TRUE
export GRASS_PSFILE=seguin_verify_c.ps
d.mon start=PS
d.mon select=PS
d.rast cont_map
#d.rast.num cont_map dp=2 
d.vect stresult_conf_contour_1 color=grey display=shape,attr attrcol=level
d.legend at=8,12,15,85 map=cont_map labelnum=5 color=black thin=500
d.barscale at=1,10 
echo "Solute transport 250d al=$AL at=$AT" | d.text size=6 color=black
d.mon stop=PS
ps2epsi seguin_verify_c.ps
epstopdf seguin_verify_c.epsi

###
### compute different dispersion
###

AL=10
AT=1

r.solute.transport --o -cs error=0.000000000000001 maxit=1000 solver=bicgstab top=top_conf_1 bottom=bottom_1\
 phead=gwresult_conf_1 status=tstatus_1 hc_x=hydcond_1 hc_y=hydcond_1\
 R=R_1 cs=cs_1 q=well_1 nf=poros_1 output=stresult_conf_2 dt=21600000\
 diff_x=diff_1 diff_y=diff_1 cin=cin_1 c=c_1 al=$AL at=$AT velocity=stresult_conf_vel_2

eval `r.univar stresult_conf_2 -g`

r.mapcalc "cont_map_a = stresult_conf_2/$max"
r.mapcalc "stresult_conf_vel_2_x = stresult_conf_vel_2_x *24.0 * 3600.0"
r.info stresult_conf_vel_2_x
r.contour input=cont_map_a output=stresult_conf_contour_2 step=0.1 --o


#export as postscript and convert into pdf
export GRASS_TRUECOLOR=TRUE
export GRASS_PSFILE=seguin_verify_d.ps
d.mon start=PS
d.mon select=PS
d.rast cont_map_a
#d.rast.num cont_map dp=2 
d.vect stresult_conf_contour_2 color=grey display=shape,attr attrcol=level
d.legend at=8,12,15,85 map=cont_map_a labelnum=5 color=black thin=500
d.barscale at=1,10 
echo "Solute transport 250d al=$AL at=$AT" | d.text size=6 color=black
d.mon stop=PS
ps2epsi seguin_verify_d.ps
epstopdf seguin_verify_d.epsi

