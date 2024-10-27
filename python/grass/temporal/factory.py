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

from __future__ import annotations

from typing import Literal, overload

from .core import get_tgis_message_interface
from .space_time_datasets import (
    Raster3DDataset,
    RasterDataset,
    SpaceTimeRaster3DDataset,
    SpaceTimeRasterDataset,
    SpaceTimeVectorDataset,
    VectorDataset,
)

###############################################################################


@overload
def dataset_factory(type: Literal["strds"], id: str) -> SpaceTimeRasterDataset:
    pass


@overload
def dataset_factory(type: Literal["str3ds"], id: str) -> SpaceTimeRaster3DDataset:
    pass


@overload
def dataset_factory(type: Literal["stvds"], id: str) -> SpaceTimeVectorDataset:
    pass


@overload
def dataset_factory(type: Literal["rast", "raster"], id: str) -> RasterDataset:
    pass


@overload
def dataset_factory(
    type: Literal["raster_3d", "rast3d", "raster3d"],
    id: str,
) -> Raster3DDataset:
    pass


@overload
def dataset_factory(type: Literal["vect", "vector"], id: str) -> VectorDataset:
    pass


def dataset_factory(
    type: str, id: str
) -> (
    SpaceTimeRasterDataset
    | SpaceTimeRaster3DDataset
    | SpaceTimeVectorDataset
    | RasterDataset
    | Raster3DDataset
    | VectorDataset
    | None
):
    """A factory functions to create space time or map datasets

    :param type: the dataset type: rast or raster; rast3d, raster3d or raster_3d;
                 vect or vector; strds; str3ds; stvds
    :param id: The id of the dataset ("name@mapset")
    """
    if type == "strds":
        return SpaceTimeRasterDataset(id)
    if type == "str3ds":
        return SpaceTimeRaster3DDataset(id)
    if type == "stvds":
        return SpaceTimeVectorDataset(id)
    if type in {"rast", "raster"}:
        return RasterDataset(id)
    if type in {"raster_3d", "rast3d", "raster3d"}:
        return Raster3DDataset(id)
    if type in {"vect", "vector"}:
        return VectorDataset(id)

    msgr = get_tgis_message_interface()
    msgr.error(_("Unknown dataset type: %s") % type)
    return None
