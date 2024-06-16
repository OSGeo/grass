## DESCRIPTION

*v.info* reports some basic information (metadata) about a
user-specified vector map and its topology status.

If topology info is not available (i.e., vector map cannot be opened on
level 2), vector map extends and number of features need to be counted
on the fly which may take some time.

Note that the flag **-c** only works when the vector map is connected to
one or several attribute table(s). This connection can be shown or set
with *v.db.connect*.

## EXAMPLE

### Basic metadata information

::: code
    v.info map=geology

     +----------------------------------------------------------------------------+
     | Name:            geology                                                   |
     | Mapset:          PERMANENT                                                 |
     | Project:         nc_spm_08                                                 |
     | Database:        /home/martin/grassdata                                    |
     | Title:           North Carolina geology map (polygon map)                  |
     | Map scale:       1:1                                                       |
     | Map format:      native                                                    |
     | Name of creator: helena                                                    |
     | Organization:    NC OneMap                                                 |
     | Source date:     Mon Nov  6 15:48:53 2006                                  |
     |----------------------------------------------------------------------------|
     |   Type of map: vector (level: 2)                                           |
     |                                                                            |
     |   Number of points:       0               Number of centroids:  1832       |
     |   Number of lines:        0               Number of boundaries: 3649       |
     |   Number of areas:        1832            Number of islands:    907        |
     |                                                                            |
     |   Map is 3D:              No                                               |
     |   Number of dblinks:      1                                                |
     |                                                                            |
     |   Projection: Lambert Conformal Conic                                      |
     |                                                                            |
     |               N:   318117.43741634    S:    10875.82723209                 |
     |               E:   930172.31282271    W:   123971.19498978                 |
     |                                                                            |
     |   Digitization threshold: 0                                                |
     |   Comment:                                                                 |
     |                                                                            |
     +----------------------------------------------------------------------------+
:::

### Map history

::: code
    v.info -h map=geology

    COMMAND: v.in.ogr input="geol.shp" output="geology" min_area=0.0001 snap=-1
    GISDBASE: /bigdata/grassdata05
    LOCATION: ncfromfile MAPSET: PERMANENT USER: helena DATE: Mon Nov  6 15:48:53 2006
    ---------------------------------------------------------------------------------
    1832 input polygons
    total area: 1.276093e+11 (1832 areas)
    overlapping area: 0.000000e+00 (0 areas)
    area without category: 0.000000e+00 (0 areas)
    ---------------------------------------------------------------------------------
:::

Note that while \"project\" is used by *v.info* elsewhere, history
output uses the legacy term \"location\" because \"LOCATION\" is
currently a part of the native vector format.

### Attribute columns for given layer

::: code
    v.info -c map=geology

    Displaying column types/names for database connection of layer <1>:
    INTEGER|cat
    DOUBLE PRECISION|onemap_pro
    DOUBLE PRECISION|PERIMETER
    INTEGER|GEOL250_
    INTEGER|GEOL250_ID
    CHARACTER|GEO_NAME
    DOUBLE PRECISION|SHAPE_area
    DOUBLE PRECISION|SHAPE_len
:::

### Basic metadata information in shell script style

::: code
    v.info -get map=geology

    name=geology
    mapset=PERMANENT
    project=nc_spm_08
    database=/home/martin/grassdata
    title=North Carolina geology map (polygon map)
    scale=1:1
    format=native
    creator=helena
    organization=NC OneMap
    source_date=Mon Nov  6 15:48:53 2006
    level=2
    map3d=0
    num_dblinks=1
    projection=Lambert Conformal Conic
    digitization_threshold=0.000000
    comment=
    north=318117.43741634
    south=10875.82723209
    east=930172.31282271
    west=123971.19498978
    top=0.000000
    bottom=0.000000
    nodes=4556
    points=0
    lines=0
    boundaries=3649
    centroids=1832
    areas=1832
    islands=907
    primitives=5481
:::

::: code
    v.info -g map=geology

    north=318117.43741634
    south=10875.82723209
    east=930172.31282271
    west=123971.19498978
    top=0.000000
    bottom=0.000000
:::

## PYTHON

See *[Python Scripting
Library](https://grass.osgeo.org/grass-stable/manuals/libpython/)* for
more info.

Note: The Python tab in the *wxGUI* can be used for entering the
following code:

::: code
    import grass.script as gcore

    gcore.vector_columns('geology')   # for `v.info -c`
    gcore.vector_info_topo('geology') # for `v.info shell=topo`
:::

## SEE ALSO

*[r.info](r.info.html), [r3.info](r3.info.html), [t.info](t.info.html),
[v.db.connect](v.db.connect.html)*

## AUTHORS

Original author CERL\
Updated to GRASS 6 by Radim Blazek, ITC-Irst, Trento, Italy\
Level 1 support by Markus Metz\
Updated to GRASS 7 by Martin Landa, CTU in Prague, Czech Republic
