"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from multiprocessing import Process

############################################################################


def dataset_mapcalculator(inputs, output, type, expression, base, method, 
                          nprocs=1, register_null=False, spatial=False):
    """!Perform map-calculations of maps from different space time 
       raster/raster3d datasets, using a specific sampling method 
       to select temporal related maps.

       A mapcalc expression can be provided to process the temporal 
       extracted maps.
       Mapcalc expressions are supported for raster and raster3d maps.

       @param input The name of the input space time raster/raster3d dataset
       @param output The name of the extracted new space time raster(3d) dataset
       @param type The type of the dataset: "raster" or "raster3d"
       @param method The method to be used for temporal sampling
       @param expression The r(3).mapcalc expression
       @param base The base name of the new created maps in case a 
              mapclac expression is provided
       @param nprocs The number of parallel processes to be used for 
              mapcalc processing
       @param register_null Set this number True to register empty maps
       @param spatial Check spatial overlap
    """

    # We need a database interface for fast computation
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = core.gisenv()["MAPSET"]

    input_name_list = inputs.split(",")

    # Process the first input
    if input_name_list[0].find("@") >= 0:
        id = input_name_list[0]
    else:
        id = input_name_list[0] + "@" + mapset

    if type == "raster":
        first_input = SpaceTimeRasterDataset(id)
    else:
        first_input = SpaceTimeRaster3DDataset(id)

    if not first_input.is_in_db(dbif):
        dbif.close()
        core.fatal(_("Space time %s dataset <%s> not found") % (type, id))

    # Fill the object with data from the temporal database
    first_input.select(dbif)

    # All additional inputs in reverse sorted order to avoid 
    # wrong name substitution
    input_name_list = input_name_list[1:]
    input_name_list.sort()
    input_name_list.reverse()
    input_list = []

    for input in input_name_list:

        if input.find("@") >= 0:
            id = input
        else:
            id = input + "@" + mapset

        sp = first_input.get_new_instance(id)

        if not sp.is_in_db(dbif):
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> not "
                         "found in temporal database") % (type, id))

        sp.select(dbif)

        input_list.append(copy.copy(sp))

    # Create the new space time dataset
    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    new_sp = first_input.get_new_instance(out_id)

    # Check if in database
    if new_sp.is_in_db(dbif):
        if not core.overwrite():
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> is already in database, "
                         "use overwrite flag to overwrite") % (type, out_id))

    # Sample all inputs by the first input and create a sample matrix
    if spatial:
        core.message(_("Start spatio-temporal sampling"))
    else:
        core.message(_("Start temporal sampling"))
    map_matrix = []
    id_list = []
    sample_map_list = []
    # First entry is the first dataset id
    id_list.append(first_input.get_name())

    if len(input_list) > 0:
        has_samples = False
        for dataset in input_list:
            list = dataset.sample_by_dataset(stds=first_input,
                                             method=method, spatial=spatial, 
                                             dbif=dbif)

            # In case samples are not found
            if not list and len(list) == 0:
                dbif.close()
                core.message(_("No samples found for map calculation"))
                return 0

            # The fist entries are the samples
            map_name_list = []
            if not has_samples:
                for entry in list:
                    granule = entry["granule"]
                    # Do not consider gaps
                    if granule.get_id() is None:
                        continue
                    sample_map_list.append(granule)
                    map_name_list.append(granule.get_name())
                # Attach the map names
                map_matrix.append(copy.copy(map_name_list))
                has_samples = True

            map_name_list = []
            for entry in list:
                maplist = entry["samples"]
                granule = entry["granule"]

                # Do not consider gaps in the sampler
                if granule.get_id() is None:
                    continue

                if len(maplist) > 1:
                    core.warning(_("Found more than a single map in a sample "
                                   "granule. Only the first map is used for "
                                   "computation. Use t.rast.aggregate.ds to "
                                   "create synchronous raster datasets."))

                # Store all maps! This includes non existent maps, 
                # identified by id == None
                map_name_list.append(maplist[0].get_name())

            # Attach the map names
            map_matrix.append(copy.copy(map_name_list))

            id_list.append(dataset.get_name())
    else:
        list = first_input.get_registered_maps_as_objects(dbif=dbif)

        if list is None:
            dbif.close()
            core.message(_("No maps in input dataset"))
            return 0

        map_name_list = []
        for map in list:
            map_name_list.append(map.get_name())
            sample_map_list.append(map)

        # Attach the map names
        map_matrix.append(copy.copy(map_name_list))

    # Needed for map registration
    map_list = []

    if len(map_matrix) > 0:

        core.message(_("Start mapcalc computation"))

        count = 0
        # Get the number of samples
        num = len(map_matrix[0])

        # Parallel processing
        proc_list = []
        proc_count = 0

        # For all samples
        for i in range(num):

            count += 1
            core.percent(count, num, 1)

            # Create the r.mapcalc statement for the current time step
            map_name = "%s_%i" % (base, count)
            expr = "%s = %s" % (map_name, expression)

            # Check that all maps are in the sample
            valid_maps = True
            # Replace all dataset names with their map names of the 
            # current time step
            for j in range(len(map_matrix)):
                if map_matrix[j][i] is None:
                    valid_maps = False
                    break
                # Substitute the dataset name with the map name
                expr = expr.replace(id_list[j], map_matrix[j][i])

            # Proceed with the next sample
            if not valid_maps:
                continue

            # Create the new map id and check if the map is already 
            # in the database
            map_id = map_name + "@" + mapset

            new_map = first_input.get_new_map_instance(map_id)

            # Check if new map is in the temporal database
            if new_map.is_in_db(dbif):
                if core.overwrite():
                    # Remove the existing temporal database entry
                    new_map.delete(dbif)
                    new_map = first_input.get_new_map_instance(map_id)
                else:
                    core.error(_("Map <%s> is already in temporal database, "
                                 "use overwrite flag to overwrite"))
                    continue

            # Set the time stamp
            if sample_map_list[i].is_time_absolute():
                start, end, tz = sample_map_list[i].get_absolute_time()
                new_map.set_absolute_time(start, end, tz)
            else:
                start, end, unit = sample_map_list[i].get_relative_time()
                new_map.set_relative_time(start, end, unit)

            map_list.append(new_map)

            # Start the parallel r.mapcalc computation
            core.verbose(_("Apply mapcalc expression: \"%s\"") % expr)

            if type == "raster":
                proc_list.append(Process(target=run_mapcalc2d, args=(expr,)))
            else:
                proc_list.append(Process(target=run_mapcalc3d, args=(expr,)))
            proc_list[proc_count].start()
            proc_count += 1

            if proc_count == nprocs or proc_count == num:
                proc_count = 0
                exitcodes = 0
                for proc in proc_list:
                    proc.join()
                    exitcodes += proc.exitcode

                if exitcodes != 0:
                    dbif.close()
                    core.fatal(_("Error while mapcalc computation"))

                # Empty process list
                proc_list = []

        # Register the new maps in the output space time dataset
        core.message(_("Start map registration in temporal database"))

        # Overwrite an existing dataset if requested
        if new_sp.is_in_db(dbif):
            if core.overwrite():
                new_sp.delete(dbif)
                new_sp = first_input.get_new_instance(out_id)

        # Copy the ids from the first input
        temporal_type, semantic_type, title, description = first_input.get_initial_values()
        new_sp.set_initial_values(
            temporal_type, semantic_type, title, description)
        # Insert the dataset in the temporal database
        new_sp.insert(dbif)

        count = 0

        # collect empty maps to remove them
        empty_maps = []

        # Insert maps in the temporal database and in the new space time dataset
        for new_map in map_list:

            count += 1
            core.percent(count, num, 1)

            # Read the map data
            new_map.load()

            # In case of a null map continue, do not register null maps
            if new_map.metadata.get_min() is None and \
               new_map.metadata.get_max() is None:
                if not register_null:
                    empty_maps.append(new_map)
                    continue

            # Insert map in temporal database
            new_map.insert(dbif)

            new_sp.register_map(new_map, dbif)

        # Update the spatio-temporal extent and the metadata table entries
        new_sp.update_from_registered_maps(dbif)

        core.percent(1, 1, 1)

        # Remove empty maps
        if len(empty_maps) > 0:
            names = ""
            count = 0
            for map in empty_maps:
                if count == 0:
                    names += "%s" % (map.get_name())
                else:
                    names += ",%s" % (map.get_name())
                count += 1
            if type == "raster":
                core.run_command("g.remove", rast=names, quiet=True)
            elif type == "raster3d":
                core.run_command("g.remove", rast3d=names, quiet=True)

    dbif.close()


###############################################################################

def run_mapcalc2d(expr):
    """Helper function to run r.mapcalc in parallel"""
    return core.run_command("r.mapcalc", expression=expr, 
                            overwrite=core.overwrite(), quiet=True)

###############################################################################


def run_mapcalc3d(expr):
    """Helper function to run r3.mapcalc in parallel"""
    return core.run_command("r3.mapcalc", expression=expr, 
                            overwrite=core.overwrite(), quiet=True)
