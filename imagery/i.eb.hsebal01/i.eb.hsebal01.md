## DESCRIPTION

*i.eb.hsebal01* will calculate the sensible heat flux map (h0), given
both maps of Net Radiation and soil Heat flux (Rn, g0) at instantaneous
time, the surface roughness (z0m), a map of the altitude corrected
temperature (t0dem), a point data of the frictional velocity (u\*), a
value of actual vapour pressure (ea\[KPa\]) and the (x,y) pairs for wet
and dry pixels. Full process will need those:

- *i.vi*, *i.albedo*, *r.latlong*, *i.emissivity*
- *i.evapo.potrad* (GRASS Addon)
- *i.eb.netrad*, *i.eb.soilheatflux*, *i.eb.hsebal01*
- *i.eb.evapfr*, *i.eb.eta*

(for time integration: *i.evapo.time_integration*)

*i.eb.hsebal01* performs the computation of *sensible heat flux*
\[W/m2\] after Bastiaanssen, 1995 in \[1\], used in this form in 2001 by
\[2\]. Implemented in this code in \[3\].

## NOTES

- z0m can be alculated by *i.eb.z0m* or *i.eb.z0m0* (GRASS Addons).
- ea can be calculated with standard meteorological data.  
  eoTmin=0.6108\*EXP(17.27\*Tmin/(Tmin+237.3))  
  eoTmax=0.6108\*EXP(17.27\*Tmax/(Tmax+237.3))  
  ea=(RH/100)/((eoTmin+eoTmax)/2)
- t0dem = surface temperature + (altitude \* 0.627 / 100)

## REFERENCES

\[1\] Bastiaanssen, W.G.M., 1995. Estimation of Land surface parameters
by remote sensing under clear-sky conditions. PhD thesis, Wageningen
University, Wageningen, The Netherlands.
([PDF](https://edepot.wur.nl/206553))

\[2\] Chemin Y., Alexandridis T.A., 2001. Improving spatial resolution
of ET seasonal for irrigated rice in Zhanghe, China. Asian Journal of
Geoinformatics. 5(1):3-11,2004.

\[3\] Alexandridis T.K., Cherif I., Chemin Y., Silleos N.G., Stavrinos
E., Zalidis G.C. Integrated methodology for estimating water use in
Mediterranean agricultural areas. Remote Sensing. 2009, 1, 445-465.
([PDF](https://doi.org/10.3390/rs1030445))

\[4\] Chemin, Y., 2012. A Distributed Benchmarking Framework for Actual
ET Models, in: Irmak, A. (Ed.), Evapotranspiration - Remote Sensing and
Modeling. InTech. ([PDF](https://www.intechopen.com/chapters/26115))

## SEE ALSO

*[i.eb.soilheatflux](i.eb.soilheatflux.md),
[i.eb.evapfr](i.eb.evapfr.md)*

## AUTHOR

Yann Chemin, International Rice Research Institute, Los Banos, The
Philippines.

Contact: [Yann Chemin](mailto:yann.chemin@gmail.com)
