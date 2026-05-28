## DESCRIPTION

*v.in.mapgen* allows the user to import Mapgen or Matlab vector maps
into GRASS.

## NOTES

This module only imports data into vector lines.

The user can get coastline data in Mapgen or Matlab format from NOAA's
Coastline Extractor at
<https://www.ngdc.noaa.gov/mgg/shorelines/shorelines.html>.

Matlab vector line maps are simply a series of "x y" data points. Lines
are separated by a row containing `NaN NaN`. Output from Matlab with
this command:  

```sh
    save filename.txt arrayname -ASCII
```

The user can import 3D lines from Matlab by exporting a 3 column array
and using the **-z** flag.

## SEE ALSO

*[v.in.ascii](v.in.ascii.md)*

## AUTHORS

Based on *v.in.mapgen.sh* for GRASS 5.0 by Andreas Lange  
Rewritten for GRASS 6 by Hamish Bowman
