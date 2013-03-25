#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.register
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Register raster, vector and raster3d maps in a space time datasets
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Registers raster, vector and raster3d maps in a space time datasets.
#% keywords: temporal
#% keywords: map management
#% keywords: register
#%end

#%option G_OPT_STDS_INPUT
#% required: no
#% guisection: Input
#%end

#%option G_OPT_MAP_INPUTS
#% required: no
#% guisection: Input
#%end

#%option G_OPT_MAP_TYPE
#% guidependency: input,maps
#% guisection: Input
#%end

#%option G_OPT_F_INPUT
#% key: file
#% required: no
#% description: Input file with map names, one per line. Additionally the start time and the end time can be specified per line
#% guisection: Input
#%end

#%option
#% key: start
#% type: string
#% description: Valid start date and time of the first map. Format absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer).
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option
#% key: end
#% type: string
#% description: Valid end date and time of all map. Format absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer).
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option
#% key: unit
#% type: string
#% description: Unit must be set in case of relative time stamps
#% required: no
#% multiple: no
#% options: years,months,days,hours,minutes,seconds
#% guisection: Time & Date
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for valid time interval creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative is integer: 5)
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option G_OPT_F_SEP
#% description: Field separator character of the input file
#% guisection: Input
#%end

#%flag
#% key: i
#% description: Create an interval (start and end time) in case an increment is provided
#% guisection: Time & Date
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    name = options["input"]
    maps = options["maps"]
    type = options["type"]
    file = options["file"]
    fs = options["separator"]
    start = options["start"]
    end = options["end"]
    unit = options["unit"]
    increment = options["increment"]
    interval = flags["i"]

    # Make sure the temporal database exists
    tgis.init()
    # Register maps
    tgis.register_maps_in_space_time_dataset(
        type=type, name=name, maps=maps, file=file, start=start, end=end,
        unit=unit, increment=increment, dbif=None, interval=interval, fs=fs)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
