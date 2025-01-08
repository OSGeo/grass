## DBMI library

### Purpose

Database management functions for modules and drivers.

### Authors

Original author:

- Joel Jones (CERL/UIUC) (jjones zorro.cecer.army.mil)

Ref: <https://lists.osgeo.org/pipermail/grass-dev/1995-February/002015.html>

Further authors:

- Radim Blazek (radim.blazek gmail.com)
- Brad Douglas (rez touchofmadness.com)
- Glynn Clements (glynn gclements.plus.com)
- Roberto Flor, Hamish Bowman (hamish_b yahoo.com)
- Markus Neteler (neteler itc.it)
- Huidae Cho (grass4u gmail.com)
- Paul Kelly (paul-grass stjohnspoint.co.uk)
- Martin Landa (landa.martin gmail.com)
- Moritz Lennert (mlennert club.worldonline.be)
- Daniel Calvelo Aros (dca.gis gmail.com)
- Bernhard Reiter (bernhard intevation.de)
- Alex Shevlakov (sixote yahoo.com)

### Copyright

(C) 2003-2024 by the GRASS Development Team

### License

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

### Directory contents

- `dbmi/`: DataBase Management Interface (`db_*()` functions)
  - `dbmi_base/`: contains functions for modules, drivers (`../../db/drivers/`)
  - `dbmi_client/`: contains functions for modules
  - `dbmi_driver/`: contains functions for drivers (`../../db/drivers/`)
- `sqlp/`: SQL parser library
- `stubs/`: stubs for unimplemented DB functions

The DBMI drivers are stored in
`../../db/drivers/`

The DBMI user modules are stored in
`../../db/base/`

NOTE:
Please read db/drivers/README.md

To generate `dbmi_driver/dbstubs.h` automatically, run `./mk_dbstubs_h.sh` in
`dbmi_driver/` directory (GRASS GIS 6).
