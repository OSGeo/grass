# How to add a new external library

The following procedure will walk you through adding a new third-party
library to GRASS GIS.

## License

Before you start, make sure that the library you want to include is in
compliance with the GRASS GIS licence.

### Approved Third-Party Licenses

* MIT/X
* GPLv2
* LGPL 2.1
* MPL 2.0
* BSD-3-Clause
* Public Domain

## Procedure

* Create a directory that will contain the new library's files
in `lib/external/<new-library>`

* Add the new libraries header and source files, license, and README to the newly
created directory.

* Add a new `Makefile` to the directory.

```make
MODULE_TOPDIR = ../../..

# replace SHAPE with new library name
LIB = SHAPE

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: headers
    $(MAKE) lib

# Update header file reference to the new library
headers: $(ARCH_INCDIR)/shapefil.h

$(ARCH_INCDIR)/%.h: %.h
    $(INSTALL_DATA) $< $@
```

* Update `lib/external/Makefile` to include the new subdirectory.

```make
MODULE_TOPDIR = ../..

SUBDIRS = \
    ccmath  \
    parson  \
    shapelib \
    # Add new directory here

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs
```

* Update the `lib/external/README.license` with a new entry containing
 the *library name*, *license*, *description*, *version*, *copyright*,
 and *authors name*.

```txt
* parson/ (MIT/X)
   JSON parsing and encoding functions version 1.5.2.
   Copyright (c) 2022, Krzysztof Gabis
```

* Update `lib/README` with a new entry under external libraries.

```md
 external: external libraries
    - external/parson: JSON serialization and deserialization functions
    - external/shapelib: SHAPE file management functions
    <!-- - Add new entry here -->
```

* Update `include/Make/Install.make`

```make
-cp -rL --parents lib/external/<new library> ./grass-lib-$(GRASS_VERSION_NUMBER)
```

* Add reference to library in `include/Make/Grass.make` in alphabetical order.

```make
<Uppercase library name>:<Lowercase library name>

# example for the parson library
PARSON:parson \
```

* Add reference to library in the root `Makefile`.

```make
LIBDIRS = \
    lib/external/parson \
    lib/external/shapelib \
    # New reference
    ...
```

* The library should now be able to successfully compile.

To test run the `make` command.

```bash
make
```

* If no errors are found the library should now be able to be used in development.

```c
#include <grass/<new-library.h>
```
