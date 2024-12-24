"""
Functions to create space time dataset lists

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)


(C) 2012-2022 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Soeren Gebbert
:authors: Vaclav Petras
"""

import os
import sys
from contextlib import contextmanager

import grass.script as gs

from .core import get_available_temporal_mapsets, get_tgis_message_interface, init_dbif
from .datetime_math import time_delta_to_relative_time
from .factory import dataset_factory
from .open_stds import open_old_stds

###############################################################################


def get_dataset_list(
    type, temporal_type, columns=None, where=None, order=None, dbif=None
):
    """Return a list of time stamped maps or space time datasets of a specific
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
        >>> sp = tgis.open_stds.open_new_stds(
        ...     name=name,
        ...     type="strds",
        ...     temporaltype="absolute",
        ...     title="title",
        ...     descr="descr",
        ...     semantic="mean",
        ...     dbif=None,
        ...     overwrite=True,
        ... )
        >>> mapset = tgis.get_current_mapset()
        >>> stds_list = tgis.list_stds.get_dataset_list(
        ...     "strds", "absolute", columns="name"
        ... )
        >>> rows = stds_list[mapset]
        >>> for row in rows:
        ...     if row["name"] == name:
        ...         print(True)
        ...
        True
        >>> stds_list = tgis.list_stds.get_dataset_list(
        ...     "strds",
        ...     "absolute",
        ...     columns="name,mapset",
        ...     where="mapset = '%s'" % (mapset),
        ... )
        >>> rows = stds_list[mapset]
        >>> for row in rows:
        ...     if row["name"] == name and row["mapset"] == mapset:
        ...         print(True)
        ...
        True
        >>> check = sp.delete()

    """
    id = None
    sp = dataset_factory(type, id)

    dbif, connection_state_changed = init_dbif(dbif)

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

        dbif.execute(sql, mapset=mapset)
        rows = dbif.fetchall(mapset=mapset)

        if rows:
            result[mapset] = rows

    if connection_state_changed:
        dbif.close()

    return result


###############################################################################


@contextmanager
def _open_output_file(file, encoding="utf-8", **kwargs):
    if not file:
        yield sys.stdout
    elif not isinstance(file, (str, os.PathLike)):
        yield file
    else:
        with open(file, "w", encoding=encoding, **kwargs) as stream:
            yield stream


def _write_line(items, separator, file) -> None:
    if not separator:
        separator = ","
    output = separator.join([f"{item}" for item in items])
    with _open_output_file(file) as stream:
        print(f"{output}", file=stream)


def _write_plain(rows, header, separator, file) -> None:
    def write_plain_row(items, separator, file) -> None:
        output = separator.join([f"{item}" for item in items])
        print(f"{output}", file=file)

    with _open_output_file(file) as stream:
        # Print the column names if requested
        if header:
            write_plain_row(items=header, separator=separator, file=stream)
        for row in rows:
            write_plain_row(items=row, separator=separator, file=stream)


def _write_json(rows, column_names, file) -> None:
    # Lazy import output format-specific dependencies.
    # pylint: disable=import-outside-toplevel
    import datetime
    import json

    class ResultsEncoder(json.JSONEncoder):
        """Results encoder for JSON which handles SimpleNamespace objects"""

        def default(self, o):
            """Handle additional types"""
            if isinstance(o, datetime.datetime):
                return f"{o}"
            return super().default(o)

    dict_rows = []
    for row in rows:
        new_row = {}
        for key, value in zip(column_names, row):
            new_row[key] = value
        dict_rows.append(new_row)
    meta = {"column_names": column_names}
    with _open_output_file(file) as stream:
        json.dump({"data": dict_rows, "metadata": meta}, stream, cls=ResultsEncoder)


def _write_yaml(rows, column_names, file=sys.stdout) -> None:
    # Lazy import output format-specific dependencies.
    # pylint: disable=import-outside-toplevel
    import yaml

    class NoAliasIndentListSafeDumper(yaml.SafeDumper):
        """YAML dumper class which does not create aliases and indents lists

        This avoid dates being labeled with &id001 and referenced with *id001.
        Instead, same dates are simply repeated.

        Lists have their dash-space (- ) indented instead of considering the
        dash and space to be a part of indentation. This might be better handled
        when https://github.com/yaml/pyyaml/issues/234 is resolved.
        """

        def ignore_aliases(self, data) -> bool:
            return True

        def increase_indent(self, flow: bool = False, indentless: bool = False):
            return super().increase_indent(flow=flow, indentless=False)

    dict_rows = []
    for row in rows:
        new_row = {}
        for key, value in zip(column_names, row):
            new_row[key] = value
        dict_rows.append(new_row)
    meta = {"column_names": column_names}
    with _open_output_file(file) as stream:
        print(
            yaml.dump(
                {"data": dict_rows, "metadata": meta},
                Dumper=NoAliasIndentListSafeDumper,
                default_flow_style=False,
            ),
            end="",
            file=stream,
        )


