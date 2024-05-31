"""
Imagery related functions to be used in Python scripts.

Usage:

::

    import grass.script as gs

    gs.imagery.group_to_dict(imagery_group)
    ...

(C) 2024 by Stefan Blumentrath and the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Stefan Blumentrath
"""

from grass.exceptions import CalledModuleError

from .core import read_command, warning
from .raster import raster_info


def group_to_dict(
    imagery_group_name,
    dict_key="semantic_label",
    full_info=True,
    subgroup=None,
    env=None,
):
    """Create a dictionary to represent an imagery group, with it`s raster maps
    and their associated semantic_labels. The dict_key option allows to choose
    if the semantic_labels should be the keys in the dictionary (the default)
    or the map IDs (full map names). The function can also operate on the level
    of subgroups.
    For raster maps in the imagery (sub-)group that do not have a semantic label
    a warning is given.
    If the imagery group is not found, an error message is printed and an empty
    dictionary is returned. In case of non-existing subgroups no error message
    is printed and an empty dictionary is returned.

    Example:

    >>> run_command("g.copy", raster="lsat7_2000_10,lsat7_2000_10")
    0
    >>> run_command("r.support", raster="lsat7_2000_10", semantic_label="L8_1")
    0
    >>> run_command("g.copy", raster="lsat7_2000_20,lsat7_2000_20")
    0
    >>> run_command("r.support", raster="lsat7_2000_20", semantic_label="L8_2")
    0
    >>> run_command("g.copy", raster="lsat7_2000_30,lsat7_2000_30")
    0
    >>> run_command("r.support", raster="lsat7_2000_30", semantic_label="L8_3")
    0
    >>> run_command("i.group", group="L8_group",
    >>>             input="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")
    0
    >>> group_to_dict("L8_group", full_info=False)  # doctest: +ELLIPSIS
    {"L8_1": "lsat7_2000_10", ... "L8_3": "lsat7_2000_30"}
    >>> run_command("g.remove", flags="f", type="group", name="L8_group")
    0
    >>> run_command("g.remove", flags="f", type="raster",
    >>>             name="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")
    0

    :param str table: imagery_group_name: Name of the imagery group to process (or None)
    :param str dict_key: What to use as key for dictionary "semantic_labels" or map ids
    :param env: environment

    :return: dictionary with maps and their semantic labels (or row indices in the
             imagery group)
    :rtype: dict
    """
    group_dict = {}
    try:
        maps_in_group = (
            read_command(
                "i.group",
                group=imagery_group_name,
                subgroup=subgroup,
                flags="g",
                quiet=True,
                env=env,
            )
            .strip()
            .split()
        )
    except CalledModuleError:
        return group_dict

    for idx, raster_map in enumerate(maps_in_group):
        raster_map_info = raster_info(raster_map, env=env)
        semantic_label = raster_map_info["semantic_label"]
        if not raster_map_info["semantic_label"]:
            warning(
                _(
                    "Raster map {rm} in group {igroup} does not have a semantic label."
                    "Using the numeric row index in the (sub-) group"
                ).format(rm=raster_map, igroup=imagery_group_name)
            )
            semantic_label = idx + 1
        if dict_key == "semantic_label":
            group_dict[semantic_label] = raster_map_info if full_info else raster_map
        else:
            group_dict[raster_map] = raster_map_info if full_info else semantic_label
    return group_dict
