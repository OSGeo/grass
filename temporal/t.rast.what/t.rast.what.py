#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rast.what
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Sample a space time raster dataset at specific vector point
#               coordinates and write the output to stdout using different
#               layouts
# COPYRIGHT:    (C) 2015-2017 by the GRASS Development Team
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

# %module
# % description: Sample a space time raster dataset at specific vector point coordinates and write the output to stdout using different layouts
# % keyword: temporal
# % keyword: sampling
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_V_INPUT
# % key: points
# % required: no
# %end

# %option G_OPT_M_COORDS
# % required: no
# % description: Comma separated list of coordinates
# %end

# %option G_OPT_STRDS_INPUT
# % key: strds
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# % description: Name for the output file or "-" in case stdout should be used
# % answer: -
# %end

# %option G_OPT_T_WHERE
# %end

# %option G_OPT_M_NULL_VALUE
# %end

# %option G_OPT_F_SEP
# %end

# %option
# % key: order
# % type: string
# % description: Sort the maps by category
# % required: no
# % multiple: yes
# % options: id, name, creator, mapset, creation_time, modification_time, start_time, end_time, north, south, west, east, min, max
# % answer: start_time
# %end

# %option
# % key: layout
# % type: string
# % description: The layout of the output. One point per row (row), one point per column (col), all timsteps in one row (timerow)
# % required: no
# % multiple: no
# % options: row, col, timerow
# % answer: row
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of r.what processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
# %end

# %flag
# % key: n
# % description: Output header row
# %end

# %flag
# % key: i
# % description: Use stdin as input and ignore coordinates and point option
# %end

## Temporary disabled the r.what flags due to test issues
##%flag
##% key: f
##% description: Show the category labels of the grid cell(s)
##%end

##%flag
##% key: r
##% description: Output color values as RRR:GGG:BBB
##%end

##%flag
##% key: i
##% description: Output integer category values, not cell values
##%end

# %flag
# % key: v
# % description: Show the category for vector points map
# %end

import sys
import copy
import grass.script as gscript


############################################################################


