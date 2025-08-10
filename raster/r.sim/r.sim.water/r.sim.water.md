## DESCRIPTION

*r.sim.water* is a landscape scale simulation model of overland flow
designed for spatially variable terrain, soil, cover and rainfall excess
conditions. A 2D shallow water flow is described by the bivariate form
of Saint Venant equations. The numerical solution is based on the
concept of duality between the field and particle representation of the
modeled quantity. Green's function Monte Carlo method, used to solve the
equation, provides robustness necessary for spatially variable
conditions and high resolutions (Mitas and Mitasova 1998). The key
inputs of the model include elevation (**elevation** raster map), flow
gradient vector given by first-order partial derivatives of elevation
field (**dx** and **dy** raster maps), rainfall excess rate (**rain**
raster map or **rain_value** single value) and a surface roughness
coefficient given by Manning's n (**man** raster map or **man_value**
single value). Partial derivatives raster maps can be computed along
with interpolation of a DEM using the -d option in
*[v.surf.rst](v.surf.rst.md)* module. If elevation raster map is already
provided, partial derivatives can be computed using
*[r.slope.aspect](r.slope.aspect.md)* module. Partial derivatives are
used to determine the direction and magnitude of water flow velocity. To
include a predefined direction of flow, map algebra can be used to
replace terrain-derived partial derivatives with pre-defined partial
derivatives in selected grid cells such as man-made channels, ditches or
culverts. Equations (2) and (3) from [this
report](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/reports/cerl99/rep99.html)
can be used to compute partial derivates of the predefined flow using
its direction given by aspect and slope.

The equations are

```sh
dx = tan(slope) * cos(aspect)
```

and

```sh
dy = tan(slope) * sin(aspect)
```

![r.sim.water generated depth map](r_sim_water.png)  
*Figure: Simulated water flow in a rural area showing the areas with
highest water depth highlighting streams, pooling, and wet areas during
a rainfall event.*

The module automatically converts horizontal distances from feet to
metric system using database/projection information. Rainfall excess is
defined as rainfall intensity - infiltration rate and should be provided
in \[mm/hr\]. Rainfall intensities are usually available from
meteorological stations. Infiltration rate depends on soil properties
and land cover. It varies in space and time. For saturated soil and
steady-state water flow it can be estimated using saturated hydraulic
conductivity rates based on field measurements or using reference values
which can be found in literature. Optionally, user can provide an
overland flow infiltration rate map **infil** or a single value
**infil_value** in \[mm/hr\] that control the rate of infiltration for
the already flowing water, effectively reducing the flow depth and
discharge. Overland flow can be further controlled by permeable check
dams or similar type of structures, the user can provide a map of these
structures and their permeability ratio in the map **flow_control** that
defines the probability of particles to pass through the structure (the
values will be 0-1).

Output includes a water depth raster map **depth** in \[m\], and a water
discharge raster map **discharge** in \[m3/s\]. Error of the numerical
solution can be analyzed using the **error** raster map (the resulting
water depth is an average, and err is its RMSE). The output vector
points map **output_walkers** can be used to analyze and visualize
spatial distribution of walkers at different simulation times (note that
the resulting water depth is based on the density of these walkers). The
spatial distribution of numerical error associated with path sampling
solution can be analysed using the output error raster file \[m\]. This
error is a function of the number of particles used in the simulation
and can be reduced by increasing the number of walkers given by
parameter **nwalkers**. Duration of simulation is controlled by the
**niterations** parameter. The default value is 10 minutes, reaching the
steady-state may require much longer time, depending on the time step,
complexity of terrain, land cover and size of the area. Output walker,
water depth and discharge maps can be saved during simulation using the
time series flag **-t** and **output_step** parameter defining the time
step in minutes for writing output files. Files are saved with a suffix
representing time since the start of simulation in minutes (e.g.
wdepth.05, wdepth.10). Monitoring of water depth at specific points is
supported. A vector map with observation points and a path to a logfile
must be provided. For each point in the vector map which is located in
the computational region the water depth is logged each time step in the
logfile. The logfile is organized as a table. A single header identifies
the category number of the logged vector points. In case of invalid
water depth data the value -1 is used.

