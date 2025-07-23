## DESCRIPTION

*i.eb.eta* calculates the actual evapotranspiration (ETa ; mm/d) for
diurnal period after \[1\], implemented in \[3\]. It takes input of
Diurnal Net Radiation (see *r.sun*), evaporative fraction (see
*i.eb.evapfr*) and surface skin temperature.

## NOTES

Full ETa processing will need those:

- *i.vi*, *i.albedo*, *r.latlong*, *i.emissivity*
- *i.evapo.potrad* (GRASS Addon)
- *i.eb.netrad*, *i.eb.soilheatflux*, *i.eb.hsebal01*
- *i.eb.evapfr*, *i.eb.eta*

(for time integration: *i.evapo.time_integration*)

For more details on the algorithms see \[1\]\[2\]\[3\]\[4\].

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

*[r.sun](r.sun.md), [i.eb.evapfr](i.eb.evapfr.md),
[i.eb.netrad](i.eb.netrad.md)*

## AUTHOR

Yann Chemin, Asian Institute of Technology, Thailand
