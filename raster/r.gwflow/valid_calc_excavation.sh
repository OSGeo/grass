#!/bin/sh
# Shellscript to verify r.gwflow calculation, this calculation is based on
# the example at page 167 of the following book:
#	author = "Kinzelbach, W. and Rausch, R.",
#	title = "Grundwassermodellierung",
#	publisher = "Gebr{\"u}der Borntraeger (Berlin, Stuttgart)",
#	year = "1995"
#
# set the region
g.region res=50 n=950 s=0 w=0 e=2000

r.mapcalc --o expression="phead= if(row() == 19, 5, 3)"
r.mapcalc --o expression="status=if((col() == 1 && row() == 13) ||\
                     (col() == 1 && row() == 14) ||\
		     (col() == 2 && row() == 13) ||\
		     (col() == 2 && row() == 14) ||\
		     (row() == 19), 2, 1)"

r.mapcalc --o expression="well=0.0"
r.mapcalc --o expression="hydcond=0.001"
r.mapcalc --o expression="recharge=0.000000006"
r.mapcalc --o expression="top=20"
r.mapcalc --o expression="bottom=0"
r.mapcalc --o expression="syield=0.001"
r.mapcalc --o expression="null=0.0"

#compute a steady state groundwater flow
r.gwflow --o solver=cholesky top=top bottom=bottom phead=phead \
 status=status hc_x=hydcond hc_y=hydcond q=well s=syield \
 r=recharge output=gwresult dt=864000000000 type=unconfined 

# create contour lines
r.contour input=gwresult output=gwresult_contour step=0.2 --o
#create flow lines
r.flow elevin=gwresult flout=gwresult_flow skip=3 --o

export GRASS_WIDTH=640
export GRASS_HEIGHT=480
#export as png and convert into eps and pdf
export GRASS_TRUECOLOR=TRUE
export GRASS_PNGFILE=Excavation_pit.png
d.rast gwresult
d.vect gwresult_flow color=grey
d.vect gwresult_contour color=black display=attr,shape attrcol=level lsize=16 lcolor=black
d.legend at=8,12,15,85 map=gwresult 
d.barscale at=1,10 &
echo "Groundwater flow steady state" | d.text size=6 color=black
convert Excavation_pit.png Excavation_pit.eps
convert Excavation_pit.png Excavation_pit.pdf

