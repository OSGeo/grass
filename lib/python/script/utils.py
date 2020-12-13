# -*- coding: utf-8 -*-
"""
Useful functions to be used in Python scripts.

Usage:

::

    from grass.script import utils as gutils

(C) 2014-2016 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
.. sectionauthor:: Anna Petrasova <kratochanna gmail.com>
"""

import os
import sys
import shutil
import locale
import shlex
import re
import time
import platform
import uuid
import random
import string


if sys.version_info.major >= 3:
    unicode = str


def float_or_dms(s):
    """Convert DMS to float.

    >>> round(float_or_dms('26:45:30'), 5)
    26.75833
    >>> round(float_or_dms('26:0:0.1'), 5)
    26.00003

    :param s: DMS value

    :return: float value
    """
    if s[-1] in ['E', 'W', 'N', 'S']:
        s = s[:-1]
    return sum(float(x) / 60 ** n for (n, x) in enumerate(s.split(':')))


def separator(sep):
    """Returns separator from G_OPT_F_SEP appropriately converted
    to character.

    >>> separator('pipe')
    '|'
    >>> separator('comma')
    ','

    If the string does not match any of the separator keywords,
    it is returned as is:

    >>> separator(', ')
    ', '

    :param str separator: character or separator keyword

    :return: separator character
    """
    if sep == "pipe":
        return "|"
    elif sep == "comma":
        return ","
    elif sep == "space":
        return " "
    elif sep == "tab" or sep == "\\t":
        return "\t"
    elif sep == "newline" or sep == "\\n":
        return "\n"
    return sep


def diff_files(filename_a, filename_b):
    """Diffs two text files and returns difference.

    :param str filename_a: first file path
    :param str filename_b: second file path

    :return: list of strings
    """
    import difflib
    differ = difflib.Differ()
    fh_a = open(filename_a, 'r')
    fh_b = open(filename_b, 'r')
    result = list(differ.compare(fh_a.readlines(),
                                 fh_b.readlines()))
    return result


def try_remove(path):
    """Attempt to remove a file; no exception is generated if the
    attempt fails.

    :param str path: path to file to remove
    """
    try:
        os.remove(path)
    except:
        pass


def try_rmdir(path):
    """Attempt to remove a directory; no exception is generated if the
    attempt fails.

    :param str path: path to directory to remove
    """
    try:
        os.rmdir(path)
    except:
        shutil.rmtree(path, ignore_errors=True)


def basename(path, ext=None):
    """Remove leading directory components and an optional extension
    from the specified path

    :param str path: path
    :param str ext: extension
    """
    name = os.path.basename(path)
    if not ext:
        return name
    fs = name.rsplit('.', 1)
    if len(fs) > 1 and fs[1].lower() == ext:
        name = fs[0]
    return name


class KeyValue(dict):
    """A general-purpose key-value store.

    KeyValue is a subclass of dict, but also allows entries to be read and
    written using attribute syntax. Example:

    >>> reg = KeyValue()
    >>> reg['north'] = 489
    >>> reg.north
    489
    >>> reg.south = 205
    >>> reg['south']
    205
    """

    def __getattr__(self, key):
        return self[key]

    def __setattr__(self, key, value):
        self[key] = value


def _get_encoding():
    encoding = locale.getdefaultlocale()[1]
    if not encoding:
        encoding = 'UTF-8'
    return encoding


def decode(bytes_, encoding=None):
    """Decode bytes with default locale and return (unicode) string

    No-op if parameter is not bytes (assumed unicode string).

    :param bytes bytes_: the bytes to decode
    :param encoding: encoding to be used, default value is None

    Example
    -------

    >>> decode(b'S\xc3\xbcdtirol')
    u'Südtirol'
    >>> decode(u'Südtirol')
    u'Südtirol'
    >>> decode(1234)
    u'1234'
    """
    if isinstance(bytes_, unicode):
        return bytes_
    if isinstance(bytes_, bytes):
        if encoding is None:
            enc = _get_encoding()
        else:
            enc = encoding
        return bytes_.decode(enc)
    # if something else than text
    if sys.version_info.major >= 3:
        # only text should be used
        raise TypeError("can only accept types str and bytes")
    else:
        # for backwards compatibility
        return unicode(bytes_)


