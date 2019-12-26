"""
Functions to create space time dataset lists

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)


(C) 2012-2016 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function
from .core import get_tgis_message_interface, get_available_temporal_mapsets, init_dbif
from .datetime_math import time_delta_to_relative_time
from .space_time_datasets import RasterDataset
from .factory import dataset_factory
from .open_stds import open_old_stds
import grass.script as gscript

###############################################################################


def get_dataset_list(type, temporal_type, columns=None, where=None,
                     order=None, dbif=None):
    """ Return a list of time stamped maps or space time datasets of a specific
        temporal type that are registered in the temporal database

        This method returns a dictionary, the keys are the available mapsets,
        the values are the rows from the SQL database query.

        :param type: The type of the datasets (strds, str3ds, stvds, raster,
                     raster_3d, vector)
        :param temporal_type: The temporal type of the datasets (absolute,
                              relative)
        :param columns: A comma separated list of columns that will be selected
        :param where: A where statement for selected listing without "WHERE"
        :param order: A comma separated list of columns to order the
                      datasets by category
        :param dbif: The database interface to be used

        :return: A dictionary with the rows of the SQL query for each
                 available mapset

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> tgis.core.init()
            >>> name = "list_stds_test"
            >>> sp = tgis.open_stds.open_new_stds(name=name, type="strds",
            ... temporaltype="absolute", title="title", descr="descr",
            ... semantic="mean", dbif=None, overwrite=True)
            >>> mapset = tgis.get_current_mapset()
            >>> stds_list = tgis.list_stds.get_dataset_list("strds", "absolute", columns="name")
            >>> rows =  stds_list[mapset]
            >>> for row in rows:
            ...     if row["name"] == name:
            ...         print(True)
            True
            >>> stds_list = tgis.list_stds.get_dataset_list("strds", "absolute", columns="name,mapset", where="mapset = '%s'"%(mapset))
            >>> rows =  stds_list[mapset]
            >>> for row in rows:
            ...     if row["name"] == name and row["mapset"] == mapset:
            ...         print(True)
            True
            >>> check = sp.delete()

    """
    id = None
    sp = dataset_factory(type, id)

    dbif, connected = init_dbif(dbif)

    mapsets = get_available_temporal_mapsets()

    result = {}

    for mapset in mapsets.keys():

        if temporal_type == "absolute":
            table = sp.get_type() + "_view_abs_time"
        else:
            table = sp.get_type() + "_view_rel_time"

        if columns and columns.find("all") == -1:
            sql = "SELECT " + str(columns) + " FROM " + table
        else:
            sql = "SELECT * FROM " + table

        if where:
            sql += " WHERE " + where
            sql += " AND mapset = '%s'" % (mapset)
        else:
            sql += " WHERE mapset = '%s'" % (mapset)

        if order:
            sql += " ORDER BY " + order

        dbif.execute(sql,  mapset=mapset)
        rows = dbif.fetchall(mapset=mapset)

        if rows:
            result[mapset] = rows

    if connected:
        dbif.close()

    return result

###############################################################################


def list_maps_of_stds(type, input, columns, order, where, separator,
                      method, no_header=False, gran=None, dbif=None,
                      outpath=None):
    """ List the maps of a space time dataset using different methods

        :param type: The type of the maps raster, raster3d or vector
        :param input: Name of a space time raster dataset
        :param columns: A comma separated list of columns to be printed to stdout
        :param order: A comma separated list of columns to order the
                      maps by category
        :param where: A where statement for selected listing without "WHERE"
                      e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
        :param separator: The field separator character between the columns
        :param method: String identifier to select a method out of cols,
                       comma,delta or deltagaps
        :param dbif: The database interface to be used

            - "cols" Print preselected columns specified by columns
            - "comma" Print the map ids ("name@mapset") as comma separated string
            - "delta" Print the map ids ("name@mapset") with start time,
               end time, relative length of intervals and the relative
               distance to the begin
            - "deltagaps" Same as "delta" with additional listing of gaps.
               Gaps can be easily identified as the id is "None"
            - "gran" List map using the granularity of the space time dataset,
               columns are identical to deltagaps

        :param no_header: Suppress the printing of column names
        :param gran: The user defined granule to be used if method=gran is
                     set, in case gran=None the granule of the space time
                     dataset is used
        :param outpath: The path to file where to save output
    """

    dbif, connected = init_dbif(dbif)
    msgr = get_tgis_message_interface()

    sp = open_old_stds(input, type, dbif)

    if separator is None or separator == "":
        separator = "\t"

    if outpath:
        outfile = open(outpath, 'w')

    # This method expects a list of objects for gap detection
    if method == "delta" or method == "deltagaps" or method == "gran":
        if type == "stvds":
            columns = "id,name,layer,mapset,start_time,end_time"
        else:
            columns = "id,name,mapset,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where=where,
                                                               dbif=dbif)
        elif method == "delta":
            maps = sp.get_registered_maps_as_objects(where=where,
                                                     order="start_time",
                                                     dbif=dbif)
        elif method == "gran":
            if gran is not None and gran != "":
                maps = sp.get_registered_maps_as_objects_by_granularity(gran=gran,
                                                                        dbif=dbif)
            else:
                maps = sp.get_registered_maps_as_objects_by_granularity(dbif=dbif)

        if no_header is False:
            string = ""
            string += "%s%s" % ("id", separator)
            string += "%s%s" % ("name", separator)
            if type == "stvds":
                string += "%s%s" % ("layer", separator)
            string += "%s%s" % ("mapset", separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s" % ("distance_from_begin")
            if outpath:
                outfile.write('{st}\n'.format(st=string))
            else:
                print(string)

        if maps and len(maps) > 0:

            if isinstance(maps[0], list):
                if len(maps[0]) > 0:
                    first_time, dummy = maps[0][0].get_temporal_extent_as_tuple()
                else:
                    msgr.warning(_("Empty map list"))
                    return
            else:
                first_time, dummy = maps[0].get_temporal_extent_as_tuple()

            for mymap in maps:

                if isinstance(mymap, list):
                    if len(mymap) > 0:
                        map = mymap[0]
                    else:
                        msgr.fatal(_("Empty entry in map list, this should not happen"))
                else:
                    map = mymap

                start, end = map.get_temporal_extent_as_tuple()
                if end:
                    delta = end - start
                else:
                    delta = None
                delta_first = start - first_time

                if map.is_time_absolute():
                    if end:
                        delta = time_delta_to_relative_time(delta)
                    delta_first = time_delta_to_relative_time(delta_first)

                string = ""
                string += "%s%s" % (map.get_id(), separator)
                string += "%s%s" % (map.get_name(), separator)
                if type == "stvds":
                    string += "%s%s" % (map.get_layer(), separator)
                string += "%s%s" % (map.get_mapset(), separator)
                string += "%s%s" % (start, separator)
                string += "%s%s" % (end, separator)
                string += "%s%s" % (delta, separator)
                string += "%s" % (delta_first)
                if outpath:
                    outfile.write('{st}\n'.format(st=string))
                else:
                    print(string)

    else:
        # In comma separated mode only map ids are needed
        if method == "comma":
            if columns not in ['id', 'name']:
                columns = "id"

        rows = sp.get_registered_maps(columns, where, order, dbif)
        
        if not rows:
            dbif.close()
            err = "Space time %(sp)s dataset <%(i)s> is empty"
            if where:
                err += " or where condition is wrong"
            gscript.fatal(_(err) % {
                            'sp': sp.get_new_map_instance(None).get_type(),
                            'i': sp.get_id()})

        if rows:
            if method == "comma":
                string = ""
                count = 0
                for row in rows:
                    if count == 0:
                        string += row[columns]
                    else:
                        string += ",%s" % row[columns]
                    count += 1
                if outpath:
                    outfile.write('{st}\n'.format(st=string))
                else:
                    print(string)

            elif method == "cols":
                # Print the column names if requested
                if no_header is False:
                    output = ""
                    count = 0

                    collist = columns.split(",")

                    for key in collist:
                        if count > 0:
                            output += separator + str(key)
                        else:
                            output += str(key)
                        count += 1
                    if outpath:
                        outfile.write('{st}\n'.format(st=output))
                    else:
                        print(output)

                for row in rows:
                    output = ""
                    count = 0
                    for col in row:
                        if count > 0:
                            output += separator + str(col)
                        else:
                            output += str(col)
                        count += 1
                    if outpath:
                        outfile.write('{st}\n'.format(st=output))
                    else:
                        print(output)
    if outpath:
        outfile.close()
    if connected:
        dbif.close()

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
