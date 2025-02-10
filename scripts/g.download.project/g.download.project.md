## DESCRIPTION

*g.download.project* downloads an archived (e.g., `.zip` or `.tar.gz`)
project (previously called location) from a given URL and unpacks it to
a specified or current GRASS GIS Spatial Database. URL can be also a
local file on the disk. If the archive contains a directory which
contains a project, the module will recognize that and use the project
automatically. The first directory which is a project is used. Other
projects or any other files are ignored.

## EXAMPLES

### Download the full GRASS GIS sample project within a running session

Download and unpack the full North Carolina sample project into the
user's HOME directory:

```sh
g.download.project url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz path=$HOME
```

### Download the full GRASS GIS sample project in a temporary session

In a temporary session, download and unpack the full North Carolina
sample project into the user's HOME directory:

```sh
grass --tmp-project XY --exec g.download.project url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz path=$HOME
```

## SEE ALSO

*[g.mapset](g.mapset.md), [g.mapsets](g.mapsets.md),
[r.proj](r.proj.md), [v.proj](v.proj.md),
[g.proj.all](https://grass.osgeo.org/grass-stable/manuals/addons/g.proj.all.html)*

## AUTHOR

Vaclav Petras, [NCSU GeoForAll
Lab](http://geospatial.ncsu.edu/geoforall/)
