#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.topology
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List temporal topology of a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Lists temporal topology of a space time dataset.
#% keywords: temporal
#% keywords: topology
#%end

#%option G_OPT_STDS_INPUT
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#%end

#%option G_OPT_T_WHERE
#%end

#%flag
#% key: m
#% description: Print temporal relationships and exit
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    where = options["where"]
    tmatrix = flags['m']

    # Make sure the temporal database exists
    tgis.init()

    #Get the current mapset to create the id of the space time dataset
    if name.find("@") >= 0:
        id = name
    else:
        mapset = grass.gisenv()["MAPSET"]
        id = name + "@" + mapset

    sp = tgis.dataset_factory(type, id)

    if sp.is_in_db() == False:
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            sp.get_new_map_instance(None).get_type(), id))

    # Insert content from db
    sp.select()

    # Get ordered map list
    maps = sp.get_registered_maps_as_objects(
        where=where, order="start_time", dbif=None)

    if tmatrix:
        sp.print_temporal_relationships(maps)
        return

    sp.base.print_info()

    #      0123456789012345678901234567890
    print " +-------------------- Temporal topology -------------------------------------+"
    if where:
        print " | Is subset of dataset: ...... True"
    else:
        print " | Is subset of dataset: ...... False"

    check = sp.check_temporal_topology(maps)
    if check:
        #      0123456789012345678901234567890
        print " | Temporal topology is: ...... valid"
    else:
        #      0123456789012345678901234567890
        print " | Temporal topology is: ...... invalid"

    dict_ = sp.count_temporal_types(maps)

    for key in dict_.keys():
        if key == "interval":
            #      0123456789012345678901234567890
            print " | Number of intervals: ....... %s" % (dict_[key])
        if key == "point":
            print " | Number of points: .......... %s" % (dict_[key])
        if key == "invalid":
            print " | Invalid time stamps: ....... %s" % (dict_[key])

    #      0123456789012345678901234567890
    print " | Number of gaps: ............ %i" % sp.count_gaps(maps)

    if sp.is_time_absolute():
        gran = tgis.compute_absolute_time_granularity(maps)
    else:
        gran = tgis.compute_relative_time_granularity(maps)
    print " | Granularity: ............... %s" % str(gran)

    print " +-------------------- Topological relations ---------------------------------+"
    dict_ = sp.count_temporal_relations(maps)

    if dict_:
        for key in dict_.keys():
            if key == "equal":
                #      0123456789012345678901234567890
                print " | Equal:...................... %s" % (dict_[key])
            if key == "during":
                print " | During: .................... %s" % (dict_[key])
            if key == "contains":
                print " | Contains: .................. %s" % (dict_[key])
            if key == "overlaps":
                print " | Overlaps: .................. %s" % (dict_[key])
            if key == "overlapped":
                print " | Overlapped: ................ %s" % (dict_[key])
            if key == "after":
                print " | After: ..................... %s" % (dict_[key])
            if key == "before":
                print " | Before: .................... %s" % (dict_[key])
            if key == "starts":
                print " | Starts: .................... %s" % (dict_[key])
            if key == "finishes":
                print " | Finishes: .................. %s" % (dict_[key])
            if key == "started":
                print " | Started: ................... %s" % (dict_[key])
            if key == "finished":
                print " | Finished: .................. %s" % (dict_[key])
            if key == "follows":
                print " | Follows: ................... %s" % (dict_[key])
            if key == "precedes":
                print " | Precedes: .................. %s" % (dict_[key])
    print " +----------------------------------------------------------------------------+"

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
