#!/bin/sh

#========================
#Spearfish tests
# 7x7 moving window test
# Brute-force testing in a loop

# TODO: change to/add 3x3 example

# created conf file as described in EXAMPLES, moving window, 
# see gui/wxgui/rlisetup/g.gui.rlisetup.html
echo "SAMPLINGFRAME 0|0|1|1
SAMPLEAREA -1|-1|0.015021459227467811|0.011058451816745656
MOVINGWINDOW" > $HOME/.grass7/r.li/movwindow7

export GRASS_OVERWRITE=1
g.region raster=landcover.30m -p
r.mapcalc "forests = if(landcover.30m >= 41 && landcover.30m <= 43,1,null())"

MEASURE="dominance edgedensity mpa mps padcv padrange padsd patchdensity patchnum pielou richness shannon shape simpson"
for mymeasure in $MEASURE ; do
  echo "====== $mymeasure: ========================"
  r.li.${mymeasure} forests conf=movwindow7 output=forests_${mymeasure}_mov7
  r.univar -g forests_${mymeasure}_mov7
done

# here also alpha:
mymeasure=renyi
echo "====== $mymeasure: ========================"
r.li.renyi forests conf=movwindow7 output=forests_renyi_mov7_a06 alpha=0.6
r.univar -g forests_renyi_mov7_a06

echo "====== End of r.li tests ========================"
