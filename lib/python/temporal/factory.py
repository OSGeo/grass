"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.register_maps_in_space_time_dataset(type, name, maps)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *

###############################################################################


def dataset_factory(type, id):
    """!A factory functions to create space time or map datasets

       @param type the dataset type: rast or raster, rast3d,
                    vect or vector, strds, str3ds, stvds
       @param id The id of the dataset ("name@mapset")
    """
    if type == "strds":
        sp = SpaceTimeRasterDataset(id)
    elif type == "str3ds":
        sp = SpaceTimeRaster3DDataset(id)
    elif type == "stvds":
        sp = SpaceTimeVectorDataset(id)
    elif type == "rast" or type == "raster":
        sp = RasterDataset(id)
    elif type == "rast3d":
        sp = Raster3DDataset(id)
    elif type == "vect" or type == "vector":
        sp = VectorDataset(id)
    else:
        core.error(_("Unknown dataset type: %s") % type)
        return None

    return sp

