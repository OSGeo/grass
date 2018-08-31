#!/bin/sh

#========================
# North Carolina tests

G_RLI=$HOME/.grass7/r.li
mkdir -p $G_RLI

echo "SAMPLINGFRAME 0|0|1|1
SAMPLEAREA 0.0|0.0|1.0|1.0" > $G_RLI/landcover_whole_whole # GRASS7

# North Carolina location:
export GRASS_OVERWRITE=1
g.region raster=landclass96 -p
RASTER_MAP=landclass96
#r.to.vect in=basin_50K output=basin_50K feature=area

echo "-------------------------"
r.li.dominance $RASTER_MAP conf=landcover_whole_whole output=forests_dominance_whole
cat $G_RLI/output/forests_dominance_whole

echo "-------------------------"
r.li.edgedensity $RASTER_MAP conf=landcover_whole_whole output=forests_edgedens_whole
cat $G_RLI/output/forests_edgedens_whole

echo "-------------------------"
r.li.mpa $RASTER_MAP conf=landcover_whole_whole output=forests_mpa_whole
cat $G_RLI/output/forests_mpa_whole

echo "-------------------------"
r.li.mps $RASTER_MAP conf=landcover_whole_whole output=forests_mps_whole
cat $G_RLI/output/forests_mps_whole

echo "-------------------------"
r.li.padcv $RASTER_MAP conf=landcover_whole_whole output=forests_padcv_whole
cat $G_RLI/output/forests_padcv_whole

echo "-------------------------"
r.li.padrange $RASTER_MAP conf=landcover_whole_whole output=forests_padrange_whole
cat $G_RLI/output/forests_padrange_whole

echo "-------------------------"
r.li.padsd $RASTER_MAP conf=landcover_whole_whole output=forests_padsd_whole
cat $G_RLI/output/forests_padsd_whole

echo "-------------------------"
r.li.patchdensity $RASTER_MAP conf=landcover_whole_whole output=forests_p_dens_whole
cat $G_RLI/output/forests_p_dens_whole

echo "-------------------------"
r.li.patchnum $RASTER_MAP conf=landcover_whole_whole output=forests_patchnum_whole
cat $G_RLI/output/forests_patchnum_whole

echo "-------------------------"
r.li.pielou $RASTER_MAP conf=landcover_whole_whole output=forests_pielou_whole
cat $G_RLI/output/forests_pielou_whole

echo "-------------------------"
r.li.renyi $RASTER_MAP conf=landcover_whole_whole output=forests_renyi_whole7_a06 alpha=0.6
cat $G_RLI/output/forests_renyi_whole7_a06

echo "-------------------------"
r.li.richness $RASTER_MAP conf=landcover_whole_whole output=forests_richness_whole
cat $G_RLI/output/forests_richness_whole

echo "-------------------------"
r.li.shannon $RASTER_MAP conf=landcover_whole_whole output=forests_shannon_whole
cat $G_RLI/output/forests_shannon_whole

echo "====== End of r.li tests ========================"
