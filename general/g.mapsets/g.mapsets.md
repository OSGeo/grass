## DESCRIPTION

*g.mapsets* modifies/prints the user's current mapset search path. For
basic information about GRASS *mapset*, *project* and *data base* refer
to [GRASS Quickstart](helptext.md).

A *mapset* holds a distinct set of data layers, each relevant to the
same (or a subset of the same) geographic region, and each drawn in the
same map coordinate system. At the outset of every GRASS session, the
user identifies a GRASS data base, project (previosuly called location),
and mapset that are to be the user's *current data base*, *current
project*, and *current mapset* for the duration of the session; any maps
created by the user during the session will be stored under the *current
mapset* set at the session's outset (see *[g.mapset](g.mapset.md)*
\[without an "s"\] and *[g.gisenv](g.gisenv.md)* for changing the mapset
with a session).

The user can add, modify, and delete data layers that exist under their
*current mapset*. Although the user can also *access* (i.e., use) data
that are stored under *other* mapsets in the same GRASS project using
the `mapname@mapsetname` notation or mapset search path, the user can
only make permanent changes (create or modify data) located in the
*current mapset*. The user's *mapset search path* lists the order in
which other mapsets in the same GRASS project can be searched and their
data accessed by the user. The user can modify the listing and order in
which these mapsets are accessed by modifying the mapset search path;
this can be done using the *g.mapsets* command. This program allows the
user to use other's relevant map data without altering the original data
layer, and without taking up disk space with a copy of the original map.
The `mapname@mapsetname` notation may be used irrespective of the mapset
search path, i.e., any map found in another mapset with sufficient
*[g.access](g.access.md)* privileges may be called in such a manner.

*g.mapsets* shows the user available mapsets under the current GRASS
project, lists mapsets to which the user currently has access, and lists
the order in which accessible mapsets will be accessed by GRASS programs
searching for data files. The user is then given the opportunity to add
or delete mapset names from the search path, or modify the order in
which mapsets will be accessed.

When the user specifies the name of a data base element file (e.g., a
particular vector map, raster map, [imagery](i.group.md) group file,
etc.) to a GRASS program, the program searches for the named file under
each of the mapsets listed in the user's mapset search path in the order
listed there until the program finds a file of the given name. Users can
also specify a file by its mapset, to make explicit the mapset from
which the file is to be drawn; e.g., the command:

```sh
g.copy raster=soils@PERMANENT,my_soils
```

ensures that a new file named `my_soils` is to be a copy of the file
`soils` from the mapset PERMANENT.

In each project there is the special mapset **PERMANENT** included in
the mapset search path, as this mapset typically contains base maps
relevant to many applications. Often, other mapsets which contain sets
of interpreted maps will be likewise included in the user's mapset
search path. Suppose, for example, that the mapset *Soil_Maps* contains
interpreted soils map layers to which the user wants access. The mapset
*Soil_Maps* should then be included in the user's *search path*
variable.

The *mapset search path* is saved as part of the current mapset. When
the user works with that mapset in subsequent GRASS sessions, the
previously saved mapset search path will be used (and will continue to
be used until it is modified by the user with *g.mapsets*).

## NOTES

By default *g.mapsets* adds to the current *mapset search path* mapsets
named by **mapset** option. Alternatively mapsets can be removed
(**operation=remove**) from the search path or defined by
**operation=set**.

Users can restrict others' access to their mapset files through use of
*[g.access](g.access.md)*. Mapsets to which access is restricted can
still be listed in another's mapset search path; however, access to
these mapsets will remain restricted.

## EXAMPLES

### Selecting mapsets with the graphical mapset manager

Using the **-s** flag, a convenient graphical mapset manager can be
opened to select and deselect other mapsets (the actual mapset and the
PERMANENT mapset are always selected):

```sh
g.mapsets -s
```

![g_mapsets_gui](g_mapsets_gui.png)

### Print available mapsets

All available mapsets in the current project can be printed out by

```sh
g.mapsets -l

Available mapsets:
PERMANENT user1 user2
```

Mapsets can be also printed out as json by setting the format option to
"json" (**format="json"**).

```sh
    g.mapsets format="json" -l

    {
      "mapsets": [
        "PERMANENT",
        "user1",
        "user2"
      ]
    }
  
```

### Add new mapset

Add mapset 'user2' to the current mapset search path

```sh
g.mapsets mapset=user2 operation=add
```

The current mapset search path is changed accordingly

```sh
g.mapsets -p

Accessible mapsets:
user1 user2
```

### Overwrite current search path

Overwrite current search path

```sh
g.mapsets mapset=user1,PERMANENT operation=set
```

### Using shortcuts for search path

The current mapset can be defined by a shortcut "." (dot)

```sh
g.mapsets mapset=.,PERMANENT operation=set
```

*Note:* The current mapset will be always included in the search path on
the first position even if you change its position or omit the current
mapset from the **mapset** option.

```sh
g.mapsets -p

Accessible mapsets:
user1 PERMANENT
```

## SEE ALSO

*[g.access](g.access.md), [g.copy](g.copy.md), [g.gisenv](g.gisenv.md),
[g.list](g.list.md), [g.mapset](g.mapset.md)*

## AUTHORS

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory  
Greg Koerper, ManTech Environmental Technology, Inc.  
Updated to GRASS 7 by Martin Landa, Czech Technical University in
Prague, Czech Republic
