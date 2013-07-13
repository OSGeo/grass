#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.gapfill
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Replace gaps in a space time raster dataset with interpolated raster maps.
# COPYRIGHT:    (C) 2012 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Replace gaps in a space time raster dataset with interpolated raster maps.
#% keywords: temporal
#% keywords: interpolation
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: base
#% type: string
#% description: Base name of the interpolated raster maps. This name will be extended with a numerical prefix
#% required: yes
#% multiple: no
#%end

#%option
#% key: nprocs
#% type: integer
#% description: Number of interpolation processes to run in parallel
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: t
#% description: Assign the space time raster dataset start and end time to the output map
#%end

from multiprocessing import Process
import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    input = options["input"]
    base = options["base"]
    where = options["where"]
    nprocs = options["nprocs"]

    # Make sure the temporal database exists
    tgis.init()

    if input.find("@") >= 0:
        id = input
    else:
        mapset = grass.gisenv()["MAPSET"]
        id = input + "@" + mapset

    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.SpaceTimeRasterDataset(id)

    if sp.is_in_db(dbif) == False:
        grass.fatal(_("Space time %s dataset <%s> not found") %
                    (sp.get_new_map_instance(None).get_type(), id))

    sp.select(dbif)

    maps = sp.get_registered_maps_as_objects_with_gaps(where, dbif)

    num = len(maps)

    gap_list = []

    # Identify all gaps and create new names
    count = 0
    for _map in maps:
        if _map.get_id() is None:
            count += 1
            _id = "%s_%d@%s" % (base, num + count, mapset)
            _map.set_id(_id)
            if _map.map_exists() or _map.is_in_db(dbif):
                grass.fatal(_("Map with name <%s> already exists. "
                              "Please use another base name." % (_id)))

            gap_list.append(_map)

    if len(gap_list) == 0:
        grass.message(_("No gaps found"))
        return

    # Build the temporal topology
    tb = tgis.temporal_topology_builder()
    tb.build(maps)

    # Do some checks before computation
    for _map in gap_list:
        if not _map.get_precedes() or not _map.get_follows():
            grass.fatal(_("Unable to determine successor "
                          "and predecessor of a gap."))

        if len(_map.get_precedes()) > 1:
            grass.warning(_("More than one successor of the gap found. "
                            "Using the first found."))

        if len(_map.get_follows()) > 1:
            grass.warning(_("More than one predecessor of the gap found. "
                            "Using the first found."))

    # Interpolate the maps using parallel processing
    proc_list = []
    proc_count = 0
    num = len(gap_list)

    for _map in gap_list:
        predecessor = _map.get_follows()[0]
        successor = _map.get_precedes()[0]

        # Build the module inputs strings
        inputs = "%s,%s" % (predecessor.get_map_id(), successor.get_map_id())
        dpos = "0,1"
        output = "%s" % (_map.get_name())
        outpos = "0.5"

        # Start several processes in parallel
        proc_list.append(Process(
            target=run_interp, args=(inputs, dpos, output, outpos)))
        proc_list[proc_count].start()
        proc_count += 1

        if proc_count == nprocs or proc_count == num:
            proc_count = 0
            exitcodes = 0
            for proc in proc_list:
                proc.join()
                exitcodes += proc.exitcode

            if exitcodes != 0:
                dbif.close()
                grass.fatal(_("Error while interpolation computation"))

            # Empty process list
            proc_list = []

    # Insert new interpolated maps in temporal database and dataset
    for _map in gap_list:
        _map.load()
        _map.insert(dbif)
        sp.register_map(_map, dbif)

    sp.update_from_registered_maps(dbif)
    dbif.close()

###############################################################################


def run_interp(inputs, dpos, output, outpos):
    """Helper function to run r.series.interp in parallel"""
    return grass.run_command("r.series.interp", input=inputs, datapos=dpos,
                             output=output, samplingpos=outpos,
                             overwrite=grass.overwrite(), quiet=True)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
