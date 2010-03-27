"""
@package dbm_base.py

@brief Support classes for dbm.py

List of classes:
 - VectorDBInfo

(C) 2007-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx

import gselect
import gcmd
from preferences import globalSettings as UserSettings

def unicodeValue(value):
    """!Encode value"""
    enc = UserSettings.Get(group='atm', key='encoding', subkey='value')
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
    infoFlexSizer = wx.FlexGridSizer (cols=2, hgap=1, vgap=1)
    infoFlexSizer.AddGrowableCol(1)
    
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="Driver:"))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=mapDBInfo.layers[layer]['driver']))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="Database:"))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=mapDBInfo.layers[layer]['database']))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="Table:"))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=mapDBInfo.layers[layer]['table']))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="Key:"))
    infoFlexSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=mapDBInfo.layers[layer]['key']))
    
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
        
        if os.environ.has_key("LC_ALL"):
            locale = os.environ["LC_ALL"]
            os.environ["LC_ALL"] = "C"
        
        ### FIXME (implement script-style output)        
        ret = gcmd.RunCommand('v.what',
                              quiet = True,
                              read = True,
                              flags = 'a',
                              map = self.map,
                              east_north = '%f,%f' % \
                                  (float(queryCoords[0]), float(queryCoords[1])),
                              distance = float(qdist))
        
        if os.environ.has_key("LC_ALL"):
            os.environ["LC_ALL"] = locale
        
        data = {}
        if ret:
            readAttrb = False
            for item in ret.splitlines():
                try:
                    key, value = item.split(':', 1)
                except ValueError:
                    continue
                
                if key == 'Layer' and readAttrb:
                    readAttrb = False
                
                if readAttrb:
                    name, value = item.split(':', 1)
                    name = name.strip()
                    value = value.strip()
                    # append value to the column
                    if len(value) < 1:
                        value = None
                    else:
                        if self.tables[table][name]['ctype'] != type(''):
                            value = self.tables[table][name]['ctype'] (value.strip())
                        else:
                            value = unicodeValue(value.strip())
                    self.tables[table][name]['values'].append(value)
                else:
                    if not data.has_key(key):
                        data[key] = []
                    data[key].append(value.strip())
                    
                    if key == 'Table':
                        table = value.strip()
                        
                    if key == 'Key column': # skip attributes
                        readAttrb = True

        return data
    
    def SelectFromTable(self, layer, cols='*', where=None):
        """!Select records from the table

        Return number of selected records, -1 on error
        """
        if layer <= 0:
            return -1

        nselected = 0

        table = self.layers[layer]["table"] # get table desc
        # select values (only one record)
        if where is None or where is '':
            sql="SELECT %s FROM %s" % (cols, table)
        else:
            sql="SELECT %s FROM %s WHERE %s" % (cols, table, where)

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
