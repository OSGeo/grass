"""
@package dbm_base.py

@brief Support classes for dbm.py

List of classes:
 - VectorDBInfo

(C) 2007-2010 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import types

import wx

import gselect
import gcmd
from preferences import globalSettings as UserSettings

import grass.script as grass

def unicodeValue(value):
    """!Encode value"""
    enc = UserSettings.Get(group = 'atm', key = 'encoding', subkey = 'value')
    if enc:
        value = unicode(value, enc)
    elif os.environ.has_key('GRASS_DB_ENCODING'):
        value = unicode(value, os.environ['GRASS_DB_ENCODING'])
    else:
        try:
            value = unicode(value, 'ascii')
        except UnicodeDecodeError:
            value = _("Unable to decode value. Set encoding in GUI preferences ('Attributes').")
    
    return value

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
        
class VectorDBInfo(gselect.VectorDBInfo):
    """!Class providing information about attribute tables
    linked to the vector map"""
    def __init__(self, map):
        gselect.VectorDBInfo.__init__(self, map)
        
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

        if len(data) < 1:
            return None
        
        # process attributes
        table = data[0]['Table']
        for key, value in data[0]['Attributes'].iteritems():
            if len(value) < 1:
                value = None
            else:
                if self.tables[table][key]['ctype'] != types.StringType:
                    value = self.tables[table][key]['ctype'] (value)
                else:
                    value = unicodeValue(value)
            self.tables[table][key]['values'].append(value)
        
        ret = dict()
        for key, value in data[0].iteritems():
            if key == 'Attributes':
                continue
            ret[key] = list()
            ret[key].append(value)
        
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

        ret = gcmd.RunCommand('db.select',
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
