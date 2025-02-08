## DESCRIPTION

*v.in.wfs* imports OGC WFS maps (Web Feature Service) from external
servers.

## EXAMPLES

### WFS import without credentials

Import of Copernicus Sentinel-2 satellite scene footprints:

```sh
# run in Latitude-Longitude project (EPGS code 4326):
# download "sentinel:mgrs" layer:
v.in.wfs url="https://geoserver.mundialis.de/geoserver/sentinel/wfs?" name="sentinel:mgrs" output=sentinel2_mgrs
# download NRW ALKIS "ave:Flurstueck" attribute:
# set the AOI beforehand with g.region and limit import to current region with -r flag
v.in.wfs url="https://www.wfs.nrw.de/geobasis/wfs_nw_alkis_vereinfacht?" -r output=wfs_alkis_vereinfacht srs=25832
name="ave:Flurstueck" version="2.0.0" layer="Flurstueck"
```

### WFS import with API key

Download 25 ship wrecks from LINZ data service:  
(first create yourself a free API key at
<http://data.linz.govt.nz/p/web-services/>)

```sh
# run in LatLong project:
URL='http://wfs.data.linz.govt.nz/<PUT YOUR API KEY HERE>/wfs?'

# download list of available layers to wms_capabilities.xml
v.in.wfs -l url="$URL"
```

From that file we learn that the shipwreck layer is called "`v:x633`"
and that EPSG code 4326 (LatLong WGS84) is a supported SRS for this data
layer.

```sh
v.in.wfs url="$URL" output=linz_hydro_25_wrecks name="v:x633" srs="EPSG:4326" max=25
```

## REQUIREMENTS

The OGR library on the system needs to be compiled with Xerces C++ XML
Parser support (for GML).

## SEE ALSO

*[g.region](g.region.md), [r.in.wms](r.in.wms.md),
[v.import](v.import.md), [v.in.ogr](v.in.ogr.md)*

## AUTHORS

Markus Neteler, Hamish Bowman
