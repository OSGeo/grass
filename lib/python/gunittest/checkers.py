# -*- coding: utf-8 -*-
"""GRASS Python testing framework checkers

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras, Soeren Gebbert
"""

import os
import sys
import re
import doctest

from grass.script.utils import decode, encode, _get_encoding

try:
    from grass.script.core import KeyValue
except (ImportError, AttributeError):
    # TODO: we are silent about the error and use a object with different
    # interface, should be replaced by central keyvalue module
    # this can happen when translations are not available
    # TODO: grass should survive are give better error when tranlsations are not available
    # even the lazy loading after first _ call would be interesting
    # File "...grass/script/core.py", line 40, in <module>
    # AttributeError: 'NoneType' object has no attribute 'endswith'
    KeyValue = dict

# alternative term to check(er(s)) would be compare


def unify_projection(dic):
    """Unifies names of projections.

    Some projections are referred using different names like
    'Universal Transverse Mercator' and 'Universe Transverse Mercator'.
    This function replaces synonyms by a unified name.

    Example of common typo in UTM replaced by correct spelling::

        >>> unify_projection({'name': ['Universe Transverse Mercator']})
        {'name': ['Universal Transverse Mercator']}

    :param dic: The dictionary containing information about projection

    :return: The dictionary with the new values if needed or a copy of old one
    """
    # the lookup variable is a list of list, each list contains all the
    # possible name for a projection system
    lookup = [['Universal Transverse Mercator',
               'Universe Transverse Mercator']]
    dic = dict(dic)
    for l in lookup:
        for n in range(len(dic['name'])):
            if dic['name'][n] in l:
                dic['name'][n] = l[0]
    return dic


def unify_units(dic):
    """Unifies names of units.

    Some units have different spelling although they are the same units.
    This functions replaces different spelling options by unified one.

    Example of British English spelling replaced by US English spelling::

        >>> unify_units({'units': ['metres'], 'unit': ['metre']})  # doctest: +SKIP
        {'units': ['meters'], 'unit': ['meter']}

    :param dic: The dictionary containing information about units

    :return: The dictionary with the new values if needed or a copy of old one
    """
    # the lookup variable is a list of list, each list contains all the
    # possible name for a units
    lookup = [['meter', 'metre'], ['meters', 'metres'],
              ['Meter', 'Metre'], ['Meters', 'Metres'],
              ['kilometer', 'kilometre'], ['kilometers', 'kilometres'],
              ['Kilometer', 'Kilometre'], ['Kilometers', 'Kilometres'],
              ]
    dic = dict(dic)
    for l in lookup:
        import types
        if not isinstance(dic['unit'], str):
            for n in range(len(dic['unit'])):
                if dic['unit'][n] in l:
                    dic['unit'][n] = l[0]
        else:
            if dic['unit'] in l:
                dic['unit'] = l[0]
        if not isinstance(dic['units'], str):
            for n in range(len(dic['units'])):
                if dic['units'][n] in l:
                    dic['units'][n] = l[0]
        else:
            if dic['units'] in l:
                dic['units'] = l[0]
    return dic


def value_from_string(value):
    """Create value of a most fitting type from a string.

    Type conversions are applied in order ``int``, ``float``, ``string``
    where string is no conversion.

    >>> value_from_string('1')
    1
    >>> value_from_string('5.6')
    5.6
    >>> value_from_string('  5.6\t  ')
    5.6
    >>> value_from_string('hello')
    'hello'
    """
    not_float = False
    not_int = False
    # Convert values into correct types
    # We first try integer then float because
    # int('1.0') is ValueError (although int(1.0) is not)
    # while float('1') is not
    try:
        value_converted = int(value)
    except ValueError:
        not_int = True
    if not_int:
        try:
            value_converted = float(value)
        except ValueError:
            not_float = True
    # strip strings from whitespace (expecting spaces and tabs)
    if not_int and not_float:
        value_converted = value.strip()
    return value_converted


