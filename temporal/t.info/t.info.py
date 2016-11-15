#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.info
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Print information about a space-time dataset
# COPYRIGHT:    (C) 2011-2014 by the GRASS Development Team
#
#       This program is free software under the GNU General Public
#       License (version 2). Read the file COPYING that comes with GRASS
#       for details.
#
#############################################################################

#%module
#% description: Lists information about space time datasets and maps.
#% keyword: temporal
#% keyword: metadata
#% keyword: extent
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset or map
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#% options: strds, str3ds, stvds, raster, raster_3d, vector
#%end

#%flag
#% key: g
#% description: Print in shell script style
#%end

#%flag
#% key: h
#% description: Print history information in human readable shell style for space time datasets
#%end

#%flag
#% key: d
#% description: Print information about the temporal DBMI interface and exit
#% suppress_required: yes
#%end
from __future__ import print_function

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    name = options["input"]
    type_ = options["type"]
    shellstyle = flags['g']
    system = flags['d']
    history = flags['h']

    # Make sure the temporal database exists
    tgis.init()

    dbif, connected = tgis.init_dbif(None)

    rows = tgis.get_tgis_metadata(dbif)

    if system and not shellstyle:
        #      0123456789012345678901234567890
        print(" +------------------- Temporal DBMI backend information ----------------------+")
        print(" | DBMI Python interface:...... " + str(dbif.get_dbmi().__name__))
        print(" | Temporal database string:... " + str(
            tgis.get_tgis_database_string()))
        print(" | SQL template path:.......... " + str(
            tgis.get_sql_template_path()))
        if rows:
            for row in rows:
                print(" | %s .......... %s"%(row[0], row[1]))
        print(" +----------------------------------------------------------------------------+")
        return
    elif system:
        print("dbmi_python_interface=\'" + str(dbif.get_dbmi().__name__) + "\'")
        print("dbmi_string=\'" + str(tgis.get_tgis_database_string()) + "\'")
        print("sql_template_path=\'" + str(tgis.get_sql_template_path()) + "\'")
        if rows:
            for row in rows:
                print("%s=\'%s\'"%(row[0], row[1]))
        return

    if not system and not name:
        grass.fatal(_("Please specify %s=") % ("name"))

    if name.find("@") >= 0:
        id_ = name
    else:
        id_ = name + "@" + grass.gisenv()["MAPSET"]

    dataset = tgis.dataset_factory(type_, id_)

    if dataset.is_in_db(dbif) == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id_))

    dataset.select(dbif)

    if history == True and type in ["strds", "stvds", "str3ds"]:
        dataset.print_history()
        return

    if shellstyle == True:
        dataset.print_shell_info()
    else:
        dataset.print_info()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