def encode(string, encoding=None):
    """Encode string with default locale and return bytes with that encoding

    No-op if parameter is bytes (assumed already encoded).
    This ensures garbage in, garbage out.

    :param str string: the string to encode
    :param encoding: encoding to be used, default value is None

    Example
    -------

    >>> encode(b'S\xc3\xbcdtirol')
    b'S\xc3\xbcdtirol'
    >>> decode(u'Südtirol')
    b'S\xc3\xbcdtirol'
    >>> decode(1234)
    b'1234'
    """
    if isinstance(string, bytes):
        return string
    # this also tests str in Py3:
    if isinstance(string, unicode):
        if encoding is None:
            enc = _get_encoding()
        else:
            enc = encoding
        return string.encode(enc)
    # if something else than text
    if sys.version_info.major >= 3:
        # only text should be used
        raise TypeError("can only accept types str and bytes")
    else:
        # for backwards compatibility
        return bytes(string)


def text_to_string(text, encoding=None):
    """Convert text to str. Useful when passing text into environments,
       in Python 2 it needs to be bytes on Windows, in Python 3 in needs unicode.
    """
    if sys.version[0] == '2':
        # Python 2
        return encode(text, encoding=encoding)
    else:
        # Python 3
        return decode(text, encoding=encoding)


def parse_key_val(s, sep='=', dflt=None, val_type=None, vsep=None):
    """Parse a string into a dictionary, where entries are separated
    by newlines and the key and value are separated by `sep` (default: `=`)

    >>> parse_key_val('min=20\\nmax=50') == {'min': '20', 'max': '50'}
    True
    >>> parse_key_val('min=20\\nmax=50',
    ...     val_type=float) == {'min': 20, 'max': 50}
    True

    :param str s: string to be parsed
    :param str sep: key/value separator
    :param dflt: default value to be used
    :param val_type: value type (None for no cast)
    :param vsep: vertical separator (default is Python 'universal newlines' approach)

    :return: parsed input (dictionary of keys/values)
    """
    result = KeyValue()

    if not s:
        return result

    if isinstance(s, bytes):
        sep = encode(sep)
        vsep = encode(vsep) if vsep else vsep

    if vsep:
        lines = s.split(vsep)
        try:
            lines.remove('\n')
        except ValueError:
            pass
    else:
        lines = s.splitlines()

    for line in lines:
        kv = line.split(sep, 1)
        k = decode(kv[0].strip())
        if len(kv) > 1:
            v = decode(kv[1].strip())
        else:
            v = dflt

        if val_type:
            result[k] = val_type(v)
        else:
            result[k] = v

    return result


def get_num_suffix(number, max_number):
    """Returns formatted number with number of padding zeros
    depending on maximum number, used for creating suffix for data series.
    Does not include the suffix separator.

    :param number: number to be formatted as map suffix
    :param max_number: maximum number of the series to get number of digits

    >>> get_num_suffix(10, 1000)
    '0010'
    >>> get_num_suffix(10, 10)
    '10'
    """
    return '{number:0{width}d}'.format(width=len(str(max_number)),
                                       number=number)

def split(s):
    """!Platform specific shlex.split"""
    if sys.version_info >= (2, 6):
        return shlex.split(s, posix = (sys.platform != "win32"))
    elif sys.platform == "win32":
        return shlex.split(s.replace('\\', r'\\'))
    else:
        return shlex.split(s)


# source:
#    http://stackoverflow.com/questions/4836710/
#    does-python-have-a-built-in-function-for-string-natural-sort/4836734#4836734
def natural_sort(l):
    """Returns sorted list using natural sort
    (deprecated, use naturally_sorted)
    """
    return naturally_sorted(l)


def naturally_sorted(l, key=None):
    """Returns sorted list using natural sort
    """
    copy_l = l[:]
    naturally_sort(copy_l, key)
    return copy_l


def naturally_sort(l, key=None):
    """Sorts lists using natural sort
    """

    def convert(text):
        return int(text) if text.isdigit() else text.lower()

    def alphanum_key(actual_key):
        if key:
            sort_key = key(actual_key)
        else:
            sort_key = actual_key
        return [convert(c) for c in re.split('([0-9]+)', sort_key)]

    l.sort(key=alphanum_key)