# TODO: what is the default separator?
def text_to_keyvalue(text, sep=":", val_sep=",", functions=None,
                     skip_invalid=False, skip_empty=False,
                     from_string=value_from_string):
    """Convert test to key-value pairs (dictionary-like KeyValue object).

    Converts a key-value text file, where entries are separated
    by newlines and the key and value are separated by `sep`,
    into a key-value dictionary and discovers/uses the correct
    data types (float, int or string) for values.

    Besides key-value pairs it also parses values itself. Value is created
    with the best fitting type using `value_from_string()` function by default.
    When val_sep is present in value part, the resulting value is
    a list of values.

    :param text: string to convert
    :param sep: character that separates the keys and values
    :param val_sep: character that separates the values of a single key
    :param functions: list of functions to apply on the resulting dictionary
    :param skip_invalid: skip all lines which does not contain separator
    :param skip_empty: skip empty lines
    :param from_string: a function used to convert strings to values,
        use ``lambda x: x`` for no conversion

    :return: a dictionary representation of text
    :return type: grass.script.core.KeyValue or dict

    And example of converting text with text, floats, integers and list
    to a dictionary::

        >>> sorted(text_to_keyvalue('''a: Hello
        ... b: 1.0
        ... c: 1,2,3,4,5
        ... d : hello,8,0.1''').items())  # sorted items from the dictionary
        [('a', 'Hello'), ('b', 1.0), ('c', [1, 2, 3, 4, 5]), ('d', ['hello', 8, 0.1])]

    .. warning::
        And empty string is a valid input because empty dictionary is a valid
        dictionary. You need to test this separately according
        to the circumstances.
    """
    # splitting according to universal newlines approach
    # TODO: add also general split with vsep
    text = text.splitlines()
    kvdict = KeyValue()
    functions = [] if functions is None else functions

    for line in text:
        if line.find(sep) >= 0:
            key, value = line.split(sep, 1)
            key = key.strip()
            value = value.strip()
            # this strip may not be necessary, we strip each item in list
            # and also if there is only one value
        else:
            # lines with no separator (empty or invalid)
            if not line:
                if not skip_empty:
                    # TODO: here should go _ for translation
                    # TODO: the error message is not really informative
                    # in case of skipping lines we may get here with no key
                    msg = ("Empty line in the parsed text.")
                    if kvdict:
                        # key is the one from previous line
                        msg = ("Empty line in the parsed text."
                               " Previous line's key is <%s>") % key
                    raise ValueError(msg)
            else:
                # line contains something but not separator
                if not skip_invalid:
                    # TODO: here should go _ for translation
                    raise ValueError(("Line <{l}> does not contain"
                                      " separator <{s}>.").format(l=line, s=sep))
            # if we get here we are silently ignoring the line
            # because it is invalid (does not contain key-value separator) or
            # because it is empty
            continue
        if value.find(val_sep) >= 0:
            # lists
            values = value.split(val_sep)
            value_list = []
            for value in values:
                value_converted = from_string(value)
                value_list.append(value_converted)
            kvdict[key] = value_list
        else:
            # single values
            kvdict[key] = from_string(value)
    for function in functions:
        kvdict = function(kvdict)
    return kvdict


# TODO: decide if there should be some default for precision
# TODO: define standard precisions for DCELL, FCELL, CELL, mm, ft, cm, ...
# TODO: decide if None is valid, and use some default or no compare
# TODO: is None a valid value for precision?
def values_equal(value_a, value_b, precision=0.000001):
    """
    >>> values_equal(1.022, 1.02, precision=0.01)
    True
    >>> values_equal([1.2, 5.3, 6.8], [1.1, 5.2, 6.9], precision=0.2)
    True
    >>> values_equal(7, 5, precision=2)
    True
    >>> values_equal(1, 5.9, precision=10)
    True
    >>> values_equal('Hello', 'hello')
    False
    """
    # each if body needs to handle only not equal state

    if isinstance(value_a, float) and isinstance(value_b, float):
        # both values are float
        # this could be also changed to is None and raise TypeError
        # in Python 2 None is smaller than anything
        # in Python 3 None < 3 raises TypeError
        precision = float(precision)
        if abs(value_a - value_b) > precision:
            return False

    elif (isinstance(value_a, float) and isinstance(value_b, int)) or \
            (isinstance(value_b, float) and isinstance(value_a, int)):
        # on is float the other is int
        # don't accept None
        precision = float(precision)
        # we will apply precision to int-float comparison
        # rather than converting both to integer
        # (as in the original function from grass.script.core)
        if abs(value_a - value_b) > precision:
            return False

    elif isinstance(value_a, int) and isinstance(value_b, int) and \
            precision and int(precision) > 0:
        # both int but precision applies for them
        if abs(value_a - value_b) > precision:
            return False

    elif isinstance(value_a, list) and isinstance(value_b, list):
        if len(value_a) != len(value_b):
            return False
        for i in range(len(value_a)):
            # apply this function for comparison of items in the list
            if not values_equal(value_a[i], value_b[i], precision):
                return False
    else:
        if value_a != value_b:
            return False
    return True


