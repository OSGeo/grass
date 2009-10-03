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

r.mapcalc --o expression="phead=50"
r.mapcalc --o expression="status=if(col() == 1 || col() == 7 , 2, 1)"
r.mapcalc --o expression="well=if((row() == 4 && col() == 4), -0.1, 0)"
r.mapcalc --o expression="hydcond=0.0005"
r.mapcalc --o expression="recharge=0"
r.mapcalc --o expression="top_conf=20"
r.mapcalc --o expression="bottom=0"
r.mapcalc --o expression="syield=0.0001"
r.mapcalc --o expression="null=0.0"

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

export GRASS_WIDTH=640
export GRASS_HEIGHT=480

#export as png and convert into eps and pdf
export GRASS_TRUECOLOR=TRUE
export GRASS_PNGFILE=valid_calc_7x7.png
d.rast gwresult_conf
d.rast.num gwresult_conf dp=2
d.barscale at=1,10 &
echo "Groundwater flow 10.000s" | d.text size=6 color=black
convert valid_calc_7x7.png valid_calc_7x7.eps
convert valid_calc_7x7.png valid_calc_7x7.pdf
