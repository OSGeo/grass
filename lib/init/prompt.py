#!/usr/bin/env python

import os
import sys

from grass.script import core as grass

gisenv = grass.gisenv()

import gettext
gettext.install('grasslibs', os.path.join(gisenv['GISDBASE'], 'locale'), unicode = True)

location = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'], gisenv['MAPSET'])

has_mask   = os.path.isfile(os.path.join(location, 'cell', 'MASK'))
has_mask3d = os.path.isdir(os.path.join(location, 'grid3', 'G3D_MASK'))

def main():
    if has_mask and has_mask3d:
        grass.info(_("[Raster and Volume MASKs present]"))
    elif has_mask:
        grass.info(_("[Raster MASK present]"))
    elif has_mask3d:
        grass.info(_("[Volume MASK present]"))

    return 0

if __name__ == "__main__":
    sys.exit(main())
