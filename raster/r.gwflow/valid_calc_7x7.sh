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

#First compute the initial groundwater flow
r.gwflow --o solver=cg top=top_conf bottom=bottom phead=phead\
 status=status hc_x=hydcond hc_y=hydcond q=well s=syield\
 r=recharge output=gwresult_conf dt=500 type=confined budged=water_budged

count=500
# loop over the timesteps
while [ `expr $count \< 10000` -eq 1 ] ; do
  r.gwflow --o solver=cg top=top_conf bottom=bottom phead=gwresult_conf\
     status=status hc_x=hydcond hc_y=hydcond q=well s=syield\
     r=recharge output=gwresult_conf dt=500 type=confined budged=water_budged
  count=`expr $count + 500`
done