def keyvalue_equals(dict_a, dict_b, precision,
                    def_equal=values_equal, key_equal=None,
                    a_is_subset=False):
    """Compare two dictionaries.

    .. note::
        Always use keyword arguments for all parameters with defaults.
        It is a good idea to use keyword arguments also for the first
        two parameters.

    An example of key-value texts comparison::

        >>> keyvalue_equals(text_to_keyvalue('''a: Hello
        ... b: 1.0
        ... c: 1,2,3,4,5
        ... d: hello,8,0.1'''),
        ... text_to_keyvalue('''a: Hello
        ... b: 1.1
        ... c: 1,22,3,4,5
        ... d: hello,8,0.1'''), precision=0.1)
        False

    :param dict_a: first dictionary
    :param dict_b: second dictionary
    :param precision: precision with which the floating point values
        are compared (passed to equality functions)
    :param callable def_equal: function used for comparison by default
    :param dict key_equal: dictionary of functions used for comparison
        of specific keys, `def_equal` is used for the rest,
        keys in dictionary are keys in `dict_a` and `dict_b` dictionaries,
        values are the functions used to comapare the given key
    :param a_is_subset: `True` if `dict_a` is a subset of `dict_b`,
        `False` otherwise

    :return: `True` if identical, `False` if different

    Use `diff_keyvalue()` to get information about differeces.
    You can use this function to find out if there is a difference and then
    use `diff_keyvalue()` to determine all the differences between
    dictionaries.
    """
    key_equal = {} if key_equal is None else key_equal

    if not a_is_subset and sorted(dict_a.keys()) != sorted(dict_b.keys()):
        return False
    b_keys = dict_b.keys() if a_is_subset else None

    # iterate over subset or just any if not a_is_subset
    # check for missing keys in superset
    # compare matching keys
    for key in dict_a.keys():
        if a_is_subset and key not in b_keys:
            return False
        equal_fun = key_equal.get(key, def_equal)
        if not equal_fun(dict_a[key], dict_b[key], precision):
            return False
    return True


# TODO: should the return depend on the a_is_subset parameter?
# this function must have the same interface and behavior as keyvalue_equals
def diff_keyvalue(dict_a, dict_b, precision,
                  def_equal=values_equal, key_equal=None,
                  a_is_subset=False):
    """Determine the difference of two dictionaries.

    The function returns missing keys and different values for common keys::

        >>> a = {'c': 2, 'b': 3, 'a': 4}
        >>> b = {'c': 1, 'b': 3, 'd': 5}
        >>> diff_keyvalue(a, b, precision=0)
        (['d'], ['a'], [('c', 2, 1)])

    You can provide only a subset of values in dict_a, in this case
    first item in tuple is an emptu list::

        >>> diff_keyvalue(a, b, a_is_subset=True, precision=0)
        ([], ['a'], [('c', 2, 1)])

    This function behaves the same as `keyvalue_equals()`.

    :returns: A tuple of lists, fist is list of missing keys in dict_a,
        second missing keys in dict_b and third is a list of mismatched
        values as tuples (key, value_from_a, value_from_b)
    :rtype: (list, list, list)

    Comparing to the Python ``difflib`` package this function does not create
    any difference output. It just returns the dictionaries.
    Comparing to the Python ``unittest`` ``assertDictEqual()``,
    this function does not issues error or exception, it just determines
    what it the difference.
    """
    key_equal = {} if key_equal is None else key_equal

    a_keys = dict_a.keys()
    b_keys = dict_b.keys()

    missing_in_a = []
    missing_in_b = []
    mismatched = []

    if not a_is_subset:
        for key in b_keys:
            if key not in a_keys:
                missing_in_a.append(key)

    # iterate over a, so we know that it is in a
    for key in a_keys:
        # check if it is in b
        if key not in b_keys:
            missing_in_b.append(key)
        else:
            equal_fun = key_equal.get(key, def_equal)
            if not equal_fun(dict_a[key], dict_b[key], precision):
                mismatched.append((key, dict_a[key], dict_b[key]))

    return sorted(missing_in_a), sorted(missing_in_b), sorted(mismatched)


