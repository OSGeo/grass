#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rast.neighbors
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Performs a neighborhood analysis for each map in a space time
#               raster dataset.
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
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
# % description: Performs a neighborhood analysis for each map in a space time raster dataset.
# % keyword: temporal
# % keyword: aggregation
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_STRDS_OUTPUT
# %end

# %option G_OPT_T_WHERE
# %end

# %option
# % key: region_relation
# % description: Process only maps with this spatial relation to the current computational region
# % guisection: Selection
# % options: overlaps, contains, is_contained
# % required: no
# % multiple: no
# %end

# %option G_OPT_R_INPUT
# % key: selection
# % required: no
# % description: Name of an input raster map to select the cells which should be processed
# %end

# %option
# % key: size
# % type: integer
# % description: Neighborhood size
# % required: no
# % multiple: no
# % answer: 3
# %end

# %option
# % key: method
# % type: string
# % description: Aggregate operation to be performed on the raster maps
# % required: yes
# % multiple: no
# % options: average,median,mode,minimum,maximum,range,stddev,sum,count,variance,diversity,interspersion,quart1,quart3,perc90,quantile
# % answer: average
# %end

# %option
# % key: weighting_function
# % type: string
# % required: no
# % multiple: no
# % options: none,gaussian,exponential,file
# % description: Weighting function
# % descriptions: none;No weighting; gaussian;Gaussian weighting function; exponential;Exponential weighting function; file;File with a custom weighting matrix
# % answer: none
# %end

# %option
# % key: weighting_factor
# % type: double
# % required: no
# % multiple: no
# % description: Factor used in the selected weighting function (ignored for weighting_function=none and file)
# %end

# %option G_OPT_F_INPUT
# % key: weight
# % type: string
# % required: no
# % multiple: no
# % description: Text file containing weights
# %end

# %option
# % key: quantile
# % type: double
# % required: no
# % multiple: yes
# % options: 0.0-1.0
# % description: Quantile to calculate for method=quantile
# % guisection: Neighborhood
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: yes
# % multiple: no
# %end

# %option
# % key: suffix
# % type: string
# % description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
# % answer: gran
# % required: no
# % multiple: no
# %end

# %option
# % key: semantic_labels
# % type: string
# % options: input,method
# % description: Set semantic labels
# % descriptions: input;copy semantic labels from input to output;method;append method name to input label if existing, otherwise use method name
# % answer: input
# % required: no
# % multiple: no
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of r.neighbor processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
# %end

# %flag
# % key: c
# % description: Use circular neighborhood
# %end

# %flag
# % key: e
# % description: Extend existing space time raster dataset
# %end

# %flag
# % key: n
# % description: Register Null maps
# %end

# %flag
# % key: r
# % description: Ignore the current region settings and use the raster map regions
# %end

import copy

import grass.script as gs