def _write_csv(rows, column_names, separator, file=sys.stdout) -> None:
    # Lazy import output format-specific dependencies.
    # pylint: disable=import-outside-toplevel
    import csv

    # Newlines handled by the CSV writer. Set according to the package doc.
    with _open_output_file(file, newline="") as stream:
        spamwriter = csv.writer(
            stream,
            delimiter=separator,
            quotechar='"',
            doublequote=True,
            quoting=csv.QUOTE_NONNUMERIC,
            lineterminator="\n",
        )
        if column_names:
            spamwriter.writerow(column_names)
        for row in rows:
            spamwriter.writerow(row)


def _write_table(rows, column_names, output_format, separator, file):
    if output_format == "json":
        _write_json(rows=rows, column_names=column_names, file=file)
    elif output_format == "yaml":
        _write_yaml(rows=rows, column_names=column_names, file=file)
    elif output_format == "plain":
        # No particular reason for this separator except that this is the original
        # behavior.
        if not separator:
            separator = "\t"
        _write_plain(rows=rows, header=column_names, separator=separator, file=file)
    elif output_format == "csv":
        if not separator:
            separator = ","
        _write_csv(rows=rows, column_names=column_names, separator=separator, file=file)
    else:
        msg = f"Unknown value '{output_format}' for output_format"
        raise ValueError(msg)


def _get_get_registered_maps_as_objects_with_method(dataset, where, method, gran, dbif):
    if method == "deltagaps":
        return dataset.get_registered_maps_as_objects_with_gaps(where=where, dbif=dbif)
    if method == "delta":
        return dataset.get_registered_maps_as_objects(
            where=where, order="start_time", dbif=dbif
        )
    if method == "gran":
        if where:
            msg = f"The where parameter is not supported with method={method}"
            raise ValueError(msg)
        if gran is not None and gran != "":
            return dataset.get_registered_maps_as_objects_by_granularity(
                gran=gran, dbif=dbif
            )
        return dataset.get_registered_maps_as_objects_by_granularity(dbif=dbif)
    msg = f"Invalid method '{method}'"
    raise ValueError(msg)


def _get_get_registered_maps_as_objects_delta_gran(
    dataset, where, method, gran, dbif, msgr
):
    maps = _get_get_registered_maps_as_objects_with_method(
        dataset=dataset, where=where, method=method, gran=gran, dbif=dbif
    )
    if not maps:
        return []

    if isinstance(maps[0], list):
        if len(maps[0]) > 0:
            first_time, unused = maps[0][0].get_temporal_extent_as_tuple()
        else:
            msgr.warning(_("Empty map list"))
            return []
    else:
        first_time, unused = maps[0].get_temporal_extent_as_tuple()

    records = []
    for map_object in maps:
        if isinstance(map_object, list):
            if len(map_object) > 0:
                map_object = map_object[0]
            else:
                msgr.fatal(_("Empty entry in map list, this should not happen"))

        start, end = map_object.get_temporal_extent_as_tuple()
        delta = end - start if end else None
        delta_first = start - first_time

        if map_object.is_time_absolute():
            if end:
                delta = time_delta_to_relative_time(delta)
            delta_first = time_delta_to_relative_time(delta_first)
        records.append((map_object, start, end, delta, delta_first))
    return records