def get_lib_path(modname, libname=None):
    """Return the path of the libname contained in the module.
    """
    from os.path import isdir, join, sep
    from os import getenv

    if isdir(join(getenv('GISBASE'), 'etc', modname)):
        path = join(os.getenv('GISBASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and libname and \
            isdir(join(getenv('GRASS_ADDON_BASE'), 'etc', modname, libname)):
        path = join(getenv('GRASS_ADDON_BASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), 'etc', modname)):
        path = join(getenv('GRASS_ADDON_BASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), modname, modname)):
        path = join(os.getenv('GRASS_ADDON_BASE'), modname, modname)
    else:
        # used by g.extension compilation process
        cwd = os.getcwd()
        idx = cwd.find(modname)
        if idx < 0:
            return None
        path = '{cwd}{sep}etc{sep}{modname}'.format(cwd=cwd[:idx+len(modname)],
                                                    sep=sep,
                                                    modname=modname)
        if libname:
            path += '{pathsep}{cwd}{sep}etc{sep}{modname}{sep}{libname}'.format(
                cwd=cwd[:idx+len(modname)],
                sep=sep,
                modname=modname, libname=libname,
                pathsep=os.pathsep
            )

    return path


def set_path(modulename, dirname=None, path='.'):
    """Set sys.path looking in the the local directory GRASS directories.

    :param modulename: string with the name of the GRASS module
    :param dirname: string with the directory name containing the python
                    libraries, default None
    :param path: string with the path to reach the dirname locally.

    Example
    --------

    "set_path" example working locally with the source code of a module
    (r.green) calling the function with all the parameters. Below it is
    reported the directory structure on the r.green module.

    ::

        grass_prompt> pwd
        ~/Download/r.green/r.green.hydro/r.green.hydro.financial

        grass_prompt> tree ../../../r.green
        ../../../r.green
        |-- ...
        |-- libgreen
        |   |-- pyfile1.py
        |   +-- pyfile2.py
        +-- r.green.hydro
           |-- Makefile
           |-- libhydro
           |   |-- pyfile1.py
           |   +-- pyfile2.py
           |-- r.green.hydro.*
           +-- r.green.hydro.financial
               |-- Makefile
               |-- ...
               +-- r.green.hydro.financial.py

        21 directories, 125 files

    in the source code the function is called with the following parameters: ::

        set_path('r.green', 'libhydro', '..')
        set_path('r.green', 'libgreen', os.path.join('..', '..'))

    when we are executing the module: r.green.hydro.financial locally from
    the command line:  ::

        grass_prompt> python r.green.hydro.financial.py --ui

    In this way we are executing the local code even if the module was already
    installed as grass-addons and it is available in GRASS standards path.

    The function is cheching if the dirname is provided and if the
    directory exists and it is available using the path
    provided as third parameter, if yes add the path to sys.path to be
    importable, otherwise it will check on GRASS GIS standard paths.

    """
    import sys
    # TODO: why dirname is checked first - the logic should be revised
    pathlib = None
    if dirname:
        pathlib = os.path.join(path, dirname)
    if pathlib and os.path.exists(pathlib):
        # we are running the script from the script directory, therefore
        # we add the path to sys.path to reach the directory (dirname)
        sys.path.append(os.path.abspath(path))
    else:
        # running from GRASS GIS session
        path = get_lib_path(modulename, dirname)
        if path is None:
            pathname = os.path.join(modulename, dirname) if dirname else modulename
            raise ImportError("Not able to find the path '%s' directory "
                              "(current dir '%s')." % (pathname, os.getcwd()))

        sys.path.insert(0, path)

def clock():
    """
    Return time counter to measure performance for chunks of code.
    Uses time.clock() for Py < 3.3, time.perf_counter() for Py >= 3.3.
    Should be used only as difference between the calls.
    """
    if sys.version_info > (3,2):
        return time.perf_counter()
    return time.clock()


def legalize_vector_name(name, fallback_prefix="x"):
    """Make *name* usable for vectors, tables, and columns

    The returned string is a name usable for vectors, tables, and columns,
    i.e., it is a vector legal name which is a string containing only
    lowercase and uppercase ASCII letters, digits, and underscores.

    Invalid characters are replaced by underscores.
    If the name starts with an invalid character, the name is prefixed with
    *fallback_prefix*. This increases the length of the resulting name by the
    length of the prefix.

    The *fallback_prefix* can be empty which is useful when the *name* is later
    used as a suffix for some other valid name.

    ValueError is raised when provided *name* is empty or *fallback_prefix*
    does not start with a valid character.
    """
    # The implementation is based on Vect_legal_filename().
    if not name:
        raise ValueError("name cannot be empty")
    if fallback_prefix and re.match("[^A-Za-z]", fallback_prefix[0]):
        raise ValueError("fallback_prefix must start with an ASCII letter")
    if fallback_prefix and re.match("[^A-Za-z]", name[0], flags=re.ASCII):
        # We prefix here rather than just replace, because in cases of unique
        # identifiers, e.g., columns or node names, replacing the first
        # character by the same replacement character increases chances of
        # conflict (e.g. column names 10, 20, 30).
        name = "{fallback_prefix}{name}".format(**locals())
    name = re.sub("[^A-Za-z0-9_]", "_", name, flags=re.ASCII)
    keywords = ["and", "or", "not"]
    if name in keywords:
        name = "{name}_".format(**locals())
    return name


def append_node_pid(name):
    """Add node name and PID to a name (string)

    For the result to be unique, the name needs to be unique within a process.
    Given that, the result will be unique enough for use in temporary maps
    and other elements on single machine or an HPC cluster.

    The returned string is a name usable for vectors, tables, and columns
    (vector legal name) as long as provided argument *name* is.

    >>> append_node_pid("tmp_raster_1")

    ..note::

        Before you use this function for creating temporary files (i.e., normal
        files on disk, not maps and other mapset elements), see functions
        designed for it in the GRASS GIS or standard Python library. These
        take care of collisions already on different levels.
    """
    # We are using this node as a suffix, so we don't need to make sure it
    # is prefixed with additional character(s) since that's exactly what
    # happens in this function.
    # Note that this may still cause collisions when nodes are named in a way
    # that they collapse into the same name after the replacements are done,
    # but we consider that unlikely given that
    # nodes will be likely already named as something close to what we need.
    node = legalize_vector_name(platform.node(), fallback_prefix="")
    pid = os.getpid()
    return "{name}_{node}_{pid}".format(**locals())


def append_uuid(name):
    """Add UUID4 to a name (string)

    To generate a name of an temporary mapset element which is unique in a
    system, use :func:`append_node_pid()` in a combination with a name unique
    within your process.

    To avoid collisions, never shorten the name obtained from this function.
    A shortened UUID does not have the collision guarantees the full UUID has.

    For a random name of a given shorter size, see :func:`append_random()`.

    >>> append_uuid("tmp")

    ..note::

        See the note about creating temporary files in the
        :func:`append_node_pid()` description.
    """
    suffix = uuid.uuid4().hex
    return "{name}_{suffix}".format(**locals())


def append_random(name, suffix_length=None, total_length=None):
    """Add a random part to of a specified length to a name (string)

    >>> append_random("tmp", 8)
    >>> append_random("tmp", total_length=16)

    ..note::

        Note that this will be influeced by the random seed set for the Python
        random package.

    ..note::

        See the note about creating temporary files in the
        :func:`append_node_pid()` description.
    """
    if suffix_length and total_length:
        raise ValueError(
            "Either suffix_length or total_length can be provided, not both"
        )
    if not suffix_length and not total_length:
        raise ValueError(
            "suffix_length or total_length has to be provided"
        )
    if total_length:
        # remove len of name and one underscore
        name_length = len(name)
        suffix_length = total_length - name_length - 1
        if suffix_length <= 0:
            raise ValueError(
                "No characters left for the suffix:"
                " total_length <{total_length}> is too small"
                " or name <{name}> ({name_length}) is too long".format(
                    **locals()
                )
            )
    # We don't do lower and upper case because that could cause conflicts in
    # contexts which are case-insensitive.
    # We use lowercase because that's what is in UUID4 hex string.
    allowed_chars = string.ascii_lowercase + string.digits
    # The following can be shorter with random.choices from Python 3.6.
    suffix = ''.join(random.choice(allowed_chars) for _ in range(suffix_length))
    return "{name}_{suffix}".format(**locals())
