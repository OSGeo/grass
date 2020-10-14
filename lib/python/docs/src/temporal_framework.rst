GRASS GIS Temporal Framework
================================

Introduction
------------

The GRASS GIS Temporal Framework implements the temporal GIS functionality of GRASS GIS
and provides an API to implement spatio-temporal processing modules. The framework 
introduces space time datasets that represent time series of raster, 3D raster or vector maps.
This framework provides the following functionalities:

- Assign time stamp to maps and register maps in the temporal database
- Modification of time stamps
- Creation, renaming and deletion of space time datasets
- Registration and un-registration of maps in space time datasets
- Query of maps that are registered in space time datasets using SQL where statements
- Analysis of the spatio-temporal topology of space time datasets
- Sampling of space time datasets
- Computation of temporal and spatial relationships between registered maps
- Higher level functions that are shared between modules

Most of the functions described above are member functions of dedicated map layer and space time dataset classes.

Temporal API
------------

The temporal framework API consists of several dedicated modules. Each module contains one or several classes
as well as function definition. The API can be roughly divided in a low level and high level part. However, several
functions from the low level module are used in module implementation as well.

Low level API
^^^^^^^^^^^^^

The low level API implements the core functionality of the temporal framework. Core functionality is
for example the database interface, the temporal database creation and initialization,
the SQL object serialization, all classes that represent table entries, datetime mathematics and many more.


