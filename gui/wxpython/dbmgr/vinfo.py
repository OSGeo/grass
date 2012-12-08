"""
@package dbmgr.vinfo

@brief Support classes for Database Manager

List of classes:
 - vinfo::VectorDBInfo

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import types

import wx

from gui_core.gselect import VectorDBInfo as VectorDBInfoBase
from core.gcmd        import RunCommand
from core.settings    import UserSettings

import grass.script as grass

def unicodeValue(value):
    """!Encode value"""
    if type(value) == types.UnicodeType:
        return value
    
    enc = UserSettings.Get(group = 'atm', key = 'encoding', subkey = 'value')
    if not enc and 'GRASS_DB_ENCODING' in os.environ:
        enc = os.environ['GRASS_DB_ENCODING']
    else:
        enc = 'ascii'
    
    return unicode(value, enc, errors = 'replace')
    
def createDbInfoDesc(panel, mapDBInfo, layer):
    """!Create database connection information content"""
    infoFlexSizer = wx.FlexGridSizer (cols = 2, hgap = 1, vgap = 1)
    infoFlexSizer.AddGrowableCol(1)
    
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "Driver:"))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = mapDBInfo.layers[layer]['driver']))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "Database:"))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = mapDBInfo.layers[layer]['database']))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "Table:"))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = mapDBInfo.layers[layer]['table']))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "Key:"))
    infoFlexSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = mapDBInfo.layers[layer]['key']))
    
    return infoFlexSizer
        
class VectorDBInfo(VectorDBInfoBase):
    """!Class providing information about attribute tables
    linked to the vector map"""
    def __init__(self, map):
        VectorDBInfoBase.__init__(self, map)
        
    def GetColumns(self, table):
        """!Return list of columns names (based on their index)"""
        try:
            names = [''] * len(self.tables[table].keys())
        except KeyError:
            return []
        
        for name, desc in self.tables[table].iteritems():
            names[desc['index']] = name
        
        return names

    def SelectByPoint(self, queryCoords, qdist):
        """!Get attributes by coordinates (all available layers)

        Return line id or None if no line is found"""
        line = None
        nselected = 0

        data = grass.vector_what(map = self.map,
                                 coord = (float(queryCoords[0]), float(queryCoords[1])),
                                 distance = float(qdist))

        if len(data) < 1 or all(('Table' not in record) for record in data):
            return None
        
        # process attributes
        ret = dict()
        for key in ['Category', 'Layer', 'Table', 'Id']:
            ret[key] = list()

        for record in data:
            if not 'Table' in record:
                continue

            table = record['Table']
            for key, value in record['Attributes'].iteritems():
                if len(value) < 1:
                    value = None
                else:
                    if self.tables[table][key]['ctype'] != types.StringType:
                        value = self.tables[table][key]['ctype'] (value)
                    else:
                        value = unicodeValue(value)
                self.tables[table][key]['values'].append(value)
            
            for key, value in record.iteritems():
                if key == 'Attributes':
                    continue
                if key in ret:
                    ret[key].append(value)
            if 'Id' not in record.keys():
                ret['Id'].append(None)

        return ret
    
    def SelectFromTable(self, layer, cols = '*', where = None):
        """!Select records from the table

        Return number of selected records, -1 on error
        """
        if layer <= 0:
            return -1

        nselected = 0

        table = self.layers[layer]["table"] # get table desc
        # select values (only one record)
        if where is None or where is '':
            sql = "SELECT %s FROM %s" % (cols, table)
        else:
            sql = "SELECT %s FROM %s WHERE %s" % (cols, table, where)
        
        ret = RunCommand('db.select',
                         parent = self,
                         read = True,
                         quiet = True,
                         flags = 'v',
                         sql= sql,
                         database = self.layers[layer]["database"],
                         driver = self.layers[layer]["driver"])
        
        # self.tables[table][key][1] = str(cat)
        if ret:
            for line in ret.splitlines():
                name, value = line.split('|')
                # casting ...
                if value:
                    if self.tables[table][name]['ctype'] != type(''):
                        value = self.tables[table][name]['ctype'] (value)
                    else:
                        value = unicodeValue(value)
                else:
                    value = None
                self.tables[table][name]['values'].append(value)
                nselected = 1

        return nselected
