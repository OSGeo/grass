#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.sample
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Sample the input space time dataset with a sample space time dataset and print the result to stdout
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Sample the input space time dataset with a sample space time dataset and print the result to stdout
#% keywords: dataset
#% keywords: spacetime
#% keywords: raster
#% keywords: sample
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: sample
#% type: string
#% description: Name of the sample space time dataset 
#% required: yes
#% multiple: no
#%end

#%option
#% key: intype
#% type: string
#% description: Type of the input space time dataset, default is space time raster dataset (strds)
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%option
#% key: samtype
#% type: string
#% description: Type of the sample space time dataset, default is space time raster dataset (strds)
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%option
#% key: method
#% type: string
#% description: The method to be used for sampling
#% required: no
#% multiple: yes
#% options: start,during,overlap,contain,equal
#% answer: during,overlap,contain,equal
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character between the columns, default is tabular " | ". Do not use "," as this char is reserved to list several map ids in a sample granule
#% required: no
#%end

#%flag
#% key: h
#% description: Print column names 
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    sampler = options["sample"]
    samtype = options["samtype"]
    intype = options["intype"]
    separator = options["fs"]
    method = options["method"]
    header = flags["h"]

    method = method.split(",")

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    tgis.sample_stds_by_stds_topology(intype, samtype, input, sampler, header, separator, method)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
