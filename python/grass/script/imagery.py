"""
Imagery related functions to be used in Python scripts.

:Usage:
  .. code-block:: python

    import grass.script as gs

    gs.imagery.group_to_dict(imagery_group)
    ...

(C) 2024 by Stefan Blumentrath and the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Stefan Blumentrath
"""

from .core import read_command, warning, fatal
from .raster import raster_info


def group_to_dict(
    imagery_group_name,
    subgroup=None,
    dict_keys="semantic_labels",
    dict_values="map_names",
    fill_semantic_label=True,
    env=None,
):
    """Create a dictionary to represent an imagery group with metadata.

    Depending on the dict_keys option, the returned dictionary uses either
    the names of the raster maps ("map_names"), their row indices in the group
    ("indices") or their associated semantic_labels ("semantic_labels") as keys.
    The default is to use semantic_labels. Note that map metadata
    of the maps in the group have to be read to get the semantic label,
    in addition to the group file. The same metadata is read when the
    "metadata" is requested as dict_values. Other supported dict_values
    are "map_names" (default), "semantic_labels", or "indices".

    The function can also operate on the level of subgroups. In case a
    non-existing (or empty sub-group) is requested a warning is printed
    and an empty dictionary is returned (following the behavior of i.group).

    :Example:
      .. code-block:: pycon

        >>> run_command("g.copy", raster="lsat7_2000_10,lsat7_2000_10")
        >>> run_command("r.support", raster="lsat7_2000_10", semantic_label="L8_1")
        >>> run_command("g.copy", raster="lsat7_2000_20,lsat7_2000_20")
        >>> run_command("r.support", raster="lsat7_2000_20", semantic_label="L8_2")
        >>> run_command("g.copy", raster="lsat7_2000_30,lsat7_2000_30")
        >>> run_command("r.support", raster="lsat7_2000_30", semantic_label="L8_3")
        >>> run_command("i.group", group="L8_group",
        >>>             input="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")
        >>> group_to_dict("L8_group")  # doctest: +ELLIPSIS
        {"L8_1": "lsat7_2000_10", ... "L8_3": "lsat7_2000_30"}
        >>> run_command("g.remove", flags="f", type="group", name="L8_group")
        >>> run_command("g.remove", flags="f", type="raster",
        >>>             name="lsat7_2000_10,lsat7_2000_20,lsat7_2000_30")

    :param str imagery_group_name: Name of the imagery group to process (or None)
    :param str subgroup: Name of the imagery sub-group to process (or None)
    :param str dict_keys: What to use as key for dictionary. It can be either
                         "semantic_labels" (default), "map_names" or "indices"
    :param str dict_values: What to use as values for dictionary. It can be either
                           "map_names" (default), "semanic_labels", "indices" or
                           "metadata" (to return dictionaries with full map metadata)
    :param bool fill_semantic_label: If maps in a group do not have a semantic
                                     label, their index in the group is used
                                     instead (default). Otherwise None / "none"
                                     is used.
    :param dict env: Environment to use when parsing the imagery group

    :return: dictionary representing an imagery group with it's maps and their
             semantic labels, row indices in the group, or metadata
    :rtype: dict
    """
    group_dict = {}
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

    if dict_keys not in {"indices", "map_names", "semantic_labels"}:
        msg = f"Invalid dictionary keys <{dict_keys}> requested"
        raise ValueError(msg)

    if dict_values not in {"indices", "map_names", "semantic_labels", "metadata"}:
        msg = f"Invalid dictionary values <{dict_values}> requested"
        raise ValueError(msg)

    if subgroup and not maps_in_group:
        warning(
            _("Empty result returned for subgroup <{sg}> in group <{g}>").format(
                sg=subgroup, g=imagery_group_name
            )
        )

    for idx, raster_map in enumerate(maps_in_group):
        raster_map_info = None
        # Get raster metadata if needed
        if (
            dict_values in {"semantic_labels", "metadata"}
            or dict_keys == "semantic_labels"
        ):
            raster_map_info = raster_info(raster_map, env=env)

        # Get key for dictionary
        if dict_keys == "indices":
            key = str(idx + 1)
        elif dict_keys == "map_names":
            key = raster_map
        elif dict_keys == "semantic_labels":
            key = raster_map_info["semantic_label"]
            if not key or key == '"none"':
                if fill_semantic_label:
                    key = str(idx + 1)
                else:
                    fatal(
                        _(
                            "Semantic label missing for raster map {m} in group <{g}>."
                        ).format(m=raster_map, g=imagery_group_name)
                    )

        if dict_values == "indices":
            val = str(idx + 1)
        elif dict_values == "map_names":
            val = raster_map
        elif dict_values == "semantic_labels":
            val = raster_map_info["semantic_label"]
        elif dict_values == "metadata":
            val = raster_map_info
        if key in group_dict:
            warning(
                _(
                    "Key {k} from raster map {m} already present in group dictionary."
                    "Overwriting existing entry..."
                ).format(k=key, r=raster_map)
            )
        group_dict[key] = val
    return group_dict
