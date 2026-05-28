## DESCRIPTION

*r.topmodel* simulates TOPMODEL which is a physically based hydrologic
model.

### Parameters description

**parameters**  
This file contains TOPMODEL parameters that describe the study area. Any
lines starting with a \# sign or empty lines are ignored.

```sh
# Subcatchment name
Subcatchment 1

################################################################################
# A [m^2]: Total subcatchment area
3.31697E+07

################################################################################
# qs0 [m/h]: Initial subsurface flow per unit area
#       "The first streamflow input is assumed to represent
#        only the subsurface flow contribution in the watershed."
#                               - Liaw (1988)
0.000075

# lnTe [ln(m^2/h)]: Areal average of the soil surface transmissivity
4.

# m [m]: Parameter controlling the decline rate of transmissivity
# See Beven and Kirkby (1979)
0.0125

# Sr0 [m]: Initial root zone storage deficit
0.0025

# Srmax [m]: Maximum root zone storage deficit
0.041

# td [h]: Unsaturated zone time delay per unit storage deficit if greater than 0
#  OR
# -alpha: Effective vertical hydraulic gradient if not greater than 0.
#
# For example, -10 means alpha=10.
60.

# vch [m/h]: Main channel routing velocity
20000.

# vr [m/h]: Internal subcatchment routing velocity
10000.

################################################################################
# infex: Calculate infiltration excess if not zero (integer)
0

# K0 [m/h]: Surface hydraulic conductivity
2.

# psi [m]: Wetting front suction
0.1

# dtheta: Water content change across the wetting front
0.1

################################################################################
# d [m]: Distance from the catchment outlet
#       The first value should be the mainstream distance from
#       the subcatchment outlet to the catchment outlet.
# Ad_r:  Cumulative area ratio of subcatchment (0.0 to 1.0)
#       The first and last values should be 0 and 1, respectively.

#   d  Ad_r
    0   0.0
 1000   0.2
 2000   0.4
 3000   0.6
 4000   0.8
 5000   1.0
```

**input**  
This file contains observed weather data.

```sh
# dt [h]: Time step
24

################################################################################
# R [m/dt]:  Rainfall
# Ep [m/dt]: Potential evapotranspiration

# R             Ep
0.000033        0.000000
0.000053        0.011938
0.004821        0.000000
.
.
.
```

**timestep**  
If a time step is specified, output will be generated for the specific
time step in addition to the summary and total flows at the outlet. This
parameter can be combined with **topidxclass** to specify a time step
and topographic index class at the same time. If no **topidxclass** is
given, output will be generated for all the topographic index classes.

**toptopidxclass**  
If a topographic index class is specified, output will be generated for
the given topographic index class. This parameter can be combined with
**timestep**. If no **timestep** is given, output will be generated for
all the time steps.

**topidx**, **ntoptopidxclasses**, **outtoptopidxstats**  
The **topidx** map can optionally be used for creating a new topographic
index statistics file. This map has to be already clipped to the
catchment boundary. The entire range of topographic index values will be
divided into **ntoptopidxclasses** and the area ratio of each class will
be reported in the **outtoptopidxstats** file. These three parameters
can be omitted unless a new **topidxstats** file needs to be created.

## REFERENCES

- Beven, K. J., 1984. Infiltration into a class of vertically
  non-uniform soils. Hydrological Sciences Journal 29 (4), 425-434.
- Beven, K. J., Kirkby, M. J., 1979. A physically based, variable
  contributing area model of basin hydrology. Hydrological Sciences
  Bulletin 24 (1), 43-69.
- Beven K. J., R. Lamb, P. Quinn, R. Romanowicz, and J. Freer, 1995.
  TOPMODEL, in V.P. Singh (Ed.). Computer Models of Watershed Hydrology.
  Water Resources Publications.
- Cho, H., 2000. GIS Hydrological Modeling System by Using Programming
  Interface of GRASS. Master's Thesis, Department of Civil Engineering,
  Kyungpook National University, South Korea.
- Liaw, S. C., 1988. Streamflow Simulation Using a Physically Based
  Hydrologic Model in Humid Forested Watersheds. Dissertation, Colorado
  State University, CO. p163.
- Morel-Seytoux, H. J., Khanji, J., 1974. Derivation of an equation of
  infiltration. Water Resources Research 10 (4), 795-800.

## SEE ALSO

*[r.fill.dir](r.fill.dir.md), [r.mapcalc](r.mapcalc.md),
[r.topidx](r.topidx.md)*  
[How to run r.topmodel](http://idea.isnew.info/r.topmodel.html)

## AUTHORS

[Huidae Cho](mailto:grass4u@gmail-com), Hydro Laboratory, Kyungpook
National University, South Korea

Based on TMOD9502.FOR by Keith Beven.