def main(options, flags):
    # lazy imports
    import grass.temporal as tgis
    import grass.pygrass.modules as pymod

    # Get the options
    points = options["points"]
    coordinates = options["coordinates"]
    strds = options["strds"]
    output = options["output"]
    where = options["where"]
    order = options["order"]
    layout = options["layout"]
    null_value = options["null_value"]
    separator = gscript.separator(options["separator"])

    nprocs = int(options["nprocs"])
    write_header = flags["n"]
    use_stdin = flags["i"]
    vcat = flags["v"]

    # output_cat_label = flags["f"]
    # output_color = flags["r"]
    # output_cat = flags["i"]

    overwrite = gscript.overwrite()

    if coordinates and points:
        gscript.fatal(_("Options coordinates and points are mutually exclusive"))

    if not coordinates and not points and not use_stdin:
        gscript.fatal(
            _(
                "Please specify the coordinates, the points option or use the 'i' flag to pipe coordinate positions to t.rast.what from stdin, to provide the sampling coordinates"
            )
        )

    if vcat and not points:
        gscript.fatal(_("Flag 'v' required option 'points'"))

    if use_stdin:
        coordinates_stdin = str(sys.__stdin__.read())
        # Check if coordinates are given with site names or IDs
        stdin_length = len(coordinates_stdin.split("\n")[0].split())
        if stdin_length <= 2:
            site_input = False
        elif stdin_length >= 3:
            site_input = True
    else:
        site_input = False

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_stds(strds, "strds", dbif)
    maps = sp.get_registered_maps_as_objects(where=where, order=order, dbif=dbif)
    dbif.close()
    if not maps:
        gscript.fatal(_("Space time raster dataset <%s> is empty") % sp.get_id())

    # Setup flags are disabled due to test issues
    flags = ""
    # if output_cat_label is True:
    #    flags += "f"
    # if output_color is True:
    #    flags += "r"
    # if output_cat is True:
    #    flags += "i"
    if vcat is True:
        flags += "v"

    # Configure the r.what module
    if points:
        r_what = pymod.Module(
            "r.what",
            map="dummy",
            output="dummy",
            run_=False,
            separator=separator,
            points=points,
            overwrite=overwrite,
            flags=flags,
            null_value=null_value,
            quiet=True,
        )
    elif coordinates:
        # Create a list of values
        coord_list = coordinates.split(",")
        r_what = pymod.Module(
            "r.what",
            map="dummy",
            output="dummy",
            run_=False,
            separator=separator,
            coordinates=coord_list,
            overwrite=overwrite,
            flags=flags,
            null_value=null_value,
            quiet=True,
        )
    elif use_stdin:
        r_what = pymod.Module(
            "r.what",
            map="dummy",
            output="dummy",
            run_=False,
            separator=separator,
            stdin_=coordinates_stdin,
            overwrite=overwrite,
            flags=flags,
            null_value=null_value,
            quiet=True,
        )
    else:
        gscript.error(_("Please specify points or coordinates"))

    if len(maps) < nprocs:
        nprocs = len(maps)

    # The module queue for parallel execution
    process_queue = pymod.ParallelModuleQueue(int(nprocs))
    num_maps = len(maps)

    # 400 Maps is the absolute maximum in r.what
    # We need to determie the number of maps that can be processed
    # in parallel

    # First estimate the number of maps per process. We use 400 maps
    # simultaniously as maximum for a single process

    num_loops = int(num_maps / (400 * nprocs))
    remaining_maps = num_maps % (400 * nprocs)

    if num_loops == 0:
        num_loops = 1
        remaining_maps = 0

    # Compute the number of maps for each process
    maps_per_loop = int((num_maps - remaining_maps) / num_loops)
    maps_per_process = int(maps_per_loop / nprocs)
    remaining_maps_per_loop = maps_per_loop % nprocs

    # We put the output files in an ordered list
    output_files = []
    output_time_list = []

    count = 0
    for loop in range(num_loops):
        file_name = gscript.tempfile() + "_%i" % (loop)
        count = process_loop(
            nprocs,
            maps,
            file_name,
            count,
            maps_per_process,
            remaining_maps_per_loop,
            output_files,
            output_time_list,
            r_what,
            process_queue,
        )

    process_queue.wait()

    gscript.verbose(
        "Number of raster map layers remaining for sampling %i" % (remaining_maps)
    )
    if remaining_maps > 0:
        # Use a single process if less then 100 maps
        if remaining_maps <= 100:
            map_names = []
            for i in range(remaining_maps):
                map = maps[count]
                map_names.append(map.get_id())
                count += 1
            mod = copy.deepcopy(r_what)
            mod(map=map_names, output=file_name)
            process_queue.put(mod)
        else:
            maps_per_process = int(remaining_maps / nprocs)
            remaining_maps_per_loop = remaining_maps % nprocs

            file_name = "out_remain"
            process_loop(
                nprocs,
                maps,
                file_name,
                count,
                maps_per_process,
                remaining_maps_per_loop,
                output_files,
                output_time_list,
                r_what,
                process_queue,
            )

    # Wait for unfinished processes
    process_queue.wait()

    # Out the output files in the correct order together
    if layout == "row":
        one_point_per_row_output(
            separator,
            output_files,
            output_time_list,
            output,
            write_header,
            site_input,
            vcat,
        )
    elif layout == "col":
        one_point_per_col_output(
            separator,
            output_files,
            output_time_list,
            output,
            write_header,
            site_input,
            vcat,
        )
    else:
        one_point_per_timerow_output(
            separator,
            output_files,
            output_time_list,
            output,
            write_header,
            site_input,
            vcat,
        )


############################################################################


