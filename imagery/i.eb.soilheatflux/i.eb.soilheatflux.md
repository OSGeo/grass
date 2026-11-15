## DESCRIPTION

*i.eb.soilheatflux* calculates the soil heat flux approximation (g0)
after Bastiaanssen (1995). The main reference for implementation is
Alexandridis, 2009. It takes input of Albedo, NDVI, Surface Skin
temperature, Net Radiation (see *r.sun*), time of satellite overpass,
and a flag for the Roerink empirical modification from the HAPEX-Sahel
experiment. The "time of satellite overpass" map can be obtained as
follows:

- MODIS: a related sub dataset is included in each HDF file, and simply
  to be imported as a raster map;
- Landsat: to be generated as map from the overpass time stored in the
  metadata file (given in Greenwich Mean Time - GMT), see below.

For Landsat, the overpass map can be computed by using a two-step
method:

```sh
# 1) extract the overpass time in GMT from metadata file

i.landsat.toar -p input=dummy output=dummy2 \
   metfile=LC81250452013338LGN00_MTL.txt lsatmet=time
# ... in this example approx. 03:12am GMT

# 2) create map for computational region of Landsat scene
g.region rast=LC81250452013338LGN00_B4 -p
r.mapcalc "overpasstime = 3.211328"
```

## REFERENCES

Bastiaanssen, W.G.M., 1995. Estimation of Land surface parameters by
remote sensing under clear-sky conditions. PhD thesis, Wageningen
University, Wageningen, The Netherlands.
([PDF](https://edepot.wur.nl/206553))

Chemin Y., Alexandridis T.A., 2001. Improving spatial resolution of ET
seasonal for irrigated rice in Zhanghe, China. Asian Journal of
Geoinformatics. 5(1):3-11,2004.

Alexandridis T.K., Cherif I., Chemin Y., Silleos N.G., Stavrinos E.,
Zalidis G.C. Integrated methodology for estimating water use in
Mediterranean agricultural areas. Remote Sensing. 2009, 1, 445-465.
([PDF](https://doi.org/10.3390/rs1030445))

Chemin, Y., 2012. A Distributed Benchmarking Framework for Actual ET
Models, in: Irmak, A. (Ed.), Evapotranspiration - Remote Sensing and
Modeling. InTech. ([PDF](https://www.intechopen.com/chapters/26115))

## SEE ALSO

*[r.sun](r.sun.md), [i.albedo](i.albedo.md),
[i.emissivity](i.emissivity.md), [i.eb.hsebal01](i.eb.hsebal01.md),
[i.eb.evapfr](i.eb.evapfr.md) [i.landsat.toar](i.landsat.toar.md)*

## AUTHOR

Yann Chemin, Asian Institute of Technology, Thailand
