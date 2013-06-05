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
from factory import *

###############################################################################

def list_maps_of_stds(type, input, columns, order, where, separator, method, header, gran=None):
    """! List the maps of a space time dataset using diffetent methods

        @param type The type of the maps raster, raster3d or vector
        @param input Name of a space time raster dataset
        @param columns A comma separated list of columns to be printed to stdout
        @param order A comma separated list of columns to order the
                      space time dataset by category
        @param where A where statement for selected listing without "WHERE"
                      e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
        @param separator The field separator character between the columns
        @param method String identifier to select a method out of cols,
                       comma,delta or deltagaps
            - "cols" Print preselected columns specified by columns
            - "comma" Print the map ids ("name@mapset") as comma separated string
            - "delta" Print the map ids ("name@mapset") with start time,
                       end time, relative length of intervals and the relative
                       distance to the begin
            - "deltagaps" Same as "delta" with additional listing of gaps.
                           Gaps can be simply identified as the id is "None"
            - "gran" List map using the granularity of the space time dataset,
                      columns are identical to deltagaps
        @param header Set True to print column names
        @param gran The user defined granule to be used if method=gran is set, in case gran=None the 
            granule of the space time dataset is used
    """
    mapset = core.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    dbif, connected = init_dbif(None)
    
    sp = dataset_factory(type, id)

    if not sp.is_in_db(dbif=dbif):
        core.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select(dbif=dbif)

    if separator is None or separator == "":
        separator = "\t"

    # This method expects a list of objects for gap detection
    if method == "delta" or method == "deltagaps" or method == "gran":
        if type == "stvds":
            columns = "id,name,layer,mapset,start_time,end_time"
        else:
            columns = "id,name,mapset,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where=where, dbif=dbif)
        elif method == "delta":
            maps = sp.get_registered_maps_as_objects(where=where, order="start_time", dbif=dbif)
        elif method == "gran":
            if gran is not None and gran != "":
                maps = sp.get_registered_maps_as_objects_by_granularity(gran=gran, dbif=dbif)
            else:
                maps = sp.get_registered_maps_as_objects_by_granularity(dbif=dbif)
            
        if header:
            string = ""
            string += "%s%s" % ("id", separator)
            string += "%s%s" % ("name", separator)
            if type == "stvds":
                string += "%s%s" % ("layer", separator)
            string += "%s%s" % ("mapset", separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s" % ("distance_from_begin")
            print string

        if maps and len(maps) > 0:

            if isinstance(maps[0], list):
                if len(maps[0]) > 0:
                    first_time, dummy = maps[0][0].get_temporal_extent_as_tuple()
                else:
                    core.warning(_("Empty map list."))
                    return
            else:
                first_time, dummy = maps[0].get_temporal_extent_as_tuple()

            for mymap in maps:

                if isinstance(mymap, list):
                    if len(mymap) > 0:
                        map = mymap[0]
                    else:
                        core.fatal(_("Empty entry in map list, this should not happen."))
                else:
                    map = mymap

                start, end = map.get_temporal_extent_as_tuple()
                if end:
                    delta = end - start
                else:
                    delta = None
                delta_first = start - first_time

                if map.is_time_absolute():
                    if end:
                        delta = time_delta_to_relative_time(delta)
                    delta_first = time_delta_to_relative_time(delta_first)

                string = ""
                string += "%s%s" % (map.get_id(), separator)
                string += "%s%s" % (map.get_name(), separator)
                if type == "stvds":
                    string += "%s%s" % (map.get_layer(), separator)
                string += "%s%s" % (map.get_mapset(), separator)
                string += "%s%s" % (start, separator)
                string += "%s%s" % (end, separator)
                string += "%s%s" % (delta, separator)
                string += "%s" % (delta_first)
                print string

    else:
        # In comma separated mode only map ids are needed
        if method == "comma":
            columns = "id"

        rows = sp.get_registered_maps(columns, where, order, dbif)

        if rows:
            if method == "comma":
                string = ""
                count = 0
                for row in rows:
                    if count == 0:
                        string += row["id"]
                    else:
                        string += ",%s" % row["id"]
                    count += 1
                print string

            elif method == "cols":
                # Print the column names if requested
                if header:
                    output = ""
                    count = 0

                    collist = columns.split(",")

                    for key in collist:
                        if count > 0:
                            output += separator + str(key)
                        else:
                            output += str(key)
                        count += 1
                    print output

                for row in rows:
                    output = ""
                    count = 0
                    for col in row:
                        if count > 0:
                            output += separator + str(col)
                        else:
                            output += str(col)
                        count += 1

                    print output
    if connected:
        dbif.close()
