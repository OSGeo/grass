# MODULE:    grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.


"""Handling of raw results from benchmarking"""

import copy
import json
from types import SimpleNamespace


class ResultsEncoder(json.JSONEncoder):
    """Results encoder for JSON which handles SimpleNamespace objects"""

    def default(self, o):
        """Handle additional types"""
        if isinstance(o, SimpleNamespace):
            return o.__dict__
        return super().default(o)


def save_results(data):
    """Save results structure to JSON.

    If the provided object does not have results attribute,
    it is assumed that the list which should be results attribute was provided,
    so the provided object object is saved under new ``results`` key.

    Returns JSON as str.
    """
    if not hasattr(data, "results"):
        data = dict(results=data)
    return json.dumps(data, cls=ResultsEncoder)


def save_results_to_file(results, filename):
    """Saves results to as file as JSON.

    See :func:`save_results` for details.
    """
    text = save_results(results)
    with open(filename, "w") as file:
        file.write(text)


def load_results(data):
    """Load results structure from JSON.

    Takes str, returns nested structure with SimpleNamespace instead of the
    default dictionary object. Use attribute access to access by key
    (not dict-like syntax).
    """
    return json.loads(data, object_hook=lambda d: SimpleNamespace(**d))


def load_results_from_file(filename):
    """Loads results from a JSON file.

    See :func:`load_results` for details.
    """
    with open(filename, "r") as file:
        return load_results(file.read())


def join_results(results, prefixes=None):
    """Join multiple lists of results together

    The *results* argument either needs to be a list of result objects
    or an object with attribute *results* which is the list of result objects.
    This allows for results loaded from a file to be combined with a simple list.

    The function always returns just a simple list of result objects.
    """
    if not prefixes:
        prefixes = [None] * len(results)
    joined = []
    for result_list, prefix in zip(results, prefixes):
        if hasattr(result_list, "results"):
            # This is the actual list in the full results structure.
            result_list = result_list.results
        for result in result_list:
            result = copy.deepcopy(result)
            if prefix:
                result.label = f"{prefix}: {result.label}"
            joined.append(result)
    return joined
