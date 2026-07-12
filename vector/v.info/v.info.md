## DESCRIPTION

The *v.info* tool reports basic information about a
vector map, including its topology status.

When the attribute database connection is defined, the
information about it is included in the JSON and *shell* outputs.
If the connection is not defined for the given layer,
no fields related to attribute database are included in the output
except `num_dblinks`. To work with multiple layers within one vector map or
attribute tables connected to other layer than the first layer,
use *v.info* together with the *v.db.connect* tool.

If the vector map is connected to one or more attribute tables, the **-c**
flag can be used to obtain the names and types of the attribute columns.
The columns are always printed for the attribute table linked to the layer
specified as a parameter. If the given layer is not connected to attribute table
database, the tools produces and error.
The current attribute database connections can be displayed or configured using
*v.db.connect*.

If the topology information is unavailable (i.e., the vector map cannot be
opened at level 2), the spatial extent and number of features must be
calculated on the fly, which may take some time with large amounts of data.

Flags **-g**, **-e**, **-t** can be used to retrieve only a subset of
information. Use one or more of these flags together with the *format* set to
"json" or "shell". For example, you can retrieve non-spatial metadata with
the **-g** flag without calculating the spatial extent and number of features
when they are unavailable (see above).
Providing all three of these flags is the same as not providing any.

## NOTES

For backwards compatibility reasons,
using flags **-g**, **-e**, **-t**, **-c**, **-h** without *format*,
automatically switches to the *shell* format. In the next major release, this may
be changed to keep the *plain* format output to best serve interactive use.
Specify format *shell* or *json* explicitly for scripting.

For history (**-h** flag), always use JSON output (`format="json"`).
Do not rely on parsing the *shell* format output which is currently
fully dependent on the internal storage format. In the next major release, the
*shell* format may be changed dramatically.

## EXAMPLES

### Metadata information

Get all the basic information such as title and topology information:

=== "Command line"

    ```sh
    v.info map=geology
    ```

    Possible output:

    ```text
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
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    data = gs.parse_command("v.info", map="geology", format="json")
    print(data["title"])
    ```

    Possible output:

    ```text
    North Carolina hospitals (points map)
    ```

    The whole JSON may look like this:

    ```json
    {
        "name": "geology",
        "mapset": "PERMANENT",
        "project": "nc_spm_08",
        "database": "\/home\/martin\/grassdata",
        "title": "North Carolina geology map (polygon map)",
        "scale": "1:1",
        "creator": "helena",
        "organization": "NC OneMap",
        "source_date": "Mon Nov  6 15:48:53 2006",
        "timestamp": null,
        "format": "native",
        "level": 2,
        "num_dblinks": 1,
        "attribute_layer_number": 1,
        "attribute_layer_name": "geology",
        "attribute_database": "\/home\/martin\/grassdata\/nc_spm_08\/PERMANENT\/sqlite\/sqlite.db",
        "attribute_database_driver": "sqlite",
        "attribute_table": "geology",
        "attribute_primary_key": "cat",
        "projection": "Lambert Conformal Conic",
        "digitization_threshold": 0,
        "comment": "",
        "north": 318117.43741634465,
        "south": 10875.827232091688,
        "east": 930172.31282271142,
        "west": 123971.19498978264,
        "top": 0,
        "bottom": 0,
        "nodes": 2724,
        "points": 0,
        "lines": 0,
        "boundaries": 3649,
        "centroids": 1832,
        "areas": 1832,
        "islands": 907,
        "primitives": 5481,
        "map3d": false
    }
    ```

### Attribute table columns

The tool returns attribute column for a specified layer within the vector map.
The default layer is layer 1 which is usually the one connected to an attribute
table database, so leaving the default value produces the expected result:

=== "Command line"

    ```sh
    v.info -c map=geology format=shell
    ```

    Possible output:

    ```
    INTEGER|cat
    DOUBLE PRECISION|onemap_pro
    DOUBLE PRECISION|PERIMETER
    INTEGER|GEOL250_
    INTEGER|GEOL250_ID
    CHARACTER|GEO_NAME
    DOUBLE PRECISION|SHAPE_area
    DOUBLE PRECISION|SHAPE_len
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    data = gs.parse_command("v.info", map="geology", flags="c", format="json")
    print("names:", ", ".join(column["name"] for column in data["columns"]))
    ```

    Possible output:

    ```text
    names: cat, onemap_pro, PERIMETER, GEOL250_, GEOL250_ID, GEO_NAME, SHAPE_area, SHAPE_len
    ```

### Map history

=== "Command line"

    ```sh
    v.info -h map=geology
    ```

    Possible output:

    ```text
    COMMAND: v.in.ogr input="geol.shp" output="geology" min_area=0.0001 snap=-1
    GISDBASE: /bigdata/grassdata05
    LOCATION: ncfromfile MAPSET: PERMANENT USER: helena DATE: Mon Nov  6 15:48:53 2006
    ---------------------------------------------------------------------------------
    1832 input polygons
    total area: 1.276093e+11 (1832 areas)
    overlapping area: 0.000000e+00 (0 areas)
    area without category: 0.000000e+00 (0 areas)
    ---------------------------------------------------------------------------------
    ```

    Note that while "project" is used by *v.info* elsewhere, history output
    uses the legacy term "location" because "LOCATION" is currently a part
    of the native vector format.

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    data = gs.parse_command("v.info", map="hospitals", flags="h", format="json")
    for record in data["records"]:
        print(record["command"])
    ```

    Possible output:

    ```text
    v.in.ogr dsn="hls.shp" output="hospitals" min_area=0.0001 snap=-1
    v.db.connect -o map="hospitals@PERMANENT" driver="sqlite" database="$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db" table="hospitals" key="cat" layer="1" separator="|"
    ```

### Limiting the output

=== "Command line"

    ```sh
    v.info -g map=geology
    ```

    Possible output:

    ```text
    north=318117.43741634
    south=10875.82723209
    east=930172.31282271
    west=123971.19498978
    top=0.000000
    bottom=0.000000
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    data = gs.parse_command("v.info", map="geology", flags="g", format="json")
    print(data)
    ```

    Possible output:

    ```text
    {'north': 318117.43741634465, 'south': 10875.827232091688, 'east': 930172.3128227114, 'west': 123971.19498978264, 'top': 0, 'bottom': 0}
    ```

## SEE ALSO

*[v.db.connect](v.db.connect.md), [v.category](v.category.md), [r.info](r.info.md), [r3.info](r3.info.md), [t.info](t.info.md)*

## AUTHORS

Original author CERL  
Updated to GRASS 6 by Radim Blazek, ITC-Irst, Trento, Italy  
Level 1 support by Markus Metz  
Updated to GRASS 7 by Martin Landa, CTU in Prague, Czech Republic
