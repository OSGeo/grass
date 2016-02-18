#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.list
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List space time datasets and maps registered in the temporal database
# COPYRIGHT:	(C) 2011-2014, Soeren Gebbert and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Lists space time datasets and maps registered in the temporal database.
#% keyword: temporal
#% keyword: map management
#% keyword: list
#% keyword: time
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset or map, default is strds
#% guisection: Selection
#% required: no
#% options: strds, str3ds, stvds, raster, raster_3d, vector
#% answer: strds
#%end

#%option G_OPT_T_TYPE
#% multiple: yes
#% answer: absolute,relative
#% guisection: Selection
#%end

#%option
#% key: order
#% type: string
#% description: Columns number_of_maps and granularity only available for space time datasets
#% label: Sort the space time dataset by category
#% guisection: Formatting
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,number_of_maps,creation_time,start_time,end_time,interval,north,south,west,east,granularity
#% answer: id
#%end

#%option
#% key: columns
#% type: string
#% description: Columns number_of_maps and granularity only available for space time datasets
#% label: Columns to be printed to stdout
#% guisection: Selection
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,number_of_maps,creation_time,start_time,end_time,north,south,west,east,granularity,all
#% answer: id
#%end

#%option G_OPT_T_WHERE
#% guisection: Selection
#%end

#%option G_OPT_F_SEP
#% label: Field separator character between the output columns
#% guisection: Formatting
#%end

#%option G_OPT_F_OUTPUT
#% required: no
#%end

#%flag
#% key: c
#% description: Print the column names as first row
#% guisection: Formatting
#%end

import grass.script as gscript
import grass.temporal as tgis
import sys

############################################################################


def main():

    # Get the options
    type = options["type"]
    temporal_type = options["temporaltype"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = gscript.separator(options["separator"])
    outpath = options["output"]
    colhead = flags['c']

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.dataset_factory(type, None)
    first = True

    if  gscript.verbosity() > 0 and not outpath:
        sys.stderr.write("----------------------------------------------\n")

    for ttype in temporal_type.split(","):
        if ttype == "absolute":
            time = "absolute time"
        else:
            time = "relative time"

        stds_list = tgis.get_dataset_list(type,  ttype,  columns,  where,  order)

        # Use the correct order of the mapsets, hence first the current mapset, then
        # alphabetic ordering
        mapsets = tgis.get_tgis_c_library_interface().available_mapsets()

        if outpath:
            outfile = open(outpath, 'w')

        # Print for each mapset separately
        for key in mapsets:
            if key in stds_list.keys():
                rows = stds_list[key]

                if rows:
                    if  gscript.verbosity() > 0 and not outpath:
                        if issubclass(sp.__class__,  tgis.AbstractMapDataset):
                            sys.stderr.write(_("Time stamped %s maps with %s available in mapset <%s>:\n")%\
                                                     (sp.get_type(),  time,  key))
                        else:
                            sys.stderr.write(_("Space time %s datasets with %s available in mapset <%s>:\n")%\
                                                     (sp.get_new_map_instance(None).get_type(),  time,  key))

                    # Print the column names if requested
                    if colhead == True and first == True:
                        output = ""
                        count = 0
                        for key in rows[0].keys():
                            if count > 0:
                                output += separator + str(key)
                            else:
                                output += str(key)
                            count += 1
                        if outpath:
                            outfile.write("{st}\n".format(st=output))
                        else:
                            print output
                        first = False

                    for row in rows:
                        output = ""
                        count = 0
                        for col in row:
                            if count > 0:
                                output += separator + str(col)
                            else:
                                output += str(col)
                            count += 1
                        if outpath:
                            outfile.write("{st}\n".format(st=output))
                        else:
                            print output
    if outpath:
        outfile.close()

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