Overland flow is routed based on partial derivatives of elevation field
or other landscape features influencing water flow. Simulation equations
include a diffusion term (**diffusion_coeff** parameter) which enables
water flow to overcome elevation depressions or obstacles when water
depth exceeds a threshold water depth value (**hmax)**, given in \[m\].
When it is reached, diffusion term increases as given by **halpha** and
advection term (direction of flow) is given as "prevailing" direction of
flow computed as average of flow directions from the previous **hbeta**
number of grid cells. The model tries to keep water "shallow" with
maximum shallow water depth defined by **hmax** default 0.3 meters.
However, water depths much higher than **hmax** can be observed if water
accumulates in natural sinks or river beds. Depending on the area of
interest and the used digital elevation model, **hmax**, **halpha** and
**hbeta** might need to be adjusted in order to deal realistically with
elevation depressions or obstacles.

## NOTES

A 2D shallow water flow is described by the bivariate form of Saint
Venant equations (e.g., Julien et al., 1995). The continuity of water
flow relation is coupled with the momentum conservation equation and for
a shallow water overland flow, the hydraulic radius is approximated by
the normal flow depth. The system of equations is closed using the
Manning's relation. Model assumes that the flow is close to the
kinematic wave approximation, but we include a diffusion-like term to
incorporate the impact of diffusive wave effects. Such an incorporation
of diffusion in the water flow simulation is not new and a similar term
has been obtained in derivations of diffusion-advection equations for
overland flow, e.g., by Lettenmeier and Wood, (1992). In our
reformulation, we simplify the diffusion coefficient to a constant and
we use a modified diffusion term. The diffusion constant which we have
used is rather small (approximately one order of magnitude smaller than
the reciprocal Manning's coefficient) and therefore the resulting flow
is close to the kinematic regime. However, the diffusion term improves
the kinematic solution, by overcoming small shallow pits common in
digital elevation models (DEM) and by smoothing out the flow over slope
discontinuities or abrupt changes in Manning's coefficient (e.g., due to
a road, or other anthropogenic changes in elevations or cover).

**Green's function stochastic method of solution.**  
The Saint Venant equations are solved by a stochastic method called
Monte Carlo (very similar to Monte Carlo methods in computational fluid
dynamics or to quantum Monte Carlo approaches for solving the
Schrodinger equation (Schmidt and Ceperley, 1992, Hammond et al., 1994;
Mitas, 1996)). It is assumed that these equations are a representation
of stochastic processes with diffusion and drift components
(Fokker-Planck equations).

The Monte Carlo technique has several unique advantages which are
becoming even more important due to new developments in computer
technology. Perhaps one of the most significant Monte Carlo properties
is robustness which enables us to solve the equations for complex cases,
such as discontinuities in the coefficients of differential operators
(in our case, abrupt slope or cover changes, etc). Also, rough solutions
can be estimated rather quickly, which allows us to carry out
preliminary quantitative studies or to rapidly extract qualitative
trends by parameter scans. In addition, the stochastic methods are
tailored to the new generation of computers as they provide scalability
from a single workstation to large parallel machines due to the
independence of sampling points. Therefore, the methods are useful both
for everyday exploratory work using a desktop computer and for large,
cutting-edge applications using high performance computing.

**Suggested Manning's n for surface roughness**  
from <https://baharmon.github.io/hydrology-in-grass>

| NLCD Landcover Category       | Manning’s n value |
|-------------------------------|-------------------|
| Open Water                    | 0.001             |
| Developed, Open Space         | 0.0404            |
| Developed, Low Intensity      | 0.0678            |
| Developed, Medium Intensity   | 0.0678            |
| Developed, High Intensity     | 0.0404            |
| Barren Land                   | 0.0113            |
| Deciduous Forest              | 0.36              |
| Evergreen Forest              | 0.32              |
| Mixed Forest                  | 0.4               |
| Shrub/Scrub                   | 0.4               |
| Grassland/Herbaceuous         | 0.368             |
| Pasture/Hay                   | 0.325             |
| Cultivated Crops              | 0.325             |
| Woody Wetlands                | 0.086             |
| Emergent Herbaceuous Wetlands | 0.1825            |

