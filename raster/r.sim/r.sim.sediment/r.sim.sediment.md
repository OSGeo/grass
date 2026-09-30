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
elevation field (*dx* and *dy* raster maps are optional), overland flow water depth
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
controlled by *duration* \[minutes\] parameter. If the resulting
erosion/deposition map is noisy, higher number of walkers, given by
*nwalkers* should be used.  

Increasing the number of threads with **nprocs** speeds up the
simulation. The walkers draw their random numbers as in *r.sim.water*:
each walker from a sequence of its own determined by **random_seed**
(or the seed the **-s** flag generates, 12345 when neither is given) and
the walker's number, so the walkers move the same way whatever the value
of **nprocs**. With **nprocs=1**, runs with the same seed and inputs give
identical results. With more threads, walkers in the same cell add to
the sediment concentration in an order which depends on the threads, and
the floating point sums differ slightly with the order, so the outputs
are close to, but not identical with, the single-threaded result, and
repeated runs with the same **nprocs** may differ slightly. See
[r.sim.water](r.sim.water.md) for the range of seeds.

## NOTES

### Run summary

With the **-p** flag, a summary of the run is printed to standard output
after the maps are written. The **format** option selects plain text
(one `key: value` pair per line) or JSON. Without **-p**, nothing is
printed to standard output regardless of **format**. The values are also
stored in the history of the *sediment_flux* raster map under the same
keys (see [r.info](r.info.md)).

The keys are the same as for [r.sim.water](r.sim.water.md) with these
differences:

| Key | Meaning | Unit |
| --- | --- | --- |
| `time_step_sediment` | Time step limit derived from the sediment transport parameters, `null` when no cell exceeds the critical shear stress | s |
| `velocity_max` | Maximum flow velocity over the defined cells | m/s |
| `sigma_max` | Maximum first order reaction coefficient (detachment to transport capacity ratio) over the defined cells | 1/m |
| `mean_source_rate` | Mean sediment source (detachment) rate | kg/m^2s |
| `mean_infiltration` | Not reported | |
| `transport_capacity`, `tlimit_erosion_deposition` | Names of these maps, which are written once before the simulation starts, or `null` when not requested | |
| `outputs` | A single entry with the `simulated_time` (s), `timestamp` and `walkers_remaining` at the time of writing, and the names of the `sediment_concentration`, `sediment_flux`, `erosion_deposition` and `walkers` maps, or `null` for maps which were not requested | |

Summary of a run in JSON:

```sh
r.sim.sediment elevation=elevation water_depth=water_depth detachment_coeff=detachment \
    transport_coeff=transport shear_stress=shear_stress man_value=1 \
    sediment_flux=flux erosion_deposition=erdep transport_capacity=tc \
    duration=1 random_seed=1 -p format=json
```

```json
{
    "walkers_requested": 60,
    "walkers_generated": 78,
    "walkers_remaining": 43,
    "duration": 60,
    "simulated_time": 54.891343113611583,
    "time_step": 5.4891343113611581,
    "time_step_sediment": 70.517234241515013,
    "iterations_planned": 10,
    "iterations_completed": 10,
    "stopped_early": false,
    "mean_velocity": 0.1821780891624834,
    "velocity_max": 0.21544346900318839,
    "sigma_max": 0.052657639041437901,
    "mean_mannings_n": 1,
    "mean_source_rate": 0.56025284041612944,
    "threads": 1,
    "transport_capacity": "tc",
    "tlimit_erosion_deposition": null,
    "outputs": [
        {
            "simulated_time": 54.891343113611583,
            "timestamp": "1 minutes",
            "walkers_remaining": 43,
            "sediment_concentration": null,
            "sediment_flux": "flux",
            "erosion_deposition": "erdep",
            "walkers": null
        }
    ]
}
```

## REFERENCES

[Mitasova, H., Thaxton, C., Hofierka, J., McLaughlin, R., Moore, A.,
Mitas L.,
2004,](https://doi.org/10.1016/S0167-5648(04)80159-X)
Path sampling method for modeling overland water flow, sediment
transport and short term terrain evolution in Open Source GIS. In: C.T.
Miller, M.W. Farthing, V.G. Gray, G.F. Pinder eds., Proceedings of the
XVth International Conference on Computational Methods in Water
Resources (CMWR XV), June 13-17 2004, Chapel Hill, NC, USA, Elsevier,
pp. 1479-1490.

Mitasova H, Mitas, L., 2000, Modeling spatial processes in multiscale
framework: exploring duality between particles and fields, plenary talk
at GIScience2000 conference, Savannah, GA.

Mitas, L., and Mitasova, H., 1998, Distributed soil erosion simulation
for effective erosion prevention. Water Resources Research, 34(3),
505-516.

[Mitasova, H., Mitas, L., 2001, Multiscale soil erosion simulations for
land use
management,](https://doi.org/10.1007/978-1-4615-0575-4_11)
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