def _get_list_of_maps_delta_gran(dataset, columns, where, method, gran, dbif, msgr):
    maps = _get_get_registered_maps_as_objects_delta_gran(
        dataset=dataset, where=where, method=method, gran=gran, dbif=dbif, msgr=msgr
    )
    rows = []
    for map_object, start, end, delta, delta_first in maps:
        row = []
        # Here the names must be the same as in the database
        # to make the interface consistent.
        for column in columns:
            if column == "id":
                row.append(map_object.get_id())
            elif column == "name":
                row.append(map_object.get_name())
            elif column == "layer":
                row.append(map_object.get_layer())
            elif column == "mapset":
                row.append(map_object.get_mapset())
            elif column == "start_time":
                row.append(start)
            elif column == "end_time":
                row.append(end)
            elif column == "interval_length":
                row.append(delta)
            elif column == "distance_from_begin":
                row.append(delta_first)
            else:
                msg = f"Unsupported column '{column}'"
                raise ValueError(msg)
        rows.append(row)
    return rows


def _get_list_of_maps_stds(
    element_type,
    name,
    columns,
    order,
    where,
    method,
    output_format,
    gran=None,
    dbif=None,
):
    dbif, connection_state_changed = init_dbif(dbif)
    msgr = get_tgis_message_interface()

    dataset = open_old_stds(name, element_type, dbif)

    def check_columns(column_names, output_format, element_type):
        if element_type != "stvds" and "layer" in columns:
            msg = f"Column 'layer' is not allowed with temporal type '{element_type}'"
            raise ValueError(msg)
        if output_format == "line" and len(column_names) > 1:
            msg = (
                f"'{output_format}' output_format can have only 1 column, "
                f"not {len(column_names)}"
            )
            raise ValueError(msg)

    # This method expects a list of objects for gap detection
    if method in {"delta", "deltagaps", "gran"}:
        if not columns:
            if output_format == "list":
                # Only one column is needed.
                columns = ["id"]
            else:
                columns = ["id", "name"]
                if element_type == "stvds":
                    columns.append("layer")
                columns.extend(
                    [
                        "mapset",
                        "start_time",
                        "end_time",
                        "interval_length",
                        "distance_from_begin",
                    ]
                )
        check_columns(
            column_names=columns,
            output_format=output_format,
            element_type=element_type,
        )
        rows = _get_list_of_maps_delta_gran(
            dataset=dataset,
            columns=columns,
            where=where,
            method=method,
            gran=gran,
            dbif=dbif,
            msgr=msgr,
        )
    else:
        if columns:
            check_columns(
                column_names=columns,
                output_format=output_format,
                element_type=element_type,
            )
        elif output_format == "line":
            # For list of values, only one column is needed.
            columns = ["id"]
        else:
            columns = ["name", "mapset", "start_time", "end_time"]
        if not order:
            order = "start_time"

        rows = dataset.get_registered_maps(",".join(columns), where, order, dbif)

        # End with error for the old, custom formats. Proper formats simply return
        # empty result whatever empty is for each format (e.g., empty list for JSON).
        if not rows and (output_format in {"plain", "line"}):
            dbif.close()
            gs.fatal(
                _(
                    "Nothing found in the database for space time dataset <{name}> "
                    "(type: {element_type}): {detail}"
                ).format(
                    name=dataset.get_id(),
                    element_type=element_type,
                    detail=(
                        _(
                            "Dataset is empty or where clause is too constrained or "
                            "incorrect"
                        )
                        if where
                        else _("Dataset is empty")
                    ),
                )
            )
    if connection_state_changed:
        dbif.close()
    return rows, columns


# The code is compatible with pre-v8.2 versions, but for v9, it needs to be reviewed
# to remove the backwards compatibility which will clean it up.
def list_maps_of_stds(
    type,  # pylint: disable=redefined-builtin
    input,  # pylint: disable=redefined-builtin
    columns,
    order,
    where,
    separator,
    method,
    no_header: bool = False,
    gran=None,
    dbif=None,
    outpath=None,
    output_format=None,
) -> None:
    """List the maps of a space time dataset using different methods

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
    if not output_format:
        if method == "comma":
            output_format = "line"
        output_format = "plain"

    if columns:
        if isinstance(columns, str):
            columns = columns.split(",")

    rows, columns = _get_list_of_maps_stds(
        element_type=type,
        name=input,
        columns=columns,
        order=order,
        where=where,
        method=method,
        output_format=output_format,
        gran=gran,
        dbif=dbif,
    )

    if output_format == "line":
        _write_line(
            items=[row[0] for row in rows],
            separator=separator,
            file=outpath,
        )
    else:
        _write_table(
            rows=rows,
            column_names=None if no_header else columns,
            separator=separator,
            output_format=output_format,
            file=outpath,
        )


###############################################################################

if __name__ == "__main__":
    import doctest

    doctest.testmod()
