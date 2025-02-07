"""
Raster related functions to be used in Python scripts.

Usage:

::

    from grass.script import raster as grass
    grass.raster_history(map)


(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
"""

from __future__ import annotations

import os
import string
import time
from pathlib import Path

from .core import (
    gisenv,
    find_file,
    tempfile,
    run_command,
    read_command,
    write_command,
    feed_command,
    warning,
    fatal,
)
from grass.exceptions import CalledModuleError
from .utils import (
    encode,
    float_or_dms,
    parse_key_val,
    try_remove,
    append_node_pid,
    append_uuid,
)


def raster_history(map, overwrite=False, env=None):
    """Set the command history for a raster map to the command used to
    invoke the script (interface to `r.support`).

    :param str map: map name
    :param env: environment

    :return: True on success
    :return: False on failure

    """
    current_mapset = gisenv(env)["MAPSET"]
    if find_file(name=map, env=env)["mapset"] != current_mapset:
        warning(
            _(
                "Unable to write history for <%(map)s>. "
                "Raster map <%(map)s> not found in current mapset."
            )
            % {"map": map},
            env=env,
        )
        return False

    if overwrite is True:
        historyfile = tempfile(env=env)
        Path(historyfile).write_text(os.environ["CMDLINE"])
        run_command("r.support", map=map, loadhistory=historyfile, env=env)
        try_remove(historyfile)
    else:
        run_command("r.support", map=map, history=os.environ["CMDLINE"], env=env)
    return True


def raster_info(map, env=None):
    """Return information about a raster map (interface to
    `r.info -gre`). Example:

    >>> raster_info('elevation') # doctest: +ELLIPSIS
    {'creator': '"helena"', 'cols': '1500' ... 'south': 215000.0}

    :param str map: map name
    :param env: environment

    :return: parsed raster info

    """

    def float_or_null(s):
        if s == "NULL":
            return None
        return float(s)

    s = read_command("r.info", flags="gre", map=map, env=env)
    kv = parse_key_val(s)
    for k in ["min", "max"]:
        kv[k] = float_or_null(kv[k])
    for k in ["north", "south", "east", "west"]:
        kv[k] = float(kv[k])
    for k in ["nsres", "ewres"]:
        kv[k] = float_or_dms(kv[k])
    return kv


def mapcalc(
    exp,
    quiet=False,
    superquiet=False,
    verbose=False,
    overwrite=False,
    seed=None,
    env=None,
    **kwargs,
):
    """Interface to r.mapcalc.

    :param str exp: expression
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool superquiet: True to run extra quietly (<tt>--qq</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param seed: an integer used to seed the random-number generator for the
                 rand() function, or 'auto' to generate a random seed
    :param dict env: dictionary of environment variables for child process
    :param kwargs:
    """

    if seed == "auto":
        seed = hash((os.getpid(), time.time())) % (2**32)

    t = string.Template(exp)
    e = t.substitute(**kwargs)

    try:
        write_command(
            "r.mapcalc",
            file="-",
            stdin=e,
            env=env,
            seed=seed,
            quiet=quiet,
            superquiet=superquiet,
            verbose=verbose,
            overwrite=overwrite,
        )
    except CalledModuleError:
        fatal(
            _("An error occurred while running r.mapcalc with expression: %s") % e,
            env=env,
        )


def mapcalc_start(
    exp,
    quiet=False,
    superquiet=False,
    verbose=False,
    overwrite=False,
    seed=None,
    env=None,
    **kwargs,
):
    """Interface to r.mapcalc, doesn't wait for it to finish, returns Popen object.

    >>> output = 'newele'
    >>> input = 'elevation'
    >>> expr1 = '"%s" = "%s" * 10' % (output, input)
    >>> expr2 = '...'   # etc.
    >>> # launch the jobs:
    >>> p1 = mapcalc_start(expr1)
    >>> p2 = mapcalc_start(expr2)
    ...
    >>> # wait for them to finish:
    >>> p1.wait()
    0
    >>> p2.wait()
    1
    >>> run_command('g.remove', flags='f', type='raster', name=output)

    :param str exp: expression
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool superquiet: True to run extra quietly (<tt>--qq</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param seed: an integer used to seed the random-number generator for the
                 rand() function, or 'auto' to generate a random seed
    :param dict env: dictionary of environment variables for child process
    :param kwargs:

    :return: Popen object
    """

    if seed == "auto":
        seed = hash((os.getpid(), time.time())) % (2**32)

    t = string.Template(exp)
    e = t.substitute(**kwargs)

    p = feed_command(
        "r.mapcalc",
        file="-",
        env=env,
        seed=seed,
        quiet=quiet,
        superquiet=superquiet,
        verbose=verbose,
        overwrite=overwrite,
    )
    p.stdin.write(encode(e))
    p.stdin.close()
    return p


