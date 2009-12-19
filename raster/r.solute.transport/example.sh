#!/bin/sh
# set the region accordingly
g.region res=1 res3=1 t=10 b=0 n=100 s=0 w=0 e=200
r.mapcalc "phead=if(col() == 1 , 50, 40)"
r.mapcalc "phead=if(col() ==200  , 45 + row()/40, phead)"
r.mapcalc "status=if(col() == 1 || col() == 200 , 2, 1)"
r.mapcalc "well=if((row() == 50 && col() == 175) || (row() == 10 && col() == 135) , -0.001, 0)"
r.mapcalc "hydcond=0.00005"
r.mapcalc "reacharge=0"
r.mapcalc "top_conf=20"
r.mapcalc "top_unconf=60"
r.mapcalc "bottom=0"
r.mapcalc "poros=0.17"
r.mapcalc "syield=0.0001"
r.mapcalc "null=0.0"

#First compute a steady state groundwater flow
r.gwflow --o -s solver=cg top=top_conf bottom=bottom phead=phead status=status hc_x=hydcond hc_y=hydcond \
  q=well s=syield r=reacharge output=gwresult_conf dt=8640000000000 type=confined

#create the flow direction vectors
r.flow elevin=gwresult_conf flout=gwresult_conf_flow skip=15 --o 
# create contour lines
r.contour input=gwresult_conf output=gwresult_conf_contour step=0.5 --o


#generate the transport data
r.mapcalc "c=if(col() == 15 && row() == 75 , 500.0, 0.0)"
r.mapcalc "cs=if(col() == 15 && row() == 75 , 0.0, 0.0)"
r.mapcalc "tstatus=if(col() == 1 || col() == 200 , 3, 1)"
r.mapcalc "diff=0.0000001"
r.mapcalc "R=1.0"

r.solute.transport --o -cs solver=bicgstab top=top_conf bottom=bottom phead=gwresult_conf status=tstatus \
  hc_x=hydcond hc_y=hydcond R=R cs=cs q=well nf=poros output=stresult_conf dt=3600 diff_x=diff diff_y=diff \
  c=c  al=0.1 at=0.01

days=0
count=0

d.erase
d.rast stresult_conf
d.vect gwresult_conf_flow render=l
d.vect gwresult_conf_contour color=grey render=l
d.legend at=8,12,15,85 map=stresult_conf 
d.barscale at=1,10 
echo "Solute transport timestep: 0d" | d.text size=6 color=black
d.out.png res=1 output=/tmp/test_0000$count.png

#compute in an infinite loop the transport

while true ; do

r.solute.transport --o -cs solver=bicgstab top=top_conf bottom=bottom phead=gwresult_conf status=tstatus \
  hc_x=hydcond hc_y=hydcond R=R cs=cs q=well nf=poros output=stresult_conf dt=86400 diff_x=diff diff_y=diff \
  c=stresult_conf al=0.1 at=0.01

days=`expr $days + 1`
count=`expr $count + 1`

#show the map   
d.erase
d.rast stresult_conf 
d.vect gwresult_conf_flow render=l
d.vect gwresult_conf_contour color=grey render=l
d.legend at=8,12,15,85 map=stresult_conf 
d.barscale at=1,10 
echo "Solute transport timestep: ${days}d" | d.text size=6 color=black
if [ `expr $count \< 10` -eq 1 ] ; then
  d.out.png res=1 output=/tmp/test_0000$count.png
elif [ `expr $count \< 100` -eq 1 ] ; then
  d.out.png res=1 output=/tmp/test_000$count.png
elif [ `expr $count \< 1000` -eq 1 ] ; then
  d.out.png res=1 output=/tmp/test_00$count.png
elif [ `expr $count \< 10000` -eq 1 ] ; then
  d.out.png res=1 output=/tmp/test_0$count.png
fi

done

