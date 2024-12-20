This directory contains drivers for the DBMI library.
The driver functions are for internal usage.

The DBMI API to be used for module programming is available in:
`lib/db/`

NOTE:
`db__driver_*` functions are implemented in a driver. If some of them
are not used or defined, the driver will use stub functions in
`lib/db/stubs/`.

For some platforms like Cygwin, multiply defined symbols are not
resolved in a way that UNIX does. Even worse is that it is impossible
to build shared libraries with undefined symbols. For example,
`libgrass*dbmidriver.so` cannot be built without any implementations
of `db__driver*\*` functions which should be specific to a db driver.

To work around this problem, function pointers are defined to use
driver's implementations instead of those of the db stubs library.
To do this automatically, run `../mk_dbdriver_h.sh` (GRASS GIS 6)
in driver's directory, `#include "dbdriver.h"` from `main.c`, and
execute `init_dbdriver()`.

Function pointers are defined in `lib/db/dbmi_driver/dbstubs.h`
This header file can be generated with
`lib/db/dbmi_driver/mk_dbstubs_h.sh` (GRASS GIS 6).

Please read lib/db/README.md and

<https://grass.osgeo.org/programming8/dbmilib.html>