def one_point_per_row_output(
    separator, output_files, output_time_list, output, write_header, site_input, vcat
):
    """Write one point per row
    output is of type: x,y,start,end,value
    """
    # open the output file for writing
    out_file = open(output, "w") if output != "-" else sys.stdout

    if write_header is True:
        out_str = ""
        if vcat:
            out_str += "cat{sep}"
        if site_input:
            out_str += "x{sep}y{sep}site{sep}start{sep}end{sep}value\n"
        else:
            out_str += "x{sep}y{sep}start{sep}end{sep}value\n"
        out_file.write(out_str.format(sep=separator))

    for count in range(len(output_files)):
        file_name = output_files[count]
        gscript.verbose(_("Transforming r.what output file %s" % (file_name)))
        map_list = output_time_list[count]
        in_file = open(file_name, "r")
        for line in in_file:
            line = line.split(separator)
            if vcat:
                cat = line[0]
                x = line[1]
                y = line[2]
                values = line[4:]
                if site_input:
                    site = line[3]
                    values = line[5:]

            else:
                x = line[0]
                y = line[1]
                if site_input:
                    site = line[2]
                values = line[3:]

            for i in range(len(values)):
                start, end = map_list[i].get_temporal_extent_as_tuple()
                if vcat:
                    cat_str = "{ca}{sep}".format(ca=cat, sep=separator)
                else:
                    cat_str = ""
                if site_input:
                    coor_string = (
                        "%(x)10.10f%(sep)s%(y)10.10f%(sep)s%(site_name)s%(sep)s"
                        % (
                            {
                                "x": float(x),
                                "y": float(y),
                                "site_name": str(site),
                                "sep": separator,
                            }
                        )
                    )
                else:
                    coor_string = "%(x)10.10f%(sep)s%(y)10.10f%(sep)s" % (
                        {"x": float(x), "y": float(y), "sep": separator}
                    )
                time_string = "%(start)s%(sep)s%(end)s%(sep)s%(val)s\n" % (
                    {
                        "start": str(start),
                        "end": str(end),
                        "val": (values[i].strip()),
                        "sep": separator,
                    }
                )

                out_file.write(cat_str + coor_string + time_string)

        in_file.close()

    if out_file is not sys.stdout:
        out_file.close()


############################################################################


def one_point_per_col_output(
    separator, output_files, output_time_list, output, write_header, site_input, vcat
):
    """Write one point per col
    output is of type:
    start,end,point_1 value,point_2 value,...,point_n value

    Each row represents a single raster map, hence a single time stamp
    """
    # open the output file for writing
    out_file = open(output, "w") if output != "-" else sys.stdout

    first = True
    for count in range(len(output_files)):
        file_name = output_files[count]
        gscript.verbose(_("Transforming r.what output file %s" % (file_name)))
        map_list = output_time_list[count]
        in_file = open(file_name, "r")
        lines = in_file.readlines()

        matrix = []
        for line in lines:
            matrix.append(line.split(separator))

        num_cols = len(matrix[0])

        if first is True:
            if write_header is True:
                out_str = "start%(sep)send" % ({"sep": separator})

                # Define different separator for coordinates and sites
                if separator == ",":
                    coor_sep = ";"
                else:
                    coor_sep = ","

                for row in matrix:
                    if vcat:
                        cat = row[0]
                        x = row[1]
                        y = row[2]
                        out_str += (
                            "{sep}{cat}{csep}{x:10.10f}{csep}"
                            "{y:10.10f}".format(
                                cat=cat,
                                x=float(x),
                                y=float(y),
                                sep=separator,
                                csep=coor_sep,
                            )
                        )
                        if site_input:
                            site = row[3]
                            out_str += "{sep}{site}".format(sep=coor_sep, site=site)
                    else:
                        x = row[0]
                        y = row[1]
                        out_str += "{sep}{x:10.10f}{csep}" "{y:10.10f}".format(
                            x=float(x), y=float(y), sep=separator, csep=coor_sep
                        )
                        if site_input:
                            site = row[2]
                            out_str += "{sep}{site}".format(sep=coor_sep, site=site)

                out_file.write(out_str + "\n")

        first = False

        if vcat:
            ncol = 4
        else:
            ncol = 3
        for col in range(num_cols - ncol):
            start, end = output_time_list[count][col].get_temporal_extent_as_tuple()
            time_string = "%(start)s%(sep)s%(end)s" % (
                {"start": str(start), "end": str(end), "sep": separator}
            )
            out_file.write(time_string)
            for row in range(len(matrix)):
                value = matrix[row][col + ncol]
                out_file.write(
                    "%(sep)s%(value)s" % ({"sep": separator, "value": value.strip()})
                )
            out_file.write("\n")

        in_file.close()
    if out_file is not sys.stdout:
        out_file.close()


