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

from __future__ import annotations

import os
import shutil
import locale
import shlex
import re
import time
import platform
import uuid
import random
import string

from pathlib import Path
from typing import TYPE_CHECKING, AnyStr, Callable, TypeVar, cast, overload


if TYPE_CHECKING:
    from _typeshed import FileDescriptorOrPath, StrOrBytesPath, StrPath


# Type variables
T = TypeVar("T")
VT = TypeVar("VT")  # Value type


def float_or_dms(s) -> float:
    """Convert DMS to float.

    >>> round(float_or_dms("26:45:30"), 5)
    26.75833
    >>> round(float_or_dms("26:0:0.1"), 5)
    26.00003

    :param s: DMS value

    :return: float value
    """
    if s[-1] in {"E", "W", "N", "S"}:
        s = s[:-1]
    return sum(float(x) / 60**n for (n, x) in enumerate(s.split(":")))


def separator(sep: str) -> str:
    """Returns separator from G_OPT_F_SEP appropriately converted
    to character.

    >>> separator("pipe")
    '|'
    >>> separator("comma")
    ','

    If the string does not match any of the separator keywords,
    it is returned as is:

    >>> separator(", ")
    ', '

    :param str separator: character or separator keyword

    :return: separator character
    """
    if sep == "pipe":
        return "|"
    if sep == "comma":
        return ","
    if sep == "space":
        return " "
    if sep in {"tab", "\\t"}:
        return "\t"
    if sep in {"newline", "\\n"}:
        return "\n"
    return sep


def diff_files(
    filename_a: FileDescriptorOrPath, filename_b: FileDescriptorOrPath
) -> list[str]:
    """Diffs two text files and returns difference.

    :param str filename_a: first file path
    :param str filename_b: second file path

    :return: list of strings
    """
    import difflib

    differ = difflib.Differ()
    with open(filename_a) as fh_a, open(filename_b) as fh_b:
        return list(differ.compare(fh_a.readlines(), fh_b.readlines()))


def try_remove(path: StrOrBytesPath) -> None:
    """Attempt to remove a file; no exception is generated if the
    attempt fails.

    :param str path: path to file to remove
    """
    try:
        os.remove(path)
    except Exception:
        pass


def try_rmdir(path: StrOrBytesPath) -> None:
    """Attempt to remove a directory; no exception is generated if the
    attempt fails.

    :param str path: path to directory to remove
    """
    try:
        os.rmdir(path)
    except Exception:
        shutil.rmtree(path, ignore_errors=True)


def basename(path: StrPath, ext: str | None = None) -> str:
    """Remove leading directory components and an optional extension
    from the specified path

    :param str path: path
    :param str ext: extension
    """
    name: str = os.path.basename(path)
    if not ext:
        return name
    fs: list[str] = name.rsplit(".", 1)
    if len(fs) > 1 and fs[1].lower() == ext:
        name = fs[0]
    return name


class KeyValue(dict[str, VT]):
    """A general-purpose key-value store.

    KeyValue is a subclass of dict, but also allows entries to be read and
    written using attribute syntax.

    :Example:
      .. code-block:: pycon

        >>> reg = KeyValue()
        >>> reg["north"] = 489
        >>> reg.north
        489
        >>> reg.south = 205
        >>> reg["south"]
        205

    The keys of KeyValue are strings. To use other key types, use other mapping types.
    To use the attribute syntax, the keys must be valid Python attribute names.
    """

    def __getattr__(self, key: str) -> VT:
        return self[key]

    def __setattr__(self, key: str, value: VT) -> None:
        self[key] = value


def _get_encoding() -> str:
    try:
        # Python >= 3.11
        encoding = locale.getencoding()
    except AttributeError:
        encoding = locale.getdefaultlocale()[1]
    if not encoding:
        encoding = "UTF-8"
    return encoding


def decode(bytes_: AnyStr, encoding: str | None = None) -> str:
    """Decode bytes with default locale and return (unicode) string

    No-op if parameter is not bytes (assumed unicode string).

    :param bytes\\_: the bytes to decode
    :param encoding: encoding to be used, default value is None

    Example
    -------

    >>> decode(b"S\xc3\xbcdtirol")
    u'Südtirol'
    >>> decode("Südtirol")
    u'Südtirol'
    >>> decode(1234)
    u'1234'
    """
    if isinstance(bytes_, str):
        return bytes_
    if isinstance(bytes_, bytes):
        enc = _get_encoding() if encoding is None else encoding
        return bytes_.decode(enc)
    # only text should be used
    msg = f"can only accept types str and bytes, not {type(bytes_).__name__}"
    raise TypeError(msg)


def encode(string: AnyStr, encoding: str | None = None) -> bytes:
    """Encode string with default locale and return bytes with that encoding

    No-op if parameter is bytes (assumed already encoded).
    This ensures garbage in, garbage out.

    :param string: the string to encode
    :param encoding: encoding to be used, default value is None

    Example
    -------

    >>> encode(b"S\xc3\xbcdtirol")
    b'S\xc3\xbcdtirol'
    >>> decode("Südtirol")
    b'S\xc3\xbcdtirol'
    >>> decode(1234)
    b'1234'
    """
    if isinstance(string, bytes):
        return string
    if isinstance(string, str):
        enc = _get_encoding() if encoding is None else encoding
        return string.encode(enc)
    # if something else than text
    msg = "Can only accept types str and bytes"
    raise TypeError(msg)