:mod:`~temporal.core`
"""""""""""""""""""""

    The core functionality of the temporal framework:

    - Initialization function :func:`~temporal.core.init()`
    - Definition of global variables
    - Several global functions to access TGIS specific variables
    - Interfaces to the TGIS C-library and PyGRASS messenger objects
    - Database interface connection class :class:`~temporal.core.SQLDatabaseInterfaceConnection` 
      to sqlite3 and postgresql database backends
    - Functions to create the temporal database

:mod:`~temporal.base`
"""""""""""""""""""""

    Implements of basic dataset information and SQL conversion of such information:

    - Definition of the SQL serialize class :class:`~temporal.base.DictSQLSerializer` 
      that converts the content of temporal
      classes into SQL SELECT, INSERT or UPDATE statements
    - Definition of :class:`~temporal.base.SQLDatabaseInterface` 
      that is the base class for all temporal datatype subclasses
    - Contains classes for all datasets [#allds]_ that contain
      basic information (id, name, mapset, creator, ...)

:mod:`~temporal.spatial_extent`
"""""""""""""""""""""""""""""""

    Implements of 2d and 3d spatial extents of all datasets:

    - Implements class :class:`~temporal.spatial_extent.SpatialExtent` 
      that is the base class for all dataset specific spatial extent classes
      It provides spatial topological logic and operations for 2D and 3D extents
    - Implements spatial extent classes for all datasets [#allds]_

:mod:`~temporal.temporal_extent`
""""""""""""""""""""""""""""""""

    Implements of the temporal extent of all datasets for relative and absolute time:

    - Implements class :class:`~temporal.temporal_extent.TemporalExtent` 
      that is the base class for all dataset specific temporal extent classes
      It provides temporal topological logic and operations
    - Implements temporal extent classes for relative time and absolute time for
      all datasets [#allds]_

:mod:`~temporal.metadata`
"""""""""""""""""""""""""

    Implements the metadata base classes and datatype specific derivatives fpr all datasets [#allds]_.

:mod:`~temporal.spatial_topology_dataset_connector`
"""""""""""""""""""""""""""""""""""""""""""""""""""

    Implements the interface to link datasets by spatial topological relations

:mod:`~temporal.temporal_topology_dataset_connector`
""""""""""""""""""""""""""""""""""""""""""""""""""""

    Implements the interface to link datasets by temporal topological relations

:mod:`~temporal.c_libraries_interface`
""""""""""""""""""""""""""""""""""""""

    The RPC C-library interface for exit safe and fast access to raster, vector and 3D raster information.

:mod:`~temporal.temporal_granularity`
"""""""""""""""""""""""""""""""""""""

    The computation of the temporal granularity for a list
    of :class:`~temporal.abstract_dataset.AbstractDataset` 
    objects for absolute and relative is implemented here.

:mod:`~temporal.datetime_math`
""""""""""""""""""""""""""""""

    This module contains function to parse, convert and process datetime objects
    in the temporal framework.

Spatio-temporal algebra classes for space time raster and vector datasets are defined in:

- :mod:`~temporal.temporal_algebra`
- :mod:`~temporal.temporal_operator`
- :mod:`~temporal.temporal_raster_base_algebra`
- :mod:`~temporal.temporal_raster_algebra`
- :mod:`~temporal.temporal_raster3d_algebra`
- :mod:`~temporal.temporal_vector_algebra`

High level API
^^^^^^^^^^^^^^

The high level API utilizes the low level API. Its classes and functions are usually used to implement
temporal processing algorithms and temporal GRASS modules.

:mod:`~temporal.abstract_dataset`
"""""""""""""""""""""""""""""""""

    - Implements the base class for all datasets [#allds]_ :class:`~temporal.abstract_dataset.AbstractDataset`.
    - Implements the the select, insert and update functionality as well as
      convenient functions to access the base, extent and metadata information

:mod:`~temporal.abstract_map_dataset`
"""""""""""""""""""""""""""""""""""""

    - Implements the base class :class:`~temporal.abstract_map_dataset.AbstractMapDataset` 
      for all map layer specific classes
    - Provides the interface to all map layer specific information in the temporal database

:mod:`~temporal.abstract_space_time_dataset`
""""""""""""""""""""""""""""""""""""""""""""

    - Implements the base class :class:`~temporal.abstract_space_time_dataset.AbstractSpaceTimeDataset` 
      for all Space Time Datasets classes
    - Contains the creation and deletion functionality, the map registration and un-registration,
      access methods to map layer objects and so on
    - Provides the interface to all Space Time Dataset specific information in the temporal database

:mod:`~temporal.space_time_datasets`
""""""""""""""""""""""""""""""""""""

    This module contains all classes that represent specific datasets [#allds]_. 
    A module developer uses these map layer and Space Time Dataset object 
    representations to perform spatio-temporal tasks.

:mod:`~temporal.spatio_temporal_relationships`
""""""""""""""""""""""""""""""""""""""""""""""

    The logic to compute spatio-temporal topology for a single list or two lists of :class:`~temporal.abstract_dataset.AbstractDataset` objects
    is implemented in this module.
    The class :class:`~temporal.spatio_temporal_relationships.SpatioTemporalTopologyBuilder` 
    provides a convenient interface for topology computation.

:mod:`~temporal.gui_support`
""""""""""""""""""""""""""""

    Helper functions to support the listing of space time datasets in the automatically generated GUI.


Shared Module functionality
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Functionality that is shared between different temporal GRASS modules, such as
map listing, space time dataset creation, map registration and un-registration, 
aggregation, extraction, map calculation, statistics as well as import and export of 
space time datasets.

:mod:`~temporal.aggregation`
""""""""""""""""""""""""""""

    Aggregation of Space Time Raster Datasets based on topological relations. 
    Used in *t.rast.aggregate* and *t.rast.aggregate.ds*

:mod:`~temporal.extract`
""""""""""""""""""""""""

    Extraction of subsets from Space Time Datasets including 
    map algebra and vector selection statements. 
    Used in *t.rast.extract*, *t.rast3d.extract* and *t.vect.extract*.

:mod:`~temporal.factory`
""""""""""""""""""""""""

    Factory functions to create datasets of all types [#allds]_.

:mod:`~temporal.open_stds`
""""""""""""""""""""""""""

    Convenient functions to open existing Space Time Datasets or 
    to create new ones. Used in almost all temporal modules.

:mod:`~temporal.list_stds`
""""""""""""""""""""""""""

    Convenient functions to list datasets of all types [#allds]_ 
    registered in the temporal database.

:mod:`~temporal.mapcalc`
""""""""""""""""""""""""

    Simple temporal algebra for Space Time Raster and 3d Raster 
    datasets. Used in *t.rast.mapcalc* and *t.rast3d.mapcalc*

:mod:`~temporal.register`
"""""""""""""""""""""""""

    Convenient functions to register a single or multiple map layer in the temporal database and
    SpaceTime Datasets. Used in several modules, most important *t.register*.

:mod:`~temporal.sampling`
"""""""""""""""""""""""""

    Sampling functions used in several modules.

:mod:`~temporal.stds_export`
""""""""""""""""""""""""""""

    Functions to export of Space Time Datasets, used in *t.rast.export*, *t.rast3d.export* and *t.vect.export*.

:mod:`~temporal.stds_import`
""""""""""""""""""""""""""""

    Functions to import Space Time Datasets, used in *t.rast.import*, *t.rast3d.import* and *t.vect.import*.

:mod:`~temporal.univar_statistics`
""""""""""""""""""""""""""""""""""

    Simple statistical analysis functions for Space Time Datasets, used in *t.rast.univar*, *t.rast3d.univar*
    and *t.vect.univar*.


.. [#allds] : Raster Map Layer, 3d Raster Map Layer, Vector Map Layer, Space time Raster Datasets (STRDS),
              Space Time 3d Raster Datasets (STR3DS) and Space Time Vector Datasets (STVDS)


Here the full list of all temporal modules:

.. toctree::
   :maxdepth: 2

   temporal


Examples
--------

Howto start example
^^^^^^^^^^^^^^^^^^^

This simple example shows how to open a space time raster dataset
to access its registered maps.

.. code-block:: python

    # Lets import the temporal framework and
    # the script framework
    import grass.temporal as tgis
    import grass.script as gs

    # Make sure the temporal database exists
    # and set the temporal GIS environment
    tgis.init()

    # We create the temporal database interface for fast processing
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    # The id of a space time raster dataset is build from its name and its mapset
    id = "test@PERMANENT"

    # We create a space time raster dataset object 
    strds = tgis.SpaceTimeRasterDataset(id)

    # Check if the space time raster dataset is in the temporal database
    if strds.is_in_db(dbif=dbif) == False:
        dbif.close()
        gs.fatal(_("Space time %s dataset <%s> not found") % (
            strds.get_new_map_instance(None).get_type(), id))

    # Fill the object with the content from the temporal database
    strds.select(dbif=dbif)

    # Print information about the space time raster dataset to stdout
    strds.print_info()

    # Get all maps that are registered in the strds and print
    # information about the maps to stdout
    maps = strds.get_registered_maps_as_objects(dbif=dbif)

    # We iterate over the temporal sorted map list
    for map in maps:
        # We fill the map object with the content 
        # from the temporal database. We use the existing
        # database connection, otherwise a new connection 
        # will be established for each map object 
        # which slows the processing down
        map.select(dbif=dbif)
        map.print_info()

    # Close the database connection
    dbif.close()


Creation of a space time dataset
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows howto create a space time dataset. The code is generic and works
for different space time datasets (raster, 3D raster and vector): 

.. code-block:: python

    # Lets import the temporal framework and
    # the script framework
    import grass.temporal as tgis
    import grass.script as gs

    # The id of the new space time dataset
    id="test@PERMANENT"
    # The title of the new space time dataset
    title="This is a test dataset" 
    # The description of the space time dataset
    description="The description"
    # The type of the space time dataset (strds, str3ds or stvds)
    type="strds"
    # The temporal type of the space time dataset (absolute or relative)
    temporal_type="absolute"

    # Make sure the temporal database exists
    # and set the temporal GIS environment
    tgis.init()

    # We use the dataset factory to create an new space time dataset instance of a specific type
    stds = tgis.dataset_factory(type, id)

    # We need a dtabase connection to insert the content of the space time dataset
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    # First we check if the dataset is already in the database
    if stds.is_in_db(dbif=dbif) and overwrite == False:
        dbif.close()
        gs.fatal(_("Space time %s dataset <%s> is already in the database. "
                   "Use the overwrite flag.") %
                 (stds.get_new_map_instance(None).get_type(), name))

    # We delete the exiting dataset and create a new one in case we are allowed to overwrite it
    if stds.is_in_db(dbif=dbif) and overwrite == True:
        gs.warning(_("Overwrite space time %s dataset <%s> "
                     "and unregister all maps.") %
                   (stds.get_new_map_instance(None).get_type(), name))
        stds.delete(dbif=dbif)
        stds = stds.get_new_instance(id)

    # We set the initial values. This function also created the command history.
    stds.set_initial_values(temporal_type=temporaltype, semantic_type="mean",
                            title=title, description=description)
                            
    # Now we can insert the new space time dataset in the database
    stds.insert(dbif=dbif)

    # Close the database connection
    dbif.close()


Temporal shifting
^^^^^^^^^^^^^^^^^

.. code-block:: python

    import grass.script as gs
    import grass.temporal as tgis

    id="test@PERMANENT"
    type="strds"

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.dataset_factory(type, id)

    if stds.is_in_db(dbif) == False:
        dbif.close()
        gs.fatal(_("Space time dataset <%s> not found in temporal database") % (id))

    stds.select(dbif=dbif)

    stds.snap(dbif=dbif)

    stds.update_command_string(dbif=dbif)
    dbif.close()


References
----------

* Gebbert, S., Pebesma, E., 2014. *TGRASS: A temporal GIS for field based environmental modeling*. Environmental Modelling & Software. 2(1):201-219. `doi:10.1016/j.envsoft.2013.11.001 <http://dx.doi.org/10.1016/j.envsoft.2013.11.001>`_
* `TGRASS related articles in the GRASS GIS Wiki <https://grasswiki.osgeo.org/wiki/Temporal_data_processing>`_
* Supplementary material of the publication *The GRASS GIS Temporal Framework*
  to be published in
  *International Journal of Geographical Information Science* in 2017, Download  
  :download:`The GRASS GIS Temporal Framework: Object oriented code design with examples <Temporal-Framework-API-Description.pdf>`

:Authors: Soeren Gebbert

:TODO: add more documentation

