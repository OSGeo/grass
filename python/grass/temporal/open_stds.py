"""Functions to open or create space time datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)


(C) 2012-2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .core import SQLDatabaseInterfaceConnection
    from .abstract_space_time_dataset import AbstractSpaceTimeDataset
import contextlib
from .abstract_map_dataset import AbstractMapDataset
from .core import get_current_mapset, get_tgis_message_interface, init_dbif
from .factory import dataset_factory

###############################################################################


def _parse_id(ident: str) -> tuple[str, str | None, str | None, str | None]:
    """Parse parts of a user given dataset name.

    :param ident: The id of a space time dataset

    :return: Tuple with ID componenets: name, mapset, semantic_lablel, layer
    """
    name = ident

    mapset = None
    if "@" in ident:
        name, mapset = ident.split("@", 1)

    semantic_label = None
    if "." in name:
        name, semantic_label = name.split(".", 1)

    layer = None
    if ":" in name:
        name, layer = name.split(":", 1)
    return name, mapset, semantic_label, layer


def open_old_stds(name, type, dbif=None):
    """This function opens an existing space time dataset and return the
    created and initialized object of the specified type.

    This function will call exit() or raise a
    grass.pygrass.messages.FatalError in case the type is wrong,
    or the space time dataset was not found.

    :param name: The name of the space time dataset, if the name does not
                 contain the mapset (name@mapset) then the space time dataset
                 is searched first in the current mapset, then on the search
                 path. The mapset of the first match  will be used to identify
                 the space time dataset.
    :param type: The type of the space time dataset (strd, str3ds, stvds,
                 raster, vector, raster3d)
    :param dbif: The optional database interface to be used

    :return: New stds object

    """
    msgr = get_tgis_message_interface()
    stds_type = type
    if stds_type not in {
        "strds",
        "str3ds",
        "stvds",
        "raster",
        "rast",
        "raster3d",
        "rast3d",
        "raster_3d",
        "vector",
        "vect",
    }:
        msgr.fatal(_("Unknown type: %s") % (stds_type))

    def try_get_stds(
        ds_id: str,
        stds_type: str,
        semantic_label: str | None,
        dbif: SQLDatabaseInterfaceConnection,
    ) -> AbstractSpaceTimeDataset | None:
        if stds_type in {"str3ds", "raster3d", "rast3d", "raster_3d"}:
            sp = dataset_factory("str3ds", ds_id)
        elif stds_type in {"stvds", "vect", "vector"}:
            sp = dataset_factory("stvds", ds_id)
        else:
            sp = dataset_factory("strds", ds_id)
            # Set the semantic label if it was given
            if semantic_label:
                sp.set_semantic_label(semantic_label)

        with contextlib.suppress(Exception):
            if sp.is_in_db(dbif):
                return sp
        return None

    # Check if the dataset name contains the mapset and the semantic label as well
    name, mapset, semantic_label, _layer = _parse_id(name)

    dbif, connection_state_changed = init_dbif(dbif)

    # Check user given ID
    sp = None
    if mapset:
        dbif.add_mapset(mapset)
        sp = try_get_stds(f"{name}@{mapset}", stds_type, semantic_label, dbif)
    else:
        # Check current mapset first
        sp = try_get_stds(
            f"{name}@{get_current_mapset()}",
            stds_type,
            semantic_label,
            dbif,
        )
        if not sp:
            for tgis_mapset in dbif.tgis_mapsets:
                if tgis_mapset == get_current_mapset():
                    continue
                sp = try_get_stds(
                    f"{name}@{tgis_mapset}",
                    stds_type,
                    semantic_label,
                    dbif,
                )
                if sp:
                    break
    if not sp:
        if connection_state_changed:
            dbif.close()
        msgr.fatal(
            _("Space time dataset <%(name)s> of type <%(sp)s> not found")
            % {"name": name, "sp": stds_type},
        )
    # Read content from temporal database
    sp.select(dbif)
    if connection_state_changed:
        dbif.close()

    return sp


###############################################################################


def check_new_stds(name, type, dbif=None, overwrite: bool = False):
    """Check if a new space time dataset of a specific type can be created

    :param name: The name of the new space time dataset
    :param type: The type of the new space time dataset (strd, str3ds,
                 stvds, raster, vector, raster3d)
    :param dbif: The temporal database interface to be used
    :param overwrite: Flag to allow overwriting

    :return: A space time dataset object that must be filled with
            content before insertion in the temporal database

    This function will raise a FatalError in case of an error.
    """
    stds_type = type
    # Get the current mapset to create the id of the space time dataset

    mapset = get_current_mapset()
    msgr = get_tgis_message_interface()

    name, mapset, _semantic_label, _layer = _parse_id(name)
    if mapset:
        if mapset != get_current_mapset():
            msgr.fatal(
                _("Space time datasets can only be created in the current mapset"),
            )
        id = f"{name}@{mapset}"
    else:
        id = f"{name}@{get_current_mapset()}"

    if stds_type in {"strds", "rast", "raster"}:
        if "." in name:
            # a dot is used as a separator for semantic label filtering
            msgr.fatal(
                _("Illegal dataset name <{}>. Character '.' not allowed.").format(name),
            )
        sp = dataset_factory("strds", id)
    elif stds_type in {"str3ds", "raster3d", "rast3d ", "raster_3d"}:
        sp = dataset_factory("str3ds", id)
    elif stds_type in {"stvds", "vect", "vector"}:
        sp = dataset_factory("stvds", id)
    else:
        msgr.error(_("Unknown type: %s") % (stds_type))
        return None

    dbif, connection_state_changed = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite is False:
        msgr.fatal(
            _(
                "Space time %(sp)s dataset <%(name)s> is already in the"
                " database. Use the overwrite flag.",
            )
            % {"sp": sp.get_new_map_instance(None).get_type(), "name": name},
        )
    if connection_state_changed:
        dbif.close()

    return sp


###############################################################################


def open_new_stds(
    name,
    type,
    temporaltype,
    title,
    descr,
    semantic,
    dbif=None,
    overwrite: bool = False,
):
    """Create a new space time dataset of a specific type

    :param name: The name of the new space time dataset
    :param type: The type of the new space time dataset (strd, str3ds,
                 stvds, raster, vector, raster3d)
    :param temporaltype: The temporal type (relative or absolute)
    :param title: The title
    :param descr: The dataset description
    :param semantic: Semantical information
    :param dbif: The temporal database interface to be used
    :param overwrite: Flag to allow overwriting

    :return: The new created space time dataset

    This function will raise a FatalError in case of an error.
    """
    dbif, connection_state_changed = init_dbif(dbif)
    msgr = get_tgis_message_interface()
    sp = check_new_stds(name, type, dbif, overwrite)

    if sp.is_in_db(dbif):
        msgr.warning(
            _(
                "Overwriting space time %(sp)s dataset <%(name)s> and "
                "unregistering all maps",
            )
            % {"sp": sp.get_new_map_instance(None).get_type(), "name": name},
        )
        id = sp.get_id()
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    msgr.verbose(
        _("Creating a new space time %s dataset")
        % sp.get_new_map_instance(None).get_type(),
    )

    sp.set_initial_values(
        temporal_type=temporaltype,
        semantic_type=semantic,
        title=title,
        description=descr,
    )

    sp.insert(dbif)

    if connection_state_changed:
        dbif.close()

    return sp


############################################################################


def check_new_map_dataset(
    name,
    layer=None,
    type="raster",
    overwrite: bool = False,
    dbif=None,
):
    """Check if a new map dataset of a specific type can be created in
     the temporal database

    :param name: The name of the new map dataset
    :param layer: The layer of the new map dataset
    :param type: The type of the new map dataset (raster, vector, raster3d)
    :param dbif: The temporal database interface to be used
    :param overwrite: Flag to allow overwriting

    :return: A map dataset object

    This function will raise a FatalError in case of an error.
    """
    mapset = get_current_mapset()
    msgr = get_tgis_message_interface()

    dbif, connection_state_changed = init_dbif(dbif)
    map_id = AbstractMapDataset.build_id(name, mapset, layer)

    new_map = dataset_factory(type, map_id)
    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif) and not overwrite:
        if connection_state_changed:
            dbif.close()
        msgr.fatal(
            _(
                "Map <%s> is already in temporal database,"
                " use overwrite flag to overwrite",
            )
            % (map_id),
        )

    if connection_state_changed:
        dbif.close()

    return new_map


############################################################################


def open_new_map_dataset(
    name,
    layer=None,
    type="raster",
    temporal_extent=None,
    overwrite: bool = False,
    dbif=None,
):
    """Create a new map dataset object of a specific type that can be
     registered in the temporal database

    :param name: The name of the new map dataset
    :param layer: The layer of the new map dataset
    :param type: The type of the new map dataset (raster, vector, raster3d)
    :param dbif: The temporal database interface to be used
    :param overwrite: Flag to allow overwriting

    :return: A map dataset object

    """
    dbif, connection_state_changed = init_dbif(dbif)
    new_map = check_new_map_dataset(name, layer, type, overwrite, dbif)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        # Remove the existing temporal database entry
        map_id = new_map.get_id()
        new_map.delete(dbif)
        new_map = new_map.get_new_instance(map_id)

    if temporal_extent:
        new_map.set_temporal_extent(temporal_extent)

    if connection_state_changed:
        dbif.close()

    return new_map
