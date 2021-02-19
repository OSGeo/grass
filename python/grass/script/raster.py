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
from __future__ import absolute_import

import os
import sys
import string
import time

from .core import *
from grass.exceptions import CalledModuleError
from .utils import encode, float_or_dms, parse_key_val, try_remove


if sys.version_info.major >= 3:
    unicode = str


def raster_history(map, overwrite=False, env=None):
    """Set the command history for a raster map to the command used to
    invoke the script (interface to `r.support`).

    :param str map: map name
    :param env: environment

    :return: True on success
    :return: False on failure

    """
    current_mapset = gisenv(env)['MAPSET']
    if find_file(name=map, env=env)['mapset'] == current_mapset:
        if overwrite is True:
            historyfile = tempfile(env=env)
            f = open(historyfile, 'w')
            f.write(os.environ['CMDLINE'])
            f.close()
            run_command('r.support', map=map, loadhistory=historyfile, env=env)
            try_remove(historyfile)
        else:
            run_command('r.support', map=map, history=os.environ['CMDLINE'], env=env)
        return True

    warning(_("Unable to write history for <%(map)s>. "
              "Raster map <%(map)s> not found in current mapset." % {'map': map, 'map': map}))
    return False


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
        if s == 'NULL':
            return None
        else:
            return float(s)

    s = read_command('r.info', flags='gre', map=map, env=env)
    kv = parse_key_val(s)
    for k in ['min', 'max']:
        kv[k] = float_or_null(kv[k])
    for k in ['north', 'south', 'east', 'west']:
        kv[k] = float(kv[k])
    for k in ['nsres', 'ewres']:
        kv[k] = float_or_dms(kv[k])
    return kv


def mapcalc(exp, quiet=False, verbose=False, overwrite=False,
            seed=None, env=None, **kwargs):
    """Interface to r.mapcalc.

    :param str exp: expression
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param seed: an integer used to seed the random-number generator for the
                 rand() function, or 'auto' to generate a random seed
    :param dict env: dictionary of environment variables for child process
    :param kwargs:
    """

    if seed == 'auto':
        seed = hash((os.getpid(), time.time())) % (2**32)

    t = string.Template(exp)
    e = t.substitute(**kwargs)

    try:
        write_command('r.mapcalc', file='-', stdin=e, env=env, seed=seed,
                      quiet=quiet, verbose=verbose, overwrite=overwrite)
    except CalledModuleError:
        fatal(_("An error occurred while running r.mapcalc"
                " with expression: %s") % e)


def mapcalc_start(exp, quiet=False, verbose=False, overwrite=False,
                  seed=None, env=None, **kwargs):
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
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param seed: an integer used to seed the random-number generator for the
                 rand() function, or 'auto' to generate a random seed
    :param dict env: dictionary of environment variables for child process
    :param kwargs:

    :return: Popen object
    """

    if seed == 'auto':
        seed = hash((os.getpid(), time.time())) % (2**32)

    t = string.Template(exp)
    e = t.substitute(**kwargs)

    p = feed_command('r.mapcalc', file='-', env=env, seed=seed,
                     quiet=quiet, verbose=verbose, overwrite=overwrite)
    p.stdin.write(encode(e))
    p.stdin.close()
    return p


def raster_what(map, coord, env=None, localized=False):
    """Interface to r.what

    >>> raster_what('elevation', [[640000, 228000]])
    [{'elevation': {'color': '255:214:000', 'label': '', 'value': '102.479'}}]

    :param str map: the map name
    :param list coord: a list of list containing all the point that you want
                       query
    :param env:
    """
    if isinstance(map, (bytes, unicode)):
        map_list = [map]
    else:
        map_list = map

    coord_list = list()
    if isinstance(coord, tuple):
        coord_list.append('%f,%f' % (coord[0], coord[1]))
    else:
        for e, n in coord:
            coord_list.append('%f,%f' % (e, n))

    sep = '|'
    # separator '|' not included in command
    # because | is causing problems on Windows
    # change separator?
    ret = read_command('r.what',
                       flags='rf',
                       map=','.join(map_list),
                       coordinates=','.join(coord_list),
                       null=_("No data"),
                       quiet=True,
                       env=env)
    data = list()
    if not ret:
        return data

    if localized:
        labels = (_("value"), _("label"), _("color"))
    else:
        labels = ('value', 'label', 'color')
    for item in ret.splitlines():
        line = item.split(sep)[3:]
        for i, map_name in enumerate(map_list):
            tmp_dict = {}
            tmp_dict[map_name] = {}
            for j in range(len(labels)):
                tmp_dict[map_name][labels[j]] = line[i*len(labels)+j]

            data.append(tmp_dict)

    return data
