## DESCRIPTION

*i.evapo.pm*, given the vegetation height (hc), humidity (RU), wind
speed at two meters height (WS), temperature (T), digital terrain model
(DEM), and net radiation (NSR) raster input maps, calculates the
potential evapotranspiration map (EPo).

Optionally the user can activate a flag (-z) that allows him setting to
zero all of the negative evapotranspiration cells; in fact these
negative values motivated by the condensation of the air water vapour
content, are sometime undesired because they can produce computational
problems. The usage of the flag -n detect that the module is run in
night hours and the appropriate soil heat flux is calculated.

The algorithm implements well known approaches: the hourly
Penman-Monteith method as presented in Allen et al. (1998) for land
surfaces and the Penman method (Penman, 1948) for water surfaces.

Land and water surfaces are idenfyied by Vh:

- where Vh gt 0 vegetation is present and evapotranspiration is
  calculated;
- where Vh = 0 bare ground is present and evapotranspiration is
  calculated;
- where Vh lt 0 water surface is present and evaporation is calculated.

For more details on the algorithms see \[1,2,3\].

## NOTES

Net solar radiation map in MJ/(m2\*h) can be computed from the
combination of the r.sun , run in mode 1, and the *r.mapcalc* commands.

The sum of the three radiation components outputted by r.sun (beam,
diffuse, and reflected) multiplied by the Wh to Mj conversion factor
(0.0036) and optionally by a clear sky factor \[0-1\] allows the
generation of a map to be used as an NSR input for the *i.evapo.PM*
command.

Example:

```sh
r.sun -s elevin=dem aspin=aspect slopein=slope lin=2 albedo=alb_Mar \
      incidout=out beam_rad=beam diff_rad=diffuse refl_rad=reflected \
      day=73 time=13:00 dist=100;
r.mapcalc "NSR = 0.0036 * (beam + diffuse + reflected)"
```

## REFERENCES

\[1\] Cannata M., 2006. [GIS embedded approach for Free & Open Source
Hydrological
Modelling](http://istgis.ist.supsi.ch:8001/geomatica/index.php?id=1).
PhD thesis, Department of Geodesy and Geomatics, Polytechnic of Milan,
Italy.

\[2\] Allen, R.G., L.S. Pereira, D. Raes, and M. Smith. 1998. Crop
Evapotranspiration: Guidelines for computing crop water requirements.
Irrigation and Drainage Paper 56, Food and Agriculture Organization of
the United Nations, Rome, pp. 300

\[3\] Penman, H. L. 1948. Natural evaporation from open water, bare soil
and grass. Proc. Roy. Soc. London, A193, pp. 120-146.

## SEE ALSO

The [HydroFOSS](http://istgis.ist.supsi.ch:8001/geomatica/) project at
IST-SUPSI (Institute of Earth Sciences - University school of applied
science for the Southern Switzerland)  
*[i.evapo.mh](i.evapo.mh.md), [i.evapo.time](i.evapo.time.md),
[r.sun](r.sun.md), [r.mapcalc](r.mapcalc.md)*

## AUTHORS

Original version of program: The
[HydroFOSS](http://istgis.ist.supsi.ch:8001/geomatica/index.php?id=1)
project, 2006, IST-SUPSI.
(<http://istgis.ist.supsi.ch:8001/geomatica/index.php?id=1>) *Massimiliano
Cannata, Scuola Universitaria Professionale della Svizzera Italiana -
Istituto Scienze della Terra*  
*Maria A. Brovelli, Politecnico di Milano - Polo regionale di Como*

Contact: [Massimiliano Cannata](mailto:massimiliano.cannata@supsi.ch)