############################################################################


def one_point_per_timerow_output(
    separator, output_files, output_time_list, output, write_header, site_input, vcat
):
    """Use the original layout of the r.what output and print instead of
    the raster names, the time stamps as header

    One point per line for all time stamps:
     x|y|1991-01-01 00:00:00;1991-01-02 00:00:00|1991-01-02 00:00:00;1991-01-03 00:00:00|1991-01-03 00:00:00;1991-01-04 00:00:00|1991-01-04 00:00:00;1991-01-05 00:00:00
     3730731.49590371|5642483.51236521|6|8|7|7
     3581249.04638104|5634411.97526282|5|8|7|7
    """
    out_file = open(output, "w") if output != "-" else sys.stdout

    matrix = []
    header = ""

    first = True
    for count in range(len(output_files)):
        file_name = output_files[count]
        gscript.verbose("Transforming r.what output file %s" % (file_name))
        map_list = output_time_list[count]
        in_file = open(file_name, "r")

        if write_header:
            if first is True:
                if vcat:
                    header = "cat{sep}".format(sep=separator)
                else:
                    header = ""
                if site_input:
                    header += "x%(sep)sy%(sep)ssite" % ({"sep": separator})
                else:
                    header += "x%(sep)sy" % ({"sep": separator})
            for map in map_list:
                start, end = map.get_temporal_extent_as_tuple()
                time_string = "%(sep)s%(start)s;%(end)s" % (
                    {"start": str(start), "end": str(end), "sep": separator}
                )
                header += time_string

        lines = in_file.readlines()

        for i in range(len(lines)):
            cols = lines[i].split(separator)

            if first is True:
                if vcat and site_input:
                    matrix.append(cols[:4])
                elif vcat or site_input:
                    matrix.append(cols[:3])
                else:
                    matrix.append(cols[:2])

            if vcat:
                matrix[i] = matrix[i] + cols[4:]
            else:
                matrix[i] = matrix[i] + cols[3:]

        first = False

        in_file.close()

    if write_header:
        out_file.write(header + "\n")

    gscript.verbose(_("Writing the output file <%s>" % (output)))
    for row in matrix:
        first = True
        for col in row:
            value = col.strip()

            if first is False:
                out_file.write("%s" % (separator))
            out_file.write(value)

            first = False

        out_file.write("\n")
    if out_file is not sys.stdout:
        out_file.close()


############################################################################


def process_loop(
    nprocs,
    maps,
    file_name,
    count,
    maps_per_process,
    remaining_maps_per_loop,
    output_files,
    output_time_list,
    r_what,
    process_queue,
):
    """Call r.what in parallel subprocesses"""
    first = True
    for process in range(nprocs):
        num = maps_per_process
        # Add the remaining maps to the first process
        if first is True:
            num += remaining_maps_per_loop
        first = False

        # Temporary output file
        final_file_name = file_name + "_%i" % (process)
        output_files.append(final_file_name)

        map_names = []
        map_list = []
        for i in range(num):
            map = maps[count]
            map_names.append(map.get_id())
            map_list.append(map)
            count += 1

        output_time_list.append(map_list)

        gscript.verbose(
            _(
                "Process maps %(samp_start)i to %(samp_end)i (of %(total)i)"
                % (
                    {
                        "samp_start": count - len(map_names) + 1,
                        "samp_end": count,
                        "total": len(maps),
                    }
                )
            )
        )
        mod = copy.deepcopy(r_what)
        mod(map=map_names, output=final_file_name)
        # print(mod.get_bash())
        process_queue.put(mod)

    return count


############################################################################

if __name__ == "__main__":
    options, flags = gscript.parser()
    main(options, flags)
