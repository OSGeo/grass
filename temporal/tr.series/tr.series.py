#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.series
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Performe different aggregation algorithms from r.series on all raster maps of a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Performe different aggregation algorithms from r.series on all raster maps of a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#% keywords: extract
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time raster dataset
#% required: yes
#% multiple: yes
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation
#% required: yes
#% multiple: no
#% options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: average
#%end

#%option
#% key: quantile
#% type: string
#% description: Quantile to calculate for method=quantile
#% required: no
#% multiple: no
#% options: 0.0-1.0
#%end

#%option G_OPT_R_OUTPUT
#%end


import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    quantile = options["quantile"]
    method = options["method"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    if input.find("@") >= 0:
        id = input
    else:
        mapset =  grass.gisenv()["MAPSET"]
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal("Dataset <%s> not found in temporal database" % (id))

    sp.select()

    rows = sp.get_registered_maps()

    if rows:
        inputs = ""

        count = 0
        for row in rows:
            if count == 0:
                inputs += row["id"]
            else:
                inputs += "," + row["id"]
            count += 1

        print inputs

        if grass.overwrite() == True:
            grass.run_command("r.series", input=inputs, output=output, overwrite=True, method=method,  quantile=quantile)
        else:
            grass.run_command("r.series", input=inputs, output=output, overwrite=False, method=method,  quantile=quantile)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()

