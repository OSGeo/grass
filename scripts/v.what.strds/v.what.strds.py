#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       v.what.strds
# AUTHOR(S):    Luca delucchi
#
# PURPOSE:      Uploads space time raster dataset values at positions of vector points to the table
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Uploads space time raster dataset values at positions of vector points to the table.
#% keyword: vector
#% keyword: temporal
#% keyword: sampling
#% keyword: position
#% keyword: querying
#% keyword: attribute table
#% keyword: time
#%end

#%option G_OPT_V_INPUT
#%end

#%option G_OPT_STRDS_INPUTS
#% key: strds
#%end

#%option G_OPT_V_OUTPUT
#% required: no
#%end

#%option G_OPT_DB_WHERE
#%end

#%option G_OPT_T_WHERE
#% key: t_where
#%end

#%flag
#% key: u
#% label: Update attribute table of input vector map
#% description: Instead of creating a new vector map update the attribute table with value(s)
#%end

import grass.script as grass
from grass.exceptions import CalledModuleError

############################################################################


class Sample(object):

    def __init__(self, start=None, end=None, raster_names=None,
                 strds_name=None, granularity=None):
        self.start = start
        self.end = end
        if raster_names is not None:
            self.raster_names = raster_names
        else:
            self.raster_names = []
        self.strds_name = strds_name
        self.granu = granularity

    def __str__(self):
        return "Start: %s\nEnd: %s\nNames: %s\n" % (str(self.start),
                                                    str(self.end),
                                                    str(self.raster_names))

    def printDay(self, date='start'):
        output = ''
        if date == 'start':
            output = str(self.start).split(' ')[0].replace('-', '_')
        elif date == 'end':
            output = str(self.end).split(' ')[0].replace('-', '_')
        else:
            grass.fatal("The values accepted by printDay in Sample are:"
                        " 'start', 'end'")
        if self.granu:
            if self.granu.find('minute') != -1 or self.granu.find('second') != -1:
                output += '_' + str(self.start).split(' ')[1].replace(':', '_')
        return output

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis
    from grass.pygrass.utils import copy as gcopy
    from grass.pygrass.messages import Messenger
    from grass.pygrass.vector import Vector
    
    # Get the options
    input = options["input"]
    output = options["output"]
    strds = options["strds"]
    where = options["where"]
    tempwhere = options["t_where"]

    if output and flags['u']:
        grass.fatal(_("Cannot combine 'output' option and 'u' flag"))
    elif not output and not flags['u']:
        grass.fatal(_("'output' option or 'u' flag must be given"))
    elif not output and flags['u']:
        grass.warning(_("Attribute table of vector {name} will be updated...").format(name=input))

    if where == "" or where == " " or where == "\n":
        where = None

    overwrite = grass.overwrite()

    quiet = True

    if grass.verbosity() > 2:
        quiet = False

    # Check the number of sample strds and the number of columns
    strds_names = strds.split(",")

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    samples = []

    first_strds = tgis.open_old_stds(strds_names[0], "strds", dbif)
    # Single space time raster dataset
    if len(strds_names) == 1:
        granu = first_strds.get_granularity()
        rows = first_strds.get_registered_maps("name,mapset,start_time,end_time",
                                               tempwhere, "start_time",
                                               dbif)

        if not rows:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is empty") %
                        first_strds.get_id())
        for row in rows:
            start = row["start_time"]
            end = row["end_time"]
            raster_maps = [row["name"] + "@" + row["mapset"], ]

            s = Sample(start, end, raster_maps, first_strds.get_name(), granu)
            samples.append(s)
    else:
        # Multiple space time raster datasets
        for name in strds_names[1:]:
            dataset = tgis.open_old_stds(name, "strds", dbif)
            if dataset.get_temporal_type() != first_strds.get_temporal_type():
                grass.fatal(_("Temporal type of space time raster "
                              "datasets must be equal\n<%(a)s> of type "
                              "%(type_a)s do not match <%(b)s> of type "
                              "%(type_b)s" % {"a": first_strds.get_id(),
                                              "type_a": first_strds.get_temporal_type(),
                                              "b": dataset.get_id(),
                                              "type_b": dataset.get_temporal_type()}))

        mapmatrizes = tgis.sample_stds_by_stds_topology("strds", "strds",
                                                        strds_names,
                                                        strds_names[0],
                                                        False, None,
                                                        "equal", False,
                                                        False)
        #TODO check granularity for multiple STRDS
        for i in range(len(mapmatrizes[0])):
            isvalid = True
            mapname_list = []
            for mapmatrix in mapmatrizes:

                entry = mapmatrix[i]

                if entry["samples"]:
                    sample = entry["samples"][0]
                    name = sample.get_id()
                    if name is None:
                        isvalid = False
                        break
                    else:
                        mapname_list.append(name)

            if isvalid:
                entry = mapmatrizes[0][i]
                map = entry["granule"]

                start, end = map.get_temporal_extent_as_tuple()
                s = Sample(start, end, mapname_list, name)
                samples.append(s)

    # Get the layer and database connections of the input vector
    if output:
        gcopy(input, output, 'vector')
    else:
        output = input

    msgr = Messenger()
    perc_curr = 0
    perc_tot = len(samples)
    pymap = Vector(output)
    try:
        pymap.open('r')
    except:
        dbif.close()
        grass.fatal(_("Unable to create vector map <%s>" % output))

    if len(pymap.dblinks) == 0:
        try:
            pymap.close()
            grass.run_command("v.db.addtable", map=output)
        except CalledModuleError:
            dbif.close()
            grass.fatal(_("Unable to add table <%s> to vector map <%s>" % output))
    if pymap.is_open():
        pymap.close()

    for sample in samples:
        raster_names = sample.raster_names
        # Call v.what.rast for each raster map

        for name in raster_names:
            coltype = "DOUBLE PRECISION"
            # Get raster map type
            raster_map = tgis.RasterDataset(name)
            raster_map.load()
            if raster_map.metadata.get_datatype() == "CELL":
                coltype = "INT"
            day = sample.printDay()
            column_name = "%s_%s" % (sample.strds_name, day)
            column_string = "%s %s" % (column_name, coltype)
            column_string.replace('.', '_')
            try:
                grass.run_command("v.db.addcolumn", map=output,
                                  column=column_string,
                                  overwrite=overwrite)
            except CalledModuleError:
                dbif.close()
                grass.fatal(_("Unable to add column %s to vector map "
                              "<%s> ") % (column_string, output))
            try:
                grass.run_command("v.what.rast", map=output, raster=name,
                                  column=column_name, where=where,
                                  quiet=quiet)
            except CalledModuleError:
                dbif.close()
                grass.fatal(_("Unable to run v.what.rast for vector map"
                              " <%s> and raster map <%s>") %
                            (output, str(raster_names)))

        msgr.percent(perc_curr, perc_tot, 1)
        perc_curr += 1

    dbif.close()
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
