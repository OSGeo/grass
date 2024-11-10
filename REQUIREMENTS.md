# Requirements to compile GRASS GIS 8

A workstation running some flavor of UNIX including
GNU/Linux, Solaris, IRIX, BSD, Mac OSX, Cygwin or MinGW (on Win32/Win64).
Ideally, you should have at least 800 MB of free disk space.
The source code package needs disk space of around
26 MB compressed and 440MB uncompressed.
The resulting binaries may need between 20 MB and 180 MB
depending on your platform and compiler flags.
During a full compilation you may need temporarily up to
550MB including the source code.

To enable and disable features please read the [INSTALL.md](INSTALL.md) file.

## General requirements

Most tools are standard tools on GNU/Linux,
for other platforms you may have to install some of them.

- **C-compiler** (cc, gcc, egcs, ...)
  gcc: [https://www.gnu.org/software/gcc/gcc.html](https://www.gnu.org/software/gcc/gcc.html)
- **GNU make** is recommended (at least version 3.81)
  [https://www.gnu.org/software/make/make.html](https://www.gnu.org/software/make/make.html)
- **zlib** compression library (already installed on most modern systems)
  It is used to internally compress GRASS raster maps.
  libz: [https://www.zlib.net](https://www.zlib.net)
- **flex** lexical analyzer generator (flex)
  _Note: lex is no longer supported, please use flex instead._\
  flex: [https://github.com/westes/flex](https://github.com/westes/flex)
- **parser generator** (yacc, bison)
  bison: [https://www.gnu.org/software/bison/bison.html](https://www.gnu.org/software/bison/bison.html)
- **PROJ** - Cartographic Projection Library
  PROJ: [https://proj.org/](https://proj.org/)
- **GDAL/OGR** for import and export of most external raster and vector map formats
  GDAL: [https://gdal.org](https://gdal.org)
- **Python >= 3.8** (for temporal framework, scripts, wxGUI, and ctypes interface)
  [https://www.python.org](https://www.python.org)
- **MkDocs** with "Material" theme Python packages for the manual pages:
  See `man/mkdocs/requirements.txt`.

## Optional packages

Note: also the respective development packages (commonly named `xxx-dev` or
`xxx-devel`) need to be installed.

- **C++ Compiler** (required for various C++ GRASS modules)
  gcc: [https://www.gnu.org/software/gcc/gcc.html](https://www.gnu.org/software/gcc/gcc.html)
- **bzip2**, needed for raster compression with bzip2
  [https://www.bzip.org](https://www.bzip.org)
- **zstd** (Zstandard), needed for raster compression with zstd
  [https://facebook.github.io/zstd](https://facebook.github.io/zstd)
- **FFTW 2.x or 3.x** (library for computing the Discrete Fourier Transform),
  required for `i.fft` and `i.ifft` and other modules
  [https://fftw.org](https://fftw.org)
- **GEOS** (Geometry Engine library),
  needed for `v.buffer` and adds extended options to the `v.select` module
  [https://libgeos.org/](https://libgeos.org/)
- **LAPACK / BLAS** (libraries for numerical computing) for GMATH library
  (GRASS Numerical Library)
  [https://www.netlib.org/lapack](https://www.netlib.org/lapack) (usually
  available on Linux distros)
  _Note: LAPACK/BLAS support is at time only needed for selected Addons._
- **NetCDF** (for 3D raster netcdf export)
  [https://www.unidata.ucar.edu/software/netcdf/](https://www.unidata.ucar.edu/software/netcdf/)
- **Mesa-3.x**, if hardware OpenGL support is missing in the X Server (OpenGL
  clone) may be required for wxNVIZ
  [https://mesa3d.org/](https://mesa3d.org/)
- **libpng** (for `r.out.png` and the PNG driver), usually already installed.
  [http://www.libpng.org/pub/png/libpng.html](http://www.libpng.org/pub/png/libpng.html)
- **LIBSVM** (for SVM classification modules `i.svm.train` and `i.svm.predict`)
  [https://www.csie.ntu.edu.tw/~cjlin/libsvm/](https://www.csie.ntu.edu.tw/~cjlin/libsvm/)
- **libtiff** (for `r.out.tiff`), usually already installed.
  [https://gitlab.com/libtiff/libtiff](https://gitlab.com/libtiff/libtiff)
- **readline** for extra command prompt functionality
  [https://tiswww.case.edu/~chet/readline/rltop.html](https://tiswww.case.edu/~chet/readline/rltop.html)
  [ftp://ftp.gnu.org/gnu/readline](ftp://ftp.gnu.org/gnu/readline)
- **PDAL** ( for LAS import modules `r.in.pdal` and `v.in.pdal`)
  [https://pdal.io](https://pdal.io)
- **PostgreSQL libraries** (for the PostgreSQL database interface and PostGIS support)
  [https://www.postgresql.org](https://www.postgresql.org)
- **MariaDB/MySQL libraries** (for the MySQL database interface)
  [https://mariadb.org/](https://mariadb.org/)
- **SQLite libraries** (for the SQLite database interface)
  [https://www.sqlite.org](https://www.sqlite.org)
- **unixODBC** (for the ODBC database interface)
  [https://www.unixodbc.org](https://www.unixodbc.org)
- **R Statistics** (for the R statistical language interface)
  [https://cran.r-project.org](https://cran.r-project.org)
- **FreeType2** (for TrueType font support and `d.text.freetype`)
  [https://freetype.org/](https://freetype.org/)
- **wxPython >= 2.8.10.1** (for wxGUI)
  [https://www.wxpython.org](https://www.wxpython.org)
- **NumPy >= 1.0.4** (for various wxGUI components and pyGRASS)
  [https://numpy.org](https://numpy.org)
- **Python dateutil Library** (`python-dateutil`, needed for the tgrass modules `t.*`)
  [https://pypi.org/project/python-dateutil/](https://pypi.org/project/python-dateutil/)
  [https://github.com/dateutil/dateutil](https://github.com/dateutil/dateutil)
- **Python PLY Library (Python Lex-Yacc)** (`python-ply`, needed for the
  temporal algebra in tgis)
  [https://www.dabeaz.com/ply](https://www.dabeaz.com/ply/)
- **Pillow (Python Imaging Library)** (highly recommended for wxGUI and
  necessary for wxGUI Cartographic Composer)
  [https://python-pillow.org/](https://python-pillow.org/)
- **Python matplotlib > 1.2** (`python-matplotlib`, needed for the several wxGUI
  tools)
  [https://matplotlib.org/](https://matplotlib.org/)
- **Python wxPython backend for python-matplotlib**
  ("python-matplotlib-wx", needed for e.g. `g.gui.timeline`)
  [https://matplotlib.org/](https://matplotlib.org/)
- **python-termcolor** (recommended for `g.search.modules`)
  [https://pypi.org/project/termcolor/](https://pypi.org/project/termcolor/)
- **FFMPEG or alternative** (for wxGUI Animation tool - `g.gui.module`),
  specifically ffmpeg tool
  [https://ffmpeg.org](https://ffmpeg.org)
- **mpeg_encode or ppmtompeg tool** (for `r.out.mpeg` module)
  [https://ffmpeg.org](https://ffmpeg.org)
- **Cairo >= 1.5.8** (for Cairo driver)
  [https://cairographics.org](https://cairographics.org)
- **AVCE00 and E00Compr Libraries** (avcimport)
  [http://avce00.maptools.org](http://avce00.maptools.org)
- **git** (git for `g.extension`)
  [https://git-scm.com](https://git-scm.com)
- **Subversion** (svn in `g.extension` to fetch code selectively from grass-addons
  on GitHub)
  [https://subversion.apache.org/](https://subversion.apache.org/)

## Note

SUN Solaris users may go here to download precompiled libraries etc.:
[https://www.sunfreeware.com](https://www.sunfreeware.com)

SGI IRIX users may go here to download precompiled libraries etc.:
[https://freeware.sgi.com](https://freeware.sgi.com)

MacOSX users may go here to download precompiled libraries etc.:
[https://fink.sourceforge.net](https://fink.sourceforge.net)

---

© _GRASS Development Team 1997-2024_

Please report bugs here:
[https://grass.osgeo.org/contribute/](https://grass.osgeo.org/contribute/)