def text_to_string(text: AnyStr, encoding: str | None = None) -> str:
    """Convert text to str. Useful when passing text into environments,
    in Python 2 it needs to be bytes on Windows, in Python 3 in needs unicode.
    """
    return decode(text, encoding=encoding)


@overload
def parse_key_val(
    s: AnyStr,
    sep: str = "=",
    dflt: T | None = None,
    val_type: None = ...,
    vsep: str | None = None,
) -> KeyValue[str | T | None]:
    pass


@overload
def parse_key_val(
    s: AnyStr,
    sep: str = "=",
    dflt: T | None = None,
    val_type: Callable[[str], T] = ...,
    vsep: str | None = None,
) -> KeyValue[T | None]:
    pass


@overload
def parse_key_val(
    s: AnyStr,
    sep: str = "=",
    dflt: T | None = None,
    val_type: Callable[[str], T] | None = None,
    vsep: str | None = None,
) -> KeyValue[str | T] | KeyValue[T | None] | KeyValue[T] | KeyValue[str | T | None]:
    pass


def parse_key_val(
    s: AnyStr,
    sep: str = "=",
    dflt: T | None = None,
    val_type: Callable[[str], T] | None = None,
    vsep: str | None = None,
) -> KeyValue[str | T] | KeyValue[T | None] | KeyValue[T] | KeyValue[str | T | None]:
    """Parse a string into a dictionary, where entries are separated
    by newlines and the key and value are separated by `sep` (default: `=`)

    >>> parse_key_val("min=20\\nmax=50") == {"min": "20", "max": "50"}
    True
    >>> parse_key_val("min=20\\nmax=50", val_type=float) == {"min": 20, "max": 50}
    True

    :param s: string to be parsed
    :param sep: key/value separator
    :param dflt: default value to be used
    :param val_type: value type (None for no cast)
    :param vsep: vertical separator (default is Python 'universal newlines' approach)

    :return: parsed input (dictionary of keys/values)
    """

    result: (
        KeyValue[str | T] | KeyValue[T | None] | KeyValue[T] | KeyValue[str | T | None]
    ) = KeyValue()

    if not s:
        return result

    if isinstance(s, bytes):
        sep = encode(sep)
        vsep = encode(vsep) if vsep else vsep

    if vsep:
        lines: list[bytes] | list[str] = s.split(vsep)
        try:
            lines.remove("\n")
        except ValueError:
            pass
    else:
        lines = s.splitlines()

    if callable(val_type):
        result = cast("KeyValue[T | None]", result)
        for line in lines:
            kv: list[bytes] | list[str] = line.split(sep, 1)
            k: str = decode(kv[0].strip())
            result[k] = val_type(decode(kv[1].strip())) if len(kv) > 1 else dflt

        if dflt is not None:
            result = cast("KeyValue[T]", result)
        return result

    result = cast("KeyValue[str | T | None]", result)
    for line in lines:
        kv = line.split(sep, 1)
        k = decode(kv[0].strip())
        result[k] = decode(kv[1].strip()) if len(kv) > 1 else dflt

    if dflt is not None:
        result = cast("KeyValue[str | T]", result)

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
    return "{number:0{width}d}".format(width=len(str(max_number)), number=number)


def split(s):
    """Same shlex.split() func on all OS platforms

    We don't use parameter posix=True on the OS MS Windows due to incorrectly
    splitting command line parameters:

    e.g. d.vect where="cat < 10"

    is split incorrectly as follows:

    'where="cat', '<', '10"'

    Should be:

    'where=cat < 10'


    :param str s: cmd string

    return list: cmd list
    """
    return shlex.split(s)


# source:
#    https://stackoverflow.com/questions/4836710/
#    does-python-have-a-built-in-function-for-string-natural-sort/4836734#4836734
def natural_sort(items):
    """Returns sorted list using natural sort
    (deprecated, use naturally_sorted)
    """
    return naturally_sorted(items)


def naturally_sorted(items, key=None):
    """Returns sorted list using natural sort"""
    copy_items = items[:]
    naturally_sort(copy_items, key)
    return copy_items


def naturally_sort(items, key=None):
    """Sorts lists using natural sort"""

    def convert(text):
        return int(text) if text.isdigit() else text.lower()

    def alphanum_key(actual_key):
        sort_key = key(actual_key) if key else actual_key
        return [convert(c) for c in re.split(r"([0-9]+)", sort_key)]

    items.sort(key=alphanum_key)


