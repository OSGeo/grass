"""Provides functions for the main GRASS GIS executable

(C) 2024 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>

This is not a stable part of the API. Use at your own risk.
"""


def decode(bytes_, *, encoding):
    """Decode bytes with default locale and return (unicode) string
    Adapted from grass.script.core.utils.

    No-op if parameter is not bytes (assumed unicode string).

    :param bytes bytes_: the bytes to decode
    :param encoding: encoding to be used, default value is the system's default
        encoding or, if that cannot be determined, 'UTF-8'.
    """
    if isinstance(bytes_, str):
        return bytes_
    elif isinstance(bytes_, bytes):
        return bytes_.decode(encoding)
    else:
        # if something else than text
        raise TypeError("can only accept types str and bytes")


def encode(string, *, encoding):
    """Encode string with default locale and return bytes with that encoding
    Adapted from grass.script.core.utils.

    No-op if parameter is bytes (assumed already encoded).
    This ensures garbage in, garbage out.

    :param str string: the string to encode
    :param encoding: encoding to be used, default value is the system's default
        encoding or, if that cannot be determined, 'UTF-8'.
    """
    if isinstance(string, bytes):
        return string
    elif isinstance(string, str):
        return string.encode(encoding)
    else:
        # if something else than text
        raise TypeError("can only accept types str and bytes")


# see https://trac.osgeo.org/grass/ticket/3508
def to_text_string(obj, *, encoding):
    """Convert `obj` to (unicode) text string"""
    return decode(obj, encoding=encoding)
