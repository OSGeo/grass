#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.sample
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Sample the input space time dataset(s) with a sample space time dataset and print the result to stdout
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Samples the input space time dataset(s) with a sample space time dataset and print the result to stdout.
#% keywords: temporal
#% keywords: sampling
#%end

#%option G_OPT_STDS_INPUTS
#%end

#%option G_OPT_STDS_INPUT
#% key: sample
#% description: Name of the sample space time dataset
#%end

#%option G_OPT_STDS_TYPE
#% key: intype
#% guisection: Required
#%end

#%option G_OPT_STDS_TYPE
#% key: samtype
#% guisection: Required
#% description: Type of the sample space time dataset
#%end

#%option G_OPT_T_SAMPLE
#% key: method
#% answer: during,overlap,contain,equal
#%end

#%option
#% key: fs
#% type: string
#% description: Field separator character between the output columns, default is tabular " | ". Do not use "," as this char is reserved to list several map ids in a sample granule
#% required: no
#%end

#%flag
#% key: c
#% description: Print column names
#%end

#%flag
#% key: s
#% description: Check spatial overlap to perform spatio-temporal sampling
#%end


import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    inputs = options["inputs"]
    sampler = options["sample"]
    samtype = options["samtype"]
    intype = options["intype"]
    separator = options["fs"]
    method = options["method"]
    header = flags["c"]
    spatial = flags["s"]
    
    # Make sure the temporal database exists
    tgis.init()

    tgis.sample_stds_by_stds_topology(intype, samtype, inputs, sampler,
                                      header, separator, method, spatial, True)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
