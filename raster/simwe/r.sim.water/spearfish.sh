#!/bin/sh

# sample script for Spearfish
# written by
#   andreas.philipp geo.uni-augsburg.de
# modified by JH, HM, MN

dem=elevation.dem
g.region rast=${dem} -p

# Manning n
man05=0.05

# rainfall [mm/hr]
rain01=5

# infitration [mm/hr]
infil0=0.

echo "Preparing input maps..."
r.slope.aspect --o elevation=$dem dx=${dem}_dx dy=${dem}_dy
r.mapcalc "${dem}_rain =if(${dem},$rain01,null())"
r.mapcalc "${dem}_manin=if(${dem},$man05,null())"
r.mapcalc "${dem}_infil=if(${dem},$infil0,null())"

echo "r.sim.water --o elevin=${dem} dxin=${dem}_dx dyin=${dem}_dy \
      rain=${dem}_rain manin=${dem}_manin infil=${dem}_infil \
      depth=${dem}_depth disch=${dem}_disch err=${dem}_err"
  
r.sim.water --o elevin=${dem} dxin=${dem}_dx dyin=${dem}_dy \
  rain=${dem}_rain manin=${dem}_manin infil=${dem}_infil \
  depth=${dem}_depth disch=${dem}_disch err=${dem}_err

r.info -r ${dem}_depth
r.info -r ${dem}_disch
r.info -r ${dem}_err

echo "Written:
 Output water depth raster file:     ${dem}_depth
 Output water discharge raster file: ${dem}_disch
 Output simulation error raster file: ${dem}_err
"

# cleanup
g.remove --q rast=${dem}_dx,${dem}_dy,${dem}_rain,${dem}_manin,${dem}_infil
