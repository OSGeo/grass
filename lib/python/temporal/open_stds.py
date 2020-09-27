"""
Functions to open or create space time datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)


(C) 2012-2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from .core import init_dbif, get_current_mapset, get_tgis_message_interface
from .factory import dataset_factory
from .abstract_map_dataset import AbstractMapDataset

###############################################################################


def open_old_stds(name, type, dbif=None):
    """This function opens an existing space time dataset and return the
       created and initialized object of the specified type.

       This function will call exit() or raise a
       grass.pygrass.messages.FatalError in case the type is wrong,
       or the space time dataset was not found.

       :param name: The name of the space time dataset, if the name does not
                    contain the mapset (name@mapset) then the current mapset
                    will be used to identifiy the space time dataset
       :param type: The type of the space time dataset (strd, str3ds, stvds,
                    raster, vector, raster3d)
       :param dbif: The optional database interface to be used

       :return: New stds object

    """
    msgr = get_tgis_message_interface()

    # Check if the dataset name contains the mapset and the band reference as well
    if name.find("@") < 0:
        mapset = get_current_mapset()
    else:
        name, mapset = name.split('@')
    band_ref = None
    if name.find(".") > -1:
        try:
            name, band_ref = name.split('.')
        except ValueError:
            msgr.fatal("Invalid name of the space time dataset. Only one dot allowed.")
    id = name + "@" + mapset

    if type == "strds" or type == "rast" or type == "raster":
        sp = dataset_factory("strds", id)
        if band_ref:
            sp.set_band_reference(band_ref)
    elif type == "str3ds" or type == "raster3d" or type == "rast3d" or type == "raster_3d":
        sp = dataset_factory("str3ds", id)
    elif type == "stvds" or type == "vect" or type == "vector":
        sp = dataset_factory("stvds", id)
    else:
        msgr.fatal(_("Unknown type: %s") % (type))

    dbif, connected = init_dbif(dbif)

    if not sp.is_in_db(dbif):
        dbif.close()
        msgr.fatal(_("Space time %(sp)s dataset <%(name)s> not found") %
                   {'sp': sp.get_new_map_instance(None).get_type(),
                    'name': name})
    # Read content from temporal database
    sp.select(dbif)
    if connected:
        dbif.close()

    return sp

###############################################################################


def check_new_stds(name, type, dbif=None, overwrite=False):
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

    # Get the current mapset to create the id of the space time dataset

    mapset = get_current_mapset()
    msgr = get_tgis_message_interface()

    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        n, m = name.split("@")
        if mapset != m:
            msgr.fatal(_("Space time datasets can only be created in the "
                         "current mapset"))
        id = name

    if type == "strds" or type == "rast" or type == "raster":
        if name.find('.') > -1:
            # a dot is used as a separator for band reference filtering
            msgr.fatal(_("Illegal dataset name <{}>. "
                         "Character '.' not allowed.").format(name))
        sp = dataset_factory("strds", id)
    elif type == "str3ds" or type == "raster3d" or type == "rast3d " or type == "raster_3d":
        sp = dataset_factory("str3ds", id)
    elif type == "stvds" or type == "vect" or type == "vector":
        sp = dataset_factory("stvds", id)
    else:
        msgr.error(_("Unknown type: %s") % (type))
        return None

    dbif, connected = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite is False:
        msgr.fatal(_("Space time %(sp)s dataset <%(name)s> is already in the"
                     " database. Use the overwrite flag.") % {
                   'sp': sp.get_new_map_instance(None).get_type(),
                   'name': name})
    if connected:
        dbif.close()

    return sp

###############################################################################


def open_new_stds(name, type, temporaltype, title, descr, semantic,
                  dbif=None, overwrite=False):
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
    dbif, connected = init_dbif(dbif)
    msgr = get_tgis_message_interface()
    sp = check_new_stds(name, type, dbif, overwrite)

    if sp.is_in_db(dbif):
        msgr.warning(_("Overwriting space time %(sp)s dataset <%(name)s> and "
                       "unregistering all maps") % {
                     'sp': sp.get_new_map_instance(None).get_type(),
                     'name': name})
        id = sp.get_id()
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    msgr.verbose(_("Creating a new space time %s dataset") %
                 sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic,
                          title=title, description=descr)

    sp.insert(dbif)

    if connected:
        dbif.close()

    return sp

############################################################################


def check_new_map_dataset(name, layer=None, type="raster",
                          overwrite=False, dbif=None):
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

    dbif, connected = init_dbif(dbif)
    map_id = AbstractMapDataset.build_id(name, mapset, layer)

    new_map = dataset_factory(type, map_id)
    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        if not overwrite:
            if connected:
                dbif.close()
            msgr.fatal(_("Map <%s> is already in temporal database,"
                         " use overwrite flag to overwrite") % (map_id))

    if connected:
        dbif.close()

    return new_map

############################################################################


def open_new_map_dataset(name, layer=None, type="raster",
                         temporal_extent=None, overwrite=False,
                         dbif=None):
    """Create a new map dataset object of a specific type that can be
        registered in the temporal database

       :param name: The name of the new map dataset
       :param layer: The layer of the new map dataset
       :param type: The type of the new map dataset (raster, vector, raster3d)
       :param dbif: The temporal database interface to be used
       :param overwrite: Flag to allow overwriting

       :return: A map dataset object

    """

    mapset = get_current_mapset()

    dbif, connected = init_dbif(dbif)
    new_map = check_new_map_dataset(name, layer, type, overwrite, dbif)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        # Remove the existing temporal database entry
        map_id = new_map.get_id()
        new_map.delete(dbif)
        new_map = new_map.get_new_instance(map_id)

    if temporal_extent:
        new_map.set_temporal_extent(temporal_extent)

    if connected:
        dbif.close()

    return new_map