def get_lib_path(modname, libname=None):
    """Return the path of the libname contained in the module."""
    from os import getenv
    from os.path import isdir, join, sep

    if isdir(join(getenv("GISBASE"), "etc", modname)):
        path = join(os.getenv("GISBASE"), "etc", modname)
    elif (
        getenv("GRASS_ADDON_BASE")
        and libname
        and isdir(join(getenv("GRASS_ADDON_BASE"), "etc", modname, libname))
    ) or (
        getenv("GRASS_ADDON_BASE")
        and isdir(join(getenv("GRASS_ADDON_BASE"), "etc", modname))
    ):
        path = join(getenv("GRASS_ADDON_BASE"), "etc", modname)
    elif getenv("GRASS_ADDON_BASE") and isdir(
        join(getenv("GRASS_ADDON_BASE"), modname, modname)
    ):
        path = join(os.getenv("GRASS_ADDON_BASE"), modname, modname)
    else:
        # used by g.extension compilation process
        cwd = str(Path.cwd())
        idx = cwd.find(modname)
        if idx < 0:
            return None
        path = "{cwd}{sep}etc{sep}{modname}".format(
            cwd=cwd[: idx + len(modname)], sep=sep, modname=modname
        )
        if libname:
            path += "{pathsep}{cwd}{sep}etc{sep}{modname}{sep}{libname}".format(
                cwd=cwd[: idx + len(modname)],
                sep=sep,
                modname=modname,
                libname=libname,
                pathsep=os.pathsep,
            )

    return path


def set_path(modulename, dirname=None, path="."):
    """Set sys.path looking in the local directory GRASS directories.

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

        set_path("r.green", "libhydro", "..")
        set_path("r.green", "libgreen", os.path.join("..", ".."))

    when we are executing the module: r.green.hydro.financial locally from
    the command line:  ::

        grass_prompt> python r.green.hydro.financial.py --ui

    In this way we are executing the local code even if the module was already
    installed as grass-addons and it is available in GRASS standards path.

    The function is checking if the dirname is provided and if the
    directory exists and it is available using the path
    provided as third parameter, if yes add the path to sys.path to be
    importable, otherwise it will check on GRASS GIS standard paths.

    """
    import sys

    # TODO: why dirname is checked first - the logic should be revised
    pathlib_ = None
    if dirname:
        pathlib_ = os.path.join(path, dirname)
    if pathlib_ and os.path.exists(pathlib_):
        # we are running the script from the script directory, therefore
        # we add the path to sys.path to reach the directory (dirname)
        sys.path.append(os.path.abspath(path))
    else:
        # running from GRASS GIS session
        path = get_lib_path(modulename, dirname)
        if path is None:
            pathname = os.path.join(modulename, dirname) if dirname else modulename
            raise ImportError(
                "Not able to find the path '%s' directory "
                "(current dir '%s')." % (pathname, Path.cwd())
            )

        sys.path.insert(0, path)


def clock():
    """
    Return time counter to measure performance for chunks of code.
    Should be used only as difference between the calls.
    """
    return time.perf_counter()


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
        msg = "name cannot be empty"
        raise ValueError(msg)
    if fallback_prefix and re.match(r"[^A-Za-z]", fallback_prefix[0]):
        msg = "fallback_prefix must start with an ASCII letter"
        raise ValueError(msg)
    if fallback_prefix and re.match(r"[^A-Za-z]", name[0], flags=re.ASCII):
        # We prefix here rather than just replace, because in cases of unique
        # identifiers, e.g., columns or node names, replacing the first
        # character by the same replacement character increases chances of
        # conflict (e.g. column names 10, 20, 30).
        name = "{fallback_prefix}{name}".format(**locals())
    name = re.sub(r"[^A-Za-z0-9_]", "_", name, flags=re.ASCII)
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

    .. note::

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

    .. note::

        See the note about creating temporary files in the
        :func:`append_node_pid()` description.
    """
    suffix = uuid.uuid4().hex
    return "{name}_{suffix}".format(**locals())


def append_random(name, suffix_length=None, total_length=None):
    """Add a random part to of a specified length to a name (string)

    >>> append_random("tmp", 8)
    >>> append_random("tmp", total_length=16)

    .. note::

        Note that this will be influenced by the random seed set for the Python
        random package.

    .. note::

        See the note about creating temporary files in the
        :func:`append_node_pid()` description.
    """
    if suffix_length and total_length:
        msg = "Either suffix_length or total_length can be provided, not both"
        raise ValueError(msg)
    if not suffix_length and not total_length:
        msg = "suffix_length or total_length has to be provided"
        raise ValueError(msg)
    if total_length:
        # remove len of name and one underscore
        name_length = len(name)
        suffix_length = total_length - name_length - 1
        if suffix_length <= 0:
            msg = (
                "No characters left for the suffix:"
                " total_length <{total_length}> is too small"
                " or name <{name}> ({name_length}) is too long".format(**locals())
            )
            raise ValueError(msg)
    # We don't do lower and upper case because that could cause conflicts in
    # contexts which are case-insensitive.
    # We use lowercase because that's what is in UUID4 hex string.
    allowed_chars = string.ascii_lowercase + string.digits
    # The following can be shorter with random.choices from Python 3.6.
    suffix = "".join(random.choice(allowed_chars) for _ in range(suffix_length))
    return "{name}_{suffix}".format(**locals())
