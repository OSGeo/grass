#!/bin/sh

# sample script for Spearfish
# written by
#   andreas.philipp geo.uni-augsburg.de
# modified by JH, HM, MN, SG

dem=elevation.10m
output=simwe
g.region raster=${output}
g.region n=4920800 s=4917800 w=602500 e=606000
g.region -p

# rainfall [mm/hr]
rain01=25

# infitration [mm/hr]
infil0=0.

echo "Preparing input maps..."
r.slope.aspect --o elevation=$dem dx=${output}_dx dy=${output}_dy
r.mapcalc --o expr="${output}_rain =if(${dem}, $rain01, null())"
r.mapcalc --o expr="${output}_manin=if(${dem}, soils * 0.005, null())"
r.mapcalc --o expr="${output}_infil=if(${dem}, $infil0, null())"
r.mapcalc --o expr="${output}_null=if(${dem}, float(0.0), null())"

# Create the observation points
v.random --o output=observation_points n=5 -z

echo "r.sim.water --o -t elevation=${dem} dx=${output}_dx dy=${output}_dy rain=${output}_rain man=${output}_manin infil=${output}_infil depth=${output}_depth disch=${output}_disch err=${output}_err outwalk=${output}_walker niter=5 outiter=1 observation=observation_points logfile=simwe.log"
r.sim.water --o -t elevation=${dem} dx=${output}_dx dy=${output}_dy \
            rain=${output}_rain man=${output}_manin infil=${output}_infil \
            depth=${output}_depth disch=${output}_disch err=${output}_err \
            outwalk=${output}_walker niter=5 outiter=1 observation=observation_points \
            logfile=simwe.log

echo "Plotting the observation points with R. Result in Rplots.pdf"
R --vanilla -e "df = read.table(\"simwe.log\", header=1); par(mfrow=c(3,2)); plot(df\$STEP, df\$CAT0001, type=\"l\"); plot(df\$STEP, df\$CAT0002, type=\"l\"); plot(df\$STEP, df\$CAT0003, type=\"l\"); plot(df\$STEP, df\$CAT0004, type=\"l\");plot(df\$STEP, df\$CAT0005, type=\"l\")"

echo "Export of manning, soil and gradients"
r.out.vtk elevation=${dem} input=${output}_manin,soils vectormaps=${output}_dx,${output}_dy,${output}_null output=manning_soils_gradient.vtk null=0.0

echo "Build topology and exporting walker vector points for each time step"
for i in `g.list vect | grep ${output}` ; do
	v.build $i
	echo "v.out.vtk input=$i output=$i.vtk"
	v.out.vtk input=$i output=$i.vtk
done

echo "Exporting the raster maps for each time step"
for i in `g.list rast | grep ${output}` ; do
	echo "r.out.vtk elevation=${dem} input=$i output=$i.vtk null=0.0"
	r.out.vtk elevation=${dem} input=$i output=$i.vtk null=0.0
done

echo "Now open paraview from this command line and load the vtk files as time series using the File->Open menu entry"
echo "Step throu the time steps and adjust the color tables"

# cleanup
g.remove --q -f type=raster name=${output}_dx,${output}_dy,${output}_rain,${output}_manin,${output}_infil,${output}_null
g.remove --q -f type=vector name=observation_points
