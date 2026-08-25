## DESCRIPTION

*r.ros* is part of the wildfire simulation toolset. Preparational steps
for the fire simulation are the calculation of the rate of spread (ROS)
with *r.ros*, and the creating of spread map with *r.spread*.
Eventually, the fire path(s) based on starting point(s) are calculated
with *r.spreadpath*.

*r.ros* is used for fire (wildfire) modeling. The input is fuel model
and moisture and the outputs are rate of spread (ROS) values. The module
generates the base ROS value, maximum ROS value, direction of the
maximum ROS, and optionally the maximum potential spotting distance of
wildfire for each raster cell in the current geographic region. These
three or four raster map layers serve as inputs for the
*[r.spread](r.spread.md)* module which is the next step in fire
simulation.

The *r.ros* module and two related modules *[r.spread](r.spread.md)*,
and *[r.spreadpath](r.spreadpath.md)* can be used not only for wildfire
modeling but also generally to simulate other events where spread of
something is involved and elliptical spread is appropriate.

The calculation of the two ROS values for each raster cell is based on
the Fortran code by Pat Andrews (1983) of the Northern Forest Fire
Laboratory, USDA Forest Service. The direction of the maximum ROS
results from the vector addition of the forward ROS in wind direction
and that in upslope direction. The spotting distance, if required, will
be calculated by a separate function, spot_dist(), which is based on
Lathrop and Xu (in preparation), Chase (1984) and Rothermel (1991). More
information on *r.ros* and *[r.spread](r.spread.md)* can be found in Xu
(1994).

The **output** parameter is a basename (prefix) for all generated raster
maps and each map gets a unique suffix:

- `.base` for the base (perpendicular) ROS (cm/minute)
- `.max` for the maximum (forward) ROS (cm/minute),
- `.maxdir` for the direction of the maximum ROS, clockwise from north
  (degree), and optionally
- `.spotdist` for the maximum potential spotting distance (meters).

So, if the output parameter is `blackforest_ros`, *r.ros* creates
`blackforest_ros.base`, `blackforest_ros.max`, `blackforest_ros.maxdir`,
and (optionally) `blackforest_ros.spotdist` raster maps.

If only one or two of the options **moisture_1h**, **moisture_10h**, and
**moisture_100h** are given, the module will assign values to the
missing option using the formula:

```sh
moisture_100h = moisture_10h + 1 = moisture_1h + 2
```

However, at least one of them should be given.

Options **velocity** and **direction** must be both given or both
omitted. If none is given, the module will assume a no-wind condition.

Options **slope** and **aspect** must be also given together. If none is
given, the module will assume a topographically flat condition. Option
**elevation** must be given if **-s** (spotting) flag is used.

## EXAMPLES

Assume we have inputs, the following generates ROSes and spotting
distances:

```sh
r.ros -s model=fire_model moisture_1h=1hour_moisture moisture_live=live_moisture \
    velocity=wind_speed direction=wind_direction \
    slope=slope aspect=aspect elevation=elevation output=ros
```

## NOTES

1. *r.ros* is supposed to be run before running
    *[r.spread](r.spread.md)* module. The combination of these two
    modules forms a simulation of the spread of wildfires.
2. The user should be sure that the inputs to *r.ros* are in proper
    units.
3. The output units for the base and maximum ROSes are in cm/minute
    rather than ft/minute, which is due to that a possible zero
    ft/minute base ROS value and a positive integer ft/minute maximum
    ROS would result in calculation failure in the
    *[r.spread](r.spread.md)* module. As far as the user just use
    *r.ros* together with *[r.spread](r.spread.md)*, there is no need to
    concern about these output units.

## REFERENCES

- **Albini,** F. A., 1976, Computer-based models of wildland fire
  behavior: a user's manual, USDA Forest Service, Intermountain Forest
  and Range Experiment Station, Ogden, Utah.
- **Andrews**, P. L., 1986, BEHAVE: fire behavior prediction and fuel
  modeling system -- BURN subsystem, Part 1, USDA Forest Service,
  Intermountain Research Station, Gen. Tech. Rep. INT-194, Ogden, Utah.
- **Chase**, Carolyn, H., 1984, Spotting distance from wind-driven
  surface fires -- extensions of equations for pocket calculators, US
  Forest Service, Res. Note INT-346, Ogden, Utah.
- **Lathrop**, Richard G. and Jianping Xu, A geographic information
  system-based approach for calculating spotting distance. (in
  preparation)
- **Rothermel**, R. E., 1972, A mathematical model for predicting fire
  spread in wildland fuels, USDA Forest Service, Intermountain Forest
  and Range Experiment Station, Res. Pap. INT-115, Ogden, Utah.
- **Rothermel**, Richard, 1991, Predicting behavior and size of crown
  fires in the northern Rocky Mountains, US Forest Service, Res. Paper
  INT-438, Ogden, Utah.
- **Xu**, Jianping, 1994, Simulating the spread of wildfires using a
  geographic information system and remote sensing, Ph. D. Dissertation,
  Rutgers University, New Brunswick, Jersey
  ([ref](https://dl.acm.org/citation.cfm?id=921466)).

## SEE ALSO

*[g.region](g.region.md), [r.slope.aspect](r.slope.aspect.md),
[r.spread](r.spread.md), [r.spreadpath](r.spreadpath.md)* Sample data
download:
[firedemo.sh](https://grass.osgeo.org/sampledata/firedemo_grass7.sh)
(run this script within the "Fire simulation data set" project).

## AUTHOR

Jianping Xu, Center for Remote Sensing and Spatial Analysis, Rutgers
University.