def proj_info_equals(text_a, text_b):
    """Test if two PROJ_INFO texts are equal."""
    def compare_sums(list_a, list_b, precision):
        """Compare difference of sums of two list using precision"""
        # derived from the code in grass.script.core
        if abs(sum(list_a) - sum(list_b)) > precision:
            return False
    sep = ':'
    val_sep = ','
    key_equal = {'+towgs84': compare_sums}
    dict_a = text_to_keyvalue(text_a, sep=sep, val_sep=val_sep,
                              functions=[unify_projection])
    dict_b = text_to_keyvalue(text_b, sep=sep, val_sep=val_sep,
                              functions=[unify_projection])
    return keyvalue_equals(dict_a, dict_b,
                            precision=0.000001,
                            def_equal=values_equal,
                            key_equal=key_equal)


def proj_units_equals(text_a, text_b):
    """Test if two PROJ_UNITS texts are equal."""
    def lowercase_equals(string_a, string_b, precision=None):
        # we don't need a warning for unused precision
        # pylint: disable=W0613
        """Test equality of two strings ignoring their case using ``lower()``.

        Precision is accepted as require by `keyvalue_equals()` but ignored.
        """
        return string_a.lower() == string_b.lower()
    sep = ':'
    val_sep = ','
    key_equal = {'unit': lowercase_equals, 'units': lowercase_equals}
    dict_a = text_to_keyvalue(text_a, sep=sep, val_sep=val_sep,
                              functions=[unify_units])
    dict_b = text_to_keyvalue(text_b, sep, val_sep,
                              functions=[unify_units])
    return keyvalue_equals(dict_a, dict_b,
                            precision=0.000001,
                            def_equal=values_equal,
                            key_equal=key_equal)


# TODO: support also float (with E, e, inf, nan, ...?) and int (###, ##.)
# http://hg.python.org/cpython/file/943d3e289ab4/Lib/decimal.py#l6098
# perhaps a separate function?
# alternative names: looks like, correspond with/to
# TODO: change checking over lines?
# TODO: change parameter order?
# TODO: the behavior with last \n is strange but now using DOTALL and $
def check_text_ellipsis(reference, actual):
    r"""
    >>> check_text_ellipsis("Vector map <...> contains ... points.",
    ...                     "Vector map <bridges> contains 5268 points.")
    True
    >>> check_text_ellipsis("user: ...\\nname: elevation",
    ...                     "user: some_user\\nname: elevation")
    True
    >>> check_text_ellipsis("user: ...\\nname: elevation",
    ...                     "user: \\nname: elevation")
    False

    The ellipsis is always considered even if it is followed by another
    dots. Consequently, a dot at the end of the sentence with preceding
    ellipsis will work as well as a line filled with undefined number of dots.

    >>> check_text_ellipsis("The result is ....",
    ...                     "The result is 25.")
    True
    >>> check_text_ellipsis("max ..... ...",
    ...                     "max ....... 6")
    True

    However, there is no way how to express that the dot should be in the
    beginning and the ellipsis is at the end of the group of dots.

    >>> check_text_ellipsis("The result is ....",
    ...                     "The result is .25")
    False

    The matching goes over lines (TODO: should this be changed?):
    >>> check_text_ellipsis("a=11\nb=...", "a=11\nb=22\n")
    True

    This function is based on regular expression containing .+ but no other
    regular expression matching will be done.

    >>> check_text_ellipsis("Result: [569] (...)",
    ...                     "Result: 9 (too high)")
    False
    """
    ref_escaped = re.escape(reference)
    exp = re.compile(r'\\\.\\\.\\\.')  # matching escaped ...
    ref_regexp = exp.sub('.+', ref_escaped) + "$"
    if re.match(ref_regexp, actual, re.DOTALL):
        return True
    else:
        return False


