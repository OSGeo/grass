# GRASS GIS Mac OS X Build

(and other Mac notes)

## Table of Contents

| :exclamation:  Instructions below are not up-to-date. Update in progress. |
|---------------------------------------------------------------------------|

- Quickstart
- About
- Building GRASS
  - Optimization
  - Configure Example
  - Leopard Notes
  - Building an Installer Package
  - Bundling Libraries and Programs
- Developer Notes
- Help Files
- Addon Modules
- Dependency Build Notes
  - Universal Aqua Tcl/Tk
  - Universal GPSBabel
  - Universal NetPBM
  - Universal FFMPEG

## Quickstart

To build a Mac OS X application, add this to your configure options:

`--prefix=/Applications --enable-macosx-app`

## About

This builds GRASS as a Mac OS X application, more or less.

The startup wrapper is a simple AppleScript that fires up a custom
grass.sh (instead of the standard grass6x) and quits, leaving grass.sh
running in a new Terminal window. The custom grass.sh takes care of some
default and required environment settings (so the user doesn't have to
fiddle with .bash_profile), makes sure X11 in running, then starts
init.sh.

## Building GRASS

Building GRASS as a Mac OS X application is pretty much the same as for
a unix build (see the [INSTALL.md](../INSTALL.md) document for details). For an
application, just add this option to configure:

`--enable-macosx-app`

With this option, the `--prefix` specified is the path where GRASS.app
will be installed. The default GRASS prefix is `/usr/local`, but for a Mac
OS X application it should be `/Applications`. It is not possible to have
alternative default prefixes in configure, so you must set
`--prefix=/Applications` yourself for configure.

The makefile tries to figure out the location of the GDAL programs, from
the configured GDAL libs. This is set in grass.sh, and if it doesn't
correctly figure it out (some GRASS commands fail to find needed GDAL
commands), set `GDAL_BIN` before running make:

`export GDAL_BIN=[/path/to/gdal/bin]`

### Universal Build

The GRASS build system is very friendly to a Universal build (unlike
libtool-based builds). *This applies to a standard unix build as well as
the app build.* First, a couple things to check:

**• The Universal SDK**. *This is only needed when building on OSX 10.4
PPC - the Intel system is all Universal, and so is 10.5 for both
architectures (though you may still want to use an SDK).* Using the SDK
forces GCC to use an alternate root for everything, so if a dependency
is not in the SDK (/usr/local is NOT in the SDK), you will get
configure, compile and link errors about missing stuff. To fix this,
create symlinks in the SDK to where you have your extra dependencies.
The SDKs in Leopard are mostly taken care of.

For example, /usr/local:

```sh
sudo ln -s /usr/local /Developer/SDKs/MacOSX10.4u.sdk/usr/local
```

For Leopard, there may be problems locating bin programs. Add the
following:

```sh
sudo ln -s /usr/local/bin /Developer/SDKs/MacOSX10.4u.sdk/usr/local/bin
sudo ln -s /usr/local/bin /Developer/SDKs/MacOSX10.5.sdk/usr/local/bin
```

If there are subfolders involved that are not already in the SDK, add
them first. Say you have some libraries in /Users/Shared/unix (I put my
static libraries there):

```sh
sudo mkdir -p /Developer/SDKs/MacOSX10.4u.sdk/Users/Shared
ln -s /Users/Shared/unix /Developer/SDKs/MacOSX10.4u.sdk/Users/Shared/unix
```

For /Library/Frameworks:

```sh
sudo mkdir -p /Developer/SDKs/MacOSX10.4u.sdk/Library
sudo ln -s /Library/Frameworks \
/Developer/SDKs/MacOSX10.4u.sdk/Library/Frameworks
```

**• Universal Dependencies**. Make sure all your extra dependencies are
Universal.

**• Tiger+**. This will NOT work on anything less than OSX 10.4.

If those check OK, here's what to do. Simply add the universal flags to
all compile and link commands. This is done by setting CFLAGS, CXXFLAGS
and LDFLAGS before configuring the build. The basic flags are:

`-arch ppc -arch i386`

If you are building on OSX 10.4 PPC only, also add this:

`-isysroot /Developer/SDKs/MacOSX10.4u.sdk`

Put it all together, for an Intel Mac on OSX 10.4 or any Mac on 10.5+:

```sh
export CFLAGS="-arch ppc -arch i386"
export CXXFLAGS="-arch ppc -arch i386"
export LDFLAGS="-arch ppc -arch i386"
```

