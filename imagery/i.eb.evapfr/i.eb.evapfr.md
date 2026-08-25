## DESCRIPTION

*i.eb.evapfr* calculates the evaporative fraction after Bastiaanssen
1995. The main implementation follows Alexandridis et al. (2009). The
module takes as input the net radiation (see *r.sun*, *i.eb.netrad*),
soil heat flux (see *i.eb.soilheatflux*) and sensible heat flux (see
*i.eb.hsebal01*). A flag adds a root zone empirical soil moisture output
from the article of Bastiaanssen, et al. (2000).

## REFERENCES

Bastiaanssen, W.G.M., 1995. Estimation of Land surface parameters by
remote sensing under clear-sky conditions. PhD thesis, Wageningen
University, Wageningen, The Netherlands.
([PDF](https://edepot.wur.nl/206553))

Bastiaanssen, W.G.M., Molden, D.J., Makin, I.W., 2000. Remote sensing
for irrigated agriculture: examples from research and possible
applications. Agricultural water management 46.2: 137-155.

Chemin Y., Alexandridis T.A., 2001. Improving spatial resolution of ET
seasonal for irrigated rice in Zhanghe, China. Asian Journal of
Geoinformatics. 5(1):3-11.

Alexandridis T.K., Cherif I., Chemin Y., Silleos N.G., Stavrinos E.,
Zalidis G.C., 2009. Integrated methodology for estimating water use in
Mediterranean agricultural areas. Remote Sensing. 1, 445-465.
([PDF](https://doi.org/10.3390/rs1030445))

Chemin, Y., 2012. A Distributed Benchmarking Framework for Actual ET
Models, in: Irmak, A. (Ed.), Evapotranspiration - Remote Sensing and
Modeling. InTech. ([PDF](https://www.intechopen.com/chapters/26115))

## SEE ALSO

*[i.eb.hsebal01](i.eb.hsebal01.md), [i.eb.netrad](i.eb.netrad.md),
[i.eb.soilheatflux](i.eb.soilheatflux.md), [r.sun](r.sun.md)*

## AUTHOR

Yann Chemin, Asian Institute of Technology, Thailand
