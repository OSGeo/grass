#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.topology
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      List temporal topology of a space time dataset
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

# %module
# % description: Lists temporal topology of a space time dataset.
# % keyword: temporal
# % keyword: topology
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# %end

# %option G_OPT_T_WHERE
# %end

# %flag
# % key: m
# % description: Print temporal topological relationships and exit
# %end

# %flag
# % key: s
# % description: Print spatio-temporal topological relationships and exit
# %end
from __future__ import print_function

import grass.script as grass


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    name = options["input"]
    type = options["type"]
    where = options["where"]
    temporal_relations = flags["m"]
    spatio_temporal_relations = flags["s"]

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.open_old_stds(name, type)

    # Get ordered map list
    maps = sp.get_registered_maps_as_objects(where=where, order="start_time", dbif=None)

    spatial = None

    if spatio_temporal_relations:
        if sp.get_type() == "strds":
            spatial = "2D"
        else:
            spatial = "3D"

    if temporal_relations or spatio_temporal_relations:
        sp.print_spatio_temporal_relationships(maps=maps, spatial=spatial)
        return

    sp.base.print_info()

    #      0123456789012345678901234567890
    print(
        " +-------------------- Temporal topology -------------------------------------+"
    )
    if where:
        print(" | Is subset of dataset: ...... True")
    else:
        print(" | Is subset of dataset: ...... False")

    check = sp.check_temporal_topology(maps)
    if check:
        #      0123456789012345678901234567890
        print(" | Temporal topology is: ...... valid")
    else:
        #      0123456789012345678901234567890
        print(" | Temporal topology is: ...... invalid")

    dict_ = sp.count_temporal_types(maps)

    for key in dict_.keys():
        if key == "interval":
            #      0123456789012345678901234567890
            print(" | Number of intervals: ....... %s" % (dict_[key]))
        if key == "point":
            print(" | Number of points: .......... %s" % (dict_[key]))
        if key == "invalid":
            print(" | Invalid time stamps: ....... %s" % (dict_[key]))

    #      0123456789012345678901234567890
    print(" | Number of gaps: ............ %i" % sp.count_gaps(maps))

    if sp.is_time_absolute():
        gran = tgis.compute_absolute_time_granularity(maps)
    else:
        gran = tgis.compute_relative_time_granularity(maps)
    print(" | Granularity: ............... %s" % str(gran))

    print(
        " +-------------------- Topological relations ---------------------------------+"
    )
    dict_ = sp.count_temporal_relations(maps)

    if dict_:
        for key in dict_.keys():
            if key == "equal":
                #      0123456789012345678901234567890
                print(" | Equal:...................... %s" % (dict_[key]))
            if key == "during":
                print(" | During: .................... %s" % (dict_[key]))
            if key == "contains":
                print(" | Contains: .................. %s" % (dict_[key]))
            if key == "overlaps":
                print(" | Overlaps: .................. %s" % (dict_[key]))
            if key == "overlapped":
                print(" | Overlapped: ................ %s" % (dict_[key]))
            if key == "after":
                print(" | After: ..................... %s" % (dict_[key]))
            if key == "before":
                print(" | Before: .................... %s" % (dict_[key]))
            if key == "starts":
                print(" | Starts: .................... %s" % (dict_[key]))
            if key == "finishes":
                print(" | Finishes: .................. %s" % (dict_[key]))
            if key == "started":
                print(" | Started: ................... %s" % (dict_[key]))
            if key == "finished":
                print(" | Finished: .................. %s" % (dict_[key]))
            if key == "follows":
                print(" | Follows: ................... %s" % (dict_[key]))
            if key == "precedes":
                print(" | Precedes: .................. %s" % (dict_[key]))
    print(
        " +----------------------------------------------------------------------------+"
    )


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
