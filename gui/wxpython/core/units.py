"""!
@package core.units

@brief Units management

@todo Probably will be replaced by Python ctypes fns in the near
future(?)

Usage:
@code
from core.units import Units
@endcode

Classes:
 - units::BaseUnits

(C) 2009, 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from core.utils import _

class BaseUnits:
    def __init__(self):
        self._units = dict()
        self._units['length'] = { 0 : { 'key' : 'mu', 'label' : _('map units') },
                             1 : { 'key' : 'me', 'label' : _('meters') },
                             2 : { 'key' : 'km', 'label' : _('kilometers') },
                             3 : { 'key' : 'mi', 'label' : _('miles') },
                             4 : { 'key' : 'ft', 'label' : _('feet') } }
        
        self._units['area']   = { 0 : { 'key' : 'mu', 'label' : _('sq map units') },
                             1 : { 'key' : 'me', 'label' : _('sq meters') },
                             2 : { 'key' : 'km', 'label' : _('sq kilometers') },
                             3 : { 'key' : 'ar', 'label' : _('acres') },
                             4 : { 'key' : 'ht', 'label' : _('hectares') } }

    def GetUnitsList(self, type):
        """!Get list of units (their labels)
        
        @param type units type ('length' or 'area')
        
        @return list of units labels
        """
        result = list()
        try:
            keys = self._units[type].keys()
            keys.sort()
            for idx in keys:
                result.append(self._units[type][idx]['label'])
        except KeyError:
            pass
        
        return result

    def GetUnitsKey(self, type, index):
        """!Get units key based on index
        
        @param type units type ('length' or 'area')
        @param index units index
        """
        return self._units[type][index]['key']

    def GetUnitsIndex(self, type, key):
        """!Get units index based on key
        
        @param type units type ('length' or 'area')
        @param key units key, e.g. 'me' for meters

        @return index
        """
        for k, u in self._units[type].iteritems():
            if u['key'] == key:
                return k
        return 0

Units = BaseUnits()

def ConvertValue(value, type, units):
    """!Convert value from map units to given units

    Inspired by vector/v.to.db/units.c

    @param value value to be converted
    @param type units type ('length', 'area')
    @param unit  destination units
    """
    # get map units
    # TODO
    
    f = 1
    if type == 'length':
        if units == 'me':
            f = 1.0
        elif units == 'km':
            f = 1.0e-3
        elif units == 'mi':
            f = 6.21371192237334e-4
        elif units == 'ft':
            f = 3.28083989501312
    else: # -> area
        if units == 'me':
            f = 1.0
        elif units == 'km':
            f = 1.0e-6
        elif units == 'mi':
            f = 3.86102158542446e-7
        elif units == 'ft':
            f = 10.7639104167097
        elif units == 'ar':
            f = 2.47105381467165e-4
        elif units == 'ht':
            f = 1.0e-4

    return f * value
