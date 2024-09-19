## DESCRIPTION

*g.download.location* downloads an archived (e.g., `.zip` or `.tar.gz`)
project (previously called location) from a given URL and unpacks it to
a specified or current GRASS GIS Spatial Database. URL can be also a
local file on the disk. If the archive contains a directory which
contains a project, the module will recognize that and use the project
automatically. The first directory which is a project is used. Other
projects or any other files are ignored.

## EXAMPLES

### Download the full GRASS GIS sample project within a running session

Download and unpack the full North Carolina sample project into the
user\'s HOME directory:

```
g.download.location url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz path=$HOME
```

### Download the full GRASS GIS sample project in a temporary session

In a temporary session, download and unpack the full North Carolina
sample project into the user\'s HOME directory:

```
grass --tmp-location XY --exec g.download.location url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz path=$HOME
```

## SEE ALSO

*[g.mapset](g.mapset.html), [g.mapsets](g.mapsets.html),
[r.proj](r.proj.html), [v.proj](v.proj.html),
[g.proj.all](g.proj.all.html)*

## AUTHOR

Vaclav Petras, [NCSU GeoForAll
Lab](http://geospatial.ncsu.edu/osgeorel/)
