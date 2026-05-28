## DESCRIPTION

*i.biomass* calculates the biomass growth for a day after \[1\]\[2\].
Input:

- fPAR, the modified Photosynthetic Active Radiation for crops.
- Light Use Efficiency \[0.0-1.0\], in Uzbekistan cotton is at 1.9 most
  of the time.
- Latitude \[0.0-90.0\], from *r.latlong*.
- DOY \[1-366\].
- Transmissivity of the atmosphere single-way \[0.0-1.0\], mostly around
  0.7+ in clear sky.
- Water availability \[0.0-1.0\], possibly using direct output from
  *i.eb.evapfr*.

## NOTES

*i.biomass* can use the output of *i.eb.evapfr* directly as water
availability input.

## TODO

Remove Latitude, DOY and Tsw from input and replace with a raster input
compatible with *r.sun* output.

## REFERENCES

\[1\] Bastiaanssen, W.G.M., Ali, S., 2002. A new crop yield forecasting
model based on satellite measurements applied across the Indus Basin,
Pakistan. Agriculture, Ecosystems and Environment, 94(3):321-340.
([PDF](https://edepot.wur.nl/206553))

\[2\] Chemin, Y., Platonov, A., Abdullaev, I., Ul-Hassan, M. 2005.
Supplementing farm level water productivity assessment by remote sensing
in transition economies. Water International. 30(4):513-521.

## SEE ALSO

*[i.eb.evapfr](i.eb.evapfr.md), [r.latlong](r.latlong.md),
[r.sun](r.sun.md)*

## AUTHOR

Yann Chemin, Bec de Mortagne, France
