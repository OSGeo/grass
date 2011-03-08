#!/bin/sh

# sample script for Spearfish
# written by
#   andreas.philipp geo.uni-augsburg.de
# modified by JH, HM, MN, SG


dem=elevation.10m
output=simwe
g.region rast=${output}
g.region n=4920800 s=4917800 w=602500 e=606000
g.region -p

# Manning n
man05=0.05

# rainfall [mm/hr]
rain01=2.5

# infitration [mm/hr]
infil0=0.

echo "Preparing input maps..."
r.slope.aspect --o elevation=$dem dx=${output}_dx dy=${output}_dy
r.mapcalc --o expr="${output}_rain =if(${dem},$rain01,null())"
r.mapcalc --o expr="${output}_manin=if(${dem},$man05,null())"
r.mapcalc --o expr="${output}_infil=if(${dem},$infil0,null())"

echo "r.sim.water --o -t elevation=${dem} dx=${output}_dx dy=${output}_dy rain=${output}_rain man=${output}_manin infil=${output}_infil depth=${output}_depth disch=${output}_disch err=${output}_err outwalk=${output}_walker niter=5 outiter=1"
      r.sim.water --o -t elevation=${dem} dx=${output}_dx dy=${output}_dy rain=${output}_rain man=${output}_manin infil=${output}_infil depth=${output}_depth disch=${output}_disch err=${output}_err outwalk=${output}_walker niter=5 outiter=1

echo "Build topology and exporting walker vector points for each time step"
for i in `g.mlist vect | grep ${output}` ; do
	v.build $i
	echo "v.out.vtk input=$i output=$i.vtk"
	v.out.vtk input=$i output=$i.vtk
done

echo "Exporting the raster maps for each time step"
for i in `g.mlist rast | grep ${output}` ; do
	echo "r.out.vtk elevation=${dem} input=$i output=$i.vtk null=0.0"
	r.out.vtk elevation=${dem} input=$i output=$i.vtk null=0.0
done

echo "Now open paraview from this command line and load the vtk files as time series using the File->Open menu entry"
echo "Step throu the time steps and adjust the color tables"

# cleanup
g.remove --q rast=${output}_dx,${output}_dy,${output}_rain,${output}_manin,${output}_infil
