## DESCRIPTION

*g.gisenv* outputs and modifies the user's current GRASS GIS variable
settings. When a user runs GRASS, certain variables are set specifying
the GRASS data base, project, mapset, peripheral device drivers, etc.,
being used in the current GRASS session. These variable name settings
are recognized as long as the user is running a GRASS session.

## OPTIONS

No prompts are given to the user when running *g.gisenv*.

If run without arguments, *g.gisenv* lists all of the user's current
GRASS variable settings. Results are sent to standard output, and may
look like this:

```sh
GISDBASE=/opt/grassdata/
LOCATION_NAME=nc_spm_08_grass7
MAPSET=/user1
GUI=gui
```

If the user specifies a **get**=*variable_name* on the command line

```sh
g.gisenv MAPSET
```

only the value for that particular GRASS variable is output to standard
output. Possible variable names depend on the user's system, see
[variables list](variables.md) for details. Note that the variable names
are case-insensitive.

While other variables may be associated with each GRASS session (e.g.,
GRASS_GUI, GIS_LOCK, and other variables), those stated below are
essential.

### GISDBASE

The *GISDBASE* is a directory in which all users' GRASS data are stored.
Within the *GISDBASE*, data are segregated into subdirectories (called
"projects") based on the map coordinate system used and the geographic
extent of the data. Each "project" directory itself contains
subdirectories called "mapsets"; each "mapset" stores "data base
elements" - the directories (e.g., the `cell`, `cellhd`, `vector`, etc.,
directories) in which GRASS data files are actually stored.

### LOCATION_NAME

The user must choose to work with the data under a single GRASS project
(previously called "location") within any given GRASS session; this
project is then called the *current GRASS project*, and is specified by
the variable *LOCATION_NAME*. The *LOCATION_NAME* is the GRASS data base
location whose data will be affected by any GRASS commands issued during
the user's current GRASS session, and is a subdirectory of the current
*GISDBASE*. Each "project" directory can contain multiple "mapset"
directories (including the special mapset *PERMANENT*). Maps stored
under the same GRASS *LOCATION_NAME* (and/or within the same *MAPSET*)
must use the same coordinate system and typically fall within the
boundaries of the same geographic region.

### MAPSET

Each "mapset" contains a set of maps relevant to the *LOCATION_NAME*
directory in which it appears. Each *LOCATION_NAME* can contain multiple
mapsets. (Mapsets which fall under the same *LOCATION_NAME* all contain
data geographically relevant to the *LOCATION_NAME*, and all store data
in the same map coordinate system. Frequently, maps are placed into
different mapsets to distinguish file ownership - e.g., each user might
have one or more own mapset(s), storing any maps that the user has
created and/or are relevant to the own work.) During each GRASS session,
the user must choose one mapset to be the *current mapset*; the current
mapset setting is given by *MAPSET*, and is a subdirectory of
*LOCATION_NAME*. During a single GRASS session, the user can use
available data in any of the mapsets stored under the current
*LOCATION_NAME* directory that are in the user's mapset search path and
accessible by the user. However, within a single GRASS session, the user
only has *write* access to data stored under the *current mapset*
(specified by the variable *MAPSET*).

Each "mapset" stores GRASS data base elements (i.e., the directories in
which GRASS data files are stored). Any maps created or modified by the
user in the current GRASS session will be stored here. The *MAPSET*
directory *PERMANENT* is generally reserved for the set of maps that
form the base set for all users working under each *LOCATION_NAME*.

Once within a GRASS session, GRASS users have access only to the data
under a single GRASS data base directory (the *current GRASS data base*,
specified by the variable *GISDBASE*), and to a single GRASS project
directory (the *current project*, specified by the variable
*LOCATION_NAME*). Within a single session, the user may only *modify*
the data in the *current mapset* (specified by the variable *MAPSET*),
but may *use* data available under other mapsets under the same
*LOCATION_NAME*.

All of these names must be legal names on the user's current system.

The full path to the current mapset is determined from *GISDBASE*,
*LOCATION_NAME*, *MAPSET* variables, in the example above:
`/opt/grassdata/spearfish/PERMANENT`. The full path can be printed using
*g.gisenv* by providing multiple variables:

```sh
g.gisenv get=GISDBASE,LOCATION_NAME,MAPSET sep='/'
/opt/grassdata/nc_spm_08_grass7/user1
```

## NOTES

The output from *g.gisenv* when invoked without arguments is directly
usable by Bash. The following command will cast each variable into the
UNIX environment:

```sh
eval `g.gisenv`
```

This works only for *Bash*, *sh*, *ksh*, etc. The format of the output
is not compatible with some other UNIX shells.

By default the GRASS variables are stored in *gisrc* file (defined by
environmental variable *GISRC*). If **store=mapset** is given then the
variables are stored in `<gisdbase>/<project>/<mapset>/VAR` after the
current GRASS session is closed.

## EXAMPLES

### Cache for raster operations

The maximum memory to be used, i.e. the cache size for raster rows, is
set to 300 MB by default (GRASS variable *MEMORYMB*). To speed up raster
operations, it is recommended to increase this setting if enough RAM is
available. It is important to note that parallel processes will each
consume this amount of RAM. Set the maximum memory to be used (in MB),
i.e. the cache size for raster rows:

```sh
# set to 6 GB (default: 300 MB)
g.gisenv set="MEMORYMB=6000"
```

### Number of threads for parallel computing

Set the number of threads for parallel computing:

```sh
# set to use 12 threads (default: 1)
g.gisenv set="NPROCS=12"
```

### GRASS Debugging

To print debugging messages, the variable *DEBUG* must be set to level
equal or greater than 0:

```sh
g.gisenv set="DEBUG=3"
```

Levels: (recommended levels)

- 0 - silence
- 1 - message is printed once or few times per module
- 3 - each row (raster) or line (vector)
- 5 - each cell (raster) or point (vector)

To disable debugging messages:

```sh
g.gisenv unset="DEBUG"
```

The variable DEBUG controls debugging messages from GRASS libraries and
modules.

Similarly *WX_DEBUG* controls debugging messages from [wxGUI](wxGUI.md).

## SEE ALSO

*[g.access](g.access.md), [g.filename](g.filename.md),
[g.findfile](g.findfile.md), [g.mapsets](g.mapsets.md)*

See also [list of selected GRASS gisenv
variables](variables.md#list-of-selected-grass-gisenv-variables)

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
