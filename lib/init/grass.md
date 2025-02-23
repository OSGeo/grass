---
description: GRASS startup program
---

# GRASS startup program

## SYNOPSIS

**grass** \[**-h** \| **-help** \| **--help**\] \[**-v** \|
**--version**\] \| \[**-c** \| **-c geofile** \| **-c
EPSG:code\[:datum_trans\]**\] \| **-e** \| **-f** \| \[**--text** \|
**--gtext** \| **--gui**\] \| **--config** \| \[**--tmp-project** \|
**--tmp-mapset**\] \[\[\[**\<GISDBASE\>/**\]**\<PROJECT\>/**\]
**\<MAPSET\>**\] \[**--exec EXECUTABLE**\]

### Flags

**-h** \| **-help** \| **--help**  
Prints a brief usage message and exits

**-v** \| **--version**  
Prints the version of GRASS and exits

**-c XY**  
Creates new GRASS project (location) without coordinate reference system
in specified GISDBASE

**-c geofile**  
Creates new GRASS project in specified GISDBASE with coordinate
reference system based on georeferenced file

**-c EPSG:code**  
Creates new GRASS project in specified GISDBASE with coordinate
reference system defined by EPSG code

**-c EPSG:code:datum_trans**  
Creates new GRASS project in specified GISDBASE with coordinate
reference system defined by EPSG code and datum transform parameters

**-e**  
Exit after creation of project or mapset. Only with **-c** flag

**-f**  
Forces removal of .gislock if exists (use with care!). Only with --text
flag

**--text**  
Indicates that Text-based User Interface should be used (skip welcome
screen)

**--gtext**  
Indicates that Text-based User Interface should be used (show welcome
screen)

**--gui**  
Indicates that Graphical User Interface (*[wxGUI](wxGUI.md)*) should be
used

**--config**  
Prints GRASS configuration parameters (options: arch, build, compiler,
date, path, python_path, revision, svn_revision, version)

**--exec EXECUTABLE**  
Execute GRASS module or script. The provided executable will be executed
in a GRASS GIS non-interactive session.

**--tmp-project**  
Run using a temporary project which is created based on the given
coordinate reference system and deleted at the end of the execution (use
with the --exec flag). The active mapset will be the PERMANENT mapset.

**--tmp-mapset**  
Run using a temporary mapset which is created in the specified project
and deleted at the end of the execution (use with the --exec flag).

### Parameters

**GISDBASE**  
Initial database directory which should be a fully qualified path (e.g.,
`/usr/local/share/grassdata`)

**PROJECT**  
Initial project directory which is a subdirectory of GISDBASE (project
was previously called location)

**MAPSET**  
Initial mapset directory which is a subdirectory of PROJECT

*Note*: These parameters must be specified in one of the following ways:

```sh
    MAPSET
    PROJECT/MAPSET
    GISDBASE/PROJECT/MAPSET
```

## DESCRIPTION

This command is used to launch GRASS GIS. It will parse the command line
arguments and then initialize GRASS for the user. Since GRASS modules
require a specific environment, this program must be called before any
other GRASS module can run. The command line arguments are optional and
provide the user with a method to indicate the desired user interface,
as well as the desired mapset to work on.

The startup program will remember both the desired user interface and
mapset. Thus, the next time the user runs GRASS, typing *grass* (without
any options) will start GRASS with the previous settings for the user
interface and mapset selected.

If you specify a graphical user interface (**--gui**) the *grass*
program will try to verify that the system you specified exists and that
you can access it successfully. If any of these checks fail then *grass*
will automatically switch back to the text user interface mode.

### Running non-interactive jobs

The **--exec** flag can run an executable on path, GRASS module, or a
script. All are executed as a subprocess and any additional arguments
are passed to it. A script needs to be specified by full or relative
path and on unix-likes systems, the script file must have its executable
bit set. Calling the interpreter (e.g., `python`) and providing the
script as a parameter is possible, too. When it is finished GRASS will
automatically exit using the return code given by the subprocess.
Although the execution itself is non-interactive (no GUI or shell), the
subprocess itself can be interactive if that is what the user requires.

### Config flag

The flag **--config option** prints GRASS GIS configuration and version
parameters, with the options:

- **arch**: system architecture (e.g., `x86_64-pc-linux-gnu`)
- **build**: (e.g.,
  `./configure --with-cxx --enable-largefile --with-proj [...]`)
- **compiler**: (e.g., `gcc`)
- **date**: (e.g., `2024-04-10T11:44:54+00:00`)
- **path**: (e.g., `/usr/lib64/grass`)
- **python_path**: (e.g., `/usr/lib64/grass/etc/python`)
- **revision**: (e.g., `745ee7ec9`)
- **svn_revision**: (e.g., `062bffc8`)
- **version**: (e.g., `8.4.0`)

## SAMPLE DATA

The GRASS GIS project provides several free sample geospatial datasets
as ready-to-use projects. They are available to download at
<https://grass.osgeo.org/download/sample-data/>. The "North Carolina
data set" is a modern package of geospatial data from North Carolina
(USA), and it includes raster, vector, LiDAR and satellite data. This is
the most extensively used data set in the documentation and the examples
throughout the user manual pages are based upon it.

## ENVIRONMENT VARIABLES

A number of environment variables are available at GRASS startup to
assist with automation and customization. Most users will not need to
bother with these.

In addition to these shell environment variables GRASS maintains a
number of GIS environment variables in the `$HOME/.grass8/rc` file. User
changes to this file will be read during the next startup of GRASS. If
this file becomes corrupted the user may edit it by hand or remove it to
start afresh. See the list of *[implemented GRASS
variables](variables.md)* for more information. The rest of this help
page will only consider shell environment variables.

Note that you will need to set these variables using the appropriate
method required for the UNIX shell that you use (e.g. in a Bash shell
you must `export` the variables for them to propagate).

### User Interface Environment Variable

The *grass* program will check for the existence of an environment
variable called GRASS_GUI which indicates the type of user interface for
GRASS to use. If this variable is not set when *grass* is run, then it
will be created and then saved in the `$HOME/.grass8/rc` file for the
next time GRASS is run. It can be set to `text`, `gtext` or `gui`.

There is an order of precedence in the way *grass* determines the user
interface to use. The following is the hierarchy from highest precedence
to lowest.

1. Command line argument
2. Environment variable GRASS_GUI
3. Value set in `$HOME/.grass8/rc` (GUI)
4. Default value - `gui`

### Python Environment Variables

If you choose to use *[wxGUI](wxGUI.md)* interface, then the
GRASS_PYTHON environment variable can be used to override your system
default `python` command.

Suppose for example your system has Python 3.6 installed and you install
a personal version of the Python 3.8 binaries under `$HOME/bin`. You can
use the above variables to have GRASS use the Python 3.8 binaries
instead.

```sh
   GRASS_PYTHON=python3.8
```

### Addon Path to Extra User Scripts

This environment variable allows the user to extend the GRASS program
search paths to include locally developed/installed GRASS modules or
user scripts.

```sh
   GRASS_ADDON_PATH=/usr/mytools
   GRASS_ADDON_PATH=/usr/mytools:/usr/local/othertools
```

In this example above path(s) would be added to the standard GRASS path
environment.

### Addon Base for Extra Local GRASS Addon Modules

This environment variable allows the user to extend the GRASS program
search paths to include locally installed (see
*[g.extension](g.extension.md)* for details) [GRASS
Addon](https://grasswiki.osgeo.org/wiki/GRASS_AddOns) modules which are
not distributed with the standard GRASS release.

```sh
   GRASS_ADDON_BASE=/usr/grass-addons
```

In this example above path would be added to the standard GRASS path
environment.

If not defined by user, this variable is set by GRASS startup program to
`$HOME/.grass8/addons` on GNU/Linux and
`%APPDATA%\Roaming\GRASS8\addons` on MS Windows.

### HTML Browser Variable

The GRASS_HTML_BROWSER environment variable allows the user to set the
HTML web browser to use for displaying help pages.

## EXAMPLES

The following are some examples of how you could start GRASS

Start GRASS using the default user interface. The user will be prompted
to choose the appropriate project and mapset.

```sh
grass
```

Start GRASS using the graphical user interface. The user will be
prompted to choose the appropriate project and mapset.

```sh
grass --gui
```

Start GRASS using the text-based user interface. Appropriate project and
mapset must be set by environmental variables (see examples below)
otherwise taken from the last GRASS session.

```sh
grass --text
```

Start GRASS using the text-based user interface. The user will be
prompted to choose the appropriate project and mapset.

```sh
grass --gtext
```

Start GRASS using the default user interface and automatically launch
into the given mapset, bypassing the mapset selection menu:

```sh
grass $HOME/grassdata/spearfish70/user1
```

Start GRASS using the graphical user interface and try to obtain the
project and mapset from environment variables:

```sh
grass --gui -
```

Creates a new GRASS project with EPSG code 4326 (latitude-longitude,
WGS84) in the specified GISDBASE:

```sh
grass -c EPSG:4326 $HOME/grassdata/myproject
```

Creates a new GRASS project with EPSG code 5514 (S-JTSK / Krovak East
North - SJTSK) with datum transformation parameters used in Czech
Republic in the specified GISDBASE:

```sh
grass -c EPSG:5514:3 $HOME/grassdata/myproject
```

Creates a new GRASS project from PROJ definition string (here:
[gnomonic](https://proj4.org/operations/projections/gnom.html)) in the
specified GISDBASE:

```sh
grass -c XY $HOME/grassdata/gnomonic --exec g.proj -c proj4='+proj=gnom +lat_0=90 +lon_0=-50'
```

Creates a new GRASS project based on georeferenced Shapefile:

```sh
grass -c myvector.shp $HOME/grassdata/myproject
```

Creates a new GRASS project based on georeferenced GeoTIFF file:

```sh
grass -c myraster.tif $HOME/grassdata/myproject
```

### Batch jobs with the exec interface

Creating a new project based on a geodata file's projection (**-c**) and
exit (**-e**) immediately:

```sh
grass -c elevation.tiff -e /path/to/grassdata/test1/
```

Linking external raster data to PERMANENT Mapset:

```sh
grass /path/to/grassdata/test1/PERMANENT/ --exec r.external input=basins.tiff output=basins
grass /path/to/grassdata/test1/PERMANENT/ --exec r.external input=elevation.tiff output=elevation
```

Get statistics for one raster map:

```sh
grass /path/to/grassdata/test1/PERMANENT/ --exec r.univar map=elevation
```

Compare the rasters visually:

```sh
grass /path/to/grassdata/test1/PERMANENT/ --exec g.gui.mapswipe first=elevation second=basins
```

#### Execution of shell and Python scripts instead of single commands

A sequence of commands can be bundled in a script and executed using the
exec interface.

**Shell script example:** the command to execute a shell script might
be:

```sh
grass /path/to/grassdata/test1/PERMANENT/ --exec sh test.sh
```

A very simple bash script ("test.sh") may look like this:

```sh
#!/bin/bash

g.region -p
g.list type=raster
r.info elevation
```

**Python script example:** the command to execute a Python script might
be:

```sh
grass /path/to/grassdata/test1/PERMANENT/ --exec python test.py
```

A very simple Python script ("test.py") may look like this:

```python
#!/usr/bin/env python3

# import GRASS Python bindings (see also pygrass)
import grass.script as gs

gs.message('Current GRASS GIS environment:')
print(gs.gisenv())

gs.message('Available raster maps:')
for raster in gs.list_strings(type='raster'):
    print(raster)

gs.message('Available vector maps:')
for vector in gs.list_strings(type='vector'):
    print(vector)
```

#### Using temporary project

Creating a new temporary project based on a georeferenced file's
coordinate reference system (CRS) and simultaneously starting
computation in a shell script:

```sh
grass --tmp-project elevation.tiff --exec test.sh
```

The same, but using an EPSG code and a Python script:

```sh
grass --tmp-project EPSG:3358 --exec test.py
```

Finally, for special cases, we can create an XY project without any CRS:

```sh
grass --tmp-project XY --exec test.py
```

Temporary project is automatically deleted after computation, so the
script is expected to export, link or otherwise preserve the output data
before ending.

A single command can be also executed, e.g. to examine properties of the
temporary project:

```sh
grass --tmp-project EPSG:3358 --exec g.proj -p
```

A temporary XY project with single command is useful, e.g. to show help
text of a module:

```sh
grass --tmp-project XY --exec r.neighbors --help
```

#### Using temporary mapset

A single command can be executed, e.g., to examine properties of a
project (here using the NC SPM sample dataset):

```sh
grass --tmp-mapset /path/to/grassdata/nc_spm_08/ --exec g.proj -p
```

Computation in a Python script can be executed in the same way:

```sh
grass --tmp-mapset /path/to/grassdata/nc_spm_08/ --exec processing.py
```

Additional parameters are just passed to the script, so we can run the
script with different sets of parameters (here 5, 8 and 3, 9) in
different temporary mapsets which is good for parallel processing.

```sh
grass --tmp-mapset /path/to/grassdata/nc_spm_08/ --exec processing.py 5 8
grass --tmp-mapset /path/to/grassdata/nc_spm_08/ --exec processing.py 3 9
```

The same applies to Bash scripts (and other scripts supported on you
platform):

```sh
grass --tmp-mapset /path/to/grassdata/nc_spm_08/ --exec processing.sh 5 8
```

The temporary mapset is automatically deleted after computation, so the
script is expected to export, link or otherwise preserve the output data
before ending.

#### Troubleshooting

Importantly, to avoid an `"[Errno 8] Exec format error"` there must be a
[shebang](https://en.wikipedia.org/wiki/Shebang_%28Unix%29) line at the
top of the script (like `#!/bin/sh`, `#!/bin/bash`, or
`#!/usr/bin/env python3`) indicating which interpreter to be used for
the script. The script file must have its executable bit set.

## CAVEAT

If you start GRASS using the *[wxGUI](wxGUI.md)* interface you must have
a `python` command in your $PATH variable. That is, the command must be
named `python` and not something like `python3.6`. Rarely some Python
installations do not create a `python` command. In these cases you can
override `python` by GRASS_PYTHON environmental variable.

Furthermore, if you have more than one version of Python installed, make
sure that the version you want to use with GRASS is set by GRASS_PYTHON
environmental variable.

## SEE ALSO

List of [GRASS environment variables](variables.md)

[GRASS GIS Web site](https://grass.osgeo.org)  
[GRASS GIS User Wiki](https://grasswiki.osgeo.org/wiki/)  
[GRASS GIS Bug Tracker](https://github.com/OSGeo/grass/issues)  
[GRASS GIS 8 Programmer's Manual](https://grass.osgeo.org/programming8/)

## AUTHORS (of this page)

Justin Hickey  
Markus Neteler  
Hamish Bowman  
Martin Landa, Czech Technical University in Prague, Czech Republic
