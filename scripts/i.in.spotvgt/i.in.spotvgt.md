## DESCRIPTION

*i.in.spotvgt* imports SPOT Vegetation (1km, global) NDVI data sets.
After import the digital numbers (DN) are remapped to VEGETATION NDVI
values and the NDVI color table is applied. The imported DN map is
removed after remapping.

Apparently missing raster cells due to bad pixel quality are
reconstructed by the SPOT operating team in the NDVI file. The
differences between the filtered (-a flag) and raw NDVI map should be
compared.

## NOTES

The SPOT VGT files are delivered in HDF4 (Hierarchical Data Format
Release 4) format. It is required to have the GDAL libraries installed
with HDF4 support.

## EXAMPLE

### Export of entire world SPOT VGT maps

When working with SPOT VGT with entire world extent, it is recommended
to zoom to *w=180W* and *e=180E* instead of using the map extent for map
export. These entire world SPOT VGT data are exceeding -180.0 degree
which can lead to unhelpful large East-West coordinates in the exported
file. It is also recommended to then use an export command which
respects the user settings. Example:

```sh
# import:
i.in.spotvgt 0001_NDV.HDF

# export:
g.region w=180W e=180E n=75:00:16.071429N s=56:00:16.069919S res=0:00:32.142857 -p
r.out.gdal 0001_NDV format=GTiff out=spotndvi.tif
```

## SEE ALSO

*[r.in.gdal](r.in.gdal.md), [r.out.gdal](r.out.gdal.md)*

## REFERENCES

[SPOT Vegetation (1km, global) NDVI data set
server](http://free.vgt.vito.be/)  
[SPOT Vegetation FAQ](http://www.vgt.vito.be/faqnew/index.html)  
[1.13 How must I read the 8 bits of the status
map?](http://www.vgt.vito.be/faqnew/) - Quality map

## AUTHOR

Markus Neteler