def main():
    # lazy imports
    import grass.pygrass.modules as pymod
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    region_relation = options["region_relation"]
    size = options["size"]
    base = options["basename"]
    register_null = flags["n"]
    use_raster_region = flags["r"]
    method = options["method"]
    nprocs = options["nprocs"]
    time_suffix = options["suffix"]
    new_labels = options["semantic_labels"]
    quantiles = (
        [float(quant) for quant in options["quantile"].split(",")]
        if options["quantile"]
        else None
    )

    if method == "quantile" and not options["quantile"]:
        gs.fatal(_("The method <quantile> requires input in the 'quantile' option."))

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    overwrite = gs.overwrite()

    sp = tgis.open_old_stds(input, "strds", dbif)

    spatial_extent = None
    if region_relation:
        spatial_extent = gs.parse_command("g.region", flags="3gu")

    maps = sp.get_registered_maps_as_objects(
        where=where,
        spatial_extent=spatial_extent,
        spatial_relation=region_relation,
        dbif=dbif,
    )

    if not maps:
        dbif.close()
        gs.warning(_("Space time raster dataset <{}> is empty").format(sp.get_id()))
        return

    output_strds = tgis.check_new_stds(output, "strds", dbif=dbif, overwrite=overwrite)
    output_exists = output_strds.is_in_db(dbif)
    # Configure the r.neighbor module
    neighbor_module = pymod.Module(
        "r.neighbors",
        flags="c" if flags["c"] else "",
        input="dummy",
        output="dummy",
        run_=False,
        finish_=False,
        selection=options["selection"],
        size=int(size),
        method=method,
        quantile=quantiles,
        weighting_function=options["weighting_function"],
        weighting_factor=(
            float(options["weighting_factor"]) if options["weighting_factor"] else None
        ),
        weight=options["weight"],
        overwrite=overwrite,
        quiet=True,
    )

    gregion_module = pymod.Module(
        "g.region",
        raster="dummy",
        run_=False,
        finish_=False,
    )

    # The module queue for parallel execution
    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    count = 0
    num_maps = len(maps)
    new_maps = []

    # run r.neighbors all selected maps
    for map in maps:
        count += 1
        if sp.get_temporal_type() == "absolute" and time_suffix == "gran":
            suffix = tgis.create_suffix_from_datetime(
                map.temporal_extent.get_start_time(), sp.get_granularity()
            )
            map_name = f"{base}_{suffix}"
        elif sp.get_temporal_type() == "absolute" and time_suffix == "time":
            suffix = tgis.create_time_suffix(map)
            map_name = f"{base}_{suffix}"
        else:
            map_name = tgis.create_numeric_suffix(base, count, time_suffix)

        new_map = tgis.open_new_map_dataset(
            map_name,
            None,
            type="raster",
            temporal_extent=map.get_temporal_extent(),
            overwrite=overwrite,
            dbif=dbif,
        )
        semantic_label = map.metadata.get_semantic_label()
        if new_labels == "input":
            if semantic_label is not None:
                new_map.set_semantic_label(semantic_label)
        elif new_labels == "method":
            if semantic_label is not None:
                semantic_label = f"{semantic_label}_{method}"
            else:
                semantic_label = method
            new_map.set_semantic_label(semantic_label)
        new_maps.append(new_map)

        mod = copy.deepcopy(neighbor_module)
        mod(input=map.get_id(), output=new_map.get_id())

        if use_raster_region is True:
            reg = copy.deepcopy(gregion_module)
            reg(raster=map.get_id())
            gs.verbose(reg.get_bash())
            mm = pymod.MultiModule([reg, mod], sync=False, set_temp_region=True)
            process_queue.put(mm)
        else:
            process_queue.put(mod)
        gs.verbose(mod.get_bash())

    # Wait for unfinished processes
    process_queue.wait()
    proc_list = process_queue.get_finished_modules()

    # Check return status of all finished modules
    error = 0
    for proc in proc_list:
        if proc.returncode != 0:
            gs.error(
                _("Error running module: {mod}\n    stderr: {error}").format(
                    mod=proc.get_bash(), error=proc.outputs.stderr
                )
            )
            error += 1

    if error > 0:
        gs.fatal(_("Error running modules."))

    # Open a new space time raster dataset
    if not output_exists or (overwrite and not flags["e"]):
        # Get basic metadata
        temporal_type, semantic_type, title, description = sp.get_initial_values()

        # Create new STRDS
        output_strds = tgis.open_new_stds(
            output,
            "strds",
            temporal_type,
            title,
            description,
            semantic_type,
            dbif,
            overwrite,
        )

    # Append to existing
    elif output_exists and flags["e"]:
        output_strds = tgis.open_old_stds(output, "strds", dbif)

    num_maps = len(new_maps)
    # collect empty maps to remove them
    empty_maps = []

    # Register the maps in the database
    for count, raster_map in enumerate(new_maps, 1):
        if count % 10 == 0:
            gs.percent(count, num_maps, 1)

        # Do not register empty maps
        raster_map.load()
        if (
            raster_map.metadata.get_min() is None
            and raster_map.metadata.get_max() is None
        ):
            if not register_null:
                empty_maps.append(raster_map)
                continue

        # Insert map in temporal database
        raster_map.insert(dbif)
        output_strds.register_map(raster_map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    output_strds.update_from_registered_maps(dbif)
    gs.percent(1, 1, 1)

    if output_exists:
        output_strds.update_command_string(dbif=dbif)

    # Remove empty maps
    if len(empty_maps) > 0:
        gs.run_command(
            "g.remove",
            flags="f",
            type="raster",
            name=",".join([raster_map.get_name() for raster_map in empty_maps]),
            quiet=True,
        )

    dbif.close()


############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()
    main()