The [NLCD user
guide](https://www.usgs.gov/centers/eros/science/annual-nlcd-science-product-user-guide)
provides more information about the different NLCD classes.

Increasing the number of threads with **nprocs** does not really speed
up the simulation.

## EXAMPLE

Using the North Carolina full sample dataset:

```sh
# set computational region
g.region raster=elev_lid792_1m -p

# compute dx, dy
r.slope.aspect elevation=elev_lid792_1m dx=elev_lid792_dx dy=elev_lid792_dy

# simulate (this may take a minute or two)
r.sim.water elevation=elev_lid792_1m dx=elev_lid792_dx dy=elev_lid792_dy depth=water_depth disch=water_discharge nwalk=10000 rain_value=100 niter=5
```

Now, let's visualize the result using rendering to a file (note the
further management of computational region and usage of
[d.mon](d.mon.md) module which are not needed when working in GUI):

```sh
# increase the computational region by 350 meters
g.region e=e+350
# initiate the rendering
d.mon start=cairo output=r_sim_water_water_depth.png
# render raster, legend, etc.
d.rast map=water_depth_1m
d.legend raster=water_depth_1m title="Water depth [m]" label_step=0.10 font=sans at=20,80,70,75
d.barscale at=67,10 length=250 segment=5 font=sans
d.northarrow at=90,25
# finish the rendering
d.mon stop=cairo
```

![r.sim.water generated depth map](r_sim_water_water_depth.png)  
*Figure: Simulated water depth map in the rural area of the North
Carolina sample dataset.*

## ERROR MESSAGES

If the module fails with

```sh
ERROR: nwalk (7000001) > maxw (7000000)!
```

then a lower **nwalkers** parameter value has to be selected.

## REFERENCES

- Mitasova, H., Thaxton, C., Hofierka, J., McLaughlin, R., Moore, A.,
  Mitas L., 2004, [Path sampling method for modeling overland water
  flow, sediment transport and short term terrain evolution in Open
  Source
  GIS.](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/II.6.8_Mitasova_044.pdf)
  In: C.T. Miller, M.W. Farthing, V.G. Gray, G.F. Pinder eds.,
  Proceedings of the XVth International Conference on Computational
  Methods in Water Resources (CMWR XV), June 13-17 2004, Chapel Hill,
  NC, USA, Elsevier, pp. 1479-1490.
- Mitasova H, Mitas, L., 2000, [Modeling spatial processes in multiscale
  framework: exploring duality between particles and
  fields,](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/gisc00/duality.html)
  plenary talk at GIScience2000 conference, Savannah, GA.
- Mitas, L., and Mitasova, H., 1998, Distributed soil erosion simulation
  for effective erosion prevention. Water Resources Research, 34(3),
  505-516.
- Mitasova, H., Mitas, L., 2001, [Multiscale soil erosion simulations
  for land use
  management,](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/LLEmiterev1.pdf)
  In: Landscape erosion and landscape evolution modeling, Harmon R. and
  Doe W. eds., Kluwer Academic/Plenum Publishers, pp. 321-347.
- Hofierka, J, Mitasova, H., Mitas, L., 2002. GRASS and modeling
  landscape processes using duality between particles and fields.
  Proceedings of the Open source GIS - GRASS users conference 2002 -
  Trento, Italy, 11-13 September 2002.
  [PDF](http://www.ing.unitn.it/~grass/conferences/GRASS2002/proceedings/proceedings/pdfs/Mitasova_Helena_2.pdf)
- Hofierka, J., Knutova, M., 2015, Simulating aspects of a flash flood
  using the Monte Carlo method and GRASS GIS: a case study of the Malá
  Svinka Basin (Slovakia), Open Geosciences. Volume 7, Issue 1, ISSN
  (Online) 2391-5447, DOI:
  [10.1515/geo-2015-0013](https://doi.org/10.1515/geo-2015-0013), April
  2015
- Neteler, M. and Mitasova, H., 2008, [Open Source GIS: A GRASS GIS
  Approach. Third Edition.](https://grassbook.org) The International
  Series in Engineering and Computer Science: Volume 773. Springer New
  York Inc, p. 406.

## SEE ALSO

*[v.surf.rst](v.surf.rst.md), [r.slope.aspect](r.slope.aspect.md),
[r.sim.sediment](r.sim.sediment.md)*

## AUTHORS

Helena Mitasova, Lubos Mitas  
North Carolina State University  
*<hmitaso@unity.ncsu.edu>*

Jaroslav Hofierka  
GeoModel, s.r.o. Bratislava, Slovakia  
*[hofierka@geomodel.sk](mailto:hofi@geomodel.sk)*

Chris Thaxton  
North Carolina State University  
*<csthaxto@unity.ncsu.edu>*