def check_text_ellipsis_doctest(reference, actual):
    """
    >>> check_text_ellipsis_doctest("user: ...\\nname: elevation",
    ...                     "user: some_user\\nname: elevation")
    True
    >>> check_text_ellipsis_doctest("user: ...\\nname: elevation",
    ...                     "user: \\nname: elevation")
    True

    This function is using doctest's function to check the result, so we
    will discuss here how the underlying function behaves.

    >>> checker = doctest.OutputChecker()
    >>> checker.check_output("user: some_user\\nname: elevation",
    ...                      "user: some_user\\nname: elevation",
    ...                      optionflags=None)
    True
    >>> checker.check_output("user: user1\\nname: elevation",
    ...                      "user: some_user\\nname: elevation",
    ...                      optionflags=doctest.ELLIPSIS)
    False
    >>> checker.check_output("user: ...\\nname: elevation",
    ...                      "user: some_user\\nname: elevation",
    ...                      optionflags=doctest.ELLIPSIS)
    True

    The ellipsis matches also an empty string, so the following matches:

    >>> checker.check_output("user: ...\\nname: elevation",
    ...                      "user: \\nname: elevation",
    ...                      optionflags=doctest.ELLIPSIS)
    True

    It is robust concerning misspelled matching string but does not allow
    ellipsis followed by a dot, e.g. at the end of the sentence:

    >>> checker.check_output("user: ....\\nname: elevation",
    ...                      "user: some_user\\nname: elevation",
    ...                      optionflags=doctest.ELLIPSIS)
    False
    """
    # this can be also global
    checker = doctest.OutputChecker()
    return checker.check_output(reference, actual,
                                optionflags=doctest.ELLIPSIS)


import hashlib

# optimal size depends on file system and maybe on hasher.block_size
_BUFFER_SIZE = 2**16


# TODO: accept also open file object
def file_md5(filename):
    """Get MD5 (check) sum of a file."""
    hasher = hashlib.md5()
    with open(filename, 'rb') as f:
        buf = f.read(_BUFFER_SIZE)
        while len(buf) > 0:
            hasher.update(buf)
            buf = f.read(_BUFFER_SIZE)
    return hasher.hexdigest()


def text_file_md5(filename, exclude_lines=None, exclude_re=None,
                  prepend_lines=None, append_lines=None):
    """Get a MD5 (check) sum of a text file.

    Works in the same way as `file_md5()` function but ignores newlines
    characters and excludes lines from the file as well as prepend or
    append them if requested.

    :param exclude_lines: list of strings to be excluded
        (newline characters should not be part of the strings)
    :param exclude_re: regular expression string;
        lines matching this regular expression will not be considered
    :param prepend_lines: list of lines to be prepended to the file
        before computing the sum
    :param append_lines: list of lines  to be appended to the file
        before computing the sum
    """
    hasher = hashlib.md5()
    if exclude_re:
        regexp = re.compile(exclude_re)
    if prepend_lines:
        for line in prepend_lines:
            hasher.update(line if sys.version_info[0] == 2 else encode(line))
    with open(filename, 'r') as f:
        for line in f:
            # replace platform newlines by standard newline
            if os.linesep != '\n':
                line = line.rstrip(os.linesep) + '\n'
            if exclude_lines and line in exclude_lines:
                continue
            if exclude_re and regexp.match(line):
                continue
            hasher.update(line if sys.version_info[0] == 2 else encode(line))
    if append_lines:
        for line in append_lines:
            hasher.update(line if sys.version_info[0] == 2 else encode(line))
    return hasher.hexdigest()


def files_equal_md5(filename_a, filename_b):
    """Check equality of two files according to their MD5 sums"""
    return file_md5(filename_a) == file_md5(filename_b)


def main():  # pragma: no cover
    """Run the doctest"""
    ret = doctest.testmod()
    return ret.failed


if __name__ == '__main__':  # pragma: no cover
    sys.exit(main())
