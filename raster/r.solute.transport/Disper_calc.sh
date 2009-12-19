#!/bin/sh
# Shellscript to verify r.solute.transport calculation
# The difference between solute transport calculated with and without the CFL citeria is shown
# set the region 
g.region res=1 res3=1 t=10 b=0 n=100 s=0 w=0 e=400
r.mapcalc "phead_1=if(col() == 1 , 55, 50)"
r.mapcalc "status_1=if(col() == 1 || col() == 400 , 2, 1)"
r.mapcalc "well_1=if((row() == 50 && col() == 350), 0.0, 0)"
#r.mapcalc "well_1=0.0"
r.mapcalc "hydcond_1=0.0001"
r.mapcalc "reacharge_1=0"
r.mapcalc "top_conf_1=25"
r.mapcalc "bottom_1=0"
r.mapcalc "poros_1=0.2"
r.mapcalc "poros_2=0.2"
r.mapcalc "syield_1=0.0001"
r.mapcalc "null_1=0.0"

#generate the transport data
r.mapcalc "c_1=if(col() == 50 && row() == 50 , 0, 0.0)"
r.mapcalc "cs_1=if(col() == 50 && row() == 50 , 0.01, 0.0)"
r.mapcalc "cin_1=if(col() == 50 && row() == 50 , 0.0, 0.0)"
r.mapcalc "tstatus_1=if(col() == 1, 2, 1)"
r.mapcalc "tstatus_1=if(col() == 400 , 3, tstatus_1)"
r.mapcalc "diff_1=0.0"
r.mapcalc "R_1=1.0"

AL=1 
AT=0.1
export GRASS_TRUECOLOR=TRUE
export GRASS_WIDTH=1280
export GRASS_HEIGHT=330
#First compute a steady state groundwater flow
r.gwflow --o -s solver=cg top=top_conf_1 bottom=bottom_1 phead=phead_1\
 status=status_1 hc_x=hydcond_1 hc_y=hydcond_1 \
 q=well_1 s=syield_1 r=reacharge_1 output=gwresult_conf_1\
 dt=8640000000000 type=confined 

# create contour lines
r.contour input=gwresult_conf_1 output=gwresult_conf_contour_1 step=1 --o

###
### compute with low velocity
###

r.solute.transport --o -cs maxit=10000 solver=bicgstab top=top_conf_1 bottom=bottom_1\
 phead=gwresult_conf_1 status=tstatus_1 hc_x=hydcond_1 hc_y=hydcond_1\
 R=R_1 cs=cs_1 q=well_1 nf=poros_1 output=stresult_conf_1 dt=86400000 error=0.000000000001 \
 diff_x=diff_1 diff_y=diff_1 cin=cin_1 c=c_1 al=$AL at=$AT velocity=stresult_conf_vel_1

eval `r.univar stresult_conf_1 -g`

r.mapcalc "cont_map = stresult_conf_1/$max"
r.mapcalc "stresult_conf_vel_1_x = stresult_conf_vel_1_x *24.0 * 3600.0"
r.info stresult_conf_vel_1_x
r.contour input=cont_map output=stresult_conf_contour_1 step=0.05 --o

#export as postscript and convert into pdf
export GRASS_PNGFILE=Disper_calc_${AL}_${AT}_a.png
d.mon start=PNG
d.mon select=PNG
d.rast cont_map
d.vect stresult_conf_contour_1 color=grey display=shape,attr attrcol=level 
d.vect gwresult_conf_contour_1 color=blue display=shape,attr attrcol=level lcolor=blue
d.barscale at=1,85 
echo "Solute transport 1000d al=$AL at=$AT" | d.text size=6 color=black
d.mon stop=PNG
convert Disper_calc_${AL}_${AT}_a.png Disper_calc_${AL}_${AT}_a.eps
convert Disper_calc_${AL}_${AT}_a.png Disper_calc_${AL}_${AT}_a.pdf

###
### compute without CFL criteria 
###

r.solute.transport --o -s loops=1 maxit=10000 solver=bicgstab top=top_conf_1 bottom=bottom_1\
 phead=gwresult_conf_1 status=tstatus_1 hc_x=hydcond_1 hc_y=hydcond_1\
 R=R_1 cs=cs_1 q=well_1 nf=poros_2 output=stresult_conf_2 dt=86400000 error=0.000000000001 \
 diff_x=diff_1 diff_y=diff_1 cin=cin_1 c=c_1 al=$AL at=$AT velocity=stresult_conf_vel_2

eval `r.univar stresult_conf_2 -g`

r.mapcalc "cont_map_a = stresult_conf_2/$max"
r.mapcalc "stresult_conf_vel_2_x = stresult_conf_vel_2_x *24.0 * 3600.0"
r.info stresult_conf_vel_2_x
r.contour input=cont_map_a output=stresult_conf_contour_2 step=0.05 --o

#export as postscript and convert into pdf
export GRASS_PNGFILE=Disper_calc_${AL}_${AT}_b.png
d.mon start=PNG
d.mon select=PNG
d.rast cont_map_a
d.vect stresult_conf_contour_2 color=grey display=shape,attr attrcol=level 
d.vect gwresult_conf_contour_1 color=blue display=shape,attr attrcol=level lcolor=blue
d.barscale at=1,85 
echo "Solute transport 1000d al=$AL at=$AT" | d.text size=6 color=black
d.mon stop=PNG
convert Disper_calc_${AL}_${AT}_b.png Disper_calc_${AL}_${AT}_b.eps
convert Disper_calc_${AL}_${AT}_b.png Disper_calc_${AL}_${AT}_b.pdf

# compute the differences between the maps
r.mapcalc "diff=stresult_conf_1 - stresult_conf_2 "

#export as postscript and convert into pdf
export GRASS_PNGFILE=Disper_calc_${AL}_${AT}_ab_diff.png
d.mon start=PNG
d.mon select=PNG
d.rast diff
d.vect stresult_conf_contour_1 color=red   display=shape,attr attrcol=level lcolor=red 
d.vect stresult_conf_contour_2 color=black display=shape,attr attrcol=level lcolor=black
d.vect gwresult_conf_contour_1 color=blue display=shape,attr attrcol=level lcolor=blue
d.legend at=12,17,10,55 map=diff labelnum=5 color=black 
d.barscale at=1,10 
echo "Difference between maps" | d.text size=6 color=black
d.mon stop=PNG
convert Disper_calc_${AL}_${AT}_ab_diff.png Disper_calc_${AL}_${AT}_ab_diff.eps
convert Disper_calc_${AL}_${AT}_ab_diff.png Disper_calc_${AL}_${AT}_ab_diff.pdf

