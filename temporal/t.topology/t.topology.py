#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.topology
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List and modify temporal topology of a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List and modify temporal topology of a space time dataset
#% keywords: spacetime dataset
#% keywords: remove
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is space time raster dataset (strds)
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%option G_OPT_DB_WHERE
#%end

#%flag
#% key: c
#% description: Check temporal topology
#%end

#%flag
#% key: g
#% description: Compute granularity
#%end

#%flag
#% key: t
#% description: Print temporal relation matrix and exit
#%end

#%flag
#% key: r
#% description: Print temporal relation count
#%end

#%flag
#% key: m
#% description: Print temporal map type count
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    where = options["where"]
    tmatrix = flags['t']
    relation = flags['r']
    map_types = flags['m']
    check_topo = flags['c']
    comp_gran = flags['g']

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    #Get the current mapset to create the id of the space time dataset
    if name.find("@") >= 0:
        id = name
    else:
        mapset =  grass.gisenv()["MAPSET"]
        id = name + "@" + mapset

    sp = tgis.dataset_factory(type, id)

    if sp.is_in_db() == False:
        grass.fatal("Dataset <" + name + "> not found in temporal database")
        
    # Insert content from db
    sp.select()

    # Get ordered map list
    maps = sp.get_registered_maps_as_objects(where=where, order="start_time", dbif=None)

    if check_topo:
        check = sp.check_temporal_topology(maps)
        if check:
            print "Temporal topology is valid"
        else:
            print "Temporal topology is invalid"

    if tmatrix:
        matrix = sp.print_temporal_relation_matrix(maps)

    if relation:
        dict = sp.count_temporal_relations(maps)

        for key in dict.keys():
            print key, dict[key]

    if map_types:
        dict = sp.count_temporal_types(maps)
        
        for key in dict.keys():
            print key, dict[key]

        if dict["invalid"] == 0:
            print "Gaps", sp.count_gaps(maps)
        else:
            print "Gaps", None

    if comp_gran:
        gran = tgis.compute_absolute_time_granularity(maps)
        print "Granularity", gran

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

