# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

(C) 2011-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import sys
from abstract_dataset import *
from temporal_granularity import *
from spatio_temporal_relationships import *

###############################################################################


class AbstractSpaceTimeDataset(AbstractDataset):
    """!Abstract space time dataset class

       This class represents a space time dataset. Convenient functions
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

    def create_map_register_name(self):
        """!Create the name of the map register table of this space time
            dataset

            The name, mapset and the map type are used to create the table name

            ATTENTION: It must be assured that the base object has selected its
            content from the database.

            @return The name of the map register table
        """

        return self.base.get_name() + "_" + \
                self.base.get_mapset() + "_" + \
                self.get_new_map_instance(None).get_type() + "_register"

    @abstractmethod
    def get_new_map_instance(self, ident=None):
        """!Return a new instance of a map which is associated
           with the type of this object

           @param ident The unique identifier of the new object
        """

    @abstractmethod
    def get_map_register(self):
        """!Return the name of the map register table
           @return The map register table name
        """

    @abstractmethod
    def set_map_register(self, name):
        """!Set the name of the map register table

           This table stores all map names which are registered
           in this space time dataset.

           This method only modifies this object and does not commit
           the modifications to the temporal database.

           @param name The name of the register table
        """

    def print_self(self):
        """!Print the content of the internal structure to stdout"""
        self.base.print_self()
        self.temporal_extent.print_self()
        self.spatial_extent.print_self()
        self.metadata.print_self()

    def print_info(self):
        """!Print information about this class in human readable style"""

        if self.get_type() == "strds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- Space Time Raster Dataset -----------------------------+"
        if self.get_type() == "str3ds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- Space Time 3D Raster Dataset --------------------------+"
        if self.get_type() == "stvds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- Space Time Vector Dataset -----------------------------+"
        print " |                                                                            |"
        self.base.print_info()
        self.temporal_extent.print_info()
        self.spatial_extent.print_info()
        self.metadata.print_info()
        print " +----------------------------------------------------------------------------+"

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        self.base.print_shell_info()
        self.temporal_extent.print_shell_info()
        self.spatial_extent.print_shell_info()
        self.metadata.print_shell_info()

    def print_history(self):
        """!Print history information about this class in human readable
            shell style
        """
        self.metadata.print_history()

    def set_initial_values(self, temporal_type, semantic_type,
                           title=None, description=None):
        """!Set the initial values of the space time dataset

            In addition the command creation string is generated
            an inserted into the metadata object.

            This method only modifies this object and does not commit
            the modifications to the temporal database.

            The insert() function must be called to commit
            this content into the temporal database.

           @param temporal_type The temporal type of this space
                                time dataset (absolute or relative)
           @param semantic_type The semantic type of this dataset
           @param title The title
           @param description The description of this dataset
        """

        if temporal_type == "absolute":
            self.base.set_ttype("absolute")
        elif temporal_type == "relative":
            self.base.set_ttype("relative")
        else:
            core.fatal(_("Unknown temporal type \"%s\"") % (temporal_type))

        self.base.set_semantic_type(semantic_type)
        self.metadata.set_title(title)
        self.metadata.set_description(description)
        self.metadata.set_command(self.create_command_string())

    def update_command_string(self, dbif=None):
        """!Append the current command string to any existing command string
           in the metadata class and calls metadata update
           @param dbif The database interface to be used
        """

        self.metadata.select(dbif=dbif)
        command = self.metadata.get_command()
        if command is None:
            command = ""
        command += self.create_command_string()
        self.metadata.set_command(command)
        self.metadata.update(dbif=dbif)

    def create_command_string(self):
        """!Create the command string that was used to create this
           space time dataset.

           The command string should be set with self.metadata.set_command()

           @return The command string
           """
        # The grass module

        command = "# %s \n"%(str(datetime.today().strftime("%Y-%m-%d %H:%M:%S")))
        command += os.path.basename(sys.argv[0])

        # We will wrap the command line to fit into 80 character
        length = len(command)
        for token in sys.argv[1:]:

            # We need to remove specific characters
            token = token.replace("\'", " ")
            token = token.replace("\"", " ")

            # Check for sub strings
            if token.find("=") > 0:
                first = token.split("=")[0]
                second = ""

                for t in token.split("=")[1:]:
                    second += t

                token = "%s=\"%s\"" % (first, second)

            if length + len(token) >= 76:
                command += "\n    %s" % (token)
                length = len(token) + 4
            else:
                command += " %s" % (token)
                length += len(token) + 1

        command += "\n"
        return command

    def get_semantic_type(self):
        """!Return the semantic type of this dataset
           @return The semantic type
        """
        return self.base.get_semantic_type()

    def get_initial_values(self):
        """!Return the initial values: temporal_type,
           semantic_type, title, description"""

        temporal_type = self.get_temporal_type()
        semantic_type = self.base.get_semantic_type()
        title = self.metadata.get_title()
        description = self.metadata.get_description()

        return temporal_type, semantic_type, title, description

    def get_granularity(self):
        """!Return the granularity of the space time dataset

           Granularity can be of absolute time or relative time.
           In case of absolute time a string containing an integer
           value and the time unit (years, months, days, hours, minuts,
                                    seconds).
           In case of relative time an integer value is expected.

           @return The granularity
        """

        return self.temporal_extent.get_granularity()

    def set_granularity(self, granularity):
        """!Set the granularity

           The granularity is usually computed by the space time dataset at
           runtime.

           Granularity can be of absolute time or relative time.
           In case of absolute time a string containing an integer
           value and the time unit (years, months, days, hours, minuts,
                                    seconds).
           In case of relative time an integer value is expected.

           This method only modifies this object and does not commit
           the modifications to the temporal database.

           @param granularity The granularity of the dataset
        """

        temporal_type = self.get_temporal_type()

        check = check_granularity_string(granularity, temporal_type)
        if not check:
            core.fatal(_("Wrong granularity: \"%s\"") % str(granularity))

        if temporal_type == "absolute":
            self.base.set_ttype("absolute")
        elif temporal_type == "relative":
            self.base.set_ttype("relative")
        else:
            core.fatal(_("Unknown temporal type \"%s\"") % (temporal_type))

        self.temporal_extent.set_granularity(granularity)

    def set_relative_time_unit(self, unit):
        """!Set the relative time unit which may be of type:
           years, months, days, hours, minutes or seconds

           All maps registered in a (relative time)
           space time dataset must have the same unit

           This method only modifies this object and does not commit
           the modifications to the temporal database.

           @param unit The relative time unit
        """

        temporal_type = self.get_temporal_type()

        if temporal_type == "relative":
            if not self.check_relative_time_unit(unit):
                core.fatal(_("Unsupported temporal unit: %s") % (unit))
            self.relative_time.set_unit(unit)

    def get_map_time(self):
        """!Return the type of the map time, interval, point, mixed or invalid
        """

        return self.temporal_extent.get_map_time()

    def count_temporal_types(self, maps=None, dbif=None):
        """!Return the temporal type of the registered maps as dictionary

           The map list must be ordered by start time

           The temporal type can be:

           - point    -> only the start time is present
           - interval -> start and end time
           - invalid  -> No valid time point or interval found

           @param maps A sorted (start_time) list of AbstractDataset objects
           @param dbif The database interface to be used
        """

        if maps is None:
            maps = get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif)

        time_invalid = 0
        time_point = 0
        time_interval = 0

        tcount = {}
        for i in range(len(maps)):
            # Check for point and interval data
            if maps[i].is_time_absolute():
                start, end, tz = maps[i].get_absolute_time()
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
        """!Count the number of gaps between temporal neighbors

           @param maps A sorted (start_time) list of AbstractDataset objects
           @param dbif The database interface to be used
           @return The numbers of gaps between temporal neighbors
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif)

        gaps = 0

        # Check for gaps
        for i in range(len(maps)):
            if i < len(maps) - 1:
                relation = maps[i + 1].temporal_relation(maps[i])
                if relation == "after":
                    gaps += 1

        return gaps

    def print_spatio_temporal_relationships(self, maps=None, spatial=None,
                                            dbif=None):
        """!Print the spatio-temporal relationships for each map of the space
           time dataset or for each map of the optional list of maps

           @param maps a ordered by start_time list of map objects, if None the
                       registred maps of the space time dataset are used
           @param spatial This indicates if the spatial topology is created as
                          well: spatial can be None (no spatial topology), "2D"
                          using west, east, south, north or "3D" using west,
                          east, south, north, bottom, top
           @param dbif The database interface to be used
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif)

        print_spatio_temporal_topology_relationships(maps1=maps, maps2=maps,
                                              spatial=spatial, dbif=dbif)

    def count_temporal_relations(self, maps=None, dbif=None):
        """!Count the temporal relations between the registered maps.

           The map list must be ordered by start time.
           Temporal relations are counted by analysing the sparse upper right
           side temporal relationships matrix.

           @param maps A sorted (start_time) list of AbstractDataset objects
           @param dbif The database interface to be used
           @return A dictionary with counted temporal relationships
        """

        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif)

        return count_temporal_topology_relationships(maps)

    def check_temporal_topology(self, maps=None, dbif=None):
        """!Check the temporal topology of all maps of the current space time
           dataset or of an optional list of maps

           Correct topology means, that time intervals are not overlap or
           that intervals does not contain other intervals.
           Equal time intervals  are not allowed.

           The optional map list must be ordered by start time

           Allowed and not allowed temporal relationships for correct topology:
           @verbatim
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
           @endverbatim

           @param maps An optional list of AbstractDataset objects, in case of
                       None all maps of the space time dataset are checked
           @param dbif The database interface to be used
           @return True if topology is correct
        """
        if maps is None:
            maps = self.get_registered_maps_as_objects(
                where=None, order="start_time", dbif=dbif)

        relations = count_temporal_topology_relationships(maps)

        if relations == None:
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
        """!Sample this space time dataset with the temporal topology
           of a second space time dataset

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

           @code
           list = self.sample_by_dataset(stds=sampler, method=[
               "during","overlap","contain","equal"])
           for entry in list:
               granule = entry["granule"]
               maplist = entry["samples"]
               for map in maplist:
                   map.select()
                   map.print_info()
           @endcode

           A valid temporal topology (no overlapping or inclusion allowed)
           is needed to get correct results in case of gaps in the sample
           dataset.

           Gaps between maps are identified as unregistered maps with id==None.

           The objects are initialized with their id's' and the spatio-temporal
           extent (temporal type, start time, end time, west, east, south,
           north, bottom and top).
           In case more map information are needed, use the select()
           method for each listed object.

           @param stds The space time dataset to be used for temporal sampling
           @param method This option specifies what sample method should be
                         used. In case the registered maps are of temporal
                         point type, only the start time is used for sampling.
                         In case of mixed of interval data the user can chose
                         between:

                  - start: Select maps of which the start time is
                    located in the selection granule
                    @verbatim
                    map    :        s
                    granule:  s-----------------e

                    map    :        s--------------------e
                    granule:  s-----------------e

                    map    :        s--------e
                    granule:  s-----------------e
                    @endverbatim

                  - during: Select maps which are temporal
                    during the selection granule
                    @verbatim
                    map    :     s-----------e
                    granule:  s-----------------e
                    @endverbatim

                  - overlap: Select maps which temporal overlap
                    the selection granule
                    @verbatim
                    map    :     s-----------e
                    granule:        s-----------------e

                    map    :     s-----------e
                    granule:  s----------e
                    @endverbatim

                  - contain: Select maps which temporally contain
                    the selection granule
                    @verbatim
                    map    :  s-----------------e
                    granule:     s-----------e
                    @endverbatim

                  - equal: Select maps which temporally equal
                    to the selection granule
                    @verbatim
                    map    :  s-----------e
                    granule:  s-----------e
                    @endverbatim

                  - follows: Select maps which temporally follow
                    the selection granule
                    @verbatim
                    map    :              s-----------e
                    granule:  s-----------e
                    @endverbatim

                  - precedes: Select maps which temporally precedes
                    the selection granule
                    @verbatim
                    map    :  s-----------e
                    granule:              s-----------e
                    @endverbatim

                  All these methods can be combined. Method must be of
                  type tuple including the identification strings.

           @param spatial If set True additional the spatial overlapping
                          is used for selection -> spatio-temporal relation.
                          The returned map objects will have temporal and
                          spatial extents
           @param dbif The database interface to be used

           @return A list of lists of map objects or None in case nothing was
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
                if name == "contain":
                    use_contain = True
                if name == "equal":
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
            core.error(_("The space time datasets must be of "
                         "the same temporal type"))
            return None

        if stds.get_map_time() != "interval":
            core.error(_("The temporal map type of the sample "
                         "dataset must be interval"))
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

        dbif, connected = init_dbif(dbif)

        obj_list = []
        sample_maps = stds.get_registered_maps_as_objects_with_gaps(
            where=None, dbif=dbif)

        for granule in sample_maps:
            # Read the spatial extent
            if spatial:
                granule.spatial_extent.select(dbif)
            start, end = granule.get_temporal_extent_as_tuple()

            where = create_temporal_relation_sql_where_statement(
                    start, end, use_start, use_during, use_overlap,
                    use_contain, use_equal, use_follows, use_precedes)

            maps = self.get_registered_maps_as_objects(
                where, "start_time", dbif)

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
                    map.set_relative_time(start, end,
                                          self.get_relative_time_unit())

                maplist.append(copy.copy(map))

            result["samples"] = maplist

            obj_list.append(copy.copy(result))

        if connected:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects_by_granularity(self, gran=None,
                                                      dbif=None):
        """!Return all registered maps as ordered (by start_time) object list
           with "gap" map objects (id==None) for spatio-temporal topological
           operations that require the temporal extent only.

           Each list entry is a list of map objects
           which are potentially located in the actual granule.

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

           @param gran The granularity string to be used, if None the
                       granularity of the space time dataset is used.
                       Absolute time has the format "number unit", relative
                       time has the format "number".
                       The unit in case of absolute time can be one of "second,
                       seconds, minute, minutes, hour, hours, day, days, week,
                       weeks, month, months, year, years". The unit of the
                       relative time granule is always the space time dataset
                       unit and can not be changed.
           @param dbif The database interface to be used

           @return ordered object list, or None in case nothing found
        """

        dbif, connected = init_dbif(dbif)

        obj_list = []

        if gran is None:
            gran = self.get_granularity()

        check = check_granularity_string(gran, self.get_temporal_type())
        if not check:
            core.fatal(_("Wrong granularity: \"%s\"") % str(gran))

        start, end = self.get_temporal_extent_as_tuple()

        if start is None or end is None:
            return None

        # Time instances and mixed time
        is_irregular = False

        # We need to adjust the end time in case the the dataset has no
        # interval time, so we can catch time instances at the end
        if self.get_map_time() != "interval":
            is_irregular = True

            if self.is_time_absolute():
                end = increment_datetime_by_string(end, gran)
            else:
                end = end + gran

        while start < end:
            if self.is_time_absolute():
                next = increment_datetime_by_string(start, gran)
            else:
                next = start + gran

            # First we search for intervals that are are equal the granule or
            # contain it
            where = create_temporal_relation_sql_where_statement(
                    start=start, end=next, use_start=False, use_during=False,
                    use_overlap=False, use_contain=True, use_equal=True,
                    use_follows=False, use_precedes=False)
            rows = self.get_registered_maps("id", where, "start_time", dbif)

            found_gap = False

            if rows is not None and len(rows) != 0:
                if len(rows) > 1:
                    core.warning(_("More than one map found in a granule. "
                                   "Temporal granularity seems to be invalid "
                                   "or the chosen granularity is not a "
                                   "greatest common divider of all intervals "
                                   "and gaps in the dataset."))

                maplist = []
                for row in rows:

                    map = self.get_new_map_instance(row["id"])

                    if self.is_time_absolute():
                        map.set_absolute_time(start, next)
                    elif self.is_time_relative():
                        map.set_relative_time(start, next,
                                              self.get_relative_time_unit())

                    maplist.append(copy.copy(map))

                obj_list.append(copy.copy(maplist))
            else:
                # We may found a gap or a gap after a time instance
                found_gap = True

                # Searching for time instances and intervals that are during
                # the current granule or overlapping it
                where = create_temporal_relation_sql_where_statement(
                        start=start, end=next, use_start=True, use_during=True,
                        use_overlap=True, use_contain=False, use_equal=False,
                        use_follows=False, use_precedes=False)

                rows = self.get_registered_maps("id,start_time,end_time",
                                                where, "start_time", dbif)

                if rows is not None and len(rows) != 0:
                    # No gap if we found something in the granule with
                    # intervaltime
                    if len(rows) > 1:
                        core.warning(_("More than one map found in a granule. "
                                       "Temporal granularity seems to be "
                                       "invalid or the chosen granularity is "
                                       "not a greatest common divider of all "
                                       "time instances in the dataset."))

                    maplist = []
                    count = 0
                    for row in rows:
                        if count == 0:
                            if row["end_time"] is not None or row["start_time"] != start:
                                found_gap = False
                        count += 1

                        map = self.get_new_map_instance(row["id"])

                        if self.is_time_absolute():
                            if row["end_time"] is not None or row["start_time"] != start:
                                map.set_absolute_time(start, next)
                            else:
                                map.set_absolute_time(start, None)
                        elif self.is_time_relative():
                            if row["end_time"] is not None or row["start_time"] != start:
                                map.set_relative_time(start, next,
                                                 self.get_relative_time_unit())
                            else:
                                map.set_relative_time(start, None,
                                                 self.get_relative_time_unit())

                        maplist.append(copy.copy(map))

                    obj_list.append(copy.copy(maplist))

                # In case of irregular time (point, mixed) the last granule
                # does not belong to the dataset and will be ignored
                if is_irregular:
                    if next == end:
                        found_gap = False

            # Gap handling
            if found_gap:
                # Append a map object with None as id to indicate a gap
                map = self.get_new_map_instance(None)

                if self.is_time_absolute():
                    map.set_absolute_time(start, next)
                elif self.is_time_relative():
                    map.set_relative_time(start, next,
                                          self.get_relative_time_unit())

                maplist = []
                maplist.append(copy.copy(map))

                obj_list.append(copy.copy(maplist))

            start = next

        if connected:
            dbif.close()

        if obj_list:
            return obj_list
        return None

    def get_registered_maps_as_objects_with_gaps(self, where=None, dbif=None):
        """!Return all or a subset of the registered maps as
           ordered (by start_time) object list with
           "gap" map objects (id==None) for spatio-temporal topological
           operations that require the spatio-temporal extent only.

           Gaps between maps are identified as maps with id==None

           The objects are initialized with their id's' and the spatio-temporal
           extent (temporal type, start time, end time, west, east, south,
           north, bottom and top).
           In case more map information are needed, use the select()
           method for each listed object.

           @param where The SQL where statement to select a
                        subset of the registered maps without "WHERE"
           @param dbif The database interface to be used

           @return ordered object list, in case nothing found None is returned
        """

        dbif, connected = init_dbif(dbif)

        obj_list = []

        maps = self.get_registered_maps_as_objects(where, "start_time", dbif)

        if maps  is not None and len(maps) > 0:
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
                            map.set_relative_time(start, end,
                                                 self.get_relative_time_unit())
                        obj_list.append(copy.copy(map))

        if connected:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects_with_temporal_topology(self, where=None,
                                                            order="start_time",
                                                            dbif=None):
        """!Return all or a subset of the registered maps as ordered object
           list with spatio-temporal topological relationship informations.

           The objects are initialized with their id's' and the spatio-temporal
           extent (temporal type, start time, end time, west, east, south,
           north, bottom and top).
           In case more map information are needed, use the select()
           method for each listed object.

           @param where The SQL where statement to select a subset of
                        the registered maps without "WHERE"
           @param order The SQL order statement to be used to order the
                        objects in the list without "ORDER BY"
           @param dbif The database interface to be used
           @return The ordered map object list,
                   In case nothing found None is returned
        """

        dbif, connected = init_dbif(dbif)
        obj_list = self.get_registered_maps_as_objects(where, order, dbif)

        tb = SpatioTemporalTopologyBuilder()
        tb.build(obj_list)

        if connected:
            dbif.close()

        return obj_list

    def get_registered_maps_as_objects(self, where=None, order="start_time",
                                       dbif=None):
        """!Return all or a subset of the registered maps as ordered object
           list for spatio-temporal topological operations that require the
           spatio-temporal extent only

           The objects are initialized with their id's' and the spatio-temporal
           extent (temporal type, start time, end time, west, east, south,
           north, bottom and top).
           In case more map information are needed, use the select()
           method for each listed object.

           @param where The SQL where statement to select a subset of
                         the registered maps without "WHERE"
           @param order The SQL order statement to be used to order the
                         objects in the list without "ORDER BY"
           @param dbif The database interface to be used
           @return The ordered map object list,
                   In case nothing found None is returned
        """

        dbif, connected = init_dbif(dbif)

        obj_list = []

        # Older temporal databases have no bottom and top columns
        # in their views so we need a work around to set the full
        # spatial extent as well
        has_bt_columns = True
        try:
            rows = self.get_registered_maps(
                "id,start_time,end_time, west,east,south,north,bottom,top",
                where, order, dbif)
        except:
            try:
                dbif.rollback()
                rows = self.get_registered_maps("id,start_time,end_time",
                                                where, order, dbif)
                has_bt_columns = False
                core.warning(_("Old temporal database format. The top and "\
                               "bottom column is missing in the views, using"\
                               "a work around."))
            except:
                raise

        if rows is not None:
            for row in rows:
                map = self.get_new_map_instance(row["id"])
                if self.is_time_absolute():
                    map.set_absolute_time(row["start_time"], row["end_time"])
                elif self.is_time_relative():
                    map.set_relative_time(row["start_time"], row["end_time"],
                                          self.get_relative_time_unit())
                # The fast way
                if has_bt_columns:
                    map.set_spatial_extent_from_values(west=row["west"],
                                                       east=row["east"],
                                           south=row["south"], top=row["top"],
                                           north=row["north"],
                                           bottom=row["bottom"])
                # The slow work around
                else:
                    map.spatial_extent.select(dbif)

                obj_list.append(copy.copy(map))

        if connected:
            dbif.close()

        return obj_list

    def get_registered_maps(self, columns=None, where=None, order=None,
                            dbif=None):
        """!Return SQL rows of all registered maps.

           In case columns are not specified, each row includes all columns
           specified in the datatype specific view.

           @param columns Columns to be selected as SQL compliant string
           @param where The SQL where statement to select a subset
                        of the registered maps without "WHERE"
           @param order The SQL order statement to be used to order the
                        objects in the list without "ORDER BY"
           @param dbif The database interface to be used

           @return SQL rows of all registered maps,
                   In case nothing found None is returned
        """

        dbif, connected = init_dbif(dbif)

        rows = None

        if self.get_map_register() is not None:
            # Use the correct temporal table
            if self.get_temporal_type() == "absolute":
                map_view = self.get_new_map_instance(
                    None).get_type() + "_view_abs_time"
            else:
                map_view = self.get_new_map_instance(
                    None).get_type() + "_view_rel_time"

            if columns is not None and columns != "":
                sql = "SELECT %s FROM %s  WHERE %s.id IN (SELECT id FROM %s)" %\
                      (columns, map_view, map_view, self.get_map_register())
            else:
                sql = "SELECT * FROM %s  WHERE %s.id IN (SELECT id FROM %s)" % \
                      (map_view, map_view, self.get_map_register())

            if where is not None and where != "":
                sql += " AND (%s)" % (where.split(";")[0])
            if order is not None and order != "":
                sql += " ORDER BY %s" % (order.split(";")[0])

            try:
                dbif.cursor.execute(sql)
                rows = dbif.cursor.fetchall()
            except:
                if connected:
                    dbif.close()
                core.error(_("Unable to get map ids from register table <%s>")
                           % (self.get_map_register()))
                raise

        if connected:
            dbif.close()

        return rows

    @staticmethod
    def shift_map_list(maps, gran):
        """!Temporally shift each map in the list with the provided granularity

           This method does not perform any temporal database operations.

           @param maps A list of maps  with initialized temporal extent
           @param gran The granularity to be used for shifting
           @return The modified map list, None if nothing to shift or wrong
                   granularity
           @code

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

           @endcode
        """
        if maps is None:
            return None

        if not check_granularity_string(gran, maps[-1].get_temporal_type()):
            core.error(_("Wrong granularity format: %s" % (gran)))
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
        """!Temporally shift each registered map with the provided granularity

           @param gran The granularity to be used for shifting
           @param dbif The database interface to be used
           @return True something to shift, False if nothing to shift or wrong
                   granularity

        """
        if not check_granularity_string(gran, self.get_temporal_type()):
            core.error(_("Wrong granularity format: %s" % (gran)))
            return False

        dbif, connected = init_dbif(dbif)

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

        self. _update_map_timestamps(maps, date_list, dbif)

        if connected:
            dbif.close()

    @staticmethod
    def snap_map_list(maps):
        """!For each map in the list snap the end time to the start time of its
           temporal nearest neighbor in the future.

           Maps with equal time stamps are not snapped.

           The granularity of the map list will be used to create the end time
           of the last map in case it has a time instance as timestamp.

           This method does not perform any temporal database operations.

           @param maps A list of maps with initialized temporal extent
           @return The modified map list, None nothing to shift or wrong
                   granularity

           Usage:

           @code

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

           @endcode
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
                    maps[i].set_relative_time(start, start_next,
                                              maps[i].get_relative_time_unit())
            else:
                if maps[i].is_time_absolute():
                    maps[i].set_absolute_time(start, end)
                elif maps[i].is_time_relative():
                    maps[i].set_relative_time(start, end,
                                              maps[i].get_relative_time_unit())
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
                maps[-1].set_relative_time(start, end,
                                           maps[-1].get_relative_time_unit())

        return maps

    def snap(self, dbif=None):
        """!For each registered map snap the end time to the start time of
           its temporal nearest neighbor in the future

           Maps with equal time stamps are not snapped

           @param dbif The database interface to be used

        """
        dbif, connected = init_dbif(dbif)

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
                end =  increment_datetime_by_string(start,
                                                    self.get_granularity())
            elif self.is_time_relative():
                end = start + self.get_granularity()

        date_list.append((start, end))

        self. _update_map_timestamps(maps, date_list, dbif)

        if connected:
            dbif.close()

    def _update_map_timestamps(self, maps, date_list, dbif):
        """!Update the timestamps of maps with the start and end time
           stored in the date_list.

           The number of dates in the list must be equal to the number
           of maps.

           @param maps A list of map objects
           @param date_list A list with date tuples (start_time, end_time)
           @param dbif The database interface to be used
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
                map.update_absolute_time(start_time=start, end_time=end,
                                         dbif=dbif)
            elif self.is_time_relative():
                map.update_relative_time(start_time=start, end_time=end,
                                         unit=self.get_relative_time_unit(),
                                         dbif=dbif)

            # Save the datasets that must be updated
            datasets = map.get_registered_datasets(dbif)
            if datasets:
                for dataset in datasets:
                    datatsets_to_modify[dataset["id"]] = dataset["id"]

        self.update_from_registered_maps(dbif)

        # Update affected datasets
        if datatsets_to_modify:
            for dataset in datatsets_to_modify:
                if dataset != self.get_id():
                    ds = self.get_new_instance(ident=dataset)
                    ds.select(dbif)
                    ds.update_from_registered_maps(dbif)

    def rename(self, ident, dbif=None):
        """!Rename the space time dataset

           This method renames the space time dataset, the map register table
           and updates the entries in registered maps stds register.

           @param ident The new identifier "name@mapset"
           @param dbif The database interface to be used
        """

        dbif, connected = init_dbif(dbif)

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
        statement += "ALTER TABLE %s RENAME TO \"%s\";\n" % \
                     (old_map_register_table, new_map_register_table)

        # We need to rename the space time dataset in the maps register table
        if maps:
            for map in maps:
                map.select()
                statement += "UPDATE %s SET id = \"%s\" WHERE id = \"%s\";\n"%\
                             (map.get_stds_register(), ident, old_ident)

        # Execute the accumulated statements
        dbif.execute_transaction(statement)

        if connected:
            dbif.close()

    def delete(self, dbif=None, execute=True):
        """!Delete a space time dataset from the temporal database

           This method removes the space time dataset from the temporal
           database and drops its map register table

           @param dbif The database interface to be used
           @param execute If True the SQL DELETE and DROP table
                          statements will be executed.
                          If False the prepared SQL statements are returned
                          and must be executed by the caller.

           @return The SQL statements if execute == False, else an empty string
        """
        # First we need to check if maps are registered in this dataset and
        # unregister them

        # Commented because of performance issue calling g.message thousend
        # times
        # core.verbose(_("Delete space time %s  dataset <%s> from temporal "
        #               "database") % \
        #             (self.get_new_map_instance(ident=None).get_type(),
        #              self.get_id()))

        statement = ""
        dbif, connected = init_dbif(dbif)

        # SELECT all needed information from the database
        self.metadata.select(dbif)

        if self.get_map_register() is not None:
            core.verbose(_("Drop map register table: %s") % (
                self.get_map_register()))
            rows = self.get_registered_maps("id", None, None, dbif)
            # Unregister each registered map in the table
            if rows is not None:
                for row in rows:
                    # Unregister map
                    map = self.get_new_map_instance(row["id"])
                    statement += self.unregister_map(
                        map=map, dbif=dbif, execute=False)

            # Safe the DROP table statement
            statement += "DROP TABLE " + self.get_map_register() + ";\n"

        # Remove the primary key, the foreign keys will be removed by trigger
        statement += self.base.get_delete_statement()

        if execute:
            dbif.execute_transaction(statement)

        self.reset(None)

        if connected:
            dbif.close()

        if execute:
            return ""

        return statement

    def register_map(self, map, dbif=None):
        """!Register a map in the space time dataset.

            This method takes care of the registration of a map
            in a space time dataset.

            In case the map is already registered this function
            will break with a warning and return False.

            This method raises a ScriptError in case of a fatal error

           @param map The AbstractMapDataset object that should be registered
           @param dbif The database interface to be used
           @return True if success, False otherwise
        """
        dbif, connected = init_dbif(dbif)

        if map.is_in_db(dbif) == False:
            dbif.close()
            core.fatal(_("Only maps with absolute or relative valid time can "
                         "be registered"))
        # Commented because of performance issue calling g.message thousend
        # times
        #if map.get_layer():
        #    core.verbose(_("Register %s map <%s> with layer %s in space "
        #                   "time %s dataset <%s>") % (map.get_type(),
        #                                              map.get_map_id(),
        #                                              map.get_layer(),
        #                                              map.get_type(),
        #                                              self.get_id()))
        #else:
        #    core.verbose(_("Register %s map <%s> in space time %s "
        #                   "dataset <%s>") % (map.get_type(), map.get_map_id(),
        #                                      map.get_type(), self.get_id()))

        # First select all data from the database
        map.select(dbif)

        if not map.check_for_correct_time():
            if map.get_layer():
                core.fatal(_("Map <%(id)s> with layer %(l)s has invalid time")
                           % {'id': map.get_map_id(), 'l': map.get_layer()})
            else:
                core.fatal(_("Map <%s> has invalid time") % (map.get_map_id()))

        map_id = map.base.get_id()
        map_name = map.base.get_name()
        map_mapset = map.base.get_mapset()
        map_register_table = map.get_stds_register()
        map_rel_time_unit = map.get_relative_time_unit()
        map_ttype = map.get_temporal_type()

        #print "Map register table", map_register_table

        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        stds_register_table = self.get_map_register()
        stds_ttype = self.get_temporal_type()

        # The gathered SQL statemets are stroed here
        statement = ""

        # Check temporal types
        if stds_ttype != map_ttype:
            if map.get_layer():
                core.fatal(_("Temporal type of space time dataset <%(id)s> and"
                             " map <%(map)s> with layer %(l)s are different") \
                             % {'id': self.get_id(), 'map': map.get_map_id(),
                                'l': map.get_layer()})
            else:
                core.fatal(_("Temporal type of space time dataset <%(id)s> and"
                             " map <%(map)s> are different") % {
                             'id': self.get_id(), 'map': map.get_map_id()})

        # In case no map has been registered yet, set the
        # relative time unit from the first map
        if (self.metadata.get_number_of_maps() is None or \
            self.metadata.get_number_of_maps() == 0) and \
            self.map_counter == 0 and self.is_time_relative():

            self.set_relative_time_unit(map_rel_time_unit)
            statement += self.relative_time.get_update_all_statement_mogrified(
                dbif)
            # Commented because of performance issue calling g.message thousend times
            #core.verbose(_("Set temporal unit for space time %s dataset "
            #               "<%s> to %s") % (map.get_type(), self.get_id(),
            #                                map_rel_time_unit))

        stds_rel_time_unit = self.get_relative_time_unit()

        # Check the relative time unit
        if self.is_time_relative() and (stds_rel_time_unit != map_rel_time_unit):
            if map.get_layer():
                core.fatal(_("Relative time units of space time dataset "
                             "<%(id)s> and map <%(map)s> with layer %(l)s are"
                             " different") % {'id': self.get_id(),
                                              'map': map.get_map_id(),
                                              'l': map.get_layer()})
            else:
                core.fatal(_("Relative time units of space time dataset "
                             "<%(id)s> and map <%(map)s> are different") % \
                             {'id': self.get_id(), 'map': map.get_map_id()})

        if stds_mapset != map_mapset:
            dbif.close()
            core.fatal(_("Only maps from the same mapset can be registered"))

        # Check if map is already registered
        if stds_register_table is not None:
            if dbif.dbmi.paramstyle == "qmark":
                sql = "SELECT id FROM " + \
                    stds_register_table + " WHERE id = (?)"
            else:
                sql = "SELECT id FROM " + \
                    stds_register_table + " WHERE id = (%s)"
            try:
                dbif.cursor.execute(sql, (map_id,))
                row = dbif.cursor.fetchone()
            except:
                core.warning(_("Error in strds_register_table request"))
                raise

            if row is not None and row[0] == map_id:
                if connected == True:
                    dbif.close()

                if map.get_layer() is not None:
                    core.warning(_("Map <%(map)s> with layer %(l)s is already "
                                   "registered.") % {'map': map.get_map_id(),
                                                     'l': map.get_layer()})
                else:
                    core.warning(_("Map <%s> is already registered.") % (
                                 map.get_map_id()))
                return False

        # Create tables
        sql_path = get_sql_template_path()

        # We need to create the map raster register table precedes we can
        # register the map
        if map_register_table is None:
            # Create a unique id
            uuid_rand = "map_" + str(uuid.uuid4()).replace("-", "")

            map_register_table = uuid_rand + "_" + \
                self.get_type() + "_register"

            # Read the SQL template
            sql = open(os.path.join(sql_path,
                                    "map_stds_register_table_template.sql"),
                                    'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("MAP_NAME", map_name + "_" + map_mapset)
            sql = sql.replace("TABLE_NAME", uuid_rand)
            sql = sql.replace("MAP_ID", map_id)
            sql = sql.replace("STDS", self.get_type())

            statement += sql

            # Set the stds register table name and put it into the DB
            map.set_stds_register(map_register_table)
            statement += map.metadata.get_update_statement_mogrified(dbif)

            # Commented because of performance issue calling g.message thousend times
            #if map.get_layer():
            #    core.verbose(_("Created register table <%s> for "
            #                   "%s map <%s> with layer %s") %
            #                    (map_register_table, map.get_type(),
            #                     map.get_map_id(), map.get_layer()))
            #else:
            #    core.verbose(_("Created register table <%s> for %s map <%s>") %
            #                    (map_register_table, map.get_type(),
            #                     map.get_map_id()))

        # We need to create the table and register it
        if stds_register_table is None:
            # Create table name
            stds_register_table = self.create_map_register_name()
            # Read the SQL template
            sql = open(os.path.join(sql_path,
                                    "stds_map_register_table_template.sql"),
                                    'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset)
            sql = sql.replace("SPACETIME_ID", self.base.get_id())
            sql = sql.replace("STDS", self.get_type())
            statement += sql

            # Set the map register table name and put it into the DB
            self.set_map_register(stds_register_table)
            statement += self.metadata.get_update_statement_mogrified(dbif)

            # Commented because of performance issue calling g.message thousend times
            #core.verbose(_("Created register table <%s> for space "
            #               "time %s  dataset <%s>") %
            #              (stds_register_table, map.get_type(), self.get_id()))

        # We need to execute the statement at this time
        if statement != "":
            dbif.execute_transaction(statement)

        statement = ""

        # Register the stds in the map stds register table
        # Check if the entry is already there
        if dbif.dbmi.paramstyle == "qmark":
            sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
        else:
            sql = "SELECT id FROM " + map_register_table + " WHERE id = %s"
        try:
            dbif.cursor.execute(sql, (self.base.get_id(),))
            row = dbif.cursor.fetchone()
        except:
            row = None

        # In case of no entry make a new one
        if row is None:
            if dbif.dbmi.paramstyle == "qmark":
                sql = "INSERT INTO " + map_register_table + \
                    " (id) " + "VALUES (?);\n"
            else:
                sql = "INSERT INTO " + map_register_table + \
                    " (id) " + "VALUES (%s);\n"

            statement += dbif.mogrify_sql_statement(
                (sql, (self.base.get_id(),)))

        # Now put the raster name in the stds map register table
        if dbif.dbmi.paramstyle == "qmark":
            sql = "INSERT INTO " + stds_register_table + \
                " (id) " + "VALUES (?);\n"
        else:
            sql = "INSERT INTO " + stds_register_table + \
                " (id) " + "VALUES (%s);\n"

        statement += dbif.mogrify_sql_statement((sql, (map_id,)))

        # Now execute the insert transaction
        dbif.execute_transaction(statement)

        if connected:
            dbif.close()

        # increase the counter
        self.map_counter += 1

        return True

    def unregister_map(self, map, dbif=None, execute=True):
        """!Unregister a map from the space time dataset.

           This method takes care of the un-registration of a map
           from a space time dataset.

           @param map The map object to unregister
           @param dbif The database interface to be used
           @param execute If True the SQL DELETE and DROP table
                           statements will be executed.
                           If False the prepared SQL statements are
                           returned and must be executed by the caller.

           @return The SQL statements if execute == False, else an empty
                   string, None in case of a failure
        """

        statement = ""

        dbif, connected = init_dbif(dbif)

        # First select needed data from the database
        map.metadata.select(dbif)

        map_id = map.get_id()
        map_register_table = map.get_stds_register()
        stds_register_table = self.get_map_register()

        # Commented because of performance issue calling g.message thousend times
        #if map.get_layer() is not None:
        #    core.verbose(_("Unregister %s map <%s> with layer %s") % \
        #                 (map.get_type(), map.get_map_id(), map.get_layer()))
        #else:
        #    core.verbose(_("Unregister %s map <%s>") % (
        #        map.get_type(), map.get_map_id()))

        # Check if the map is registered in the space time raster dataset
        if map_register_table is not None:
            if dbif.dbmi.paramstyle == "qmark":
                sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
            else:
                sql = "SELECT id FROM " + map_register_table + " WHERE id = %s"
            try:
                dbif.cursor.execute(sql, (self.base.get_id(),))
                row = dbif.cursor.fetchone()
            except:
                row = None

            # Break if the map is not registered
            if row is None:
                if map.get_layer() is not None:
                    core.warning(_("Map <%(map)s> with layer %(l)s is not "
                                   "registered in space time dataset "
                                   "<%(base)s>") % {'map': map.get_map_id(),
                                                    'l': map.get_layer(),
                                                    'base': self.base.get_id()})
                else:
                    core.warning(_("Map <%(map)s> is not registered in space "
                                   "time dataset <%(base)s>") % {
                                   'map': map.get_map_id(),
                                   'base': self.base.get_id()})
                if connected == True:
                    dbif.close()
                return ""

        # Remove the space time raster dataset from the raster dataset register
        if map_register_table is not None:
            if dbif.dbmi.paramstyle == "qmark":
                sql = "DELETE FROM " + map_register_table + " WHERE id = ?;\n"
            else:
                sql = "DELETE FROM " + map_register_table + " WHERE id = %s;\n"

            statement += dbif.mogrify_sql_statement(
                (sql, (self.base.get_id(),)))

        # Remove the raster map from the space time raster dataset register
        if stds_register_table is not None:
            if dbif.dbmi.paramstyle == "qmark":
                sql = "DELETE FROM " + stds_register_table + " WHERE id = ?;\n"
            else:
                sql = "DELETE FROM " + \
                    stds_register_table + " WHERE id = %s;\n"

            statement += dbif.mogrify_sql_statement((sql, (map_id,)))

        if execute:
            dbif.execute_transaction(statement)

        if connected:
            dbif.close()

        # decrease the counter
        self.map_counter -= 1

        if execute:
            return ""

        return statement

    def update_from_registered_maps(self, dbif=None):
        """!This methods updates the spatial and temporal extent as well as
           type specific metadata. It should always been called after maps
           are registered or unregistered/deleted from the space time dataset.

           The update of the temporal extent checks if the end time is set
           correctly.
           In case the registered maps have no valid end time (None) the
           maximum start time
           will be used. If the end time is earlier than the maximum start
           time, it will be replaced by the maximum start time.

           An other solution to automate this is to use the deactivated trigger
           in the SQL files. But this will result in a huge performance issue
           in case many maps are registered (>1000).

           @param dbif The database interface to be used
        """
        core.verbose(_("Update metadata, spatial and temporal extent from "
                       "all registered maps of <%s>") % (self.get_id()))

        # Nothing to do if the register is not present
        if not self.get_map_register():
            return

        dbif, connected = init_dbif(dbif)

        map_time = None

        use_start_time = False

        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        sql_path = get_sql_template_path()

        #We create a transaction
        sql_script = ""

        # Update the spatial and temporal extent from registered maps
        # Read the SQL template
        sql = open(os.path.join(sql_path,
                   "update_stds_spatial_temporal_extent_template.sql"),
                   'r').read()
        sql = sql.replace(
            "GRASS_MAP", self.get_new_map_instance(None).get_type())
        sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset)
        sql = sql.replace("SPACETIME_ID", self.base.get_id())
        sql = sql.replace("STDS", self.get_type())

        sql_script += sql
        sql_script += "\n"

        # Update type specific metadata
        sql = open(os.path.join(sql_path, "update_" +
            self.get_type() + "_metadata_template.sql"), 'r').read()
        sql = sql.replace(
            "GRASS_MAP", self.get_new_map_instance(None).get_type())
        sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset)
        sql = sql.replace("SPACETIME_ID", self.base.get_id())
        sql = sql.replace("STDS", self.get_type())

        sql_script += sql
        sql_script += "\n"

        dbif.execute_transaction(sql_script)

        # Read and validate the selected end time
        self.select()

        if self.is_time_absolute():
            start_time, end_time, tz = self.get_absolute_time()
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
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register);"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(
                    None).get_type())
                sql = sql.replace("SPACETIME_NAME",
                    stds_name + "_" + stds_mapset)
            else:
                sql = """SELECT max(start_time) FROM GRASS_MAP_relative_time
                         WHERE GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register);"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(
                    None).get_type())
                sql = sql.replace("SPACETIME_NAME",
                    stds_name + "_" + stds_mapset)

            dbif.cursor.execute(sql)
            row = dbif.cursor.fetchone()

            if row is not None:
                # This seems to be a bug in sqlite3 Python driver
                if dbif.dbmi.__name__ == "sqlite3":
                    tstring = row[0]
                    # Convert the unicode string into the datetime format
                    if self.is_time_absolute():
                        if tstring.find(":") > 0:
                            time_format = "%Y-%m-%d %H:%M:%S"
                        else:
                            time_format = "%Y-%m-%d"

                        max_start_time = datetime.strptime(
                            tstring, time_format)
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
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(
                    None).get_type())
                sql = sql.replace("SPACETIME_NAME",
                    stds_name + "_" + stds_mapset)
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())
            elif self.is_time_relative():
                sql = """UPDATE STDS_relative_time SET end_time =
               (SELECT max(start_time) FROM GRASS_MAP_relative_time WHERE
               GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(
                    None).get_type())
                sql = sql.replace("SPACETIME_NAME",
                    stds_name + "_" + stds_mapset)
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())

            dbif.execute_transaction(sql)

        # Count the temporal map types
        maps = self.get_registered_maps_as_objects(dbif=dbif)
        tlist = self.count_temporal_types(maps)

        if tlist["interval"] > 0 and tlist["point"] == 0 and \
           tlist["invalid"] == 0:
            map_time = "interval"
        elif tlist["interval"] == 0 and tlist["point"] > 0 and \
             tlist["invalid"] == 0:
            map_time = "point"
        elif tlist["interval"] > 0 and tlist["point"] > 0 and \
             tlist["invalid"] == 0:
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

        if connected:
            dbif.close()

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
