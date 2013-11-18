"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.aggregate_raster_maps(dataset, mapset, inputs, base, start, end,
    count, method, register_null, dbif)

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


def collect_map_names(sp, dbif, start, end, sampling):
    """!Gather all maps from dataset using a specific sample method

       @param sp The space time raster dataset to select aps from
       @param dbif The temporal database interface to use
       @param start The start time of the sample interval, may be relative or
              absolute
       @param end The end time of the sample interval, may be relative or
              absolute
       @param sampling The sampling methods to use
    """

    use_start = False
    use_during = False
    use_overlap = False
    use_contain = False
    use_equal = False
    use_follows = False
    use_precedes = False

    # Initialize the methods
    if sampling:
        for name in sampling.split(","):
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
        use_start = True

    if sp.get_map_time() != "interval":
        use_start = True
        use_during = False
        use_overlap = False
        use_contain = False
        use_equal = False
        use_follows = False
        use_precedes = False

    where = create_temporal_relation_sql_where_statement(start, end,
                                                         use_start,
                                                         use_during,
                                                         use_overlap,
                                                         use_contain,
                                                         use_equal,
                                                         use_follows,
                                                         use_precedes)

    rows = sp.get_registered_maps("id", where, "start_time", dbif)

    if not rows:
        return None

    names = []
    for row in rows:
        names.append(row["id"])

    return names

###############################################################################


def aggregate_raster_maps(inputs, base, start, end, count, method,
                          register_null, dbif):
    """!Aggregate a list of raster input maps with r.series

       @param inputs The names of the raster maps to be aggregated
       @param base The basename of the new created raster maps
       @param start The start time of the sample interval, may be relative or
                    absolute
       @param end The end time of the sample interval, may be relative or
                  absolute
       @param count The number to be attached to the basename of the new
                    created raster map
       @param method The aggreation method to be used by r.series
       @param register_null If true null maps will be registered in the space
                            time raster dataset, if false not
       @param dbif The temporal database interface to use
    """

    msgr = get_tgis_message_interface()

    msgr.verbose(_("Aggregate %s raster maps") % (len(inputs)))
    output = "%s_%i" % (base, count)

    mapset = get_current_mapset()
    map_id = output + "@" + mapset
    new_map = RasterDataset(map_id)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        if core.overwrite() == True:
            # Remove the existing temporal database entry
            new_map.delete(dbif)
            new_map = RasterDataset(map_id)
        else:
            msgr.error(_("Raster map <%s> is already in temporal database, " \
                         "use overwrite flag to overwrite"))
            return

    msgr.verbose(_("Compute aggregation of maps between %(st)s - %(end)s" % {
                   'st': str(start), 'end': str(end)}))

    # Create the r.series input file
    filename = core.tempfile(True)
    file = open(filename, 'w')

    for name in inputs:
        string = "%s\n" % (name)
        file.write(string)

    file.close()
    # Run r.series
    ret = core.run_command("r.series", flags="z", file=filename,
                           output=output, overwrite=core.overwrite(),
                           method=method)

    if ret != 0:
        dbif.close()
        core.fatal(_("Error while r.series computation"))

    # Read the raster map data
    new_map.load()

    # In case of a null map continue, do not register null maps
    if new_map.metadata.get_min() is None and new_map.metadata.get_max() is None:
        if not register_null:
            core.run_command("g.remove", rast=output)
            return None

    return new_map
