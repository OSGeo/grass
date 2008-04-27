#!/bin/sh
# Shellscript to verify r.gwflow calculation, this calculation is based on
# the example at page 133 of the following book:
#	author = "Kinzelbach, W. and Rausch, R.",
#	title = "Grundwassermodellierung",
#	publisher = "Gebr{\"u}der Borntraeger (Berlin, Stuttgart)",
#	year = "1995"
#
# set the region
g.region res=100 n=700 s=0 w=0 e=700

r.mapcalc "phead=50"
r.mapcalc "status=if(col() == 1 || col() == 7 , 2, 1)"
r.mapcalc "well=if((row() == 4 && col() == 4), -0.1, 0)"
r.mapcalc "hydcond=0.0005"
r.mapcalc "recharge=0"
r.mapcalc "top_conf=20"
r.mapcalc "bottom=0"
r.mapcalc "syield=0.0001"
r.mapcalc "null=0.0"

#First compute the steady state groundwater flow
r.gwflow --o solver=cholesky top=top_conf bottom=bottom phead=phead\
 status=status hc_x=hydcond hc_y=hydcond q=well s=syield\
 r=recharge output=gwresult_conf dt=500 type=confined 

count=500

while [ `expr $count \< 10000` -eq 1 ] ; do
  r.gwflow --o solver=cholesky top=top_conf bottom=bottom phead=gwresult_conf\
     status=status hc_x=hydcond hc_y=hydcond q=well s=syield\
     r=recharge output=gwresult_conf dt=500 type=confined
  count=`expr $count + 500`
done

#create the visualization
d.mon start=x0
d.mon select=x0
d.erase
d.rast gwresult_conf
d.rast.num gwresult_conf dp=2
d.barscale at=1,10 &
echo "Groundwater flow 10.000s" | d.text size=6 color=black

export GRASS_WIDTH=640
export GRASS_HEIGHT=480

#export as png and convert into eps and pdf
export GRASS_TRUECOLOR=TRUE
export GRASS_PNGFILE=valid_calc_7x7.png
d.mon start=PNG
d.mon select=PNG
d.rast gwresult_conf
d.rast.num gwresult_conf dp=2
d.barscale at=1,10 &
echo "Groundwater flow 10.000s" | d.text size=6 color=black
d.mon stop=PNG
convert valid_calc_7x7.png valid_calc_7x7.eps
convert valid_calc_7x7.png valid_calc_7x7.pdf
