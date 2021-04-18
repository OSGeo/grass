"""
The abstract_space_time_dataset module provides the AbstractSpaceTimeDataset
class that is the base class for all space time datasets.

(C) 2011-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function
import sys
import uuid
import os
import copy
from datetime import datetime
from abc import ABCMeta, abstractmethod
from .core import (
    init_dbif,
    get_sql_template_path,
    get_tgis_metadata,
    get_current_mapset,
    get_enable_mapset_check,
)
from .abstract_dataset import AbstractDataset, AbstractDatasetComparisonKeyStartTime
from .temporal_granularity import (
    check_granularity_string,
    compute_absolute_time_granularity,
    compute_relative_time_granularity,
)
from .spatio_temporal_relationships import (
    count_temporal_topology_relationships,
    print_spatio_temporal_topology_relationships,
    SpatioTemporalTopologyBuilder,
    create_temporal_relation_sql_where_statement,
)
from .datetime_math import increment_datetime_by_string, string_to_datetime

###############################################################################


class AbstractSpaceTimeDataset(AbstractDataset):
    """Abstract space time dataset class

    Base class for all space time datasets.

    This class represents an abstract space time dataset. Convenient functions
    to select, update, insert or delete objects of this type in the SQL
    temporal database exists as well as functions to register or unregister
    raster maps.

    Parts of the temporal logic are implemented in the SQL temporal
    database, like the computation of the temporal and spatial extent as
    well as the collecting of metadata.
    """

    __metaclass__ = ABCMeta

    def __init__(self, ident):
        AbstractDataset.__init__(self)
        self.reset(ident)
        self.map_counter = 0

        # SpaceTimeRasterDataset related only
        self.band_reference = None

    def get_name(self, band_reference=True):
        """Get dataset name including band reference filter if enabled.

        :param bool band_reference: True to return dataset name
        including band reference filter if defined
        (eg. "landsat.L8_1") otherwise dataset name is returned only
        (eg. "landsat").

        :return str: dataset name

        """
        dataset_name = super(AbstractSpaceTimeDataset, self).get_name()

        if band_reference and self.band_reference:
            return "{}.{}".format(dataset_name, self.band_reference)

        return dataset_name

    def create_map_register_name(self):
        """Create the name of the map register table of this space time
        dataset

        A uuid and the map type are used to create the table name

        ATTENTION: It must be assured that the base object has selected its
        content from the database.

        :return: The name of the map register table
        """

        uuid_rand = str(uuid.uuid4()).replace("-", "")

        table_name = (
            self.get_new_map_instance(None).get_type() + "_map_register_" + uuid_rand
        )
        return table_name

    @abstractmethod
    def get_new_map_instance(self, ident=None):
        """Return a new instance of a map which is associated
        with the type of this object

        :param ident: The unique identifier of the new object
        """

    @abstractmethod
    def get_map_register(self):
        """Return the name of the map register table
        :return: The map register table name
        """

    @abstractmethod
    def set_map_register(self, name):
        """Set the name of the map register table

        This table stores all map names which are registered
        in this space time dataset.

        This method only modifies this object and does not commit
        the modifications to the temporal database.

        :param name: The name of the register table
        """

    def print_self(self):
        """Print the content of the internal structure to stdout"""
        self.base.print_self()
        self.temporal_extent.print_self()
        self.spatial_extent.print_self()
        self.metadata.print_self()

    def print_info(self):
        """Print information about this class in human readable style"""

        if self.get_type() == "strds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print(
                " +-------------------- Space Time Raster Dataset -----------------------------+"
            )
        if self.get_type() == "str3ds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print(
                " +-------------------- Space Time 3D Raster Dataset --------------------------+"
            )
        if self.get_type() == "stvds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print(
                " +-------------------- Space Time Vector Dataset -----------------------------+"
            )
        print(
            " |                                                                            |"
        )
        self.base.print_info()
        self.temporal_extent.print_info()
        self.spatial_extent.print_info()
        self.metadata.print_info()
        print(
            " +----------------------------------------------------------------------------+"
        )

    def print_shell_info(self):
        """Print information about this class in shell style"""
        self.base.print_shell_info()
        self.temporal_extent.print_shell_info()
        self.spatial_extent.print_shell_info()
        self.metadata.print_shell_info()

    def print_history(self):
        """Print history information about this class in human readable
        shell style
        """
        self.metadata.print_history()

    def set_initial_values(
        self, temporal_type, semantic_type=None, title=None, description=None
    ):
        """Set the initial values of the space time dataset

         In addition the command creation string is generated
         an inserted into the metadata object.

         This method only modifies this object and does not commit
         the modifications to the temporal database.

         The insert() function must be called to commit
         this content into the temporal database.

        :param temporal_type: The temporal type of this space
                             time dataset (absolute or relative)
        :param semantic_type: The semantic type of this dataset
        :param title: The title
        :param description: The description of this dataset
        """

        if temporal_type == "absolute":
            self.base.set_ttype("absolute")
        elif temporal_type == "relative":
            self.base.set_ttype("relative")
        else:
            self.msgr.fatal(_('Unknown temporal type "%s"') % (temporal_type))

        self.base.set_semantic_type(semantic_type)
        self.metadata.set_title(title)
        self.metadata.set_description(description)
        self.metadata.set_command(self.create_command_string())

    def set_aggregation_type(self, aggregation_type):
        """Set the aggregation type of the space time dataset

        :param aggregation_type: The aggregation type of the space time
                                 dataset
        """
        self.metadata.set_aggregation_type(aggregation_type)

    def update_command_string(self, dbif=None):
        """Append the current command string to any existing command string
        in the metadata class and calls metadata update

        :param dbif: The database interface to be used
        """

        self.metadata.select(dbif=dbif)
        command = self.metadata.get_command()
        if command is None:
            command = ""
        command += self.create_command_string()
        self.metadata.set_command(command)
        self.metadata.update(dbif=dbif)

    def create_command_string(self):
        """Create the command string that was used to create this
        space time dataset.

        The command string should be set with self.metadata.set_command()

        :return: The command string
        """
        # The grass module

        command = "# %s \n" % (str(datetime.today().strftime("%Y-%m-%d %H:%M:%S")))
        command += os.path.basename(sys.argv[0])

        # We will wrap the command line to fit into 80 character
        length = len(command)
        for token in sys.argv[1:]:

            # We need to remove specific characters
            token = token.replace("'", " ")
            token = token.replace('"', " ")

            # Check for sub strings
            if token.find("=") > 0:
                first = token.split("=")[0]
                second = ""

                flag = 0
                for t in token.split("=")[1:]:
                    if flag == 0:
                        second += t
                        flag = 1
                    else:
                        second += "=" + t

                token = '%s="%s"' % (first, second)

            if length + len(token) >= 76:
                command += "\n    %s" % (token)
                length = len(token) + 4
            else:
                command += " %s" % (token)
                length += len(token) + 1

        command += "\n"
        return str(command)

    def get_semantic_type(self):
        """Return the semantic type of this dataset
        :return: The semantic type
        """
        return self.base.get_semantic_type()

    def get_initial_values(self):
        """Return the initial values: temporal_type,
        semantic_type, title, description"""

        temporal_type = self.get_temporal_type()
        semantic_type = self.base.get_semantic_type()
        title = self.metadata.get_title()
        description = self.metadata.get_description()

        return temporal_type, semantic_type, title, description

    def get_granularity(self):
        """Return the granularity of the space time dataset

        Granularity can be of absolute time or relative time.
        In case of absolute time a string containing an integer
        value and the time unit (years, months, days, hours, minuts,
        seconds). In case of relative time an integer value is expected.

        :return: The granularity
        """

        return self.temporal_extent.get_granularity()

    def set_granularity(self, granularity):
        """Set the granularity

        The granularity is usually computed by the space time dataset at
        runtime.

        Granularity can be of absolute time or relative time.
        In case of absolute time a string containing an integer
        value and the time unit (years, months, days, hours, minuts,
        seconds). In case of relative time an integer value is expected.

        This method only modifies this object and does not commit
        the modifications to the temporal database.

        :param granularity: The granularity of the dataset
        """

        temporal_type = self.get_temporal_type()

        check = check_granularity_string(granularity, temporal_type)
        if not check:
            self.msgr.fatal(_('Wrong granularity: "%s"') % str(granularity))

        if temporal_type == "absolute":
            self.base.set_ttype("absolute")
        elif temporal_type == "relative":
            self.base.set_ttype("relative")
        else:
            self.msgr.fatal(_('Unknown temporal type "%s"') % (temporal_type))

        self.temporal_extent.set_granularity(granularity)

    def set_relative_time_unit(self, unit):
        """Set the relative time unit which may be of type:
        years, months, days, hours, minutes or seconds

        All maps registered in a (relative time)
        space time dataset must have the same unit

        This method only modifies this object and does not commit
        the modifications to the temporal database.

        :param unit: The relative time unit
        """

        temporal_type = self.get_temporal_type()

        if temporal_type == "relative":
            if not self.check_relative_time_unit(unit):
                self.msgr.fatal(_("Unsupported temporal unit: %s") % (unit))
            self.relative_time.set_unit(unit)

    def insert(self, dbif=None, execute=True):
        """Insert the space time dataset content into the database from the internal
        structure

        The map register table will be created, so that maps
        can be registered.

        :param dbif: The database interface to be used
        :param execute: If True the SQL statements will be executed.
                        If False the prepared SQL statements are
                        returned and must be executed by the caller.
        :return: The SQL insert statement in case execute=False, or an
                 empty string otherwise
        """

        dbif, connection_state_changed = init_dbif(dbif)

        # We need to create the register table if it does not exist
        stds_register_table = self.get_map_register()

        # Create the map register table
        sql_path = get_sql_template_path()
        statement = ""

        # We need to create the map register table
        if stds_register_table is None:
            # Create table name
            stds_register_table = self.create_map_register_name()
            # Assure that the table and index do not exist
            # dbif.execute_transaction("DROP INDEX IF EXISTS %s; DROP TABLE IF EXISTS   %s;"%(stds_register_table + "_index", stds_register_table))

            # Read the SQL template
            sql = open(
                os.path.join(sql_path, "stds_map_register_table_template.sql"), "r"
            ).read()

            # Create a raster, raster3d or vector tables
            sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
            statement += sql

            if dbif.get_dbmi().__name__ == "sqlite3":
                statement += "CREATE INDEX %s_index ON %s (id);" % (
                    stds_register_table,
                    stds_register_table,
                )

            # Set the map register table name
            self.set_map_register(stds_register_table)

            self.msgr.debug(
                1,
                _("Created register table <%s> for space " "time %s  dataset <%s>")
                % (
                    stds_register_table,
                    self.get_new_map_instance(None).get_type(),
                    self.get_id(),
                ),
            )

        statement += AbstractDataset.insert(self, dbif=dbif, execute=False)

        if execute:
            dbif.execute_transaction(statement)
            statement = ""

        if connection_state_changed:
            dbif.close()

        return statement

    def get_map_time(self):
        """Return the type of the map time, interval, point, mixed or invalid"""

        return self.temporal_extent.get_map_time()

    def count_temporal_types(self, maps=None, dbif=None):
        """Return the temporal type of the registered maps as dictionary

        The map list must be ordered by start time

        The temporal type can be:

        - point    -> only the start time is present
        - interval -> start and end time
        - invalid  -> No valid time point or interval found

        :param maps: A sorted (start_time) list of AbstractDataset objects
        :param dbif: The database interface to be used
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif
            )

        time_invalid = 0
        time_point = 0
        time_interval = 0

        tcount = {}
        for i in range(len(maps)):
            # Check for point and interval data
            if maps[i].is_time_absolute():
                start, end = maps[i].get_absolute_time()
            if maps[i].is_time_relative():
                start, end, unit = maps[i].get_relative_time()

            if start is not None and end is not None:
                time_interval += 1
            elif start is not None and end is None:
                time_point += 1
            else:
                time_invalid += 1

        tcount["point"] = time_point
        tcount["interval"] = time_interval
        tcount["invalid"] = time_invalid

        return tcount

    def count_gaps(self, maps=None, dbif=None):
        """Count the number of gaps between temporal neighbors

        :param maps: A sorted (start_time) list of AbstractDataset objects
        :param dbif: The database interface to be used
        :return: The numbers of gaps between temporal neighbors
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif
            )

        gaps = 0

        # Check for gaps
        for i in range(len(maps)):
            if i < len(maps) - 1:
                relation = maps[i + 1].temporal_relation(maps[i])
                if relation == "after":
                    gaps += 1

        return gaps

    def print_spatio_temporal_relationships(self, maps=None, spatial=None, dbif=None):
        """Print the spatio-temporal relationships for each map of the space
        time dataset or for each map of the optional list of maps

        :param maps: a ordered by start_time list of map objects, if None
                     the registered maps of the space time dataset are used
        :param spatial: This indicates if the spatial topology is created as
                        well: spatial can be None (no spatial topology),
                        "2D" using west, east, south, north or "3D" using
                        west, east, south, north, bottom, top
        :param dbif: The database interface to be used
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif
            )

        print_spatio_temporal_topology_relationships(
            maps1=maps, maps2=maps, spatial=spatial, dbif=dbif
        )

    def count_temporal_relations(self, maps=None, dbif=None):
        """Count the temporal relations between the registered maps.

        The map list must be ordered by start time.
        Temporal relations are counted by analysing the sparse upper right
        side temporal relationships matrix.

        :param maps: A sorted (start_time) list of AbstractDataset objects
        :param dbif: The database interface to be used
        :return: A dictionary with counted temporal relationships
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif
            )

        return count_temporal_topology_relationships(maps1=maps, dbif=dbif)

    def check_temporal_topology(self, maps=None, dbif=None):
        """Check the temporal topology of all maps of the current space time
        dataset or of an optional list of maps

        Correct topology means, that time intervals are not overlap or
        that intervals does not contain other intervals.
        Equal time intervals  are not allowed.

        The optional map list must be ordered by start time

        Allowed and not allowed temporal relationships for correct topology:

        - after      -> allowed
        - precedes   -> allowed
        - follows    -> allowed
        - precedes   -> allowed

        - equal      -> not allowed
        - during     -> not allowed
        - contains   -> not allowed
        - overlaps   -> not allowed
        - overlapped -> not allowed
        - starts     -> not allowed
        - finishes   -> not allowed
        - started    -> not allowed
        - finished   -> not allowed


        :param maps: An optional list of AbstractDataset objects, in case of
                    None all maps of the space time dataset are checked
        :param dbif: The database interface to be used
        :return: True if topology is correct
        """
        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif
            )

        relations = count_temporal_topology_relationships(maps1=maps, dbif=dbif)

        if relations is None:
            return False

        map_time = self.get_map_time()

        if map_time == "interval" or map_time == "mixed":
            if "equal" in relations and relations["equal"] > 0:
                return False
            if "during" in relations and relations["during"] > 0:
                return False
            if "contains" in relations and relations["contains"] > 0:
                return False
            if "overlaps" in relations and relations["overlaps"] > 0:
                return False
            if "overlapped" in relations and relations["overlapped"] > 0:
                return False
            if "starts" in relations and relations["starts"] > 0:
                return False
            if "finishes" in relations and relations["finishes"] > 0:
                return False
            if "started" in relations and relations["started"] > 0:
                return False
            if "finished" in relations and relations["finished"] > 0:
                return False
        elif map_time == "point":
            if "equal" in relations and relations["equal"] > 0:
                return False
        else:
            return False

        return True

    def sample_by_dataset(self, stds, method=None, spatial=False, dbif=None):
        """Sample this space time dataset with the temporal topology
        of a second space time dataset

        In case spatial is True, the spatial overlap between
        temporal related maps is performed. Only
        temporal related and spatial overlapping maps are returned.

        Return all registered maps as ordered (by start_time) object list.
        Each list entry is a list of map
        objects which are potentially located in temporal relation to the
        actual granule of the second space time dataset.

        Each entry in the object list is a dict. The actual sampler
        map and its temporal extent (the actual granule) and
        the list of samples are stored:

        .. code-block:: python

            list = self.sample_by_dataset(stds=sampler, method=[
                "during","overlap","contains","equal"])
            for entry in list:
                granule = entry["granule"]
                maplist = entry["samples"]
                for map in maplist:
                    map.select()
                    map.print_info()


        A valid temporal topology (no overlapping or inclusion allowed)
        is needed to get correct results in case of gaps in the sample
        dataset.

        Gaps between maps are identified as unregistered maps with id==None.

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param stds: The space time dataset to be used for temporal sampling
        :param method: This option specifies what sample method should be
                      used. In case the registered maps are of temporal
                      point type, only the start time is used for sampling.
                      In case of mixed of interval data the user can chose
                      between:

               - Example ["start", "during", "equals"]

               - start: Select maps of which the start time is
                 located in the selection granule::

                     map    :        s
                     granule:  s-----------------e

                     map    :        s--------------------e
                     granule:  s-----------------e

                     map    :        s--------e
                     granule:  s-----------------e

               - contains: Select maps which are temporal
                  during the selection granule::

                     map    :     s-----------e
                     granule:  s-----------------e

               - overlap: Select maps which temporal overlap
                 the selection granule, this includes overlaps and
                 overlapped::

                     map    :     s-----------e
                     granule:        s-----------------e

                     map    :     s-----------e
                     granule:  s----------e

               - during: Select maps which temporally contains
                 the selection granule::

                     map    :  s-----------------e
                     granule:     s-----------e

               - equals: Select maps which temporally equal
                 to the selection granule::

                     map    :  s-----------e
                     granule:  s-----------e

               - follows: Select maps which temporally follow
                 the selection granule::

                     map    :              s-----------e
                     granule:  s-----------e

               - precedes: Select maps which temporally precedes
                 the selection granule::

                     map    :  s-----------e
                     granule:              s-----------e

               All these methods can be combined. Method must be of
               type tuple including the identification strings.

        :param spatial: If set True additional the 2d spatial overlapping
                       is used for selection -> spatio-temporal relation.
                       The returned map objects will have temporal and
                       spatial extents
        :param dbif: The database interface to be used

        :return: A list of lists of map objects or None in case nothing was
                found None
        """

        if self.get_temporal_type() != stds.get_temporal_type():
            self.msgr.error(
                _("The space time datasets must be of " "the same temporal type")
            )
            return None

        if stds.get_map_time() != "interval":
            self.msgr.error(
                _("The temporal map type of the sample " "dataset must be interval")
            )
            return None

        dbif, connection_state_changed = init_dbif(dbif)
        relations = copy.deepcopy(method)

        # Tune the temporal relations
        if "start" in relations:
            if "overlapped" not in relations:
                relations.append("overlapped")
            if "starts" not in relations:
                relations.append("starts")
            if "started" not in relations:
                relations.append("started")
            if "finishes" not in relations:
                relations.append("finishes")
            if "contains" not in relations:
                relations.append("contains")
            if "equals" not in relations:
                relations.append("equals")

        if "overlap" in relations or "over" in relations:
            if "overlapped" not in relations:
                relations.append("overlapped")
            if "overlaps" not in relations:
                relations.append("overlaps")

        if "contain" in relations:
            if "contains" not in relations:
                relations.append("contains")

        # Remove start, equal, contain and overlap
        relations = [
            relation.upper().strip()
            for relation in relations
            if relation not in ["start", "overlap", "contain"]
        ]

        #  print(relations)

        tb = SpatioTemporalTopologyBuilder()
        if spatial:
            spatial = "2D"
        else:
            spatial = None

        mapsA = self.get_registered_maps_as_objects(dbif=dbif)
        mapsB = stds.get_registered_maps_as_objects_with_gaps(dbif=dbif)
        tb.build(mapsB, mapsA, spatial)

        obj_list = []
        for map in mapsB:
            result = {}
            maplist = []
            # Get map relations
            map_relations = map.get_temporal_relations()
            # print(map.get_temporal_extent_as_tuple())
            # for key in map_relations.keys():
            #    if key not in ["NEXT", "PREV"]:
            #        print(key, map_relations[key][0].get_temporal_extent_as_tuple())

            result["granule"] = map
            # Append the maps that fulfill the relations
            for relation in relations:
                if relation in map_relations.keys():
                    for sample_map in map_relations[relation]:
                        if sample_map not in maplist:
                            maplist.append(sample_map)

            # Add an empty map if no map was found
            if not maplist:
                empty_map = self.get_new_map_instance(None)
                empty_map.set_spatial_extent(map.get_spatial_extent())
                empty_map.set_temporal_extent(map.get_temporal_extent())
                maplist.append(empty_map)

            result["samples"] = maplist

            obj_list.append(result)

        if connection_state_changed:
            dbif.close()

        return obj_list

    def sample_by_dataset_sql(self, stds, method=None, spatial=False, dbif=None):
        """Sample this space time dataset with the temporal topology
        of a second space time dataset using SQL queries.

        This function is very slow for huge large space time datasets
        but can run several times in the same process without problems.

        The sample dataset must have "interval" as temporal map type,
        so all sample maps have valid interval time.

        In case spatial is True, the spatial overlap between
        temporal related maps is performed. Only
        temporal related and spatial overlapping maps are returned.

        Return all registered maps as ordered (by start_time) object list
        with "gap" map objects (id==None). Each list entry is a list of map
        objects which are potentially located in temporal relation to the
        actual granule of the second space time dataset.

        Each entry in the object list is a dict. The actual sampler
        map and its temporal extent (the actual granule) and
        the list of samples are stored:

        .. code-block:: python

            list = self.sample_by_dataset(stds=sampler, method=[
                "during","overlap","contain","equal"])
            for entry in list:
                granule = entry["granule"]
                maplist = entry["samples"]
                for map in maplist:
                    map.select()
                    map.print_info()


        A valid temporal topology (no overlapping or inclusion allowed)
        is needed to get correct results in case of gaps in the sample
        dataset.

        Gaps between maps are identified as unregistered maps with id==None.

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param stds: The space time dataset to be used for temporal sampling
        :param method: This option specifies what sample method should be
                      used. In case the registered maps are of temporal
                      point type, only the start time is used for sampling.
                      In case of mixed of interval data the user can chose
                      between:

               - Example ["start", "during", "equals"]

               - start: Select maps of which the start time is
                 located in the selection granule::

                     map    :        s
                     granule:  s-----------------e

                     map    :        s--------------------e
                     granule:  s-----------------e

                     map    :        s--------e
                     granule:  s-----------------e

               - contains: Select maps which are temporal
                 during the selection granule::

                     map    :     s-----------e
                     granule:  s-----------------e

               - overlap: Select maps which temporal overlap
                 the selection granule, this includes overlaps and
                 overlapped::

                     map    :     s-----------e
                     granule:        s-----------------e

                     map    :     s-----------e
                     granule:  s----------e

               - during: Select maps which temporally contains
                 the selection granule::

                     map    :  s-----------------e
                     granule:     s-----------e

               - equals: Select maps which temporally equal
                 to the selection granule::

                     map    :  s-----------e
                     granule:  s-----------e

               - follows: Select maps which temporally follow
                 the selection granule::

                     map    :              s-----------e
                     granule:  s-----------e

               - precedes: Select maps which temporally precedes
                 the selection granule::

                     map    :  s-----------e
                     granule:              s-----------e

               All these methods can be combined. Method must be of
               type tuple including the identification strings.

        :param spatial: If set True additional the 2d spatial overlapping
                       is used for selection -> spatio-temporal relation.
                       The returned map objects will have temporal and
                       spatial extents
        :param dbif: The database interface to be used

        :return: A list of lists of map objects or None in case nothing was
                found None
        """

        use_start = False
        use_during = False
        use_overlap = False
        use_contain = False
        use_equal = False
        use_follows = False
        use_precedes = False

        # Initialize the methods
        if method is not None:
            for name in method:
                if name == "start":
                    use_start = True
                if name == "during":
                    use_during = True
                if name == "overlap":
                    use_overlap = True
                if name == "contain" or name == "contains":
                    use_contain = True
                if name == "equal" or name == "equals":
                    use_equal = True
                if name == "follows":
                    use_follows = True
                if name == "precedes":
                    use_precedes = True
        else:
            use_during = True
            use_overlap = True
            use_contain = True
            use_equal = True

        if self.get_temporal_type() != stds.get_temporal_type():
            self.msgr.error(
                _("The space time datasets must be of " "the same temporal type")
            )
            return None

        if stds.get_map_time() != "interval":
            self.msgr.error(
                _("The temporal map type of the sample " "dataset must be interval")
            )
            return None

        # In case points of time are available, disable the interval specific
        # methods
        if self.get_map_time() == "point":
            use_start = True
            use_during = False
            use_overlap = False
            use_contain = False
            use_equal = False
            use_follows = False
            use_precedes = False

        dbif, connection_state_changed = init_dbif(dbif)

        obj_list = []
        sample_maps = stds.get_registered_maps_as_objects_with_gaps(
            where=None, dbif=dbif
        )

        for granule in sample_maps:
            # Read the spatial extent
            if spatial:
                granule.spatial_extent.select(dbif)
            start, end = granule.get_temporal_extent_as_tuple()

            where = create_temporal_relation_sql_where_statement(
                start,
                end,
                use_start,
                use_during,
                use_overlap,
                use_contain,
                use_equal,
                use_follows,
                use_precedes,
            )

            maps = self.get_registered_maps_as_objects(where, "start_time", dbif)

            result = {}
            result["granule"] = granule
            num_samples = 0
            maplist = []

            if maps is not None:
                for map in maps:
                    # Read the spatial extent
                    if spatial:
                        map.spatial_extent.select(dbif)
                        # Ignore spatial disjoint maps
                        if not granule.spatial_overlapping(map):
                            continue

                    num_samples += 1
                    maplist.append(copy.copy(map))

            # Fill with empty map in case no spatio-temporal relations found
            if maps is None or num_samples == 0:
                map = self.get_new_map_instance(None)

                if self.is_time_absolute():
                    map.set_absolute_time(start, end)
                elif self.is_time_relative():
                    map.set_relative_time(start, end, self.get_relative_time_unit())

                maplist.append(copy.copy(map))

            result["samples"] = maplist

            obj_list.append(copy.copy(result))

        if connection_state_changed:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects_by_granularity(self, gran=None, dbif=None):
        """Return all registered maps as ordered (by start_time) object list
        with "gap" map objects (id==None) for spatio-temporal topological
        operations that require the temporal extent only.

        Each list entry is a list of AbstractMapDatasets objects
        which are potentially equal the actual granule, contain the
        actual granule or are located in the actual granule.
        Hence for each granule a list of AbstractMapDatasets can be
        expected.

        Maps that overlap the granule are ignored.

        The granularity of the space time dataset is used as increment in
        case the granule is not user defined.

        A valid temporal topology (no overlapping or inclusion allowed)
        is needed to get correct results.

        Space time datasets with interval time, time instances and mixed
        time are supported.

        Gaps between maps are identified as unregistered maps with id==None.

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param gran: The granularity string to be used, if None the
                    granularity of the space time dataset is used.
                    Absolute time has the format "number unit", relative
                    time has the format "number".
                    The unit in case of absolute time can be one of "second,
                    seconds, minute, minutes, hour, hours, day, days, week,
                    weeks, month, months, year, years". The unit of the
                    relative time granule is always the space time dataset
                    unit and can not be changed.
        :param dbif: The database interface to be used

        :return: ordered list of map lists. Each list represents a single
                granule, or None in case nothing found
        """

        dbif, connection_state_changed = init_dbif(dbif)

        if gran is None:
            gran = self.get_granularity()

        check = check_granularity_string(gran, self.get_temporal_type())
        if not check:
            self.msgr.fatal(_('Wrong granularity: "%s"') % str(gran))

        start, end = self.get_temporal_extent_as_tuple()

        if start is None or end is None:
            return None

        maps = self.get_registered_maps_as_objects(dbif=dbif, order="start_time")

        if not maps:
            return None

        # We need to adjust the end time in case the the dataset has no
        # interval time, so we can catch time instances at the end
        if self.get_map_time() != "interval":
            if self.is_time_absolute():
                end = increment_datetime_by_string(end, gran)
            else:
                end = end + gran

        maplist = AbstractSpaceTimeDataset.resample_maplist_by_granularity(
            maps, start, end, gran
        )
        if connection_state_changed:
            dbif.close()

        return maplist

    @staticmethod
    def resample_maplist_by_granularity(maps, start, end, gran):
        """Resample a list of AbstractMapDatasets by a given granularity

        The provided map list must be sorted by start time.
        A valid temporal topology (no overlapping or inclusion allowed)
        is needed to receive correct results.

        Maps with interval time, time instances and mixed
        time are supported.

        The temporal topology search order is as follows:

        1. Maps that are equal to the actual granule are used
        2. If no euqal found then maps that contain the actual granule
           are used
        3. If no maps are found that contain the actual granule then maps
           are used that overlaps the actual granule
        4. If no overlaps maps found then overlapped maps are used
        5. If no overlapped maps are found then maps are used that are
           durin the actual granule


        Each entry in the resulting list is a list of
        AbstractMapDatasets objects.
        Hence for each granule a list of AbstractMapDatasets can be
        expected.

        Gaps between maps are identified as unregistered maps with id==None.

        :param maps: An ordered list (by start time) of AbstractMapDatasets
                objects. All maps must have the same temporal type
                and the same unit in case of relative time.
        :param start: The start time of the provided map list
        :param end:   The end time of the provided map list
        :param gran: The granularity string to be used, if None the
                granularity of the space time dataset is used.
                Absolute time has the format "number unit", relative
                time has the format "number".
                The unit in case of absolute time can be one of "second,
                seconds, minute, minutes, hour, hours, day, days, week,
                weeks, month, months, year, years". The unit of the
                relative time granule is always the space time dataset
                unit and can not be changed.

        :return: ordered list of map lists. Each list represents a single
            granule, or None in case nothing found

        Usage:

         .. code-block:: python

             >>> import grass.temporal as tgis
             >>> maps = []
             >>> for i in range(3):
             ...     map = tgis.RasterDataset("map%i@PERMANENT"%i)
             ...     check = map.set_relative_time(i + 2, i + 3, "days")
             ...     maps.append(map)
             >>> grans = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(maps,0,8,1)
             >>> for map_list in grans:
             ...    print(map_list[0].get_id(), map_list[0].get_temporal_extent_as_tuple())
             None (0, 1)
             None (1, 2)
             map0@PERMANENT (2, 3)
             map1@PERMANENT (3, 4)
             map2@PERMANENT (4, 5)
             None (5, 6)
             None (6, 7)
             None (7, 8)

             >>> maps = []
             >>> map1 = tgis.RasterDataset("map1@PERMANENT")
             >>> check = map1.set_relative_time(2, 6, "days")
             >>> maps.append(map1)
             >>> map2 = tgis.RasterDataset("map2@PERMANENT")
             >>> check = map2.set_relative_time(7, 13, "days")
             >>> maps.append(map2)
             >>> grans = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(maps,0,16,2)
             >>> for map_list in grans:
             ...    print(map_list[0].get_id(), map_list[0].get_temporal_extent_as_tuple())
             None (0, 2)
             map1@PERMANENT (2, 4)
             map1@PERMANENT (4, 6)
             map2@PERMANENT (6, 8)
             map2@PERMANENT (8, 10)
             map2@PERMANENT (10, 12)
             map2@PERMANENT (12, 14)
             None (14, 16)

             >>> maps = []
             >>> map1 = tgis.RasterDataset("map1@PERMANENT")
             >>> check = map1.set_relative_time(2, None, "days")
             >>> maps.append(map1)
             >>> map2 = tgis.RasterDataset("map2@PERMANENT")
             >>> check = map2.set_relative_time(7, None, "days")
             >>> maps.append(map2)
             >>> grans = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(maps,0,16,2)
             >>> for map_list in grans:
             ...    print(map_list[0].get_id(), map_list[0].get_temporal_extent_as_tuple())
             None (0, 2)
             map1@PERMANENT (2, 4)
             None (4, 6)
             map2@PERMANENT (6, 8)
             None (8, 10)
             None (10, 12)
             None (12, 14)
             None (14, 16)

             >>> maps = []
             >>> map1 = tgis.RasterDataset("map1@PERMANENT")
             >>> check = map1.set_absolute_time(datetime(2000, 4,1), datetime(2000, 6, 1))
             >>> maps.append(map1)
             >>> map2 = tgis.RasterDataset("map2@PERMANENT")
             >>> check = map2.set_absolute_time(datetime(2000, 8,1), datetime(2000, 12, 1))
             >>> maps.append(map2)
             >>> grans = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(maps,datetime(2000,1,1),datetime(2001,4,1),"1 month")
             >>> for map_list in grans:
             ...    print(map_list[0].get_id(), map_list[0].get_temporal_extent_as_tuple())
             None (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 2, 1, 0, 0))
             None (datetime.datetime(2000, 2, 1, 0, 0), datetime.datetime(2000, 3, 1, 0, 0))
             None (datetime.datetime(2000, 3, 1, 0, 0), datetime.datetime(2000, 4, 1, 0, 0))
             map1@PERMANENT (datetime.datetime(2000, 4, 1, 0, 0), datetime.datetime(2000, 5, 1, 0, 0))
             map1@PERMANENT (datetime.datetime(2000, 5, 1, 0, 0), datetime.datetime(2000, 6, 1, 0, 0))
             None (datetime.datetime(2000, 6, 1, 0, 0), datetime.datetime(2000, 7, 1, 0, 0))
             None (datetime.datetime(2000, 7, 1, 0, 0), datetime.datetime(2000, 8, 1, 0, 0))
             map2@PERMANENT (datetime.datetime(2000, 8, 1, 0, 0), datetime.datetime(2000, 9, 1, 0, 0))
             map2@PERMANENT (datetime.datetime(2000, 9, 1, 0, 0), datetime.datetime(2000, 10, 1, 0, 0))
             map2@PERMANENT (datetime.datetime(2000, 10, 1, 0, 0), datetime.datetime(2000, 11, 1, 0, 0))
             map2@PERMANENT (datetime.datetime(2000, 11, 1, 0, 0), datetime.datetime(2000, 12, 1, 0, 0))
             None (datetime.datetime(2000, 12, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0))
             None (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2001, 2, 1, 0, 0))
             None (datetime.datetime(2001, 2, 1, 0, 0), datetime.datetime(2001, 3, 1, 0, 0))
             None (datetime.datetime(2001, 3, 1, 0, 0), datetime.datetime(2001, 4, 1, 0, 0))

        """

        if not maps:
            return None

        first = maps[0]

        # Build the gaplist
        gap_list = []
        while start < end:
            if first.is_time_absolute():
                next = increment_datetime_by_string(start, gran)
            else:
                next = start + gran

            map = first.get_new_instance(None)
            map.set_spatial_extent_from_values(0, 0, 0, 0, 0, 0)
            if first.is_time_absolute():
                map.set_absolute_time(start, next)
            else:
                map.set_relative_time(start, next, first.get_relative_time_unit())

            gap_list.append(copy.copy(map))
            start = next

        tb = SpatioTemporalTopologyBuilder()
        tb.build(gap_list, maps)

        relations_order = ["EQUAL", "DURING", "OVERLAPS", "OVERLAPPED", "CONTAINS"]

        gran_list = []
        for gap in gap_list:
            # If not temporal relations then gap
            if not gap.get_temporal_relations():
                gran_list.append(
                    [
                        gap,
                    ]
                )
            else:
                relations = gap.get_temporal_relations()

                map_list = []

                for relation in relations_order:
                    if relation in relations:
                        map_list += relations[relation]
                        break

                if map_list:
                    new_maps = []
                    for map in map_list:
                        new_map = map.get_new_instance(map.get_id())
                        new_map.set_temporal_extent(gap.get_temporal_extent())
                        new_map.set_spatial_extent(map.get_spatial_extent())
                        new_maps.append(new_map)
                    gran_list.append(new_maps)
                else:
                    gran_list.append(
                        [
                            gap,
                        ]
                    )

        if gran_list:
            return gran_list

        return None

    def get_registered_maps_as_objects_with_gaps(self, where=None, dbif=None):
        """Return all or a subset of the registered maps as
        ordered (by start_time) object list with
        "gap" map objects (id==None) for spatio-temporal topological
        operations that require the spatio-temporal extent only.

        Gaps between maps are identified as maps with id==None

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param where: The SQL where statement to select a
                     subset of the registered maps without "WHERE"
        :param dbif: The database interface to be used

        :return: ordered object list, in case nothing found None is returned
        """

        dbif, connection_state_changed = init_dbif(dbif)

        obj_list = []

        maps = self.get_registered_maps_as_objects(where, "start_time", dbif)

        if maps is not None and len(maps) > 0:
            for i in range(len(maps)):
                obj_list.append(maps[i])
                # Detect and insert gaps
                if i < len(maps) - 1:
                    relation = maps[i + 1].temporal_relation(maps[i])
                    if relation == "after":
                        start1, end1 = maps[i].get_temporal_extent_as_tuple()
                        start2, end2 = maps[i + 1].get_temporal_extent_as_tuple()
                        end = start2
                        if end1 is not None:
                            start = end1
                        else:
                            start = start1

                        map = self.get_new_map_instance(None)

                        if self.is_time_absolute():
                            map.set_absolute_time(start, end)
                        elif self.is_time_relative():
                            map.set_relative_time(
                                start, end, self.get_relative_time_unit()
                            )
                        map.set_spatial_extent_from_values(0, 0, 0, 0, 0, 0)
                        obj_list.append(copy.copy(map))

        if connection_state_changed:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects_with_temporal_topology(
        self, where=None, order="start_time", dbif=None
    ):
        """Return all or a subset of the registered maps as ordered object
        list with spatio-temporal topological relationship information.

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param where: The SQL where statement to select a subset of
                     the registered maps without "WHERE"
        :param order: The SQL order statement to be used to order the
                     objects in the list without "ORDER BY"
        :param dbif: The database interface to be used
        :return: The ordered map object list,
                In case nothing found None is returned
        """

        dbif, connection_state_changed = init_dbif(dbif)
        obj_list = self.get_registered_maps_as_objects(where, order, dbif)

        tb = SpatioTemporalTopologyBuilder()
        tb.build(obj_list)

        if connection_state_changed:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects(self, where=None, order="start_time", dbif=None):
        """Return all or a subset of the registered maps as ordered object
        list for spatio-temporal topological operations that require the
        spatio-temporal extent only

        The objects are initialized with their id's' and the spatio-temporal
        extent (temporal type, start time, end time, west, east, south,
        north, bottom and top).
        In case more map information are needed, use the select()
        method for each listed object.

        :param where: The SQL where statement to select a subset of
                      the registered maps without "WHERE"
        :param order: The SQL order statement to be used to order the
                      objects in the list without "ORDER BY"
        :param dbif: The database interface to be used
        :return: The ordered map object list,
                In case nothing found None is returned
        """

        dbif, connection_state_changed = init_dbif(dbif)

        obj_list = []

        # Older temporal databases have no bottom and top columns
        # in their views so we need a work around to set the full
        # spatial extent as well

        rows = get_tgis_metadata(dbif)
        db_version = 0

        if rows:
            for row in rows:
                if row["key"] == "tgis_db_version":
                    db_version = int(float(row["value"]))

        if db_version >= 1:
            has_bt_columns = True
            columns = "id,start_time,end_time, west,east,south,north,bottom,top"
        else:
            has_bt_columns = False
            columns = "id,start_time,end_time, west,east,south,north"

        rows = self.get_registered_maps(columns, where, order, dbif)

        if rows is not None:
            for row in rows:
                map = self.get_new_map_instance(row["id"])
                if self.is_time_absolute():
                    map.set_absolute_time(row["start_time"], row["end_time"])
                elif self.is_time_relative():
                    map.set_relative_time(
                        row["start_time"],
                        row["end_time"],
                        self.get_relative_time_unit(),
                    )
                # The fast way
                if has_bt_columns:
                    map.set_spatial_extent_from_values(
                        west=row["west"],
                        east=row["east"],
                        south=row["south"],
                        top=row["top"],
                        north=row["north"],
                        bottom=row["bottom"],
                    )
                # The slow work around
                else:
                    map.spatial_extent.select(dbif)

                obj_list.append(copy.copy(map))

        if connection_state_changed:
            dbif.close()

        return obj_list

    def _update_where_statement_by_band_reference(self, where):
        """Update given SQL WHERE statement by band reference.

        Call this method only when self.band_reference is defined.

        :param str where: SQL WHERE statement to be updated

        :return: updated SQL WHERE statement
        """

        def leading_zero(value):
            try:
                if value.startswith("0"):
                    return value.lstrip("0")
                else:
                    return "{0:02d}".format(int(value))
            except ValueError:
                return value

            return None

        # initialized WHERE statement
        if where:
            where += " AND "
        else:
            where = ""

        # be case-insensitive
        if "_" in self.band_reference:
            # fully-qualified band reference
            where += "band_reference IN ('{}'".format(self.band_reference.upper())

            # be zero-padding less sensitive
            shortcut, identifier = self.band_reference.split("_", -1)
            identifier_zp = leading_zero(identifier)
            if identifier_zp:
                where += ", '{fl}_{zp}'".format(
                    fl=shortcut.upper(), zp=identifier_zp.upper()
                )

            # close WHERE statement
            where += ")"
        else:
            # shortcut or band identifier given
            shortcut_identifier = leading_zero(self.band_reference)
            if shortcut_identifier:
                where += (
                    "{br} LIKE '{si}\_%' {esc} OR {br} LIKE '%\_{si}' {esc} OR "
                    "{br} LIKE '{orig}\_%' {esc} OR {br} LIKE '%\_{orig}' {esc}".format(
                        br="band_reference",
                        si=shortcut_identifier,
                        orig=self.band_reference.upper(),
                        esc="ESCAPE '\\'",
                    )
                )
            else:
                where += "band_reference = '{}'".format(self.band_reference)

        return where

    def get_registered_maps(self, columns=None, where=None, order=None, dbif=None):
        """Return SQL rows of all registered maps.

        In case columns are not specified, each row includes all columns
        specified in the datatype specific view.

        :param columns: Columns to be selected as SQL compliant string
        :param where: The SQL where statement to select a subset
                     of the registered maps without "WHERE"
        :param order: The SQL order statement to be used to order the
                     objects in the list without "ORDER BY"
        :param dbif: The database interface to be used

        :return: SQL rows of all registered maps,
                In case nothing found None is returned
        """

        dbif, connection_state_changed = init_dbif(dbif)

        rows = None

        if self.get_map_register() is not None:
            # Use the correct temporal table
            if self.get_temporal_type() == "absolute":
                map_view = self.get_new_map_instance(None).get_type() + "_view_abs_time"
            else:
                map_view = self.get_new_map_instance(None).get_type() + "_view_rel_time"

            if columns is not None and columns != "":
                sql = "SELECT %s FROM %s  WHERE %s.id IN (SELECT id FROM %s)" % (
                    columns,
                    map_view,
                    map_view,
                    self.get_map_register(),
                )
            else:
                sql = "SELECT * FROM %s  WHERE %s.id IN (SELECT id FROM %s)" % (
                    map_view,
                    map_view,
                    self.get_map_register(),
                )

            # filter by band reference identifier
            if self.band_reference:
                where = self._update_where_statement_by_band_reference(where)

            if where is not None and where != "":
                sql += " AND (%s)" % (where.split(";")[0])
            if order is not None and order != "":
                sql += " ORDER BY %s" % (order.split(";")[0])
            try:
                dbif.execute(sql, mapset=self.base.mapset)
                rows = dbif.fetchall(mapset=self.base.mapset)
            except:
                if connection_state_changed:
                    dbif.close()
                self.msgr.error(
                    _("Unable to get map ids from register table " "<%s>")
                    % (self.get_map_register())
                )
                raise

        if connection_state_changed:
            dbif.close()

        return rows

    @staticmethod
    def shift_map_list(maps, gran):
        """Temporally shift each map in the list with the provided granularity

        This method does not perform any temporal database operations.

        :param maps: A list of maps  with initialized temporal extent
        :param gran: The granularity to be used for shifting
        :return: The modified map list, None if nothing to shift or wrong
                granularity

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> maps = []
            >>> for i in range(5):
            ...   map = tgis.RasterDataset(None)
            ...   if i%2 == 0:
            ...       check = map.set_relative_time(i, i + 1, 'years')
            ...   else:
            ...       check = map.set_relative_time(i, None, 'years')
            ...   maps.append(map)
            >>> for map in maps:
            ...   map.temporal_extent.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 0
             | End time:................... 1
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 1
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 2
             | End time:................... 3
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 3
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 4
             | End time:................... 5
             | Relative time unit:......... years
            >>> maps = tgis.AbstractSpaceTimeDataset.shift_map_list(maps, 5)
            >>> for map in maps:
            ...   map.temporal_extent.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 5
             | End time:................... 6
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 6
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 7
             | End time:................... 8
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 8
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 9
             | End time:................... 10
             | Relative time unit:......... years

        """
        if maps is None:
            return None

        if not check_granularity_string(gran, maps[-1].get_temporal_type()):
            return None

        for map in maps:
            start, end = map.get_temporal_extent_as_tuple()
            if map.is_time_absolute():
                start = increment_datetime_by_string(start, gran)
                if end is not None:
                    end = increment_datetime_by_string(end, gran)
                map.set_absolute_time(start, end)
            elif map.is_time_relative():
                start = start + int(gran)
                if end is not None:
                    end = end + int(gran)
                map.set_relative_time(start, end, map.get_relative_time_unit())

        return maps

    def shift(self, gran, dbif=None):
        """Temporally shift each registered map with the provided granularity

        :param gran: The granularity to be used for shifting
        :param dbif: The database interface to be used
        :return: True something to shift, False if nothing to shift or wrong
                granularity

        """
        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to shift dataset <%(ds)s> of type "
                    "%(type)s in the temporal database. The mapset "
                    "of the dataset does not match the current "
                    "mapset"
                )
                % ({"ds": self.get_id()}, {"type": self.get_type()})
            )

        if not check_granularity_string(gran, self.get_temporal_type()):
            self.msgr.error(_("Wrong granularity format: %s" % (gran)))
            return False

        dbif, connection_state_changed = init_dbif(dbif)

        maps = self.get_registered_maps_as_objects(dbif=dbif)

        if maps is None:
            return False

        date_list = []

        # We need to make a dry run to avoid a break
        # in the middle of the update process when the increment
        # results in wrong number of days in a month
        for map in maps:
            start, end = map.get_temporal_extent_as_tuple()

            if self.is_time_absolute():
                start = increment_datetime_by_string(start, gran)
                if end is not None:
                    end = increment_datetime_by_string(end, gran)
            elif self.is_time_relative():
                start = start + int(gran)
                if end is not None:
                    end = end + int(gran)

            date_list.append((start, end))

        self._update_map_timestamps(maps, date_list, dbif)

        if connection_state_changed:
            dbif.close()

    @staticmethod
    def snap_map_list(maps):
        """For each map in the list snap the end time to the start time of its
        temporal nearest neighbor in the future.

        Maps with equal time stamps are not snapped.

        The granularity of the map list will be used to create the end time
        of the last map in case it has a time instance as timestamp.

        This method does not perform any temporal database operations.

        :param maps: A list of maps with initialized temporal extent
        :return: The modified map list, None nothing to shift or wrong
                granularity

        Usage:

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> maps = []
            >>> for i in range(5):
            ...   map = tgis.RasterDataset(None)
            ...   if i%2 == 0:
            ...       check = map.set_relative_time(i, i + 1, 'years')
            ...   else:
            ...       check = map.set_relative_time(i, None, 'years')
            ...   maps.append(map)
            >>> for map in maps:
            ...   map.temporal_extent.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 0
             | End time:................... 1
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 1
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 2
             | End time:................... 3
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 3
             | End time:................... None
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 4
             | End time:................... 5
             | Relative time unit:......... years
            >>> maps = tgis.AbstractSpaceTimeDataset.snap_map_list(maps)
            >>> for map in maps:
            ...   map.temporal_extent.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 0
             | End time:................... 1
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 1
             | End time:................... 2
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 2
             | End time:................... 3
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 3
             | End time:................... 4
             | Relative time unit:......... years
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 4
             | End time:................... 5
             | Relative time unit:......... years

        """
        if maps is None or len(maps) == 0:
            return None

        # We need to sort the maps temporally by start time
        maps = sorted(maps, key=AbstractDatasetComparisonKeyStartTime)

        for i in range(len(maps) - 1):
            start, end = maps[i].get_temporal_extent_as_tuple()
            start_next, end = maps[i + 1].get_temporal_extent_as_tuple()

            # Maps with equal time stamps can not be snapped
            if start != start_next:
                if maps[i].is_time_absolute():
                    maps[i].set_absolute_time(start, start_next)
                elif maps[i].is_time_relative():
                    maps[i].set_relative_time(
                        start, start_next, maps[i].get_relative_time_unit()
                    )
            else:
                if maps[i].is_time_absolute():
                    maps[i].set_absolute_time(start, end)
                elif maps[i].is_time_relative():
                    maps[i].set_relative_time(
                        start, end, maps[i].get_relative_time_unit()
                    )
        # Last map
        start, end = maps[-1].get_temporal_extent_as_tuple()
        # We increment the start time with the dataset
        # granularity if the end time is None
        if end is None:
            if maps[-1].is_time_absolute():
                gran = compute_absolute_time_granularity(maps)
                end = increment_datetime_by_string(start, gran)
                maps[-1].set_absolute_time(start, end)
            elif maps[-1].is_time_relative():
                gran = compute_relative_time_granularity(maps)
                end = start + gran
                maps[-1].set_relative_time(
                    start, end, maps[-1].get_relative_time_unit()
                )

        return maps

    def snap(self, dbif=None):
        """For each registered map snap the end time to the start time of
        its temporal nearest neighbor in the future

        Maps with equal time stamps are not snapped

        :param dbif: The database interface to be used

        """

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to snap dataset <%(ds)s> of type "
                    "%(type)s in the temporal database. The mapset "
                    "of the dataset does not match the current "
                    "mapset"
                )
                % ({"ds": self.get_id()}, {"type": self.get_type()})
            )

        dbif, connection_state_changed = init_dbif(dbif)

        maps = self.get_registered_maps_as_objects(dbif=dbif)

        if maps is None:
            return

        date_list = []

        for i in range(len(maps) - 1):
            start, end = maps[i].get_temporal_extent_as_tuple()
            start_next, end = maps[i + 1].get_temporal_extent_as_tuple()

            # Maps with equal time stamps can not be snapped
            if start != start_next:
                date_list.append((start, start_next))
            else:
                # Keep the original time stamps
                date_list.append((start, end))

        # Last map
        start, end = maps[-1].get_temporal_extent_as_tuple()
        # We increment the start time with the dataset
        # granularity if the end time is None
        if end is None:
            if self.is_time_absolute():
                end = increment_datetime_by_string(start, self.get_granularity())
            elif self.is_time_relative():
                end = start + self.get_granularity()

        date_list.append((start, end))

        self._update_map_timestamps(maps, date_list, dbif)

        if connection_state_changed:
            dbif.close()

    def _update_map_timestamps(self, maps, date_list, dbif):
        """Update the timestamps of maps with the start and end time
        stored in the date_list.

        The number of dates in the list must be equal to the number
        of maps.

        :param maps: A list of map objects
        :param date_list: A list with date tuples (start_time, end_time)
        :param dbif: The database interface to be used
        """

        datatsets_to_modify = {}
        # Now update the maps
        count = 0
        for map in maps:
            start = date_list[count][0]
            end = date_list[count][1]
            map.select(dbif)
            count += 1

            if self.is_time_absolute():
                map.update_absolute_time(start_time=start, end_time=end, dbif=dbif)
            elif self.is_time_relative():
                map.update_relative_time(
                    start_time=start,
                    end_time=end,
                    unit=self.get_relative_time_unit(),
                    dbif=dbif,
                )

            # Save the datasets that must be updated
            datasets = map.get_registered_stds(dbif)
            if datasets:
                for dataset in datasets:
                    datatsets_to_modify[dataset] = dataset

        self.update_from_registered_maps(dbif)

        # Update affected datasets
        if datatsets_to_modify:
            for dataset in datatsets_to_modify:
                if dataset != self.get_id():
                    ds = self.get_new_instance(ident=dataset)
                    ds.select(dbif)
                    ds.update_from_registered_maps(dbif)

    def rename(self, ident, dbif=None):
        """Rename the space time dataset

        This method renames the space time dataset, the map register table
        and updates the entries in registered maps stds register.

        Renaming does not work with Postgresql yet.

        :param ident: The new identifier "name@mapset"
        :param dbif: The database interface to be used
        """

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to rename dataset <%(ds)s> of type "
                    "%(type)s in the temporal database. The mapset "
                    "of the dataset does not match the current "
                    "mapset"
                )
                % ({"ds": self.get_id()}, {"type": self.get_type()})
            )

        dbif, connection_state_changed = init_dbif(dbif)

        if dbif.get_dbmi().__name__ != "sqlite3":
            self.msgr.fatal(
                _("Renaming of space time datasets is not " "supported for PostgreSQL.")
            )

        # SELECT all needed information from the database
        self.select(dbif)

        # We need to select the registered maps here
        maps = self.get_registered_maps_as_objects(None, "start_time", dbif)

        # Safe old identifier
        old_ident = self.get_id()
        # We need to rename the old table
        old_map_register_table = self.get_map_register()

        # Set new identifier
        self.set_id(ident)
        # Create map register table name from new identifier
        new_map_register_table = self.create_map_register_name()
        # Set new map register table name
        self.set_map_register(new_map_register_table)

        # Get the update statement, we update the table entry of the old
        # identifier
        statement = self.update(dbif, execute=False, ident=old_ident)

        # We need to rename the raster register table
        statement += 'ALTER TABLE %s RENAME TO "%s";\n' % (
            old_map_register_table,
            new_map_register_table,
        )

        # We need to take care of the stds index in the sqlite3 database
        if dbif.get_dbmi().__name__ == "sqlite3":
            statement += "DROP INDEX %s_index;\n" % (old_map_register_table)
            statement += "CREATE INDEX %s_index ON %s (id);" % (
                new_map_register_table,
                new_map_register_table,
            )

        # We need to rename the space time dataset in the maps register table
        if maps:
            for map in maps:
                map.remove_stds_from_register(stds_id=old_ident, dbif=dbif)
                map.add_stds_to_register(stds_id=ident, dbif=dbif)

        # Execute the accumulated statements
        dbif.execute_transaction(statement)

        if connection_state_changed:
            dbif.close()

    def delete(self, dbif=None, execute=True):
        """Delete a space time dataset from the temporal database

        This method removes the space time dataset from the temporal
        database and drops its map register table

        :param dbif: The database interface to be used
        :param execute: If True the SQL DELETE and DROP table
                       statements will be executed.
                       If False the prepared SQL statements are returned
                       and must be executed by the caller.

        :return: The SQL statements if execute == False, else an empty
                 string
        """
        # First we need to check if maps are registered in this dataset and
        # unregister them

        self.msgr.verbose(
            _("Delete space time %s  dataset <%s> from temporal " "database")
            % (self.get_new_map_instance(ident=None).get_type(), self.get_id())
        )

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to delete dataset <%(ds)s> of type "
                    "%(type)s from the temporal database. The mapset"
                    " of the dataset does not match the current "
                    "mapset"
                )
                % {"ds": self.get_id(), "type": self.get_type()}
            )

        statement = ""
        dbif, connection_state_changed = init_dbif(dbif)

        # SELECT all needed information from the database
        self.metadata.select(dbif)

        if self.get_map_register() is not None:
            self.msgr.debug(
                1, _("Drop map register table: %s") % (self.get_map_register())
            )
            rows = self.get_registered_maps("id", None, None, dbif)
            # Unregister each registered map in the table
            if rows is not None:
                for row in rows:
                    # Unregister map
                    map = self.get_new_map_instance(row["id"])
                    statement += self.unregister_map(map=map, dbif=dbif, execute=False)

            # Safe the DROP table statement
            statement += "DROP TABLE IF EXISTS " + self.get_map_register() + ";\n"

        # Remove the primary key, the foreign keys will be removed by trigger
        statement += self.base.get_delete_statement()

        if execute:
            dbif.execute_transaction(statement)

        self.reset(None)

        if connection_state_changed:
            dbif.close()

        if execute:
            return ""

        return statement

    def is_map_registered(self, map_id, dbif=None):
        """Check if a map is registered in the space time dataset

        :param map_id: The map id
        :param dbif: The database interface to be used
        :return: True if success, False otherwise
        """
        stds_register_table = self.get_map_register()

        dbif, connection_state_changed = init_dbif(dbif)

        is_registered = False

        # Check if map is already registered
        if stds_register_table is not None:
            if dbif.get_dbmi().paramstyle == "qmark":
                sql = "SELECT id FROM " + stds_register_table + " WHERE id = (?)"
            else:
                sql = "SELECT id FROM " + stds_register_table + " WHERE id = (%s)"
            try:
                dbif.execute(sql, (map_id,), mapset=self.base.mapset)
                row = dbif.fetchone(mapset=self.base.mapset)
            except:
                self.msgr.warning(_("Error in register table request"))
                raise

            if row is not None and row[0] == map_id:
                is_registered = True

        if connection_state_changed is True:
            dbif.close()

        return is_registered

    def register_map(self, map, dbif=None):
        """Register a map in the space time dataset.

         This method takes care of the registration of a map
         in a space time dataset.

         In case the map is already registered this function
         will break with a warning and return False.

         This method raises a FatalError exception in case of a fatal error

        :param map: The AbstractMapDataset object that should be registered
        :param dbif: The database interface to be used
        :return: True if success, False otherwise
        """

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to register map in dataset <%(ds)s> of "
                    "type %(type)s. The mapset of the dataset does "
                    "not match the current mapset"
                )
                % {"ds": self.get_id(), "type": self.get_type()}
            )

        dbif, connection_state_changed = init_dbif(dbif)

        if map.is_in_db(dbif) is False:
            dbif.close()
            self.msgr.fatal(
                _(
                    "Only a map that was inserted in the temporal "
                    "database can be registered in a space time "
                    "dataset"
                )
            )

        if map.get_layer():
            self.msgr.debug(
                1,
                "Register %s map <%s> with layer %s in space "
                "time %s dataset <%s>"
                % (
                    map.get_type(),
                    map.get_map_id(),
                    map.get_layer(),
                    map.get_type(),
                    self.get_id(),
                ),
            )
        else:
            self.msgr.debug(
                1,
                "Register %s map <%s> in space time %s "
                "dataset <%s>"
                % (map.get_type(), map.get_map_id(), map.get_type(), self.get_id()),
            )

        # First select all data from the database
        map.select(dbif)

        if not map.check_for_correct_time():
            if map.get_layer():
                self.msgr.fatal(
                    _("Map <%(id)s> with layer %(l)s has invalid " "time")
                    % {"id": map.get_map_id(), "l": map.get_layer()}
                )
            else:
                self.msgr.fatal(_("Map <%s> has invalid time") % (map.get_map_id()))

        # Get basic info
        map_id = map.base.get_id()
        map_mapset = map.base.get_mapset()
        map_rel_time_unit = map.get_relative_time_unit()
        map_ttype = map.get_temporal_type()

        stds_mapset = self.base.get_mapset()
        stds_register_table = self.get_map_register()
        stds_ttype = self.get_temporal_type()

        # The gathered SQL statemets are stroed here
        statement = ""

        # Check temporal types
        if stds_ttype != map_ttype:
            if map.get_layer():
                self.msgr.fatal(
                    _(
                        "Temporal type of space time dataset "
                        "<%(id)s> and map <%(map)s> with layer %(l)s"
                        " are different"
                    )
                    % {
                        "id": self.get_id(),
                        "map": map.get_map_id(),
                        "l": map.get_layer(),
                    }
                )
            else:
                self.msgr.fatal(
                    _(
                        "Temporal type of space time dataset "
                        "<%(id)s> and map <%(map)s> are different"
                    )
                    % {"id": self.get_id(), "map": map.get_map_id()}
                )

        # In case no map has been registered yet, set the
        # relative time unit from the first map
        if (
            (
                self.metadata.get_number_of_maps() is None
                or self.metadata.get_number_of_maps() == 0
            )
            and self.map_counter == 0
            and self.is_time_relative()
        ):

            self.set_relative_time_unit(map_rel_time_unit)
            statement += self.relative_time.get_update_all_statement_mogrified(dbif)

            self.msgr.debug(
                1,
                _("Set temporal unit for space time %s dataset " "<%s> to %s")
                % (map.get_type(), self.get_id(), map_rel_time_unit),
            )

        stds_rel_time_unit = self.get_relative_time_unit()

        # Check the relative time unit
        if self.is_time_relative() and (stds_rel_time_unit != map_rel_time_unit):
            if map.get_layer():
                self.msgr.fatal(
                    _(
                        "Relative time units of space time dataset "
                        "<%(id)s> and map <%(map)s> with layer %(l)s"
                        " are different"
                    )
                    % {
                        "id": self.get_id(),
                        "map": map.get_map_id(),
                        "l": map.get_layer(),
                    }
                )
            else:
                self.msgr.fatal(
                    _(
                        "Relative time units of space time dataset "
                        "<%(id)s> and map <%(map)s> are different"
                    )
                    % {"id": self.get_id(), "map": map.get_map_id()}
                )

        if get_enable_mapset_check() is True and stds_mapset != map_mapset:
            dbif.close()
            self.msgr.fatal(_("Only maps from the same mapset can be registered"))

        # Check if map is already registered
        if self.is_map_registered(map_id, dbif=dbif):
            if map.get_layer() is not None:
                self.msgr.warning(
                    _("Map <%(map)s> with layer %(l)s is already" " registered.")
                    % {"map": map.get_map_id(), "l": map.get_layer()}
                )
            else:
                self.msgr.warning(
                    _("Map <%s> is already registered.") % (map.get_map_id())
                )
            return False

        # Register the stds in the map stds register table column
        statement += map.add_stds_to_register(
            stds_id=self.base.get_id(), dbif=dbif, execute=False
        )

        # Now put the raster name in the stds map register table
        if dbif.get_dbmi().paramstyle == "qmark":
            sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (?);\n"
        else:
            sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (%s);\n"

        statement += dbif.mogrify_sql_statement((sql, (map_id,)))

        # Now execute the insert transaction
        dbif.execute_transaction(statement)

        if connection_state_changed:
            dbif.close()

        # increase the counter
        self.map_counter += 1

        return True

    def unregister_map(self, map, dbif=None, execute=True):
        """Unregister a map from the space time dataset.

        This method takes care of the un-registration of a map
        from a space time dataset.

        :param map: The map object to unregister
        :param dbif: The database interface to be used
        :param execute: If True the SQL DELETE and DROP table
                        statements will be executed.
                        If False the prepared SQL statements are
                        returned and must be executed by the caller.

        :return: The SQL statements if execute == False, else an empty
                string, None in case of a failure
        """

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to unregister map from dataset <%(ds)s>"
                    " of type %(type)s in the temporal database."
                    " The mapset of the dataset does not match the"
                    " current mapset"
                )
                % {"ds": self.get_id(), "type": self.get_type()}
            )

        statement = ""

        dbif, connection_state_changed = init_dbif(dbif)

        # Check if the map is registered in the space time raster dataset
        if self.is_map_registered(map.get_id(), dbif) is False:
            if map.get_layer() is not None:
                self.msgr.warning(
                    _(
                        "Map <%(map)s> with layer %(l)s is not "
                        "registered in space time dataset "
                        "<%(base)s>"
                    )
                    % {
                        "map": map.get_map_id(),
                        "l": map.get_layer(),
                        "base": self.base.get_id(),
                    }
                )
            else:
                self.msgr.warning(
                    _(
                        "Map <%(map)s> is not registered in space "
                        "time dataset <%(base)s>"
                    )
                    % {"map": map.get_map_id(), "base": self.base.get_id()}
                )
            if connection_state_changed is True:
                dbif.close()
            return ""

        # Remove the space time dataset from the dataset register
        # We need to execute the statement here, otherwise the space time
        # dataset will not be removed correctly
        map.remove_stds_from_register(self.base.get_id(), dbif=dbif, execute=True)

        # Remove the map from the space time dataset register
        stds_register_table = self.get_map_register()
        if stds_register_table is not None:
            if dbif.get_dbmi().paramstyle == "qmark":
                sql = "DELETE FROM " + stds_register_table + " WHERE id = ?;\n"
            else:
                sql = "DELETE FROM " + stds_register_table + " WHERE id = %s;\n"

            statement += dbif.mogrify_sql_statement((sql, (map.get_id(),)))

        if execute:
            dbif.execute_transaction(statement)
            statement = ""

        if connection_state_changed:
            dbif.close()

        # decrease the counter
        self.map_counter -= 1

        return statement

    def update_from_registered_maps(self, dbif=None):
        """This methods updates the modification time, the spatial and
        temporal extent as well as type specific metadata. It should always
        been called after maps are registered or unregistered/deleted from
        the space time dataset.

        The update of the temporal extent checks if the end time is set
        correctly.
        In case the registered maps have no valid end time (None) the
        maximum start time
        will be used. If the end time is earlier than the maximum start
        time, it will be replaced by the maximum start time.

        :param dbif: The database interface to be used
        """

        if (
            get_enable_mapset_check() is True
            and self.get_mapset() != get_current_mapset()
        ):
            self.msgr.fatal(
                _(
                    "Unable to update dataset <%(ds)s> of type "
                    "%(type)s in the temporal database. The mapset"
                    " of the dataset does not match the current "
                    "mapset"
                )
                % {"ds": self.get_id(), "type": self.get_type()}
            )

        self.msgr.verbose(
            _(
                "Update metadata, spatial and temporal extent from"
                " all registered maps of <%s>"
            )
            % (self.get_id())
        )

        # Nothing to do if the map register is not present
        if not self.get_map_register():
            return

        dbif, connection_state_changed = init_dbif(dbif)

        map_time = None

        use_start_time = False

        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        sql_path = get_sql_template_path()
        stds_register_table = self.get_map_register()

        # We create a transaction
        sql_script = ""

        # Update the spatial and temporal extent from registered maps
        # Read the SQL template
        sql = open(
            os.path.join(sql_path, "update_stds_spatial_temporal_extent_template.sql"),
            "r",
        ).read()
        sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
        sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
        sql = sql.replace("SPACETIME_ID", self.base.get_id())
        sql = sql.replace("STDS", self.get_type())

        sql_script += sql
        sql_script += "\n"

        # Update type specific metadata
        sql = open(
            os.path.join(
                sql_path, "update_" + self.get_type() + "_metadata_template.sql"
            ),
            "r",
        ).read()
        sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
        sql = sql.replace("SPACETIME_ID", self.base.get_id())

        sql_script += sql
        sql_script += "\n"

        dbif.execute_transaction(sql_script)

        # Read and validate the selected end time
        self.select(dbif)

        if self.is_time_absolute():
            start_time, end_time = self.get_absolute_time()
        else:
            start_time, end_time, unit = self.get_relative_time()

        # In case no end time is set, use the maximum start time of
        # all registered maps as end time
        if end_time is None:
            use_start_time = True
        else:
            # Check if the end time is smaller than the maximum start time
            if self.is_time_absolute():
                sql = """SELECT max(start_time) FROM GRASS_MAP_absolute_time
                         WHERE GRASS_MAP_absolute_time.id IN
                        (SELECT id FROM SPACETIME_REGISTER_TABLE);"""
                sql = sql.replace(
                    "GRASS_MAP", self.get_new_map_instance(None).get_type()
                )
                sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
            else:
                sql = """SELECT max(start_time) FROM GRASS_MAP_relative_time
                         WHERE GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_REGISTER_TABLE);"""
                sql = sql.replace(
                    "GRASS_MAP", self.get_new_map_instance(None).get_type()
                )
                sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)

            dbif.execute(sql, mapset=self.base.mapset)
            row = dbif.fetchone(mapset=self.base.mapset)

            if row is not None:
                # This seems to be a bug in sqlite3 Python driver
                if dbif.get_dbmi().__name__ == "sqlite3":
                    tstring = row[0]
                    # Convert the unicode string into the datetime format
                    if self.is_time_absolute():
                        max_start_time = string_to_datetime(tstring)
                        if max_start_time is None:
                            max_start_time = end_time
                    else:
                        max_start_time = row[0]
                else:
                    max_start_time = row[0]

                if end_time < max_start_time:
                    use_start_time = True

        # Set the maximum start time as end time
        if use_start_time:
            if self.is_time_absolute():
                sql = """UPDATE STDS_absolute_time SET end_time =
               (SELECT max(start_time) FROM GRASS_MAP_absolute_time WHERE
               GRASS_MAP_absolute_time.id IN
                        (SELECT id FROM SPACETIME_REGISTER_TABLE)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace(
                    "GRASS_MAP", self.get_new_map_instance(None).get_type()
                )
                sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())
            elif self.is_time_relative():
                sql = """UPDATE STDS_relative_time SET end_time =
               (SELECT max(start_time) FROM GRASS_MAP_relative_time WHERE
               GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_REGISTER_TABLE)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace(
                    "GRASS_MAP", self.get_new_map_instance(None).get_type()
                )
                sql = sql.replace("SPACETIME_REGISTER_TABLE", stds_register_table)
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())

            dbif.execute_transaction(sql)

        # Count the temporal map types
        maps = self.get_registered_maps_as_objects(dbif=dbif)
        tlist = self.count_temporal_types(maps)

        if tlist["interval"] > 0 and tlist["point"] == 0 and tlist["invalid"] == 0:
            map_time = "interval"
        elif tlist["interval"] == 0 and tlist["point"] > 0 and tlist["invalid"] == 0:
            map_time = "point"
        elif tlist["interval"] > 0 and tlist["point"] > 0 and tlist["invalid"] == 0:
            map_time = "mixed"
        else:
            map_time = "invalid"

        # Compute the granularity

        if map_time != "invalid":
            # Smallest supported temporal resolution
            if self.is_time_absolute():
                gran = compute_absolute_time_granularity(maps)
            elif self.is_time_relative():
                gran = compute_relative_time_granularity(maps)
        else:
            gran = None

        # Set the map time type and update the time objects
        self.temporal_extent.select(dbif)
        self.metadata.select(dbif)
        if self.metadata.get_number_of_maps() > 0:
            self.temporal_extent.set_map_time(map_time)
            self.temporal_extent.set_granularity(gran)
        else:
            self.temporal_extent.set_map_time(None)
            self.temporal_extent.set_granularity(None)
        self.temporal_extent.update_all(dbif)

        # Set the modification time
        self.base.set_mtime(datetime.now())
        self.base.update(dbif)

        if connection_state_changed:
            dbif.close()


###############################################################################

if __name__ == "__main__":
    import doctest

    doctest.testmod()
