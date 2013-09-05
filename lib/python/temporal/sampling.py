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


def sample_stds_by_stds_topology(intype, sampletype, inputs, sampler, header,
                                 separator, method, spatial=False,
                                 print_only=True):
    """!Sample the input space time datasets with a sample
       space time dataset, return the created map matrix and optionally
       print the result to stdout

        In case multiple maps are located in the current granule,
        the map names are separated by comma.

        In case a layer is present, the names map ids are extended
        in this form: "name:layer@mapset"

        Attention: Do not use the comma as separator for printing

        @param intype  Type of the input space time dataset (strds, stvds or str3ds)
        @param sampletype Type of the sample space time dataset (strds, stvds or str3ds)
        @param inputs Name or comma separated names of space time datasets
        @param sampler Name of a space time dataset used for temporal sampling
        @param header Set True to print column names
        @param separator The field separator character between the columns
        @param method The method to be used for temporal sampling
                       (start,during,contain,overlap,equal)
        @param spatial Perform spatial overlapping check
        @param print_only If set True (default) then the result of the sampling will be
                    printed to stdout, if set to False the resulting map matrix
                    will be returned.

        @return The map matrix or None if nothing found
    """
    mapset = core.gisenv()["MAPSET"]

    # Make a method list
    method = method.split(",")

    # Split the inputs
    input_list = inputs.split(",")
    sts = []

    for input in input_list:
        if input.find("@") >= 0:
            id = input
        else:
            id = input + "@" + mapset

        st = dataset_factory(intype, id)
        sts.append(st)

    if sampler.find("@") >= 0:
        sid = sampler
    else:
        sid = sampler + "@" + mapset

    sst = dataset_factory(sampletype, sid)

    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    for st in sts:
        if st.is_in_db(dbif) == False:
            core.fatal(_("Dataset <%s> not found in temporal database") % (st.get_id()))
        st.select(dbif)

    if sst.is_in_db(dbif) == False:
        core.fatal(_("Dataset <%s> not found in temporal database") % (sid))

    sst.select(dbif)

    if separator is None or separator == "" or separator.find(",") >= 0:
        separator = " | "

    mapmatrizes = []
    for st in sts:
        mapmatrix = st.sample_by_dataset(sst, method, spatial, dbif)
        if mapmatrix and len(mapmatrix) > 0:
            mapmatrizes.append(mapmatrix)

    if len(mapmatrizes) > 0:

        # Simply return the map matrix
        if not print_only:
            dbif.close()
            return mapmatrizes

        if header:
            string = ""
            string += "%s%s" % (sst.get_id(), separator)
            for st in sts:
                string += "%s%s" % (st.get_id(), separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s" % ("distance_from_begin")
            print string

        first_time, dummy = mapmatrizes[0][0]["granule"].get_temporal_extent_as_tuple()

        for i in range(len(mapmatrizes[0])):
            mapname_list = []
            for mapmatrix in mapmatrizes:
                mapnames = ""
                count = 0
                entry = mapmatrix[i]
                for sample in entry["samples"]:
                    if count == 0:
                        mapnames += str(sample.get_id())
                    else:
                        mapnames += ",%s" % str(sample.get_id())
                    count += 1
                mapname_list.append(mapnames)

            entry = mapmatrizes[0][i]
            map = entry["granule"]

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
            for mapnames in mapname_list:
                string += "%s%s" % (mapnames, separator)
            string += "%s%s" % (start, separator)
            string += "%s%s" % (end, separator)
            string += "%s%s" % (delta, separator)
            string += "%s" % (delta_first)
            print string

    dbif.close()
    if len(mapmatrizes) > 0:
        return mapmatrizes

    return None
