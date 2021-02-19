"""
Object factory

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)


(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from .core import get_tgis_message_interface
from .space_time_datasets import SpaceTimeRaster3DDataset, SpaceTimeRasterDataset, \
    SpaceTimeVectorDataset, Raster3DDataset, RasterDataset, VectorDataset


###############################################################################


def dataset_factory(type, id):
    """A factory functions to create space time or map datasets

       :param type: the dataset type: rast or raster; rast3d, raster3d or raster_3d;
                    vect or vector; strds; str3ds; stvds
       :param id: The id of the dataset ("name@mapset")
    """
    if type == "strds":
        sp = SpaceTimeRasterDataset(id)
    elif type == "str3ds":
        sp = SpaceTimeRaster3DDataset(id)
    elif type == "stvds":
        sp = SpaceTimeVectorDataset(id)
    elif type == "rast" or type == "raster":
        sp = RasterDataset(id)
    elif type == "raster_3d" or type == "rast3d" or type == "raster3d":
        sp = Raster3DDataset(id)
    elif type == "vect" or type == "vector":
        sp = VectorDataset(id)
    else:
        msgr = get_tgis_message_interface()
        msgr.error(_("Unknown dataset type: %s") % type)
        return None

    return sp
