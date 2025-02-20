## DESCRIPTION

*r.sim.sediment* is a landscape scale, simulation model of soil erosion,
sediment transport and deposition caused by flowing water designed for
spatially variable terrain, soil, cover and rainfall excess conditions.
The soil erosion model is based on the theory used in the USDA WEPP
hillslope erosion model, but it has been generalized to 2D flow. The
solution is based on the concept of duality between fields and particles
and the underlying equations are solved by Green's function Monte Carlo
method, to provide robustness necessary for spatially variable
conditions and high resolutions (Mitas and Mitasova 1998). Key inputs of
the model include the following raster maps: elevation (*elevation*
\[m\]), flow gradient given by the first-order partial derivatives of
elevation field ( *dx* and *dy*), overland flow water depth
(*water_depth* \[m\]), detachment capacity coefficient
(*detachment_coeff* \[s/m\]), transport capacity coefficient
(*transport_coeff* \[s\]), critical shear stress (*shear_stress* \[Pa\])
and surface roughness coefficient called Manning's n (*man* raster map).
Partial derivatives can be computed by [v.surf.rst](v.surf.rst.md) or
[r.slope.aspect](r.slope.aspect.md) module. The data are automatically
converted from feet to metric system using database/projection
information, so the elevation always should be in meters. The water
depth file can be computed using [r.sim.water](r.sim.water.md) module.
Other parameters must be determined using field measurements or
reference literature (see suggested values in Notes and References).  

Output includes transport capacity raster map *transport_capacity* in
\[kg/ms\], transport capacity limited erosion/deposition raster map
*tlimit_erosion_deposition* \[kg/m^2s\] that are output
almost immediately and can be viewed while the simulation continues.
Sediment flow rate raster map *sediment_flux* \[kg/ms\], and net
erosion/deposition raster map \[kg/m^2s\] can take longer time
depending on time step and simulation time. Simulation time is
controlled by *niterations* \[minutes\] parameter. If the resulting
erosion/deposition map is noisy, higher number of walkers, given by
*nwalkers* should be used.  

Increasing the number of threads with **nprocs** does not really speed
up the simulation.

## REFERENCES

[Mitasova, H., Thaxton, C., Hofierka, J., McLaughlin, R., Moore, A.,
Mitas L.,
2004,](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/II.6.8_Mitasova_044.pdf)
Path sampling method for modeling overland water flow, sediment
transport and short term terrain evolution in Open Source GIS. In: C.T.
Miller, M.W. Farthing, V.G. Gray, G.F. Pinder eds., Proceedings of the
XVth International Conference on Computational Methods in Water
Resources (CMWR XV), June 13-17 2004, Chapel Hill, NC, USA, Elsevier,
pp. 1479-1490.

[Mitasova H, Mitas, L., 2000, Modeling spatial processes in multiscale
framework: exploring duality between particles and
fields,](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/gisc00/duality.html)
plenary talk at GIScience2000 conference, Savannah, GA.

Mitas, L., and Mitasova, H., 1998, Distributed soil erosion simulation
for effective erosion prevention. Water Resources Research, 34(3),
505-516.

[Mitasova, H., Mitas, L., 2001, Multiscale soil erosion simulations for
land use
management,](http://fatra.cnr.ncsu.edu/~hmitaso/gmslab/papers/LLEmiterev1.pdf)
In: Landscape erosion and landscape evolution modeling, Harmon R. and
Doe W. eds., Kluwer Academic/Plenum Publishers, pp. 321-347.

[Neteler, M. and Mitasova, H., 2008, Open Source GIS: A GRASS GIS
Approach. Third Edition.](https://grassbook.org) The International
Series in Engineering and Computer Science: Volume 773. Springer New
York Inc, p. 406.

## SEE ALSO

[v.surf.rst](v.surf.rst.md), [r.slope.aspect](r.slope.aspect.md),
[r.sim.water](r.sim.water.md)

## AUTHORS

Helena Mitasova, Lubos Mitas  
North Carolina State University  
<hmitaso@unity.ncsu.edu>  
  
Jaroslav Hofierka  
GeoModel, s.r.o. Bratislava, Slovakia  

[hofierka@geomodel.sk](mailto:hofi@geomodel.sk)

Chris Thaxton  
North Carolina State University  
<csthaxto@unity.ncsu.edu>  

<csthaxto@unity.ncsu.edu>
