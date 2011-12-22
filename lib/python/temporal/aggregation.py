"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.aggregate_raster_maps(dataset, mapset, inputs, base, start, end, count, method, register_null, dbif)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *

def collect_map_names(sp, dbif, start, end, sampling):
    
    use_start = False
    use_during = False
    use_overlap = False
    use_contain = False
    use_equal = False

    # Inititalize the methods
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
    else:
        use_start = True

    if sp.get_map_time() != "interval":
        use_start = True
        use_during = False
        use_overlap = False
        use_contain = False
        use_equal = False

    where = create_temporal_relation_sql_where_statement(start, end, use_start, use_during, use_overlap, use_contain, use_equal)
   
    rows = sp.get_registered_maps("id", where, "start_time", dbif)

    if not rows:
        return None

    names = []
    for row in rows:
        names.append(row["id"])

    return names    

def aggregate_raster_maps(dataset, mapset, inputs, base, start, end, count, method, register_null, dbif):

    core.verbose(_("Aggregate %s raster maps") %(len(inputs)))
    output = "%s_%i" % (base, count)

    map_id = output + "@" + mapset

    new_map = dataset.get_new_map_instance(map_id)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        if core.overwrite() == True:
            # Remove the existing temporal database entry
            new_map.delete(dbif)
            new_map = dataset.get_new_map_instance(map_id)
        else:
            core.error(_("Raster map <%s> is already in temporal database, use overwrite flag to overwrite"))
            return

    core.verbose(_("Compute aggregation of maps between %s - %s" % (str(start), str(end))))

    # Create the r.series input file
    filename = core.tempfile(True)
    file = open(filename, 'w')

    for name in inputs:
        string = "%s\n" % (name)
        file.write(string)

    file.close()
    # Run r.series
    ret = core.run_command("r.series", flags="z", file=filename, output=output, overwrite=core.overwrite(), method=method)

    if ret != 0:
        dbif.close()
        core.fatal(_("Error while r.series computation"))

    # Read the raster map data
    new_map.load()
    
    # In case of a null map continue, do not register null maps
    if new_map.metadata.get_min() == None and new_map.metadata.get_max() == None:
        if not register_null:
            core.run_command("g.remove", rast=output)
            return

    # Set the time stamp
    if dataset.is_time_absolute():
        new_map.set_absolute_time(start, end, None)
    else:
        new_map.set_relative_time(start, end)

    # Insert map in temporal database
    new_map.insert(dbif)

    dataset.register_map(new_map, dbif)
