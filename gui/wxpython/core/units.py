"""
@package core.units

@brief Units management

.. todo::
    Probably will be replaced by Python ctypes fns in the near future(?)

Usage:

    from core.units import Units

Classes:
 - units::BaseUnits

(C) 2009, 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import six
import math

if __name__ == "__main__":
    import sys


class BaseUnits:
    def __init__(self):
        self._units = dict()
        self._units["length"] = {
            0: {"key": "mu", "label": _("map units")},
            1: {"key": "me", "label": _("meters")},
            2: {"key": "km", "label": _("kilometers")},
            3: {"key": "mi", "label": _("miles")},
            4: {"key": "ft", "label": _("feet")},
        }

        self._units["area"] = {
            0: {"key": "mu", "label": _("sq map units")},
            1: {"key": "me", "label": _("sq meters")},
            2: {"key": "km", "label": _("sq kilometers")},
            3: {"key": "ar", "label": _("acres")},
            4: {"key": "ht", "label": _("hectares")},
        }

    def GetUnitsList(self, type):
        """Get list of units (their labels)

        :param type: units type ('length' or 'area')

        :return: list of units labels
        """
        result = list()
        try:
            keys = sorted(self._units[type].keys())
            for idx in keys:
                result.append(self._units[type][idx]["label"])
        except KeyError:
            pass

        return result

    def GetUnitsKey(self, type, index):
        """Get units key based on index

        :param type: units type ('length' or 'area')
        :param index: units index
        """
        return self._units[type][index]["key"]

    def GetUnitsIndex(self, type, key):
        """Get units index based on key

        :param type: units type ('length' or 'area')
        :param key: units key, e.g. 'me' for meters

        :return: index
        """
        for k, u in six.iteritems(self._units[type]):
            if u["key"] == key:
                return k
        return 0


Units = BaseUnits()


def ConvertValue(value, type, units):
    """Convert value from map units to given units

    Inspired by vector/v.to.db/units.c

    :param value: value to be converted
    :param type: units type ('length', 'area')
    :param unit: destination units
    """
    # get map units
    # TODO

    f = 1
    if type == "length":
        if units == "me":
            f = 1.0
        elif units == "km":
            f = 1.0e-3
        elif units == "mi":
            f = 6.21371192237334e-4
        elif units == "ft":
            f = 3.28083989501312
    else:  # -> area
        if units == "me":
            f = 1.0
        elif units == "km":
            f = 1.0e-6
        elif units == "mi":
            f = 3.86102158542446e-7
        elif units == "ft":
            f = 10.7639104167097
        elif units == "ar":
            f = 2.47105381467165e-4
        elif units == "ht":
            f = 1.0e-4

    return f * value


def formatDist(distance, mapunits):
    """Formats length numbers and units in a nice way.

    Formats length numbers and units as a function of length.

    >>> formatDist(20.56915, 'metres')
    (20.57, 'm')
    >>> formatDist(6983.4591, 'metres')
    (6.983, 'km')
    >>> formatDist(0.59, 'feet')
    (0.59, 'ft')
    >>> formatDist(8562, 'feet')
    (1.622, 'miles')
    >>> formatDist(0.48963, 'degrees')
    (29.38, 'min')
    >>> formatDist(20.2546, 'degrees')
    (20.25, 'deg')
    >>> formatDist(82.146, 'unknown')
    (82.15, 'units')

    Accepted map units are 'meters', 'metres', 'feet', 'degree'.
    Returns 'units' instead of unrecognized units.

    :param distance: map units
    :param mapunits: map units

    From code by Hamish Bowman Grass Development Team 2006.
    """
    if mapunits == "metres":
        mapunits = "meters"
    outunits = mapunits
    distance = float(distance)
    divisor = 1.0

    # figure out which units to use
    if mapunits == "meters":
        if distance > 2500.0:
            outunits = "km"
            divisor = 1000.0
        else:
            outunits = "m"
    elif mapunits == "feet":
        # nano-bug: we match any "feet", but US Survey feet is really
        #  5279.9894 per statute mile, or 10.6' per 1000 miles. As >1000
        #  miles the tick markers are rounded to the nearest 10th of a
        #  mile (528'), the difference in foot flavours is ignored.
        if distance > 5280.0:
            outunits = "miles"
            divisor = 5280.0
        else:
            outunits = "ft"
    elif "degree" in mapunits:
        # was: 'degree' in mapunits and not haveCtypes (for unknown reason)
        if distance < 1:
            outunits = "min"
            divisor = 1 / 60.0
        else:
            outunits = "deg"
    else:
        return (distance, "units")

    # format numbers in a nice way
    if (distance / divisor) >= 2500.0:
        outdistance = round(distance / divisor)
    elif (distance / divisor) >= 1000.0:
        outdistance = round(distance / divisor, 1)
    elif (distance / divisor) > 0.0:
        outdistance = round(
            distance / divisor, int(math.ceil(3 - math.log10(distance / divisor)))
        )
    else:
        outdistance = float(distance / divisor)

    return (outdistance, outunits)


def doc_test():
    """Tests the module using doctest

    :return: a number of failed tests
    """
    import doctest
    from core.utils import do_doctest_gettext_workaround

    do_doctest_gettext_workaround()
    return doctest.testmod().failed


if __name__ == "__main__":
    sys.exit(doc_test())
