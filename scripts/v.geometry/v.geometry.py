#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.geometry
# AUTHOR:       Anna Petrasova
# PURPOSE:      Print geometry metrics of vector features as JSON
# COPYRIGHT:    (C) 2026 by Anna Petrasova and the GRASS Development Team
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Prints geometry metrics of vector features.
# % keyword: vector
# % keyword: geometry
# % keyword: metric
# % keyword: parallel
# %end

# %option G_OPT_V_MAP
# %end

# %option
# % key: metric
# % type: string
# % required: yes
# % multiple: yes
# % options: area,perimeter,length,count,compactness,fractal_dimension,slope,sinuosity,azimuth,coordinates,start,end,bbox
# % description: Geometry metric(s) to compute
# % descriptions: area;area size;perimeter;perimeter length of an area;length;line length;count;number of features for each category;compactness;compactness of an area, calculated as perimeter / (2 * sqrt(PI * area));fractal_dimension;fractal dimension of boundary defining a polygon, calculated as 2 * (log(perimeter) / log(area));slope;slope steepness of vector line or boundary;sinuosity;line sinuosity, calculated as line length / distance between end points;azimuth;line azimuth, calculated as angle between North direction and endnode direction at startnode;coordinates;point coordinates, X,Y or X,Y,Z;start;line/boundary starting point coordinates, X,Y or X,Y,Z;end;line/boundary end point coordinates, X,Y or X,Y,Z;bbox;bounding box of area, N,S,E,W
# %end

# %option G_OPT_V_TYPE
# % options: point,line,boundary,centroid
# % answer: point,line,boundary,centroid
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_M_UNITS
# % multiple: yes
# % options: miles,feet,meters,kilometers,acres,hectares,radians,degrees
# % description: Units (one per metric, positional; unspecified metrics use defaults)
# %end

# %option G_OPT_M_NPROCS
# %end

# %option G_OPT_F_SEP
# % answer: {NULL}
# %end

# %option G_OPT_F_FORMAT
# % options: plain,json,csv
# % answer: json
# % descriptions: plain;Plain text with pipe separator by default;json;JSON (JavaScript Object Notation);csv;CSV (Comma Separated Values)
# %end

import csv
import json
import os
import sys
from concurrent.futures import ThreadPoolExecutor

import grass.script as gs
from grass.tools import Tools


# Map v.geometry metric names to v.to.db option names. Most are identical;
# a few are renamed for clarity (e.g. "compact" -> "compactness").
METRIC_TO_VTODB_OPTION = {
    "area": "area",
    "perimeter": "perimeter",
    "length": "length",
    "count": "count",
    "compactness": "compact",
    "fractal_dimension": "fd",
    "slope": "slope",
    "sinuosity": "sinuous",
    "azimuth": "azimuth",
    "coordinates": "coor",
    "start": "start",
    "end": "end",
    "bbox": "bbox",
}

# Keys that v.to.db emits in its JSON output which v.geometry renames to
# match the user-facing metric names above.
_VTODB_KEY_RENAMES = {
    "compact": "compactness",
    "fd": "fractal_dimension",
    "sinuous": "sinuosity",
}

# Group each metric by the feature type it describes. Records from different
# metrics are merged by category, so mixing metrics from different groups
# (e.g. a line's sinuosity and an area's perimeter) would silently combine
# unrelated features that happen to share a category. "count" applies to any
# feature type and may be combined with any group.
METRIC_GROUPS = {
    "area": "area",
    "perimeter": "area",
    "compactness": "area",
    "fractal_dimension": "area",
    "bbox": "area",
    "length": "line",
    "slope": "line",
    "sinuosity": "line",
    "azimuth": "line",
    "start": "line",
    "end": "line",
    "coordinates": "point",
    "count": "any",
}


def _rename_keys(mapping):
    return {_VTODB_KEY_RENAMES.get(k, k): v for k, v in mapping.items()}


def _available_cpus():
    """Number of CPUs this process may actually use.

    Prefers affinity-aware sources over ``os.cpu_count()``, which reports
    the host total and overcounts in containers and cgroup-limited jobs.
    """
    if hasattr(os, "process_cpu_count"):  # Python 3.13+
        return os.process_cpu_count() or 1
    if hasattr(os, "sched_getaffinity"):  # Linux
        return len(os.sched_getaffinity(0))
    return os.cpu_count() or 1


