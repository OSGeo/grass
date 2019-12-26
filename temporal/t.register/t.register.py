#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.register
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Registers raster, vector and raster3d maps in a space time dataset
# COPYRIGHT:	(C) 2011-2017, Soeren Gebbert and the GRASS Development Team
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

#%module
#% description: Assigns timestamps and registers raster, vector and raster3d maps in a space time dataset.
#% keyword: temporal
#% keyword: map management
#% keyword: register
#% keyword: time
#% overwrite: yes
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
#% label: Input file with map names, one per line
#% description: Additionally the start time and the end time can be specified per line
#% guisection: Input
#%end

#%option
#% key: start
#% type: string
#% label: Valid start date and time of the first map
#% description: Format for absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer.
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option
#% key: end
#% type: string
#% label: Valid end date and time of all map
#% description: Format for absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer.
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option
#% key: unit
#% type: string
#% label: Time stamp unit
#% description: Unit must be set in case of relative timestamps
#% required: no
#% multiple: no
#% options: years,months,days,hours,minutes,seconds
#% guisection: Time & Date
#%end

#%option
#% key: increment
#% type: string
#% label: Time increment, works only in conjunction with start option
#% description: Time increment between maps for creation of valid time intervals (format for absolute time: NNN seconds, minutes, hours, days, weeks, months, years; format for relative time is of type integer: 5)
#% required: no
#% multiple: no
#% guisection: Time & Date
#%end

#%option G_OPT_F_SEP
#% label: Field separator character of the input file
#% guisection: Input
#%end

#%flag
#% key: i
#% description: Create an interval (start and end time) in case an increment and the start time are provided
#% guisection: Time & Date
#%end

import grass.script as grass


############################################################################


def main():
    # Get the options
    name = options["input"]
    maps = options["maps"]
    type = options["type"]
    file = options["file"]
    separator = grass.separator(options["separator"])
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
        unit=unit, increment=increment, dbif=None, interval=interval, fs=separator)


###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()

    # lazy imports
    import grass.temporal as tgis

    try:
        from builtins import StandardError
    except ImportError:
        # python 3
        StandardError = Exception

    try:
        tgis.profile_function(main)
    except StandardError as e:
        grass.fatal(e)
