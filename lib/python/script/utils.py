"""
Useful functions to be used in Python scripts.

Usage:

::

    from grass.script import utils as gutils

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
.. sectionauthor:: Anna Petrasova <kratochanna gmail.com>
"""

import os
import shutil
import locale
import re


def float_or_dms(s):
    """Convert DMS to float.

    >>> round(float_or_dms('26:45:30'), 5)
    26.75833
    >>> round(float_or_dms('26:0:0.1'), 5)
    26.00003

    :param s: DMS value

    :return: float value
    """
    return sum(float(x) / 60 ** n for (n, x) in enumerate(s.split(':')))


def separator(sep):
    """Returns separator from G_OPT_F_SEP appropriately converted
    to character.

    >>> separator('pipe')
    '|'
    >>> separator('comma')
    ','

    If the string does not match any of the spearator keywords,
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


def decode(string):
    """Decode string with defualt locale

    :param str string: the string to decode
    """
    enc = locale.getdefaultlocale()[1]
    if enc:
        if hasattr(string, 'decode'):
            return string.decode(enc)

    return string


def encode(string):
    """Encode string with defualt locale

    :param str string: the string to encode
    """
    enc = locale.getdefaultlocale()[1]
    if enc:
        if hasattr(string, 'encode'):
            return string.encode(enc)

    return string


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

    :param number: number to be formated as map suffix
    :param max_number: maximum number of the series to get number of digits

    >>> get_num_suffix(10, 1000)
    '0010'
    >>> get_num_suffix(10, 10)
    '10'
    """
    return '{number:0{width}d}'.format(width=len(str(max_number)),
                                       number=number)

# source:
#    http://stackoverflow.com/questions/4836710/
#    does-python-have-a-built-in-function-for-string-natural-sort/4836734#4836734
def natural_sort(l):
    """Returns sorted strings using natural sort
    """
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
    return sorted(l, key=alphanum_key)
