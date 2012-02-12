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
#% description: Register raster, vector adn raster3d maps in a space time datasets
#% keywords: temnporal
#% keywords: raster
#% keywords: vector
#% keywords: raster3d
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time dataset of type raster, vector or raster3d
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing map(s) of type raster, vector or raster3d
#% required: no
#% multiple: yes
#%end

#%option
#% key: type
#% type: string
#% description: Input space time dataset type
#% required: no
#% options: strds, stvds, str3ds
#% answer: strds
#%end

#%option
#% key: file
#% type: string
#% description: Input file with map names, one per line. Additionally the start time and the end time can be specified per line
#% required: no
#% multiple: no
#%end

#%option
#% key: start
#% type: string
#% description: The valid start date and time of the first map. Format absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer).
#% required: no
#% multiple: no
#%end

#%option
#% key: end
#% type: string
#% description: The valid end date and time of all map. Format absolute time: "yyyy-mm-dd HH:MM:SS +HHMM", relative time is of type integer).
#% required: no
#% multiple: no
#%end

#%option
#% key: unit
#% type: string
#% description: This unit must be set in case of relative time stamps
#% required: no
#% multiple: no
#% options: years,months,days,hours,minutes,seconds
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for valid time interval creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative is integer: 5)
#% required: no
#% multiple: no
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character of the input file
#% required: no
#% answer: |
#%end

