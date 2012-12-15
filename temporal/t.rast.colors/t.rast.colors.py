#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.colors
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:  Creates/modifies the color table associated with each raster map of the space time raster dataset.
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Creates/modifies the color table associated with each raster map of the space time raster dataset.
#% keywords: temporal
#% keywords: color table
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_M_COLR
#% key: color
#% type: string
#% description: Name of color table (see r.color help)
#% required: no
#% multiple: no
#% guisection: Define
#%end

#%option G_OPT_R_INPUT
#% key: raster
#% description: Raster map from which to copy color table
#% required: no
#% guisection: Define
#%end

#%option G_OPT_R3_INPUT
#% key: volume
#% description: 3D raster map from which to copy color table
#% required: no
#% guisection: Define
#%end

#%option G_OPT_F_INPUT
#% key: rules
#% description: Path to rules file
#% required: no
#% guisection: Define
#%end

#%flag
#% key: r
#% description: Remove existing color tables
#% guisection: Remove
#%end

#%flag
#% key: w
#% description: Only write new color table if one doesn't already exist
#%end

#%flag
#% key: l
#% description: List available rules then exit
#% guisection: Print
#%end

#%flag
#% key: n
#% description: Invert colors
#% guisection: Define
#%end

#%flag
#% key: g
#% description: Logarithmic scaling
#% guisection: Define
#%end

#%flag
#% key: a
#% description: Logarithmic-absolute scaling
#% guisection: Define
#%end

#%flag
#% key: e
#% description: Histogram equalization
#% guisection: Define
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    input = options["input"]
    color = options["color"]
    raster = options["raster"]
    volume = options["volume"]
    rules = options["rules"]
    remove = flags["r"]
    write = flags["w"]
    list = flags["l"]
    invert = flags["n"]
    log = flags["g"]
    abslog = flags["a"]
    equi = flags["e"]
    
    if raster == "":
        raster=None
        
    if volume == "":
        volume = None
        
    if rules == "":
        rules = None
        
    if color == "":
        color = None

    # Make sure the temporal database exists
    tgis.init()

    if input.find("@") >= 0:
        id = input
    else:
        mapset = grass.gisenv()["MAPSET"]
        id = input + "@" + mapset

    sp = tgis.SpaceTimeRasterDataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            sp.get_new_map_instance(None).get_type(), id))

    sp.select()

    rows = sp.get_registered_maps("id", None, None, None)

    if rows:
        # Create the r.colors input file
        filename = grass.tempfile(True)
        file = open(filename, 'w')

        for row in rows:
            string = "%s\n" % (row["id"])
            file.write(string)

        file.close()
        
        flags_=""
        if(remove):
            flags_+="r"
        if(write):
            flags_+="w"
        if(list):
            flags_+="l"
        if(invert):
            flags_+="n"
        if(log):
            flags_+="g"
        if(abslog):
            flags_+="a"
        if(equi):
            flags_+="e"

        ret = grass.run_command("r.colors", flags=flags_, file=filename,
                                color=color, raster=raster, volume=volume, 
                                rules=rules, overwrite=grass.overwrite())

        if ret != 0:
            grass.fatal(_("Error in r.colors call"))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
