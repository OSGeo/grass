#!/bin/sh

# assumes full NC SPM

# execute the following in old version in a separate mapset
# the switch to another mapset and execute it with a new version

g.region rural_1m res=2 -p
v.surf.rst -d input=elev_lid792_bepts elevation=elev_lid792_2m slope=dx_2m aspect=dy_2m tension=15 smooth=1.5 npmin=150
r.sim.water -t elevation=elev_lid792_2m dx=dx_2m dy=dy_2m rain_value=50 infil_value=0 man_value=0.05 depth=wdp_2m discharge=disch_2m nwalkers=100000 niterations=120 output_step=20
r.mapcalc "tranin = 0.001"
r.mapcalc "detin = 0.001"
r.mapcalc "tauin = 0.01"
g.copy rast=wdp_2m.120,wdp_2m
r.sim.sediment elevation=elev_lid792_2m dx=dx_2m dy=dy_2m water_depth=wdp_2m detachment_coeff=detin transport_coeff=tranin shear_stress=tauin man_value=0.05 nwalkers=1000000 niterations=120 transport_capacity=tcapacity tlimit_erosion_deposition=erdepmax sediment_flux=sedflow erosion_deposition=erdepsimwe

g.copy rast=disch_2m.120,disch_2m

# execute the following only in one of the mapsets

REF_MAPSET=rsim_before

r.mapcalc "elev_lid792_2m_diff = elev_lid792_2m@$REF_MAPSET - elev_lid792_2m"
r.mapcalc "dx_2m_diff = dx_2m@$REF_MAPSET - "
r.mapcalc "dy_2m_diff = dy_2m@$REF_MAPSET - dy_2m"

r.mapcalc "wdp_2m_diff = wdp_2m@$REF_MAPSET - wdp_2m"
r.mapcalc "disch_2m_diff = disch_2m@$REF_MAPSET - disch_2m"

r.mapcalc "tcapacity_diff = tcapacity@$REF_MAPSET - tcapacity"
r.mapcalc "erdepmax_diff = erdepmax@$REF_MAPSET - erdepmax"
r.mapcalc "sedflow_diff = sedflow@$REF_MAPSET - sedflow"
r.mapcalc "erdepsimwe_diff = erdepsimwe@$REF_MAPSET - erdepsimwe"

# expecting zeros

r.univar elev_lid792_2m_diff
r.univar dx_2m_diff
r.univar dy_2m_diff

r.univar wdp_2m_diff
r.univar disch_2m_diff

r.univar tcapacity_diff
r.univar erdepmax_diff
r.univar sedflow_diff
r.univar erdepsimwe_diff
