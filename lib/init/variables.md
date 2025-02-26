---
description: GRASS variables and environment variables
---

# GRASS variables and environment variables

A variable in scripting is a symbolic name that holds data which can be
used and modified during script execution. Variables allow scripts to
store and manipulate values dynamically, making them more flexible and
reusable. In GRASS GIS, there are two types of variables:

- [shell environment](#setting-shell-environment-variables) variables,
- [GRASS gisenv](#setting-grass-gisenv-variables) variables.

There are a number of *shell* environment variable groups:

- [variables for
  rendering](#list-of-selected-grass-environment-variables-for-rendering)
- [variables for internal
  use](#list-of-selected-internal-grass-environment-variables)

*Note:* Any setting which needs to be modifiable by a GRASS module (e.g.
MONITOR by *[d.mon](d.mon.md)*) has to be a GRASS gisenv variable.

## Setting shell environment variables

Setting shell environment variables depends on the shell being used:  
  
Bash:

```sh
export VARIABLE=value
```

Csh:

```sh
setenv VARIABLE value
```

Cmd.exe (Windows):

```sh
set VARIABLE=value
```

To set up shell environment variables permanently:

- To get personal BASH shell definitions (aliases, color listing option,
  ...) into GRASS, store them in:  
  `$HOME/.grass8/bashrc`
- To get personal CSH shell definitions (aliases, color listing option,
  ...) into GRASS, store them in:  
  `$HOME/.grass8/cshrc`

## Setting GRASS gisenv variables

Use *[g.gisenv](g.gisenv.md)* within GRASS. This permanently predefines
GRASS variables in the `$HOME/.grass8/rc` file (Linux, Mac, BSD, ...) or
in the `%APPDATA%\Roaming\GRASS8\rc` file (Windows) after the current
GRASS session is closed.  
  
Usage:

```sh
g.gisenv set="VARIABLE=VALUE"
```

It looks unusual with two equals signs, but *g.gisenv* serves dual duty
for getting and setting GRASS variables.

If the user just specifies a variable name, it defaults to **get** mode.
For example:

```sh
g.gisenv MAPSET
PERMANENT
```

## List of selected (GRASS related) shell environment variables

> \[ To be set from the terminal shell or startup scripts \]

GISBASE  
directory where GRASS lives. This is set automatically by the startup
script.

GISRC  
name of `$HOME/.grass8/rc` file. Defines the system wide value when
starting a GRASS session. Within a GRASS session, a temporary copy of
this file will be used.

GRASS_ADDON_PATH  
\[grass startup script, g.extension\]  
specifies additional path(s) containing local and/or custom GRASS
modules extra to the standard distribution.

GRASS_ADDON_BASE  
\[grass startup script\]  
allows specifying additional GISBASE for local GRASS modules (normally
installed as GRASS Addons by `g.extension` module) extra to standard
distribution. The default on GNU/Linux is `$HOME/.grass8/addons`, on MS
Windows `%APPDATA%\Roaming\GRASS8\addons`.

GRASS_ADDON_ETC  
\[libgis, g.findetc\]  
specify paths where support files (etc/) may be found external to
standard distribution.

GRASS_COMPATIBILITY_TEST  
\[libgis\]  
By default it is not possible to run C modules with a libgis that has a
different `GIS_H_VERSION`, the compatibility test will exit with a fatal
error. Setting this variable to 0 (zero) with
`GRASS_COMPATIBILITY_TEST=0` allows the test to be passed with a
warning.

GRASS_COMPRESSOR  
\[libraster\]  
the compression method for new raster maps can be set with the
environment variable GRASS_COMPRESSOR. Supported methods are RLE, ZLIB,
LZ4, BZIP2, and ZSTD. The default is ZSTD if available, otherwise ZLIB,
which can be changed with e.g. `GRASS_COMPRESSOR=ZSTD`, granted that
GRASS has been compiled with the requested compressor. Compressors that
are always available are RLE, ZLIB, and LZ4. The compressors BZIP2 and
ZSTD must be enabled when configuring GRASS for compilation.

GRASS_CONFIG_DIR  
\[grass startup script\]  
specifies root path for GRASS configuration directory. If not specified,
the default placement of the configuration directory is used: `$HOME` on
GNU/Linux, `$HOME/Library` on Mac OS X, and `%APPDATA%` on MS Windows.

GRASS_DB_ENCODING  
\[various modules, wxGUI\]  
encoding for vector attribute data (utf-8, ascii, iso8859-1, koi8-r)

GIS_ERROR_LOG  
If set, GIS_ERROR_LOG should be the absolute path to the log file (a
relative path will be interpreted relative to the process' cwd, not the
cwd at the point the user set the variable). If not set,
`$HOME/GIS_ERROR_LOG` is used instead. The file will only be used if it
already exists.

GRASS_ERROR_MAIL  
set to any value to send user mail on an error or warning that happens
while stderr is being redirected.

GRASS_FONT  
\[display drivers\]  
specifies the font as either the name of a font from
`$GISBASE/etc/fontcap` (or alternative fontcap file specified by
GRASS_FONT_CAP), or alternatively the full path to a FreeType font file.

GRASS_ENCODING  
\[display drivers\]  
the encoding to be assumed for text which is drawn using a freetype
font; may be any encoding know to *iconv*.

GRASS_FONT_CAP  
\[g.mkfontcap, d.font, display drivers\]  
specifies an alternative location (to `$GISBASE/etc/fontcap`) for the
font configuration file.

GRASS_FULL_OPTION_NAMES  
\[parser\]  
Generates a warning if GRASS_FULL_OPTION_NAMES is set (to anything) and
a found string is not an exact match for the given string.

GRASS_GUI  
either `text` (text user interface), `gtext` (text user interface with
GUI welcome screen), or `gui` (graphical user interface) to define
non-/graphical startup. Can also specify the name of the GUI to use,
e.g. `wxpython` (*[wxGUI](wxGUI.md)*). Also exists as a GRASS gisenv
variable (see below). If this shell variable exists at GRASS startup, it
will determine the GUI used. If it is not defined startup will default
to the last GUI used.

GRASS_HTML_BROWSER  
\[init.sh, wxgui\]  
defines name of HTML browser. For most platforms this should be an
executable in your PATH, or the full path to an executable.  
Mac OS X runs applications differently from the CLI. Therefore,
GRASS_HTML_BROWSER should be the application's signature, which is a
domain-like name, just reversed, i.e. com.apple.Safari. To find an
application's signature, type the following in a Terminal (fill in the
path to the application you are interested in, for example:
/Applications/Safari.app):  
    `grep -A 1 "CFBundleIdentifier"`
*/path/to/application.app*`/Contents/Info.plist`  
  The signature is the \<string\> following the \<key\>, without the
bracketing \<string\> tags.

GRASS_INT_ZLIB  
\[libraster\]  
if the environment variable GRASS_INT_ZLIB exists and has the value 0,
new compressed *integer* (CELL type) raster maps will be compressed
using RLE compression.  
  
If the variable doesn't exist, or the value is non-zero, zlib
compression will be used instead. Such rasters will have a `compressed`
value of 2 in the cellhd file.  
  
Obviously, decompression is controlled by the raster's `compressed`
value, not the environment variable.

GRASS_ZLIB_LEVEL  
\[libgis\]  
if the environment variable GRASS_ZLIB_LEVEL exists and its value can be
parsed as an integer, it determines the compression level used when new
compressed raster maps are compressed using zlib compression. This
applies to all raster map types (CELL, FCELL, DCELL).  
  
Valid zlib compression levels are -1 to 9. The `GRASS_ZLIB_LEVEL=-1`
corresponds to the zlib default value (equivalent to
`GRASS_ZLIB_LEVEL=6`). Often `GRASS_ZLIB_LEVEL=1` gives the best
compromise between speed and compression.  
  
If the variable doesn't exist, or the value cannot be parsed as an
integer, zlib's default compression level 6 will be used.

GRASS_MESSAGE_FORMAT  
\[various modules, wxGUI\]  
it may be set to either

- `standard` - sets percentage output and message formatting style to
  standard formatting,
- `gui` - sets percentage output and message formatting style to GUI
  formatting,
- `silent` - disables percentage output and error messages,
- `plain` - sets percentage output and message formatting style to ASCII
  output without rewinding control characters.

GRASS_MOUSE_BUTTON  
\[various modules\]  
swaps mouse buttons for two-button or left-handed mice. Its value has
three digits 1, 2, and 3, which represent default left, middle, and
right buttons respectively. Setting to `132` will swap middle and right
buttons. Note that this variable should be set before a display driver
is initialized (e.g., `d.mon x0`).

GRASS_PAGER  
\[various modules\]  
it may be set to either `less`, `more`, or `cat`.

GRASS_PERL  
\[used during install process for generating man pages\]  
set Perl with path.

GRASS_PROXY  
\[used during addon install/reinstall process for generating man pages
(download commit from GitHub API server and remote modules.xml file)\]  
set the proxy with: `GRASS_PROXY="http=<value>,ftp=<value>"`.

GRASS_SKIP_MAPSET_OWNER_CHECK  
By default it is not possible to work with MAPSETs that are not owned by
current user. Setting this variable to any non-empty value allows the
check to be skipped.

GRASS_SH  
\[shell scripts on Windows\]  
path to bourne shell interpreter used to run shell scripts.

GRASS_SIGSEGV_ON_ERROR  
Raise SIGSEGV if an error occurs\]  
This variable can be set for debugging purpose. The call of
G_fatal_error() will end in a segmentation violation. GDB can be used to
trace the source of the error.

GRASS_PYTHON  
\[wxGUI, Python Ctypes\]  
set to override Python executable.  
On Mac OS X this should be the `pythonw` executable for the wxGUI to
work.

GRASS_VECTOR_LOWMEM  
\[vectorlib\]  
If the environment variable GRASS_VECTOR_LOWMEM exists, memory
consumption will be reduced when building vector topology support
structures. Recommended for creating large vectors.

GRASS_VECTOR_OGR  
\[vectorlib, v.external.out\]  
If the environment variable GRASS_VECTOR_OGR exists and vector output
format defined by *[v.external.out](v.external.out.md)* is PostgreSQL,
vector data is written by OGR data provider even the native PostGIS data
provider is available.

GRASS_VECTOR_EXTERNAL_IMMEDIATE  
\[vectorlib, v.external.out\]  
If the environment variable GRASS_VECTOR_EXTERNAL_IMMEDIATE exists and
vector output format defined by *[v.external.out](v.external.out.md)* is
non-native, vector features are written to output external datasource
immediately. By default, the vector library writes output data to a
temporary vector map in native format and when closing the map, the
features are transferred to output external datasource. Note: if output
vector format is topological PostGIS format, then the vector library
writes features immediately to output database (in this case
GRASS_VECTOR_EXTERNAL_IMMEDIATE is ignored).

GRASS_VECTOR_EXTERNAL_IGNORE  
\[vectorlib\]  
If the environment variable GRASS_VECTOR_EXTERNAL_IGNORE exists, output
vector format defined by *[v.external.out](v.external.out.md)* is
ignored. The format is always native.

GRASS_VECTOR_TEMPORARY  
\[vectorlib\]  
If the environment variable GRASS_VECTOR_TEMPORARY exists, GRASS vector
library will operate on temporary vector maps. New vector maps will be
created in temporary directory (see GRASS_VECTOR_TMPDIR_MAPSET
variable), existing vector maps will be read (if found) also from this
directory. It may be set to either:

- `keep` - the temporary vector map is not deleted when closing the map.
- `move` - the temporary vector map is moved to the current mapset when
  closing the map.
- `delete` - the temporary vector map is deleted when closing the map.

Default value is `keep`. Note that temporary vector maps are not visible
to the user via *[g.list](g.list.md)* or *[wxGUI](wxGUI.md)*. They are
used internally by the GRASS modules and deleted automatically when
GRASS session is quited.

GRASS_VECTOR_TMPDIR_MAPSET  
\[vectorlib\]  
By default GRASS temporary directory is located in
`$LOCATION/$MAPSET/.tmp/$HOSTNAME`. If GRASS_VECTOR_TMPDIR_MAPSET is set
to '0', the temporary directory is located in TMPDIR (environmental
variable defined by the user or GRASS initialization script if not
given).  
Important note: This variable is currently used only in vector library.
In other words the variable is ignored by raster or raster3d library.

GRASS_VECTOR_TOPO_DEBUG  
\[vectorlib, v.generalize\]  
If the environment variable GRASS_VECTOR_TOPO_DEBUG exists,
*[v.generalize](v.generalize.md)* runs in extremely slow debug mode.

GRASS_WXBUNDLED  
\[wxGUI\]  
set to tell wxGUI that a bundled wxPython will be used.  
When set, the wxGUI will not check the wxPython version, as this
function is incompatible with a bundled wxPython. It is up to the
packager to make sure that a compatible wxPython version is bundled.

GRASS_WXVERSION  
\[wxGUI\]  
set to tell wxGUI which version of wxPython to use.  
When set, the wxGUI will select the given wxPython version. It's useful
when multiple versions of wxPython are installed and the user wants to
run wxGUI with non-default wxPython version.

GRASS_XTERM  
\[lib/init/grass-xterm-wrapper, lib/init/grass-xterm-mac\]  
set to any value (e.g. rxvt, aterm, gnome-terminal, konsole) to
substitute 'x-terminal-emulator' or 'xterm'. The Mac OS X app startup
defaults to an internal '\$GISBASE/etc/grass-xterm-mac', which emulates
the necessary xterm functionality in Terminal.app.

GRASS_UI_TERM  
set to any value to use the terminal based parser.

GRASS_VERSION  
reports the current version number (used by R-stats interface etc);
should not be changed by user.

GRASS_NO_GLX_PBUFFERS  
\[nviz\]  
set to any value to disable the use of GLX Pbuffers.

GRASS_NO_GLX_PIXMAPS  
\[nviz\]  
Set to any value to disable the use of GLX Pixmaps.

OMP_NUM_THREADS  
\[OpenMP\]  
If OpenMP support is enabled this limits the number of threads. The
default is set to the number of CPUs on the system. Setting to '1'
effectively disables parallel processing.

TMPDIR, TEMP, TMP  
\[Various GRASS GIS commands and wxGUI\]  
The default wxGUI temporary directory is chosen from a
platform-dependent list, but the user can control the selection of this
directory by setting one of the TMPDIR, TEMP or TMP environment
variables Hence the wxGUI uses $TMPDIR if it is set, then $TEMP,
otherwise /tmp.

### List of selected GRASS environment variables for rendering

> \[ In addition to those which are understood by specific *[GRASS
> display drivers](displaydrivers.md)*, the following variables affect
> rendering. \]

GRASS_RENDER_IMMEDIATE  
tells the display library which driver to use; possible values:
*[cairo](cairodriver.md)*, *[png](pngdriver.md)*, *[ps](psdriver.md)*,
*[html](htmldriver.md)* or *default*.  
Default display driver is *[cairo](cairodriver.md)* (if available)
otherwise *[png](pngdriver.md)*.

GRASS_RENDER_WIDTH  
defines the width of output image (default is 640).

GRASS_RENDER_HEIGHT  
defines the height of output image (default is 480).

GRASS_RENDER_FILE  
the name of the resulting image file.

GRASS_RENDER_FRAME  
contains 4 coordinates, *top,bottom,left,right* (pixel values) with
respect to the top left corner of the output image, defining the initial
frame.

GRASS_RENDER_LINE_WIDTH  
defines default line width.

GRASS_RENDER_TEXT_SIZE  
defines default text size.

GRASS_RENDER_COMMAND  
external command called by display library to render data (see example
in *[display drivers](displaydrivers.md)* page for details).  
Currently only Python scripts are supported.

For specific driver-related variables see:

- *[Cairo display driver](cairodriver.md)*
- *[PNG display driver](pngdriver.md)*
- *[PS (Postscript) display driver](psdriver.md)*
- *[HTML display driver](htmldriver.md)*

### List of selected internal GRASS environment variables

> \[ These variables are intended **for internal use only** by the GRASS
> software to facilitate communication between the GIS engine, GRASS
> scripts, and the GUI. The user should not set these in a GRASS
> session. They are meant to be set locally for specific commands. \]

GRASS_OVERWRITE  
\[all modules\]  
toggles map overwrite.

- 0 - maps are protected (default),
- 1 - maps with identical names will be overwritten.

This variable is automatically created by *[g.parser](g.parser.md)* so
that the `--overwrite` option will be inherited by dependent modules as
the script runs. Setting either the GRASS_OVERWRITE environment variable
or the OVERWRITE gisenv variable detailed below will cause maps with
identical names to be overwritten.

GRASS_VERBOSE  
\[all modules\]  
toggles verbosity level

- -1 - complete silence (also errors and warnings are discarded)
- 0 - only errors and warnings are printed
- 1 - progress and important messages are printed (percent complete)
- 2 - all module messages are printed
- 3 - additional verbose messages are printed

This variable is automatically created by *[g.parser](g.parser.md)* so
that the `--verbose` or `--quiet` flags will be inherited by dependent
modules as the script runs.

GRASS_REGION  
\[libgis\]  
override region settings, separate parameters with a ";". Format is the
same as in the WIND region settings file. Otherwise use is the same as
WIND_OVERRIDE.

WIND_OVERRIDE  
\[libgis\]  
it causes programs to use the specified named region (created with e.g.
`g.region save=...`) to be used as the current region, instead of the
region from the WIND file.  
  
This allows programs such as the GUI to run external commands on an
alternate region without having to modify the WIND file then change it
back afterwards.

GRASS_MASK  
\[libgis\]  
use the raster map specified by name as mask, instead of a raster called
MASK in the current mapset.

## List of selected GRASS gisenv variables

> \[ Use *[g.gisenv](g.gisenv.md)* to get/set/unset/change them \]

DEBUG  
\[entire GRASS\]  
sets level of debug message output (0: no debug messages)

```sh
g.gisenv set=DEBUG=0
```

WX_DEBUG  
\[wxGUI\]  
sets level of debug message output for *[wxGUI](wxGUI.md)* (0: no debug
messages, 1-5 debug levels)

GISDBASE  
initial database

GIS_LOCK  
lock ID to prevent parallel GRASS use,  
process id of the start-up shell script

GUI  
See `GRASS_GUI` environmental variable for details.

LOCATION  
full path to project (previously called location) directory

LOCATION_NAME  
initial project name

MAPSET  
initial mapset

MEMORYMB  
\[entire GRASS with focus on raster related data processing\]  
sets the maximum memory to be used (in MB), i.e. the cache size for
raster rows

```sh
# set to 6 GB (default: 300 MB)
g.gisenv set="MEMORYMB=6000"
```

NPROCS  
sets the number of threads for parallel computing

```sh
# set to use 12 threads (default: 1)
g.gisenv set="NPROCS=12"
```

OVERWRITE  
\[all modules\]  
toggles map overwrite.

- 0 - maps are protected (default),
- 1 - maps with identical names will be overwritten.

This variable is automatically created by *[g.parser](g.parser.md)* so
that the `--overwrite` option will be inherited by dependent modules as
the script runs. Setting either the GRASS_OVERWRITE environment variable
or the OVERWRITE gisenv variable detailed below will cause maps with
identical names to be overwritten.

## GRASS-related Files

`$HOME/.grass8/rc`  
stores the GRASS gisenv variables (not shell environment variables)

`$HOME/.grass8/bashrc`  
stores the shell environment variables (Bash only)

`$HOME/.grass8/env.bat`  
stores the shell environment variables (MS Windows only)

`$HOME/.grass8/login`  
stores the DBMI passwords in this hidden file. Only the file owner can
access this file.

`$HOME/GIS_ERROR_LOG`  
if this file exists then all GRASS error and warning messages are logged
here. Applies to current user. To generate the file, use:
`touch $HOME/GIS_ERROR_LOG`  
See also GIS_ERROR_LOG variable.

Note: On MS Windows the files are stored in `%APPDATA%`.

## SEE ALSO

*[g.gisenv](g.gisenv.md), [g.parser](g.parser.md)*