def _resolve_nprocs(nprocs):
    """Resolve G_OPT_M_NPROCS into a worker count for ThreadPoolExecutor.

    Mirrors the semantics of G_set_omp_num_threads() in
    lib/gis/omp_threads.c: 0 means use all available cores, a positive
    number is used as-is, a negative number means cpu_count + nprocs
    (clamped to at least 1). Belongs in a library helper eventually.
    """
    nprocs = int(nprocs)
    if nprocs > 0:
        return nprocs
    available = _available_cpus()
    if nprocs == 0:
        return available
    return max(1, available + nprocs)


def _run_vtodb(metric, unit, common_kwargs):
    """Run v.to.db for a single metric and return the parsed JSON result."""
    vtodb_option = METRIC_TO_VTODB_OPTION[metric]
    kwargs = dict(common_kwargs)
    if unit:
        kwargs["units"] = unit
    result = Tools().v_to_db(option=vtodb_option, format="json", **kwargs)
    return result.json


def _merge_results(results):
    """Merge per-metric v.to.db results into a single JSON structure.

    Each metric contributes its keys to every record (matched by category),
    and its entries to the shared ``units`` and ``totals`` dicts. The
    ``results`` list must be in the caller's metric order so that the
    resulting record field order is deterministic.
    """
    merged_units = {}
    merged_totals = {}
    # category -> merged record dict
    records_by_cat = {}

    for result in results:
        merged_units.update(_rename_keys(result.get("units", {})))
        merged_totals.update(_rename_keys(result.get("totals", {})))
        for record in result.get("records", []):
            record = _rename_keys(record)
            cat = record["category"]
            if cat in records_by_cat:
                records_by_cat[cat].update(record)
            else:
                records_by_cat[cat] = dict(record)

    # Preserve category order.
    records = [records_by_cat[cat] for cat in sorted(records_by_cat)]
    return {"units": merged_units, "totals": merged_totals, "records": records}


def main():
    options, _flags = gs.parser()

    metrics = options["metric"].split(",")
    groups = {METRIC_GROUPS[m] for m in metrics} - {"any"}
    if len(groups) > 1:
        gs.fatal(
            _(
                "Cannot mix metrics from different feature types: {}. "
                "Results are merged by category, so combining e.g. line and "
                "area metrics would produce misleading records. Run "
                "v.geometry separately for each feature type."
            ).format(
                ", ".join(
                    "{} ({})".format(m, METRIC_GROUPS[m])
                    for m in metrics
                    if METRIC_GROUPS[m] != "any"
                )
            )
        )

    units_list = options["units"].split(",") if options["units"] else []
    if len(units_list) > len(metrics):
        gs.fatal(
            _("More units ({}) than metrics ({}) specified").format(
                len(units_list), len(metrics)
            )
        )
    # Pad with None so every metric has a corresponding entry.
    units_list.extend([None] * (len(metrics) - len(units_list)))

    # v.to.db requires the "columns" parameter even in print-only mode, but
    # does not use it for JSON or plain output; any valid name works.
    common_kwargs = {
        "map": options["map"],
        "type": options["type"],
        "layer": options["layer"],
        "columns": "unused",
        "flags": "p",
    }

    if len(metrics) == 1:
        results = [_run_vtodb(metrics[0], units_list[0], common_kwargs)]
    else:
        # Submit all metrics concurrently but collect results in metric
        # order so downstream column/field ordering is deterministic. Cap
        # at len(metrics); extra workers just sit idle.
        max_workers = min(_resolve_nprocs(options["nprocs"]), len(metrics))
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            futures = [
                executor.submit(_run_vtodb, m, u, common_kwargs)
                for m, u in zip(metrics, units_list, strict=True)
            ]
            results = [f.result() for f in futures]

    result = _merge_results(results)

    output_format = options["format"]
    if output_format == "json":
        print(json.dumps(result, indent=4))
        return 0

    separator = gs.separator(options["separator"])
    records = result["records"]
    if not records:
        return 0
    columns = list(records[0].keys())

    if output_format == "csv":
        if not separator:
            separator = ","
        elif len(separator) > 1:
            gs.fatal(
                _(
                    "A standard CSV separator (delimiter) is only one character "
                    "long, got: {}"
                ).format(separator)
            )
        # Force LF endings; csv.writer defaults to CRLF, which compounds
        # with text-mode stdout's newline translation on some platforms.
        writer = csv.writer(sys.stdout, delimiter=separator, lineterminator="\n")
        writer.writerow(columns)
        writer.writerows([record.get(c, "") for c in columns] for record in records)
    else:  # plain
        if not separator:
            separator = "|"
        print(separator.join(columns))
        for record in records:
            print(separator.join(str(record.get(c, "")) for c in columns))

    return 0


if __name__ == "__main__":
    sys.exit(main())
