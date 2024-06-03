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
    subgroup=None,
    dict_key="semantic_labels",
    fill_semantic_label=True,
    full_info=True,
    env=None,
):
    """Create a dictionary to represent an imagery group with metadata.

    Defined by the dict_key option, the dictionary uses either the names
    of the raster maps ("map_names"), their row indices in the group
    ("indices") or their associated semantic_labels ("semantic_labels") as keys.
    The default is to use semantic_labels as keys. Note that map metadata
    of the maps in the group have to be read to get the semantic label,
    in addition to the group file. The same metadata is read when the
    "full_info" should be returned.

    The function can also operate on the level of subgroups. In case a
    non-existing (or empty sub-group) is requested a warning is printed
    and an empty dictionary is returned (following the behavior of i.group).

    Example:

    >>> run_command("g.copy", raster="lsat7_2000_10,lsat7_2000_10")
    >>> run_command("r.support", raster="lsat7_2000_10", semantic_label="L8_1")
    >>> run_command("g.copy", raster="lsat7_2000_20,lsat7_2000_20")
    >>> run_command("r.support", raster="lsat7_2000_20", semantic_label="L8_2")
    >>> run_command("g.copy", raster="lsat7_2000_30,lsat7_2000_30")
    >>> run_command("r.support", raster="lsat7_2000_30", semantic_label="L8_3")
    >>> run_command("i.group", group="L8_group",
    >>>             input="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")
    >>> group_to_dict("L8_group", full_info=False)  # doctest: +ELLIPSIS
    {"L8_1": "lsat7_2000_10", ... "L8_3": "lsat7_2000_30"}
    >>> run_command("g.remove", flags="f", type="group", name="L8_group")
    >>> run_command("g.remove", flags="f", type="raster",
    >>>             name="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")

    :param str table: imagery_group_name: Name of the imagery group to process (or None)
    :param str dict_key: What to use as key for dictionary. Can bei either
                         "semantic_labels" (default), "map_names" or "indices"
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
    except CalledModuleError as cme:
        raise cme

    if subgroup and not maps_in_group:
        warning(
            _("Empty result returned for subgroup <{sg}> in group <{g}>").format(
                sg=subgroup, g=imagery_group_name
            )
        )

    for idx, raster_map in enumerate(maps_in_group):
        raster_map_info = None
        if full_info or dict_key == "semantic_labels":
            raster_map_info = raster_info(raster_map, env=env)

        if dict_key == "indices":
            key = str(idx + 1)
            val = raster_map_info or raster_map
        elif dict_key == "map_names":
            key = raster_map
            val = raster_map_info or str(idx + 1)
        elif dict_key == "semantic_labels":
            key = raster_map_info["semantic_label"]
            if not key or key == '"none"':
                warning(
                    _(
                        "Raster map {m} in group <{g}> does not have a semantic label."
                    ).format(m=raster_map, g=imagery_group_name)
                )
                if fill_semantic_label:
                    key = str(idx + 1)
            val = raster_map_info if full_info else raster_map
        group_dict[key] = val
    return group_dict