#%flag
#% key: i
#% description: Create an interval (start and end time) in case an increment is provided
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
    fs = options["fs"]
    start = options["start"]
    end = options["end"]
    unit = options["unit"]
    increment = options["increment"]
    interval = flags["i"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Register maps
    register_maps_in_space_time_dataset(type=type, name=name, maps=maps, file=file, start=start, end=end, \
                                             unit=unit, increment=increment, dbif=None, interval=interval, fs=fs)
    
###############################################################################

def register_maps_in_space_time_dataset(type, name, maps=None, file=None, start=None, \
                                        end=None, unit=None, increment=None, dbif = None, \
                                        interval=False, fs="|"):
    """Use this method to register maps in space time datasets. This function is generic and

       Additionally a start time string and an increment string can be specified
       to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @param type: The type of the maps raster, raster3d or vector
       @param name: The name of the space time dataset
       @param maps: A comma separated list of map names
       @param file: Input file one map with start and optional end time, one per line
       @param start: The start date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param end: The end date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param unit: The unit of the relative time: years, months, days, hours, minutes, seconds
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @param dbif: The database interface to be used
       @param interval: If True, time intervals are created in case the start time and an increment is provided
       @param fs: Field separator used in input file
    """

    start_time_in_file = False
    end_time_in_file = False
    if maps and file:
        grass.fatal(_("%s= and %s= are mutually exclusive") % ("input","file"))

    if end and increment:
        grass.fatal(_("%s= and %s= are mutually exclusive") % ("end","increment"))

    if end and not start:
        grass.fatal(_("Please specify %s= and %s=") % ("start_time","end_time"))

    if not maps and not file:
        grass.fatal(_("Please specify %s= or %s=") % ("input","file"))

    # We may need the mapset
    mapset =  grass.gisenv()["MAPSET"]

    # Check if the dataset name contains the mapset as well
    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        id = name

    sp = tgis.dataset_factory(type, id)

    connect = False

    if dbif == None:
        dbif = tgis.sql_database_interface()
        dbif.connect()
        connect = True

    # Read content from temporal database
    sp.select(dbif)

    if sp.is_in_db(dbif) == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> no found") % (sp.get_new_map_instance(None).get_type(), name))

    if sp.is_time_relative() and not unit:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> with relative time found, but no relative unit set for %s maps") % (sp.get_new_map_instance(None).get_type(), name, sp.get_new_map_instance(None).get_type()))

    dummy = sp.get_new_map_instance(None)
        
    maplist = []
    
    # Map names as comma separated string
    if maps:
        if maps.find(",") < 0:
            maplist = [maps,]
        else:
            maplist = maps.split(",")

	# Build the maplist again with the ids
	for count in range(len(maplist)):
	    row = {}
	    mapid = dummy.build_id(maplist[count], mapset, None)
		
	    row["id"] = mapid
            maplist[count] = row
            
    # Read the map list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            line_list = line.split(fs)

            # Detect start and end time
            if len(line_list) == 2:
                start_time_in_file = True
                end_time_in_file = False
            elif len(line_list) == 3:
                start_time_in_file = True
                end_time_in_file = True
            else:
                start_time_in_file = False
                end_time_in_file = False

            mapname = line_list[0].strip()
            row = {}
            
	    if start_time_in_file and  end_time_in_file:
	        row["start"] = line_list[1].strip()
	        row["end"] = line_list[2].strip()

	    if start_time_in_file and  not end_time_in_file:
	        row["start"] = line_list[1].strip()
	    
	    row["id"] = dummy.build_id(mapname, mapset)

            maplist.append(row)
    
    num_maps = len(maplist)
    for count in range(len(maplist)):
	grass.percent(count, num_maps, 1)

        # Get a new instance of the space time dataset map type
        map = sp.get_new_map_instance(maplist[count]["id"])

        # Use the time data from file
        if maplist[count].has_key("start"):
            start = maplist[count]["start"]
        if maplist[count].has_key("end"):
            end = maplist[count]["end"]

        # Put the map into the database
        if map.is_in_db(dbif) == False:
            # Break in case no valid time is provided
            if start == "" or start == None:
                dbif.close()
                if map.get_layer():
		    grass.fatal(_("Unable to register %s map <%s> with layer %s. The map has no valid time and the start time is not set.") % \
				(map.get_type(), map.get_map_id(), map.get_layer() ))
		else:
		    grass.fatal(_("Unable to register %s map <%s>. The map has no valid time and the start time is not set.") % \
				(map.get_type(), map.get_map_id() ))
            # Load the data from the grass file database
            map.load()
	    
            if sp.get_temporal_type() == "absolute":
                map.set_time_to_absolute()
            else:
                map.set_time_to_relative()
                
            #  Put it into the temporal database
            map.insert(dbif)
        else:
            map.select(dbif)
            if map.get_temporal_type() != sp.get_temporal_type():
                dbif.close()
                if map.get_layer():
		    grass.fatal(_("Unable to register %s map <%s> with layer. The temporal types are different.") %  \
		                 (map.get_type(), map.get_map_id(), map.get_layer()))
		    grass.fatal(_("Unable to register %s map <%s>. The temporal types are different.") %  \
		                 (map.get_type(), map.get_map_id()))

        # In case the time is in the input file we ignore the increment counter
        if start_time_in_file:
            count = 1

        # Set the valid time
        if start:
            tgis.assign_valid_time_to_map(ttype=sp.get_temporal_type(), map=map, start=start, end=end, unit=unit, increment=increment, mult=count, dbif=dbif, interval=interval)

        # Finally Register map in the space time dataset
        sp.register_map(map, dbif)

    # Update the space time tables
    sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()

    grass.percent(num_maps, num_maps, 1)
        
###############################################################################

def unregister_maps_from_space_time_datasets(type, name, maps, file=None, dbif=None):
    """Unregister maps from a single space time dataset or, in case no dataset name is provided,
       unregister from all datasets within the maps are registered.

       @param type: The type of the maps raster, vector or raster3d
       @param name: Name of an existing space time raster dataset. If no name is provided the raster map(s) are unregistered from all space time datasets in which they are registered.
       @param maps: A comma separated list of map names
       @param file: Input file one map per line
       @param dbif: The database interface to be used
    """

    if maps and file:
        grass.fatal(_("%s= and %s= are mutually exclusive") % ("input","file"))

    mapset =  grass.gisenv()["MAPSET"]

    if dbif == None:
        dbif = sql_database_interface()
        dbif.connect()
        connect = True

    # In case a space time dataset is specified
    if name:
        # Check if the dataset name contains the mapset as well
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "rast":
            sp = dataset_factory("strds", id)
        if type == "rast3d":
            sp = dataset_factory("str3ds", id)
        if type == "vect":
            sp = dataset_factory("stvds", id)

        if sp.is_in_db(dbif) == False:
            dbif.close()
            grass.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    maplist = []

    dummy = raster_dataset(None)

    # Map names as comma separated string
    if maps != None:
	if maps.find(",") == -1:
	    maplist = [maps,]
	else:
	    maplist = maps.split(",")
	    
	# Build the maplist
	for count in range(len(maplist)):
	    mapname = maplist[count]
	    mapid = dummy.build_id(mapname, mapset)
            maplist[count] = mapid
            
    # Read the map list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            line_list = line.split(fs)
            mapname = line_list[0].strip()
	    mapid = dummy.build_id(mapname, mapset)
            maplist.append(mapid)
            
    num_maps = len(maplist)
    count = 0
    for mapid in maplist:
	grass.percent(count, num_maps, 1)
            
        print mapid
        map = dataset_factory(type, mapid)

        # Unregister map if in database
        if map.is_in_db(dbif) == True:
            if name:
                sp.select(dbif)
                sp.unregister_map(map, dbif)
            else:
                map.select(dbif)
                map.unregister(dbif)
		
	count += 1

    if name:
        sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()
	
    grass.percent(num_maps, num_maps, 1)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
