## DESCRIPTION

*i.eb.netrad* calculates the net radiation at the time of satellite
overpass, the way it is in the SEBAL model of Bastiaanssen (1995). It
takes input of Albedo, NDVI, Surface Skin temperature, time of satellite
overpass, surface emissivity, difference of temperature from surface
skin and about 2 m height (dT), instantaneous satellite overpass
single-way atmospheric transmissivity (tsw), Day of Year (DOY), and sun
zenith angle.

## NOTES

In the old methods, dT was taken as flat images (dT=5.0), if you don't
have a dT map from ground data, you would want to try something in this
line, this is to calculate atmospherical energy balance. In the same
way, a standard tsw is used in those equations. Refer to `r_net.c` for
that and for other non-used equations, but stored in there for further
research convenience.

## TODO

Add more explanations.

## REFERENCES

- Bastiaanssen, W.G.M., 1995. Regionalization of surface flux densities
  and moisture indicators in composite terrain; a remote sensing
  approach under clear skies in mediterranean climates. PhD thesis,
  Wageningen Agricultural Univ., The Netherland, 271 pp.
  ([PDF](https://edepot.wur.nl/206553))
- Chemin, Y., 2012. A Distributed Benchmarking Framework for Actual ET
  Models, in: Irmak, A. (Ed.), Evapotranspiration - Remote Sensing and
  Modeling. InTech. ([PDF](https://www.intechopen.com/chapters/26115))

## SEE ALSO

*[i.eb.soilheatflux](i.eb.soilheatflux.md),
[i.eb.hsebal01](i.eb.hsebal01.md), [i.albedo](i.albedo.md)*

## AUTHOR

Yann Chemin, International Rice Research Institute, The Philippines
