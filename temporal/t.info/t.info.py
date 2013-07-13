#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.info
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Print information about a space-time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Lists information about space time datasets and maps.
#% keywords: temporal
#% keywords: metadata
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset or map
#%end

#%option
#% key: type
#% type: string
#% description: Type of the dataset, default is strds (space time raster dataset)
#% required: no
#% guidependency: input
#% guisection: Required
#% options: strds, str3ds, stvds, rast, rast3d, vect
#% answer: strds
#%end

#%flag
#% key: g
#% description: Print information in shell style
#%end

#%flag
#% key: h
#% description: Print history information in human readable shell style
#%end

#%flag
#% key: s
#% description: Print information about the temporal DBMI interface and exit
#% suppress_required: yes
#%end


import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    name = options["input"]
    type = options["type"]
    shellstyle = flags['g']
    system = flags['s']
    history = flags['h']

    # Make sure the temporal database exists
    tgis.init()
    
    dbif = tgis.SQLDatabaseInterfaceConnection()
    
    if system and not shellstyle:
        #      0123456789012345678901234567890
        print " +------------------- Temporal DBMI backend information ----------------------+"
        print " | DBMI Python interface:...... " + str(dbif.dbmi.__name__)
        print " | DBMI init string:........... " + str(
            tgis.get_temporal_dbmi_init_string())
        print " | SQL template path:.......... " + str(
            tgis.get_sql_template_path())
        print " +----------------------------------------------------------------------------+"
        return
    elif system:
        print "dbmi_python_interface=" + str(dbif.dbmi.__name__)
        print "dbmi_init_string=" + str(tgis.get_temporal_dbmi_init_string())
        print "sql_template_path=" + str(tgis.get_sql_template_path())
        return
        
    if not system and not name:
        grass.fatal(_("Please specify %s=") % ("name"))

    if name.find("@") >= 0:
        id = name
    else:
        mapset = grass.gisenv()["MAPSET"]
        id = name + "@" + mapset

    ds = tgis.dataset_factory(type, id)

    if ds.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    ds.select()

    if history == True:
        ds.print_history()
        return

    if shellstyle == True:
        ds.print_shell_info()
    else:
        ds.print_info()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
