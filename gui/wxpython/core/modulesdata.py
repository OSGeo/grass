"""!
@package core.modulesdata

@brief Provides information about available modules

Classes:
 - modules::modulesdata

(C) 2009-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import sys
import os

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core import globalvar
from lmgr.menudata import LayerManagerMenuData


class ModulesData(object):
    """!Holds information about modules.

    @todo add doctest
    """
    def __init__(self, modulesDesc = None):

        if modulesDesc is not None:
            self.moduleDesc = modulesDesc
        else:
            self.moduleDesc = LayerManagerMenuData().GetModules()

        self.moduleDict = self.GetDictOfModules()

    def GetCommandDesc(self, cmd):
        """!Gets the description for a given module (command).

        If the given module is not available, an empty string is returned.
        
        \code
        print data.GetCommandDesc('r.info')
        Outputs basic information about a raster map.
        \endcode
        """
        if cmd in self.moduleDesc:
            return self.moduleDesc[cmd]['desc']

        return ''

    def GetCommandItems(self):
        """!Gets list of available modules (commands).

        The list contains available module names.

        \code
        print data.GetCommandItems()[0:4]
        ['d.barscale', 'd.colorlist', 'd.colortable', 'd.correlate']
        \endcode
        """
        items = list()

        mList = self.moduleDict

        prefixes = mList.keys()
        prefixes.sort()

        for prefix in prefixes:
            for command in mList[prefix]:
                name = prefix + '.' + command
                if name not in items:
                    items.append(name)

        items.sort()

        return items

    def GetDictOfModules(self):
        """!Gets modules as a dictionary optimized for autocomplete.

        \code
        print data.GetDictOfModules()['r'][0:4]
        print data.GetDictOfModules()['r.li'][0:4]
        r: ['basins.fill', 'bitpattern', 'blend', 'buffer']
        r.li: ['cwed', 'dominance', 'edgedensity', 'mpa']
        \endcode
        """
        result = dict()
        for module in globalvar.grassCmd:
            try:
                group, name = module.split('.', 1)
            except ValueError:
                continue  # TODO

            if group not in result:
                result[group] = list()
            result[group].append(name)

            # for better auto-completion:
            # not only result['r']={...,'colors.out',...}
            # but also result['r.colors']={'out',...}
            for i in range(len(name.split('.')) - 1):
                group = '.'.join([group, name.split('.', 1)[0]])
                name = name.split('.', 1)[1]
                if group not in result:
                    result[group] = list()
                result[group].append(name)

        # sort list of names
        for group in result.keys():
            result[group].sort()

        return result

    def FindModules(self, text, findIn):
        """!Finds modules according to given text.

        @param text string to search
        @param findIn where to search for text
        (allowed values are 'description', 'keywords' and 'command')
        """
        modules = dict()
        iFound = 0
        for module, data in self.moduleDesc.iteritems():
            found = False
            if findIn == 'description':
                if text in data['desc']:
                    found = True
            elif findIn == 'keywords':
                if text in ','.join(data['keywords']):
                    found = True
            elif findIn == 'command':
                if module[:len(text)] == text:
                    found = True
            else:
                raise ValueError("Parameter findIn is not valid")

            if found:
                try:
                    group, name = module.split('.')
                except ValueError:
                    continue # TODO                
                iFound += 1
                if group not in modules:
                    modules[group] = list()
                modules[group].append(name)
        return modules, iFound

    def SetFilter(self, data = None):
        """!Sets filter modules

        If @p data is not specified, module dictionary is derived
        from an internal data structures.
        
        @todo Document this method.

        @param data data dict
        """
        if data:
            self.moduleDict = data
        else:
            self.moduleDict = self.GetDictOfModules()


def test():
    data = ModulesData()
    module = 'r.info'
    print '%s:' % module, data.GetCommandDesc(module)
    print '[0:5]:', data.GetCommandItems()[0:5]

    modules = data.GetDictOfModules()
    print 'r:', modules['r'][0:4]
    print 'r.li:', modules['r.li'][0:4]
    


if __name__ == '__main__':
    test()