def raster_what(map, coord, env=None, localized=False):
    """Interface to r.what

    >>> raster_what('elevation', [[640000, 228000]])
    [{'elevation': {'color': '255:214:000', 'label': '', 'value': '102.479'}}]

    :param str map: the map name
    :param list coord: a list of list containing all the point that you want to query
    :param env:
    """
    map_list = [map] if isinstance(map, (bytes, str)) else map

    coord_list = []
    if isinstance(coord, tuple):
        coord_list.append("%f,%f" % (coord[0], coord[1]))
    else:
        for e, n in coord:
            coord_list.append("%f,%f" % (e, n))

    sep = "|"
    # separator '|' not included in command
    # because | is causing problems on Windows
    # change separator?
    ret = read_command(
        "r.what",
        flags="rf",
        map=",".join(map_list),
        coordinates=",".join(coord_list),
        null=_("No data"),
        quiet=True,
        env=env,
    )
    data = []
    if not ret:
        return data

    if localized:
        labels = (_("value"), _("label"), _("color"))
    else:
        labels = ("value", "label", "color")
    for item in ret.splitlines():
        line = item.split(sep)[3:]
        for i, map_name in enumerate(map_list):
            tmp_dict = {map_name: {}}
            for j in range(len(labels)):
                tmp_dict[map_name][labels[j]] = line[i * len(labels) + j]

            data.append(tmp_dict)

    return data


class MaskManager:
    """Context manager for setting and managing 2D raster mask.

    The context manager makes it possible to have custom mask for the current process.
    In the following example, we set the mask using _r.mask_ which creates a new
    raster which represents the mask. The mask is deactivated at the end of the
    context by the context manager and the raster is removed.

    >>> with gs.MaskManager():
    ...     gs.run_command("r.mask", raster="state_boundary")
    ...     gs.parse_command("r.univar", map="elevation", format="json")

    The _mask_name_ can be a name of an existing raster map and in that case,
    that raster map is used directly as is. If the raster map does not exist,
    the name will be used for the mask once it is created (with _r.mask_).

    The following example uses an existing raster map directly as the mask.
    The mask is disabled at the end of the context, but the raster map is not
    removed.

    >>> with gs.MaskManager(mask_name="state_boundary"):
    ...     gs.parse_command("r.univar", map="elevation", format="json")

    Note the difference between using the name of an existing raster map directly
    and using *r.mask* to create a new mask. Both zeros and NULL values are used
    to represent mask resulting in NULL cells, while *r.mask*
    by default sets the mask in the way that only NULL values in the original raster
    result in NULL cells.

    If _mask_name_ is not provided, it generates a unique name using node (computer)
    name, PID (current process ID), and unique ID (UUID).
    In this case, the raster map representing the mask is removed if it exists at the
    end of the context.
    Optionally, the context manager can remove the raster map at the end of the context
    when _remove_ is set to `True`.
    The defaults for the removal of a mask raster are set to align with the two main use
    cases which is creating the mask within the context and using an existing raster as
    a mask.

    Name of the raster mask is available as the _mask_name_ attribute and can be used to
    directly create a mask (without the need to use *r.mask*). The following example
    uses the attribute to create a mask directly by name. This is equivalent to the
    basic case where a raster named `MASK` is created directly by the user in an
    interactive session.

    >>> with gs.MaskManager() as manager:
    ...     gs.run_command(
                "r.mapcalc", expression=f"{manager.mask_name} = row() < col()"
            )
    ...     gs.run_command(
                "r.mapcalc", expression=f"masked_elevation = elevation"
            )

    In the background, this class manages the `GRASS_MASK` environment variable.
    It modifies the current system environment or the one provided. It does not
    create a copy internally. However, the modified environment is available as
    the _env_ attribute for convenience and consistency with other managers
    which provide this attribute.

    The following code creates a copy of the global environment and lets the manager
    modify it. The copy is then available as the _env_ attribute.

    >>> with gs.MaskManager(env=os.environ.copy()) as manager:
    ...     gs.run_command(
    ...         "r.mapcalc",
    ...         expression=f"{manager.mask_name} = row() < col()",
    ...         env=manager.env
    ...     )
    ...     gs.run_command(
    ...         "r.mapcalc", expression=f"masked_elevation = elevation", env=manager.env
    ...     )
    """

    def __init__(
        self,
        mask_name: str | None = None,
        env: dict[str, str] | None = None,
        remove: bool | None = None,
    ):
        """
        Initializes the MaskManager.

        :param mask_name: Name of the raster mask. Generated if not provided.
        :param env: Environment to use. Defaults to modifying os.environ.
        :param remove: If True, the raster mask will be removed when the context exits.
                       Defaults to True if the mask name is generated,
                       and False if a mask name is provided.
        """
        self.env = env if env is not None else os.environ
        self._original_value = None

        if mask_name is None:
            self.mask_name = append_uuid(append_node_pid("mask"))
            self._remove = True if remove is None else remove
        else:
            self.mask_name = mask_name
            self._remove = False if remove is None else remove

    def __enter__(self):
        """Set mask in the given environment.

        Sets the `GRASS_MASK` environment variable to the provided or
        generated mask name.

        :return: Returns the MaskManager instance.
        """
        self._original_value = self.env.get("GRASS_MASK")
        self.env["GRASS_MASK"] = self.mask_name
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Restore the previous mask state.

        Restores the original value of `GRASS_MASK` and optionally removes
        the raster mask.

        :param exc_type: Exception type, if any.
        :param exc_val: Exception value, if any.
        :param exc_tb: Traceback, if any.
        """
        if self._original_value is not None:
            self.env["GRASS_MASK"] = self._original_value
        else:
            self.env.pop("GRASS_MASK", None)

        if self._remove:
            run_command(
                "g.remove",
                type="raster",
                name=self.mask_name,
                flags="f",
                env=self.env,
                quiet=True,
            )
