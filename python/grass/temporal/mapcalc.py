"""
Raster and 3d raster mapcalculation functions

(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
import copy
from datetime import datetime
from multiprocessing import Process
import grass.script as gscript
from grass.exceptions import CalledModuleError
from .core import (
    SQLDatabaseInterfaceConnection,
    get_current_mapset,
    get_tgis_message_interface,
)
from .open_stds import open_new_stds, open_old_stds, check_new_stds
from .datetime_math import time_delta_to_relative_time

############################################################################


def dataset_mapcalculator(
    inputs,
    output,
    type,
    expression,
    base,
    method,
    nprocs=1,
    register_null=False,
    spatial=False,
):
    """Perform map-calculations of maps from different space time
    raster/raster3d datasets, using a specific sampling method
    to select temporal related maps.

    A mapcalc expression must be provided to process the temporal
    selected maps. Temporal operators are available in addition to
    the r.mapcalc operators:

    Supported operators for relative and absolute time are:

    - td() - the time delta of the current interval in days
             and fractions of days or the unit in case of relative time
    - start_time() - The start time of the interval from the begin of
                     the time series in days and fractions of days or the
                     unit in case of relative time
    - end_time() - The end time of the current interval from the begin of
                   the time series in days and fractions of days or the
                   unit in case of relative time

    Supported operators for absolute time:

    - start_doy() - Day of year (doy) from the start time [1 - 366]
    - start_dow() - Day of week (dow) from the start time [1 - 7],
                    the start of the week is monday == 1
    - start_year() - The year of the start time [0 - 9999]
    - start_month() - The month of the start time [1 - 12]
    - start_week() - Week of year of the start time [1 - 54]
    - start_day() - Day of month from the start time [1 - 31]
    - start_hour() - The hour of the start time [0 - 23]
    - start_minute() - The minute of the start time [0 - 59]
    - start_second() - The second of the start time [0 - 59]

    - end_doy() - Day of year (doy) from the end time [1 - 366]
    - end_dow() - Day of week (dow) from the end time [1 - 7],
                  the start of the week is monday == 1
    - end_year() - The year of the end time [0 - 9999]
    - end_month() - The month of the end time [1 - 12]
    - end_week() - Week of year of the end time [1 - 54]
    - end_day() - Day of month from the end time [1 - 31]
    - end_hour() - The hour of the end time [0 - 23]
    - end_minute() - The minute of the end time [0 - 59]
    - end_second() - The minute of the end time [0 - 59]

    :param inputs: The names of the input space time raster/raster3d datasets
    :param output: The name of the extracted new space time raster(3d) dataset
    :param type: The type of the dataset: "raster" or "raster3d"
    :param expression: The r(3).mapcalc expression
    :param base: The base name of the new created maps in case a
           mapclac expression is provided
    :param method: The method to be used for temporal sampling
    :param nprocs: The number of parallel processes to be used for
           mapcalc processing
    :param register_null: Set this number True to register empty maps
    :param spatial: Check spatial overlap
    """

    # We need a database interface for fast computation
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = get_current_mapset()
    msgr = get_tgis_message_interface()

    input_name_list = inputs.split(",")

    first_input = open_old_stds(input_name_list[0], type, dbif)

    # skip sampling when only one dataset specified (with different
    # band filters)
    input_name_list_uniq = []
    for input_name in input_name_list:
        ds = open_old_stds(input_name, type, dbif)
        ds_name = ds.get_name(band_reference=False)
        if ds_name not in input_name_list_uniq:
            input_name_list_uniq.append(ds_name)
    do_sampling = len(input_name_list_uniq) > 1

    # All additional inputs in reverse sorted order to avoid
    # wrong name substitution
    input_name_list = input_name_list[1:]
    input_name_list.sort()
    input_name_list.reverse()
    input_list = []

    for input in input_name_list:
        sp = open_old_stds(input, type, dbif)
        input_list.append(copy.copy(sp))

    new_sp = check_new_stds(output, type, dbif, gscript.overwrite())

    # Sample all inputs by the first input and create a sample matrix
    if spatial:
        msgr.message(_("Starting spatio-temporal sampling..."))
    else:
        msgr.message(_("Starting temporal sampling..."))
    map_matrix = []
    id_list = []
    sample_map_list = []

    if len(input_list) > 0 and do_sampling:
        # First entry is the first dataset id
        id_list.append(first_input.get_name())

        has_samples = False
        for dataset in input_list:
            list = dataset.sample_by_dataset(
                stds=first_input, method=method, spatial=spatial, dbif=dbif
            )

            # In case samples are not found
            if not list or len(list) == 0:
                dbif.close()
                msgr.message(_("No samples found for map calculation"))
                return 0

            # The fist entries are the samples
            map_name_list = []
            if not has_samples:
                for entry in list:
                    granule = entry["granule"]
                    # Do not consider gaps
                    if granule.get_id() is None:
                        continue
                    sample_map_list.append(granule)
                    map_name_list.append(granule.get_name())
                # Attach the map names
                map_matrix.append(copy.copy(map_name_list))
                has_samples = True

            map_name_list = []
            for entry in list:
                maplist = entry["samples"]
                granule = entry["granule"]

                # Do not consider gaps in the sampler
                if granule.get_id() is None:
                    continue

                if len(maplist) > 1:
                    msgr.warning(
                        _(
                            "Found more than a single map in a sample "
                            "granule. Only the first map is used for "
                            "computation. Use t.rast.aggregate.ds to "
                            "create synchronous raster datasets."
                        )
                    )

                # Store all maps! This includes non existent maps,
                # identified by id == None
                map_name_list.append(maplist[0].get_name())

            # Attach the map names
            map_matrix.append(copy.copy(map_name_list))

            id_list.append(dataset.get_name())

    else:
        input_list.insert(0, first_input)
        for dataset in input_list:
            list = dataset.get_registered_maps_as_objects(dbif=dbif)

            if list is None:
                dbif.close()
                msgr.message(_("No maps registered in input dataset"))
                return 0

            map_name_list = []
            for map in list:
                map_name_list.append(map.get_name())
                sample_map_list.append(map)

            # Attach the map names
            map_matrix.append(copy.copy(map_name_list))

            id_list.append(dataset.get_name())

    # Needed for map registration
    map_list = []

    if len(map_matrix) > 0:

        msgr.message(_("Starting mapcalc computation..."))

        count = 0
        # Get the number of samples
        num = len(map_matrix[0])

        # Parallel processing
        proc_list = []
        proc_count = 0

        # For all samples
        for i in range(num):

            count += 1
            msgr.percent(count, num, 10)

            # Create the r.mapcalc statement for the current time step
            map_name = "{base}_{suffix}".format(
                base=base, suffix=gscript.get_num_suffix(count, num)
            )
            # Remove spaces and new lines
            expr = expression.replace(" ", "")

            # Check that all maps are in the sample
            valid_maps = True
            # Replace all dataset names with their map names of the
            # current time step
            for j in range(len(map_matrix)):
                if map_matrix[j][i] is None:
                    valid_maps = False
                    break
                # Substitute the dataset name with the map name
                expr = expr.replace(id_list[j], map_matrix[j][i])

            # Proceed with the next sample
            if not valid_maps:
                continue

            # Create the new map id and check if the map is already
            # in the database
            map_id = map_name + "@" + mapset

            new_map = first_input.get_new_map_instance(map_id)

            # Check if new map is in the temporal database
            if new_map.is_in_db(dbif):
                if gscript.overwrite():
                    # Remove the existing temporal database entry
                    new_map.delete(dbif)
                    new_map = first_input.get_new_map_instance(map_id)
                else:
                    msgr.error(
                        _(
                            "Map <%s> is already in temporal database, "
                            "use overwrite flag to overwrite"
                        )
                    )
                    continue

            # Set the time stamp
            if sample_map_list[i].is_time_absolute():
                start, end = sample_map_list[i].get_absolute_time()
                new_map.set_absolute_time(start, end)
            else:
                start, end, unit = sample_map_list[i].get_relative_time()
                new_map.set_relative_time(start, end, unit)

            # Parse the temporal expressions
            expr = _operator_parser(expr, sample_map_list[0], sample_map_list[i])
            # Add the output map name
            expr = "%s=%s" % (map_name, expr)

            map_list.append(new_map)

            msgr.verbose(_('Apply mapcalc expression: "%s"') % expr)

            # Start the parallel r.mapcalc computation
            if type == "raster":
                proc_list.append(Process(target=_run_mapcalc2d, args=(expr,)))
            else:
                proc_list.append(Process(target=_run_mapcalc3d, args=(expr,)))
            proc_list[proc_count].start()
            proc_count += 1

            if proc_count == nprocs or proc_count == num or count == num:
                proc_count = 0
                exitcodes = 0
                for proc in proc_list:
                    proc.join()
                    exitcodes += proc.exitcode

                if exitcodes != 0:
                    dbif.close()
                    msgr.fatal(_("Error while mapcalc computation"))

                # Empty process list
                proc_list = []

        # Register the new maps in the output space time dataset
        msgr.message(_("Starting map registration in temporal database..."))

        (
            temporal_type,
            semantic_type,
            title,
            description,
        ) = first_input.get_initial_values()

        new_sp = open_new_stds(
            output,
            type,
            temporal_type,
            title,
            description,
            semantic_type,
            dbif,
            gscript.overwrite(),
        )
        count = 0

        # collect empty maps to remove them
        empty_maps = []

        # Insert maps in the temporal database and in the new space time
        # dataset
        for new_map in map_list:

            count += 1
            msgr.percent(count, num, 10)

            # Read the map data
            new_map.load()

            # In case of a null map continue, do not register null maps
            if (
                new_map.metadata.get_min() is None
                and new_map.metadata.get_max() is None
            ):
                if not register_null:
                    empty_maps.append(new_map)
                    continue

            # Insert map in temporal database
            new_map.insert(dbif)

            new_sp.register_map(new_map, dbif)

        # Update the spatio-temporal extent and the metadata table entries
        new_sp.update_from_registered_maps(dbif)

        # Remove empty maps
        if len(empty_maps) > 0:
            n_empty, n_tot = len(empty_maps), len(map_list)
            msgr.warning(
                _("Removing {}/{} ({}%) maps because empty...").format(
                    n_empty, n_tot, n_empty * 100.0 / n_tot
                )
            )
            names = ""
            count = 0
            for map in empty_maps:
                if count == 0:
                    names += "%s" % (map.get_name())
                else:
                    names += ",%s" % (map.get_name())
                count += 1
            if type == "raster":
                gscript.run_command(
                    "g.remove", flags="f", type="raster", name=names, quiet=True
                )
            elif type == "raster3d":
                gscript.run_command(
                    "g.remove", flags="f", type="raster_3d", name=names, quiet=True
                )

    dbif.close()


###############################################################################


def _run_mapcalc2d(expr):
    """Helper function to run r.mapcalc in parallel"""
    try:
        gscript.run_command(
            "r.mapcalc", expression=expr, overwrite=gscript.overwrite(), quiet=True
        )
    except CalledModuleError:
        exit(1)


###############################################################################


def _run_mapcalc3d(expr):
    """Helper function to run r3.mapcalc in parallel"""
    try:
        gscript.run_command(
            "r3.mapcalc", expression=expr, overwrite=gscript.overwrite(), quiet=True
        )
    except CalledModuleError:
        exit(1)


###############################################################################


def _operator_parser(expr, first, current):
    """This method parses the expression string and substitutes
    the temporal operators with numerical values.

    Supported operators for relative and absolute time are:

    - td() - the time delta of the current interval in days
      and fractions of days or the unit in case of relative time
    - start_time() - The start time of the interval from the begin of the
                     time series in days and fractions of days or the unit
                     in case of relative time
    - end_time() - The end time of the current interval from the begin of
                   the time series in days and fractions of days or the
                   unit in case of relative time

    Supported operators for absolute time:

    - start_doy() - Day of year (doy) from the start time [1 - 366]
    - start_dow() - Day of week (dow) from the start time [1 - 7],
                    the start of the week is monday == 1
    - start_year() - The year of the start time [0 - 9999]
    - start_month() - The month of the start time [1 - 12]
    - start_week() - Week of year of the start time [1 - 54]
    - start_day() - Day of month from the start time [1 - 31]
    - start_hour() - The hour of the start time [0 - 23]
    - start_minute() - The minute of the start time [0 - 59]
    - start_second() - The second of the start time [0 - 59]
    - end_doy() - Day of year (doy) from the end time [1 - 366]
    - end_dow() - Day of week (dow) from the end time [1 - 7],
                  the start of the week is monday == 1
    - end_year() - The year of the end time [0 - 9999]
    - end_month() - The month of the end time [1 - 12]
    - end_week() - Week of year of the end time [1 - 54]
    - end_day() - Day of month from the end time [1 - 31]
    - end_hour() - The hour of the end time [0 - 23]
    - end_minute() - The minute of the end time [0 - 59]
    - end_second() - The minute of the end time [0 - 59]

    The modified expression is returned.

    """
    is_time_absolute = first.is_time_absolute()

    expr = _parse_td_operator(expr, is_time_absolute, first, current)
    expr = _parse_start_time_operator(expr, is_time_absolute, first, current)
    expr = _parse_end_time_operator(expr, is_time_absolute, first, current)
    expr = _parse_start_operators(expr, is_time_absolute, current)
    expr = _parse_end_operators(expr, is_time_absolute, current)

    return expr


###############################################################################


def _parse_start_operators(expr, is_time_absolute, current):
    """
    Supported operators for absolute time:
    - start_doy() - Day of year (doy) from the start time [1 - 366]
    - start_dow() - Day of week (dow) from the start time [1 - 7],
                    the start of the week is monday == 1
    - start_year() - The year of the start time [0 - 9999]
    - start_month() - The month of the start time [1 - 12]
    - start_week() - Week of year of the start time [1 - 54]
    - start_day() - Day of month from the start time [1 - 31]
    - start_hour() - The hour of the start time [0 - 23]
    - start_minute() - The minute of the start time [0 - 59]
    - start_second() - The second of the start time [0 - 59]
    """

    start, end = current.get_absolute_time()
    msgr = get_tgis_message_interface()

    if expr.find("start_year()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_year()", str(start.year))

    if expr.find("start_month()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_month()", str(start.month))

    if expr.find("start_week()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_week()", str(start.isocalendar()[1]))

    if expr.find("start_day()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_day()", str(start.day))

    if expr.find("start_hour()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_hour()", str(start.hour))

    if expr.find("start_minute()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_minute()", str(start.minute))

    if expr.find("start_second()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_second()", str(start.second))

    if expr.find("start_dow()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        expr = expr.replace("start_dow()", str(start.isoweekday()))

    if expr.find("start_doy()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("start_*")
                )
            )
        year = datetime(start.year, 1, 1)
        delta = start - year

        expr = expr.replace("start_doy()", str(delta.days + 1))

    return expr


###############################################################################


def _parse_end_operators(expr, is_time_absolute, current):
    """
    Supported operators for absolute time:
    - end_doy() - Day of year (doy) from the end time [1 - 366]
    - end_dow() - Day of week (dow) from the end time [1 - 7],
                  the start of the week is monday == 1
    - end_year() - The year of the end time [0 - 9999]
    - end_month() - The month of the end time [1 - 12]
    - end_week() - Week of year of the end time [1 - 54]
    - end_day() - Day of month from the end time [1 - 31]
    - end_hour() - The hour of the end time [0 - 23]
    - end_minute() - The minute of the end time [0 - 59]
    - end_second() - The minute of the end time [0 - 59]

    In case of time instances the end* expression will be replaced by
    null()
    """

    start, end = current.get_absolute_time()
    msgr = get_tgis_message_interface()

    if expr.find("end_year()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_year()", "null()")
        else:
            expr = expr.replace("end_year()", str(end.year))

    if expr.find("end_month()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_month()", "null()")
        else:
            expr = expr.replace("end_month()", str(end.month))

    if expr.find("end_week()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_week()", "null()")
        else:
            expr = expr.replace("end_week()", str(end.isocalendar()[1]))

    if expr.find("end_day()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_day()", "null()")
        else:
            expr = expr.replace("end_day()", str(end.day))

    if expr.find("end_hour()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_hour()", "null()")
        else:
            expr = expr.replace("end_hour()", str(end.hour))

    if expr.find("end_minute()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_minute()", "null()")
        else:
            expr = expr.replace("end_minute()", str(end.minute))

    if expr.find("end_second()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_second()", "null()")
        else:
            expr = expr.replace("end_second()", str(end.second))

    if expr.find("end_dow()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_dow()", "null()")
        else:
            expr = expr.replace("end_dow()", str(end.isoweekday()))

    if expr.find("end_doy()") >= 0:
        if not is_time_absolute:
            msgr.fatal(
                _(
                    "The temporal operators <%s> support only absolute "
                    "time." % ("end_*")
                )
            )
        if not end:
            expr = expr.replace("end_doy()", "null()")
        else:
            year = datetime(end.year, 1, 1)
            delta = end - year

            expr = expr.replace("end_doy()", str(delta.days + 1))

    return expr


###############################################################################


def _parse_td_operator(expr, is_time_absolute, first, current):
    """Parse the time delta operator td(). This operator
    represents the size of the current sample time interval
    in days and fraction of days for absolute time,
    and in relative units in case of relative time.

    In case of time instances, the td() operator will be of type null().
    """
    if expr.find("td()") >= 0:
        td = "null()"
        if is_time_absolute:
            start, end = current.get_absolute_time()
            if end is not None:
                td = time_delta_to_relative_time(end - start)
        else:
            start, end, unit = current.get_relative_time()
            if end is not None:
                td = end - start
        expr = expr.replace("td()", str(td))

    return expr


###############################################################################


def _parse_start_time_operator(expr, is_time_absolute, first, current):
    """Parse the start_time() operator. This operator represent
    the time difference between the start time of the sample space time
    raster dataset and the start time of the current sample interval or
    instance. The time is measured  in days and fraction of days for absolute
    time, and in relative units in case of relative time."""
    if expr.find("start_time()") >= 0:
        if is_time_absolute:
            start1, end = first.get_absolute_time()
            start, end = current.get_absolute_time()
            x = time_delta_to_relative_time(start - start1)
        else:
            start1, end, unit = first.get_relative_time()
            start, end, unit = current.get_relative_time()
            x = start - start1
        expr = expr.replace("start_time()", str(x))

    return expr


###############################################################################


def _parse_end_time_operator(expr, is_time_absolute, first, current):
    """Parse the end_time() operator. This operator represent
    the time difference between the start time of the sample space time
    raster dataset and the end time of the current sample interval. The time
    is measured  in days and fraction of days for absolute time,
    and in relative units in case of relative time.

    The end_time() will be represented by null() in case of a time instance.
    """
    if expr.find("end_time()") >= 0:
        x = "null()"
        if is_time_absolute:
            start1, end = first.get_absolute_time()
            start, end = current.get_absolute_time()
            if end:
                x = time_delta_to_relative_time(end - start1)
        else:
            start1, end, unit = first.get_relative_time()
            start, end, unit = current.get_relative_time()
            if end:
                x = end - start1
        expr = expr.replace("end_time()", str(x))

    return expr
