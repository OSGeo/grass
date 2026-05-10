## DESCRIPTION

*g.extension* downloads and installs, removes or updates extensions
(addons) from the official [GRASS Addons
repository](https://grass.osgeo.org/grass8/manuals/addons/) or from
user-specified source code repositories into the local GRASS
installation.

Two types of extensions are supported:

- Python scripts: they are installed without the need of compilation or
  (usually) the need of special dependencies.
- Source code (mostly written in C programming language; may also be
  written in C++, Fortran or other languages): while on MS-Windows
  systems the requested GRASS extension is downloaded pre-compiled
  from the GRASS site, on Unix based systems the installation is
  preceded by the automated download of the extension's source code
  along with subsequent compilation and installation. This requires a
  compiler environment to be present on the user's computer.

### Managing installed extensions

Re-running *g.extension* on an installed GRASS Addon extension
re-installs the requested extension which may include updates.

To bulk-update all locally installed GRASS extensions,
*[g.extension.all](g.extension.all.md)* module is available.

### Where the extensions are installed

GRASS extensions are installed by *g.extension* into a dedicated
directory. The default is a directory for application data and settings
inside the user's home directory. On GNU/Linux it is
`$HOME/.grass8/addons`, on MS-Windows it is
`%APPDATA%\Roaming\GRASS8\addons`. The name of the directory is stored
in the `GRASS_ADDON_BASE` environmental variable.

The flag **-s** changes this install target directory to the GRASS
installation directory (determined by `GISBASE` environmental variable,
e.g. `/usr/`) rather than the default directory defined as per
`GRASS_ADDON_BASE` (see also documentation for
[variables](variables.md)). *g.extension* checks if the user has
permission to write to `GISBASE` or `GRASS_ADDON_BASE`.

The place where the extensions are installed can be customized by the
option **prefix**. Ensuring that these extensions will be accessible in
GRASS is in this case in the responsibility of the user.

### Source code sources and repositories

#### GRASS Addons repository on GitHub

By default, *g.extension* installs extensions from the official GRASS
Addons GitHub repository. However, different sources can be
specified using the **url** option.

Individual extensions can also be installed by providing a URL to the
source code on GitHub or OSGeo Trac. The latter, however, works only for
certain directories where the download of ZIP files was enabled by
project administrators of the trac server.

#### Local source code directory

Optionally, new extension can be also installed from a source code
placed in a local directory on disk. This is advantageous when
developing a new module. To keep the directory clean, the directory
content is copied to a temporary directory and the compilation happens
there.

#### Source code in a ZIP or TAR archive

In addition, new extension can be also installed from a ZIP file or an
archive file from the TAR family (e.g., `.tar.gz` or `.bz2`). The file
can be on disk (specified with a path), or on the web (specified by an
URL).

#### Online repositories: GitHub, GitLab and Bitbucket

For well known general hosting services, namely GitHub, GitLab and
Bitbucket, *g.extension* supports the download of a repository. Here the
user only needs to provide a base URL to the repository web page (with
or without the `https://` part). For GitHub, GitLab and Bitbucket, the
latest source code in the default branch is downloaded, unless a
specific branch is requested in the *branch* option. Of course, a user
can still specify the full URL of a ZIP file e.g. for a specific release
and install the archived code in this way (ZIP file mechanism will be
applied).

For the official repository, *g.extension* supports listing available
extensions (addons) and few other metadata-related operations which
depend on a specific infrastructure. For other sources and repositories,
this is not supported because it is assumed that other sources contain
only one extension, typically a module or group of modules with a
Makefile at the root of the repository.

#### Needed directory layout

When none of the above sources is identified, *g.extension* assumes that
the source is in a GitHub repository and uses the *git* command line
tool to obtain the source code. The expected structure of the repository
should be the same as the one of the official repository.

Non-official sources are supported on all operating systems except for
MS-Windows.

### Compilation and installation

On MS-Windows systems, where compilation tools are typically not readily
locally installed, *g.extension* downloads a precompiled executable from
the GRASS project server. On all other operating systems where it is
not difficult to install compilation tools, *g.extension* downloads the
source code of the requested extension (addon) and compiles it locally.
This applies for both C and Python modules as well as any other
extensions. The reason is that more things such as manual page are
compiled, not only the source code (which is really necessary to compile
just in case of C).

### Using an alternative addon server (MS-Windows)

On MS-Windows, *g.extension* downloads precompiled addon ZIPs from the
official WinGRASS server (`http://wingrass.fsv.cvut.cz`) by default.
The base URL of the server can be redirected to a mirror or a private
addon server by setting `ADDONS_BASE_URL`. The setting affects both the
addon listing (`g.extension -l`, `-c`, `-g`) and the install download
URL, so listing and install always come from the same place.

The setting can be applied in two ways:

- From inside GRASS (persists across sessions, settable from the GUI's
  variables panel as well):

  ```sh
  g.gisenv set="ADDONS_BASE_URL=https://your.server.example"
  ```

- As a shell environment variable named `GRASS_ADDONS_BASE_URL` (useful
  for one-off shell sessions or CI jobs). The gisenv setting takes
  precedence over the environment variable; if neither is set, the
  default WinGRASS server is used.

To revert to the default server, unset the value:

```sh
g.gisenv unset=ADDONS_BASE_URL
```

Alternative servers are:

* <https://ecodiv.earth/share> (Python extensions only)


Note, settings explained in this section are is honoured on MS-Windows only; on
Linux/macOS it has no effect.

## EXAMPLES

### Download and install of an extension

Download and install *r.stream.distance* into current GRASS installation

```sh
g.extension extension=r.stream.distance
```

This installs the extension from the official repository. For
convenience, a shorter syntax can be used:

```sh
g.extension r.stream.distance
```

### Installing from an alternative addon server (MS-Windows)

On MS-Windows, redirect the addon server to a mirror or private host
once per session, then use *g.extension* normally — listing and install
both go through the configured server.

```sh
# Point at the alternative server (persists across GRASS sessions)
g.gisenv set="ADDONS_BASE_URL=https://ecodiv.earth/share"

# List what the alternative server provides
g.extension -l

# Install an extension from it
g.extension extension=r.tpi

# Revert to the default WinGRASS server
g.gisenv unset=ADDONS_BASE_URL
```

For a one-off shell session, set the environment variable instead:

```sh
# cmd.exe
set GRASS_ADDONS_BASE_URL=https://ecodiv.earth/share
g.extension extension=r.tpi
```

### Download and install of an extension when behind a proxy

Example for an open http proxy:

```sh
# syntax: http://proxyurl:proxyport
g.extension extension=r.stream.distance proxy="http=http://proxy.example.com:8080"
```

Example for a proxy with proxy authentication:

```sh
# syntax: http://username:password@proxyurl:proxyport
g.extension extension=r.stream.distance proxy="http=http://username:password@proxy.example.com:8080"
```

### Managing the extensions

List all available extensions in the official GRASS Addons
repository:

```sh
g.extension -l
```

List all locally installed extensions:

```sh
g.extension -a
```

Removal of a locally installed extension:

```sh
g.extension extension=r.stream.distance operation=remove
```

### Installing from various online repositories: GitHub, GitLab, Bitbucket

Simple URL to GitHub, GitLab, Bitbucket repositories:

```sh
g.extension r.example.plus url="https://github.com/wenzeslaus/r.example.plus"
```

Simple URL to GitHub, GitLab, Bitbucket repositories from a specific
(e.g. development) branch:

```sh
g.extension r.example.plus url="https://github.com/wenzeslaus/r.example.plus" branch=master
```

Simple URL to OSGeo Trac (downloads a ZIP file, requires download to be
enabled in Trac):

```sh
g.extension r.example url=trac.osgeo.org/.../r.example
```

In general, when a ZIP file or other archive is provided, the full URL
can be used:

```sh
g.extension r.example url=http://example.com/.../r.example?format=zip
```

On Windows, this will work also, but only when pointing at a server with
pre-compiled addons and set up using the same directory layout as the
official WinGRASS server, namely `{base}/grass{MM}/addons/grass-{M.M.P}/`
containing one ZIP per extension and a `modules.xml` listing.

### Install a specific version from Addons

To install a specific version from GRASS Addons, specify the full
URL pointing to Trac code browser and include Subversion revision
number. For example, this installs the version number 57854 of
r.local.relief module:

```sh
g.extension r.local.relief url="https://trac.osgeo.org/grass/browser/grass-addons/grass7/raster/r.local.relief?rev=57854&format=zip"
```

### Installing when writing a module locally

Having source code of a GRASS module in a directory on disk one can
install it using:

```sh
g.extension r.example url=/local/directory/r.example/
```

## REQUIREMENTS

In order to clone source code repositories, the *git* command line tool
is required. The installation of single AddOns is most efficient with
versions of git that support so called *sparse checkout*, which was
introduced with version 2.25. With older versions of git, the entire
AddOns repository will be downloaded. On UNIX like systems, installation
is done with the *make* command line tool. For AddOns written in C /
C++, a respective build environment is needed.

## KNOWN ISSUES

Toolboxes in the official repository cannot be downloaded.

On MS-Windows, only the official repository is working because there is
no way of compiling the modules (a Python replacement for Python scripts
should be implemented).

On Windows, it is now possible to point at an alternative server. However, note
that this server should be set up using the same directory layout as the
official WinGRASS server, namely `{base}/grass{MM}/addons/grass-{M.M.P}/`
containing pre-compiled extensions as ZIP file, and a `modules.xml` listing.


## TROUBLESHOOTING

Since extensions have to be compiled on Unix based systems (Linux, Mac
OSX etc.) unless a Python extension is installed, a full compiler
environment must be present on the user's computer.

### ERROR: Please install GRASS development package

While GRASS is available on the user's computer, the respective
development package is lacking. If GRASS was installed from a (Linux)
repository, also the grass-dev\* package (commonly named "grass-dev" or
"grass-devel", sometimes along with the version number) must be
installed.

## SEE ALSO

*[g.extension.all](g.extension.all.md)*

[GRASS 8 Addons Manual
pages](https://grass.osgeo.org/grass8/manuals/addons/)  
[GRASS Addons](https://grasswiki.osgeo.org/wiki/GRASS_AddOns) wiki page.

## AUTHORS

Markus Neteler (original shell script)  
Martin Landa, Czech Technical University in Prague, Czech Republic
(Python rewrite)  
Vaclav Petras, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/) (support for general
sources, partial refactoring)
Paulo van Breugel, HAS green academy (add option for Windows users to
point at alternative repositories)
