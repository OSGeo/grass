## DESCRIPTION

*i.evapo.pt* Calculates the diurnal evapotranspiration after Prestley
and Taylor (1972). The Priestley-Taylor model (Priestley and Taylor,
1972) is a modification of Penman's more theoretical equation.

## NOTES

RNETD optional output from *i.evapo.potrad* is giving good results as
input for net radiation in this module.

Alpha values:

- 1.32 for estimates from vegetated areas as a result of the increase in
  surface roughness (Morton, 1983; Brutsaert and Stricker, 1979)
- 1.26 is applicable in humid climates (De Bruin and Keijman, 1979;
  Stewart and Rouse, 1976; Shuttleworth and Calder, 1979), and temperate
  hardwood swamps (Munro, 1979)
- 1.74 has been recommended for estimating potential evapotranspiration
  in more arid regions (ASCE, 1990). This worked well in Greece with
  University of Thessaloniki.

Alpha values extracted from: [Watflood
manual](http://www.civil.uwaterloo.ca/Watflood/Manual/02_03_1.htm).

## SEE ALSO

*[i.evapo.mh](i.evapo.mh.md), [i.evapo.pm](i.evapo.pm.md),
[i.evapo.time](i.evapo.time.md), [i.eb.netrad](i.eb.netrad.md),
[r.sun](r.sun.md)*

## AUTHOR

Yann Chemin, GRASS Development Team, 2007-08
