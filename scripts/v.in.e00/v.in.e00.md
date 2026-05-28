## DESCRIPTION

*v.in.e00* imports ASCII and binary E00 vector maps into GRASS.

## NOTES

Sometimes an .e00 coverage consists of multiple files, where a single
data set is contained as a series of files named filename.e00,
filename.e01, filename.e02 etc. The user must take care to download them
all, the scripts automatically detects the presence of such multiple
files.

## REFERENCES

[AVCE00 library](http://avce00.maptools.org) (providing 'avcimport' and
'e00conv')  
[OGR vector library](https://gdal.org/)

## SEE ALSO

*[v.in.ogr](v.in.ogr.md)*

## AUTHORS

Markus Neteler, Otto Dassau, [GDF Hannover
bR](http://www.gdf-hannover.de/), Germany