Or for OSX 10.4 PPC:

```sh
export SDKROOT=/Developer/SDKs/MacOSX10.4u.sdk
export CFLAGS="-arch ppc -arch i386 -isysroot $SDKROOT"
export CXXFLAGS="-arch ppc -arch i386 -isysroot $SDKROOT"
export LDFLAGS="-arch ppc -arch i386 -isysroot $SDKROOT"
```

For OSX 10.5, you can also build for 64bit (all processors except the
first Intel Core Duo), just add:

`-arch ppc64 -arch x86_64`

### Optimization

By default, GRASS configures with debugging symbols turned on (the
"-g" flag), and level 2 optimization ("-O2"). If you don't want
debugging on (it greatly increases the binary size) or want to use a
different optimization, just add an optimization flag to the above
CFLAGS and CXXFLAGS exports. Specifying an optimization disables
debugging. Some common optimizations are (see the gcc man page for
complete details):

- `-O2` most optimizations, a common default
- `-O3` high
- `-Os` optimize for speed, also keep the binary size small (the Apple standard)

If you want debugging with a different optimization flag, use one of the above
optimization flags plus "-g".

Remember to separate all flags with spaces.

### Configure Example

As an example, to build GRASS using my frameworks and Tcl/Tk as built in
the build notes below, this should work *(for a standard unix build,
just remove the `--prefix` and `--enable-macosx-app` flags)*:

```bash
./configure \
    --enable-macosx-app \
    --prefix=/Applications \
    --with-cxx \
    --with-fftw-includes=/Library/Frameworks/FFTW3.framework/unix/include \
    --with-fftw-libs=/Library/Frameworks/FFTW3.framework/unix/lib \
    --with-freetype \
    --with-freetype-includes= \
        "/Library/Frameworks/FreeType.framework/unix/include/freetype2 \
        /Library/Frameworks/FreeType.framework/unix/include" \
    --with-freetype-libs=/Library/Frameworks/FreeType.framework/unix/lib \
    --with-gdal=/Library/Frameworks/GDAL.framework/Programs/gdal-config \
    --with-geos=/Library/Frameworks/GEOS.framework/Programs/geos-config \
    --with-jpeg-includes=/Library/Frameworks/UnixImageIO.framework/unix/include \
    --with-jpeg-libs=/Library/Frameworks/UnixImageIO.framework/unix/lib \
    --with-odbc \
    --with-opengl=aqua \
    --with-png-includes=/Library/Frameworks/UnixImageIO.framework/unix/include \
    --with-png-libs=/Library/Frameworks/UnixImageIO.framework/unix/lib \
    --with-proj \
    --with-proj-includes=/Library/Frameworks/PROJ.framework/unix/include \
    --with-proj-libs=/Library/Frameworks/PROJ.framework/unix/lib \
    --with-proj-share=/Library/Frameworks/PROJ.framework/Resources/proj \
    --with-sqlite \
    --with-sqlite-includes=/Library/Frameworks/SQLite3.framework/unix/include \
    --with-sqlite-libs=/Library/Frameworks/SQLite3.framework/unix/lib \
    --with-tcltk-includes="/Library/Frameworks/Tcl.framework/Headers \
            /Library/Frameworks/Tk.framework/Headers \
            /Library/Frameworks/Tk.framework/PrivateHeaders" \
    --with-tcltk-libs=/usr/local/lib \
    --with-tiff-includes=/Library/Frameworks/UnixImageIO.framework/unix/include \
    --with-tiff-libs=/Library/Frameworks/UnixImageIO.framework/unix/lib \
    --with-x \
    --without-glw \
    --without-motif \
    --without-mysql \
    --without-postgres \
    --without-readline
```

That's a long line, but you have to be very explicit in the GRASS configure
(and yes, the UnixImageIO framework is used multiple times).

If you don't have the PROJ framework programs folder in your shell
PATH, it will fail to find nad2bin. To correct this set NAD2BIN before
running configure:

`export NAD2BIN=/Library/Frameworks/PROJ.framework/Programs/nad2bin`

Sometimes, for mysterious reasons so far, the automatic check for X11
fails. Depending on various conditions, you may immediately notice this
because configure stops. If X11 isn't found automatically, add this to
the configure line *(this applies to both the unix build and app
build)*:

`--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib`

