"""
Aggregation methods for space time raster datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.aggregate_raster_maps(dataset, mapset, inputs, base, start, end, count, method, register_null, dbif)

(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:author: Soeren Gebbert
"""

import grass.script as gscript
from grass.exceptions import CalledModuleError
from .space_time_datasets import RasterDataset
from .datetime_math import create_suffix_from_datetime
from .datetime_math import create_time_suffix
from .datetime_math import create_numeric_suffix
from .core import get_current_mapset, get_tgis_message_interface, init_dbif
from .spatio_temporal_relationships import SpatioTemporalTopologyBuilder, \
    create_temporal_relation_sql_where_statement
###############################################################################


def collect_map_names(sp, dbif, start, end, sampling):
    """Gather all maps from dataset using a specific sample method

       :param sp: The space time raster dataset to select aps from
       :param dbif: The temporal database interface to use
       :param start: The start time of the sample interval, may be relative or
              absolute
       :param end: The end time of the sample interval, may be relative or
              absolute
       :param sampling: The sampling methods to use
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
                          register_null, dbif, offset=0):
    """Aggregate a list of raster input maps with r.series

       :param inputs: The names of the raster maps to be aggregated
       :param base: The basename of the new created raster maps
       :param start: The start time of the sample interval, may be relative or
                    absolute
       :param end: The end time of the sample interval, may be relative or
                  absolute
       :param count: The number to be attached to the basename of the new
                    created raster map
       :param method: The aggreation method to be used by r.series
       :param register_null: If true null maps will be registered in the space
                            time raster dataset, if false not
       :param dbif: The temporal database interface to use
       :param offset: Offset to be added to the map counter to create the map ids
    """

    msgr = get_tgis_message_interface()

    msgr.verbose(_("Aggregating %s raster maps") % (len(inputs)))
    output = "%s_%i" % (base, int(offset) + count)

    mapset = get_current_mapset()
    map_id = output + "@" + mapset
    new_map = RasterDataset(map_id)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        if gscript.overwrite() is True:
            # Remove the existing temporal database entry
            new_map.delete(dbif)
            new_map = RasterDataset(map_id)
        else:
            msgr.error(_("Raster map <%(name)s> is already in temporal "
                         "database, use overwrite flag to overwrite" %
                       ({"name": new_map.get_name()})))
            return

    msgr.verbose(_("Computing aggregation of maps between %(st)s - %(end)s" % {
                   'st': str(start), 'end': str(end)}))

    # Create the r.series input file
    filename = gscript.tempfile(True)
    file = open(filename, 'w')

    for name in inputs:
        string = "%s\n" % (name)
        file.write(string)

    file.close()
    # Run r.series
    try:
        if len(inputs) > 1000:
            gscript.run_command("r.series", flags="z", file=filename,
                                output=output, overwrite=gscript.overwrite(),
                                method=method)
        else:
            gscript.run_command("r.series", file=filename,
                                output=output, overwrite=gscript.overwrite(),
                                method=method)

    except CalledModuleError:
        dbif.close()
        msgr.fatal(_("Error occurred in r.series computation"))

    # Read the raster map data
    new_map.load()

    # In case of a null map continue, do not register null maps
    if new_map.metadata.get_min() is None and \
       new_map.metadata.get_max() is None:
        if not register_null:
            gscript.run_command("g.remove", flags='f', type='raster',
                                name=output)
            return None

    return new_map

##############################################################################


def aggregate_by_topology(granularity_list, granularity, map_list, topo_list,
                          basename, time_suffix, offset=0, method="average",
                          nprocs=1, spatial=None, dbif=None, overwrite=False,
                          file_limit=1000):
    """Aggregate a list of raster input maps with r.series

       :param granularity_list: A list of AbstractMapDataset objects.
                                The temporal extents of the objects are used
                                to build the spatio-temporal topology with the
                                map list objects
       :param granularity: The granularity of the granularity list
       :param map_list: A list of RasterDataset objects that contain the raster
                        maps that should be aggregated
       :param topo_list: A list of strings of topological relations that are
                         used to select the raster maps for aggregation
       :param basename: The basename of the new generated raster maps
       :param time_suffix: Use the granularity truncated start time of the
                           actual granule to create the suffix for the basename
       :param offset: Use a numerical offset for suffix generation
                      (overwritten by time_suffix)
       :param method: The aggregation method of r.series (average,min,max, ...)
       :param nprocs: The number of processes used for parallel computation
       :param spatial: This indicates if the spatial topology is created as
                       well: spatial can be None (no spatial topology), "2D"
                       using west, east, south, north or "3D" using west,
                       east, south, north, bottom, top
       :param dbif: The database interface to be used
       :param overwrite: Overwrite existing raster maps
       :param file_limit: The maximum number of raster map layers that
                          should be opened at once by r.series
       :return: A list of RasterDataset objects that contain the new map names
                and the temporal extent for map registration
    """
    import grass.pygrass.modules as pymod
    import copy

    msgr = get_tgis_message_interface()

    dbif, connected = init_dbif(dbif)

    topo_builder = SpatioTemporalTopologyBuilder()
    topo_builder.build(mapsA=granularity_list, mapsB=map_list, spatial=spatial)

    # The module queue for parallel execution
    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    # Dummy process object that will be deep copied
    # and be put into the process queue
    r_series = pymod.Module("r.series", output="spam", method=[method],
                            overwrite=overwrite, quiet=True, run_=False,
                            finish_=False)
    g_copy = pymod.Module("g.copy", raster=['spam', 'spamspam'],
                          quiet=True, run_=False, finish_=False)
    output_list = []
    count = 0

    for granule in granularity_list:
        msgr.percent(count, len(granularity_list), 1)
        count += 1

        aggregation_list = []

        if "equal" in topo_list and granule.equal:
            for map_layer in granule.equal:
                aggregation_list.append(map_layer.get_name())
        if "contains" in topo_list and granule.contains:
            for map_layer in granule.contains:
                aggregation_list.append(map_layer.get_name())
        if "during" in topo_list and granule.during:
            for map_layer in granule.during:
                aggregation_list.append(map_layer.get_name())
        if "starts" in topo_list and granule.starts:
            for map_layer in granule.starts:
                aggregation_list.append(map_layer.get_name())
        if "started" in topo_list and granule.started:
            for map_layer in granule.started:
                aggregation_list.append(map_layer.get_name())
        if "finishes" in topo_list and granule.finishes:
            for map_layer in granule.finishes:
                aggregation_list.append(map_layer.get_name())
        if "finished" in topo_list and granule.finished:
            for map_layer in granule.finished:
                aggregation_list.append(map_layer.get_name())
        if "overlaps" in topo_list and granule.overlaps:
            for map_layer in granule.overlaps:
                aggregation_list.append(map_layer.get_name())
        if "overlapped" in topo_list and granule.overlapped:
            for map_layer in granule.overlapped:
                aggregation_list.append(map_layer.get_name())

        if aggregation_list:
            msgr.verbose(_("Aggregating %(len)i raster maps from %(start)s to"
                           " %(end)s")  % ({"len": len(aggregation_list),
                           "start": str(granule.temporal_extent.get_start_time()),
                               "end": str(granule.temporal_extent.get_end_time())}))

            if granule.is_time_absolute() is True and time_suffix == 'gran':
                suffix = create_suffix_from_datetime(granule.temporal_extent.get_start_time(),
                                                     granularity)
                output_name = "{ba}_{su}".format(ba=basename, su=suffix)
            elif granule.is_time_absolute() is True and time_suffix == 'time':
                suffix = create_time_suffix(granule)
                output_name = "{ba}_{su}".format(ba=basename, su=suffix)
            else:
                output_name = create_numeric_suffix(basename, count + int(offset),
                                                    time_suffix)

            map_layer = RasterDataset("%s@%s" % (output_name,
                                                 get_current_mapset()))
            map_layer.set_temporal_extent(granule.get_temporal_extent())

            if map_layer.map_exists() is True and overwrite is False:
                msgr.fatal(_("Unable to perform aggregation. Output raster "
                             "map <%(name)s> exists and overwrite flag was "
                             "not set" % ({"name": output_name})))

            output_list.append(map_layer)

            if len(aggregation_list) > 1:
                # Create the r.series input file
                filename = gscript.tempfile(True)
                file = open(filename, 'w')
                for name in aggregation_list:
                    string = "%s\n" % (name)
                    file.write(string)
                file.close()

                mod = copy.deepcopy(r_series)
                mod(file=filename, output=output_name)
                if len(aggregation_list) > int(file_limit):
                    msgr.warning(_("The limit of open files (%i) was "
                                   "reached (%i). The module r.series will "
                                   "be run with flag z, to avoid open "
                                   "files limit exceeding." % (int(file_limit),
                                                             len(aggregation_list))))
                    mod(flags="z")
                process_queue.put(mod)
            else:
                mod = copy.deepcopy(g_copy)
                mod(raster=[aggregation_list[0], output_name])
                process_queue.put(mod)

    process_queue.wait()

    if connected:
        dbif.close()

    msgr.percent(1, 1, 1)

    return output_list
