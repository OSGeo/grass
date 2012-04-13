"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.print_gridded_dataset_univar_statistics(type, input, where, extended, header, fs)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets_tools import *
    
def print_gridded_dataset_univar_statistics(type, input, where, extended, header, fs):
    """!Print univariate statistics for a space time raster or raster3d dataset
    
       @param type Must be "strds" or "str3ds"
       @param input The name of the space time dataset
       @param where A temporal database where statement
       @param extended If True compute extended statistics 
       @param header   If True print column names as header 
       @param fs Field separator 
    """
    
    # We need a database interface
    dbif = sql_database_interface_connection()
    dbif.connect()
   
    mapset =  core.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = dataset_factory(type, id)
    
    if sp.is_in_db(dbif) == False:
        dbif.close()
        core.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    sp.select(dbif)

    rows = sp.get_registered_maps("id,start_time,end_time", where, "start_time", dbif)

    if not rows:
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> is empty") % (sp.get_new_map_instance(None).get_type(), out_id))

    if header == True:
        print "id" + fs + "start" + fs + "end" + fs + "mean" + fs + "min" + fs + "max" + fs,
        print "mean_of_abs" + fs + "stddev" + fs + "variance" + fs,
        if extended == True:
            print "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells" + fs,
            print "first_quartile" + fs + "median" + fs + "third_quartile" + fs + "percentile_90" 
        else:
            print "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells" 

    for row in rows:
        id = row["id"]
        start = row["start_time"]
        end = row["end_time"]

        flag="g"

        if extended == True:
            flag += "e"

	if type == "strds":
	    stats = core.parse_command("r.univar", map=id, flags=flag)
	elif type == "str3ds":
	    stats = core.parse_command("r3.univar", map=id, flags=flag)

        print str(id) + fs + str(start) + fs + str(end),
        print fs + str(stats["mean"]) + fs + str(stats["min"]) + fs + str(stats["max"]) + fs + str(stats["mean_of_abs"]),
        print fs + str(stats["stddev"]) + fs + str(stats["variance"]) + fs + str(stats["coeff_var"]) + fs + str(stats["sum"]),

        if extended == True:
            print fs + str(stats["null_cells"]) + fs + str(stats["cells"]) + fs,
            print str(stats["first_quartile"]) + fs + str(stats["median"]) + fs + str(stats["third_quartile"]) + fs + str(stats["percentile_90"]) 
        else:
            print fs + str(stats["null_cells"]) + fs + str(stats["cells"])
        
    dbif.close()

if __name__ == "__main__":
    options, flags = core.parser()
    main()