To install the new Python GUI (see [REQUIREMENTS.md](../REQUIREMENTS.md)
and [gui/wxpython/README](../gui/wxpython/README), wxpython installer
available at [wxpython.org](https://wxpython.org/)), add this to configure (fill
in the correct version at x.x.x.x for the wxpython you have installed):

```bash
--with-python
--with-wxwidgets=/usr/local/lib/wxPython-unicode-x.x.x.x/bin/wx-config
```

Note: as of wxpython 2.8.8.0 you can use an external wxpython with the
system Python on Leopard.

If you want ffmpeg support (see build instructions below), add this:

```bash
--with-ffmpeg \
--with-ffmpeg-includes="/usr/local/include \
    /usr/local/include/libavcodec /usr/local/include/libavdevice \
    /usr/local/include/libavformat /usr/local/include/libavutil \
    /usr/local/include/libswscale" \
--with-ffmpeg-libs=/usr/local/lib
```

For cairo support (see build
instructions at
[kyngchaos.com](https://web.archive.org/web/20161112052733/http://www.kyngchaos.com/macosx/build/cairo/)),
add this:

```bash
--with-cairo \
--with-cairo-includes=/usr/local/include/cairo \
--with-cairo-libs=/usr/local/lib \
--with-cairo-ldflags=-lcairo
```

### Leopard Notes

• Linker Problem

The linker in Leopard's GCC from Xcode 3.0 has a bit of a problem with
some libraries, notably X11's libGL. When trying to link libGL (for
example), it spits back an error:

`ld: cycle in dylib re-exports with /usr/X11/lib/libGL.dylib for
architecture i386`

Xcode 3.1 does not have this problem. In other words, make sure to
install the latest Xcode.

### Building an Installer Package

After running make, make bindist will assemble an installer package.
This will have the name "GRASS-[version].pkg" and will be found in
the macosx folder in the GRASS source. On Tiger, there **will be** some
**non-fatal errors** - *this is normal* and the installer package is
still generated correctly.

The installer takes care of binary installation, and creating the
/Library/GRASS support folder and Help Viewer link.

This installer currently does NOT do any verification of installed
dependencies or system version.

### Bundling Libraries and Programs

Additional dependent libraries and programs can be bundled with the
application package to make a more portable and easily installed GRASS
application. See `macosx/pkg/bundle.make` for details.

GRASS uses `DYLD_LIBRARY_PATH`, which is set to the application's lib
folder, so it will find any libraries that are there that are not found
in their expected locations. It also adds the application bin folder to
the `PATH`, so helper programs installed there will be found.

There is NO need to bundle *everything*. Bundling readily available libraries
and programs installed in standard locations is not necessary. Some things
you might want to bundle include:

- GPSBabel (CLI)
- NetPBM programs (just what is used in GRASS)
- other support CLI programs for script modules
- ffmpeg libraries
- Postgres library (libpq)

Things to leave out:

- applications that duplicate the above items (ie GPSBabel.app)
- /Library/Frameworks stuff

## Developer Notes

The applescript startup and grass.sh can be customized if desired, ie
for a more complex startup that might give the user a dialog for startup
options. They just provide the basic core to build upon.

## Help Files

Help files are linked into the OSX help system. Independent of what
browser you choose to open help files (`GRASS_HTML_BROWSER`), you can
open Help Viewer from almost any application's Help menu (it's best to
use Mac Help from the Finder Help menu), and GRASS Help will be
available in the Library menu.

Since they are linked to the help system, if GRASS.app is moved or
renamed, the link will be broken. This may change in the future.

## Addon Modules

modbuild deprecated (not installed now). Makefile changes in progress to
allow compiling external modules.

## Dependency Build Notes

They apply to any OSX build, not just an application package build:

### Universal Aqua Tcl/Tk

The simplest way to install Tcl/Tk Aqua is to download
[ActiveTcl](https://www.activestate.com/products/tcl/) from
ActiveState. This will give you a universal 32bit Tcl/Tk Aqua (64bit is
currently not possible). Make sure to run the *convenience* commands
below.

To make it easier to configure for GRASS (and other software), run these
commands in a Terminal:

```sh
sudo ln -sf /Library/Frameworks/Tcl.framework/Versions/8.5/Tcl \
    /usr/local/lib/libtcl.dylib
sudo ln -sf /Library/Frameworks/Tcl.framework/Versions/8.5/Tcl \
    /usr/local/lib/libtcl8.5.dylib
sudo ln -sf \
    /Library/Frameworks/Tcl.framework/Versions/8.5/libtclstub8.5.a \
    /usr/local/lib/libtclstub8.5.a
sudo ln -sf /Library/Frameworks/Tk.framework/Versions/8.5/Tk \
    /usr/local/lib/libtk.dylib
sudo ln -sf /Library/Frameworks/Tk.framework/Versions/8.5/Tk \
    /usr/local/lib/libtk8.5.dylib
sudo ln -sf /Library/Frameworks/Tk.framework/Versions/8.5/libtkstub8.5.a \
    /usr/local/lib/libtkstub8.5.a
```

### Universal GPSBabel

A universal GPSBabel CLI executable is now included in the OSX binary
from [www.gpsbabel.org](https://www.gpsbabel.org/). This does not need to
be 64bit.

### Universal NetPBM

NetPBM is not too hard to build, it's just weird. These instructions
use my UnixImageIO framework for the tiff, jpeg, png and jasper
libraries.

First, grab the [NetPBM](http://netpbm.sourceforge.net/) source and
unzip it. Now it gets strange - it's an interactive configuration, and
the various stages don't communicate with each other. Cd to the source
folder in a Terminal and:

`./configure` ↵

`Platform [darwin] ==>` ↵

`Netpbm shared library directory [default] ==>` **`/usr/local`**

choose where you will install it
default is NOT /usr/local or any location at all, so you MUST set this

`regular or merge [regular] ==>` ↵

`static or shared [shared] ==>` ↵

could use static, since GRASS only needs a few of the progs

`'#include' argument or NONE [<inttypes.h>] ==>` ↵

`What is your JPEG (graphics format) library?`
`library filename or 'none' [libjpeg.so] ==>`
    **`/Library/Frameworks/UnixImageIO.framework/unix/lib/libjpeg.dylib`**

`JPEG header directory [default] ==>`
**`/Library/Frameworks/UnixImageIO.framework/Headers`**

`What is your TIFF (graphics format) library?`
`library filename or 'none' [libtiff.so] ==>`
    **`/Library/Frameworks/UnixImageIO.framework/unix/lib/libtiff.dylib`**

`TIFF header directory [default] ==>`
    **`/Library/Frameworks/UnixImageIO.framework/Headers`**

`What is your PNG (graphics format) library?`
`library filename or 'none' [libpng.so] ==>`
    **`/Library/Frameworks/UnixImageIO.framework/unix/lib/libpng.dylib`**

`PNG header directory [default] ==>`
    **`/Library/Frameworks/UnixImageIO.framework/Headers`**

`What is your Z (compression) library?`
`library filename or 'none' [libz.so] ==>` **`-lz`**

`Z header directory [default] ==>` ↵

`Documentation URL [http://netpbm.sourceforge.net/doc/] ==>` ↵

ignore warning about libz

```sh
echo "JASPERLIB = \
    /Library/Frameworks/UnixImageIO.framework/unix/lib/libjasper.dylib"  \
    >> Makefile.config
echo "JASPERHDR_DIR = /Library/Frameworks/UnixImageIO.framework/Headers" \
    >> Makefile.config
```

For Tiger:

```sh
echo "CC = /usr/bin/gcc -arch ppc -arch i386 \
    -isysroot /Developer/SDKs/MacOSX10.4u.sdk" \
    >> Makefile.config
```

For Leopard:

```sh
echo "CC = /usr/bin/gcc -arch ppc -arch i386 -arch ppc64 -arch x86\_64" \
    >> Makefile.config
```

```sh
make
make package pkgdir=/path/to/some/temp/dir
```

it will create the folder and copy all binaries there

`sudo ./installnetpbm`

then answer some more questions:

`Where is the install package you created with 'make package'?`
`package directory (/tmp/netpbm) ==>` **`/path/to/some/temp/dir`**

same location as specified in the make package step (lack of
 inter-communication!)

`install prefix (/usr/local/netpbm) ==>` **`/usr/local`**

same location used for the Netpbm shared library directory (again,
lack of inter-communication!)
(it may ask you if you want to create the dir)

It'll ask for subfolders for bin, lib, data, headers, man

For data, type: **`/usr/local/share/netpbm`**

For man, type: **`/usr/local/share/man`**

For the rest, use the defaults

don't create the manweb.conf file

And that's it.

### Universal FFMPEG

version: [0.5](http://www.ffmpeg.org/download.html)

FFMPEG is constantly changing, and release versions are rare. For
simplicity use the 0.5 release version. If you know anything about SVN
you can try for a more up-to-date FFMPEG. Some options used below are
not in more recent SVN versions, so if configure complains about an
invalid option, just remove it.

This will build a basic FFMPEG as dynamic libraries for use with GRASS.
The default static libraries don't work now because of reloc errors and
the need for extra link flags. No extra formats are included, such as
mp3lame and xvid. None of the ffmpeg programs are included either (GRASS
doesn't need them).

It's a multi-step build, 1 for each architecture. Create a folder
inside the source for each arch:

```sh
mkdir build-i386
mkdir build-ppc
```

and also for Leopard:

```sh
mkdir build-x86_64
mkdir build-ppc64
```

For i386 and ppc arches, if building on Tiger, you must add the
following to extra-cflags and extra-ldflags in the configure command:

`-isysroot /Developer/SDKs/MacOSX10.4u.sdk`

For i386:

```sh
cd build-i386
../configure \
    --arch=i386 \
    --disable-amd3dnow \
    --disable-debug \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-network \
    --disable-static \
    --disable-vhook \
    --enable-gpl \
    --enable-pthreads \
    --enable-shared \
    --enable-swscale \
    --extra-cflags="-arch i386" \
    --extra-ldflags="-arch i386"
```

If you are building *on* a PPC Mac, you need to add the flag
"`--enable-cross-compile`" to the above configure command.

```sh
make
sudo make install
```

Install at this point installs everything. We don't care about the
installed libraries yet, we just need the headers there.

Now, the PPC build:

```sh
cd ../build-ppc
../configure \
    --arch=ppc \
    --disable-debug \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-network \
    --disable-static \
    --disable-vhook \
    --enable-altivec \
    --enable-gpl \
    --enable-pthreads \
    --enable-shared \
    --enable-swscale \
    --extra-cflags="-arch ppc" \
    --extra-ldflags="-arch ppc"
make
```

Don't install this one, there is no need.

If you are building for Leopard, also do the 64bit varieties, otherwise
skip to the lipo step below.

For x86_64:

```sh
cd build-x86_64
../configure \
    --arch=x86_64 \
    --disable-amd3dnow \
    --disable-debug \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-network \
    --disable-static \
    --disable-vhook \
    --enable-gpl \
    --enable-pthreads \
    --enable-shared \
    --enable-swscale \
    --extra-cflags="-arch x86\_64" \
    --extra-ldflags="-arch x86_64"
```

Again, if you are building *on* a PPC Mac, you need to add the flag
"`--enable-cross-compile`" to the above configure command.

`make`

And ppc64:

```sh
cd ../build-ppc64
../configure \
    --arch=ppc64 \
    --disable-debug \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-network \
    --disable-static \
    --disable-vhook \
    --enable-altivec \
    --enable-gpl \
    --enable-pthreads \
    --enable-shared \
    --enable-swscale \
    --extra-cflags="-arch ppc64" \
    --extra-ldflags="-arch ppc64"
```

Here, if you are building *on* an Intel Mac, you need to add the flag
"`--enable-cross-compile`" to the above configure command. Also, it
will fail to completely identify it as 64bit, so you need to fix 2
files.

In **config.h**, find the `HAVE_PPC64` define and set it to 1:

`#define HAVE_PPC64 1`

In **config.mak**, add this line to the end of the file:

`HAVE_PPC64=yes`

Then finish:

`make`

Now use lipo to merge them together, first drop down to the ffmpeg
source folder:

`cd ..`

For each of the libraries:

libavcodec
libavdevice
libavformat
libavutil
libswscale

run the following lipo command (fill in the **`[LIBNAME]`** in all
places). For Leopard:

```sh
sudo lipo -create \
    build-ppc/[LIBNAME]/[LIBNAME].dylib \
    build-i386/[LIBNAME]/[LIBNAME].dylib \
    build-ppc64/[LIBNAME]/[LIBNAME].dylib \
    build-x86_64/[LIBNAME]/[LIBNAME].dylib \
    -output /usr/local/lib/[LIBNAME].dylib
```

For Tiger:

```sh
sudo lipo -create \
    build-ppc/[LIBNAME]/[LIBNAME].dylib \
    build-i386/[LIBNAME]/[LIBNAME].dylib \
    -output /usr/local/lib/[LIBNAME].dylib
```

Done.

© 2006-2008 by the GRASS Development Team

This program is free software under the GNU General Public License (>=v2).

\- William Kyngesburye

<kyngchaos@kyngchaos.com>

<https://www.kyngchaos.com/>
