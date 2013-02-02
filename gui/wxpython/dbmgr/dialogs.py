"""!
@package dbmgr.dialogs

@brief DBM-related dialogs

List of classes:
 - dialogs::DisplayAttributesDialog
 - dialogs::ModifyTableRecord
 - dialogs::AddColumnDialog

(C) 2007-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Refactoring by Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import os
import types

from core import globalvar
import wx
import wx.lib.scrolledpanel as scrolled

from core.gcmd        import RunCommand, GError
from core.debug       import Debug
from core.settings    import UserSettings
from dbmgr.vinfo      import VectorDBInfo, GetUnicodeValue
from gui_core.widgets import IntegerValidator, FloatValidator

class DisplayAttributesDialog(wx.Dialog):
    def __init__(self, parent, map,
                 query = None, cats = None, line = None,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 pos = wx.DefaultPosition,
                 action = "add", ignoreError = False):
        """!Standard dialog used to add/update/display attributes linked
        to the vector map.
        
        Attribute data can be selected based on layer and category number
        or coordinates.
        
        @param parent
        @param map vector map
        @param query query coordinates and distance (used for v.edit)
        @param cats {layer: cats}
        @param line feature id (requested for cats)
        @param style
        @param pos
        @param action (add, update, display)
        @param ignoreError True to ignore errors
        """
        self.parent = parent # mapdisplay.BufferedWindow
        self.map    = map
        self.action = action

        # ids/cats of selected features
        # fid : {layer : cats}
        self.cats = {}
        self.fid = -1 # feature id
        
        # get layer/table/column information
        self.mapDBInfo = VectorDBInfo(self.map)
        
        layers = self.mapDBInfo.layers.keys() # get available layers

        # check if db connection / layer exists
        if len(layers) <= 0:
            if not ignoreError:
                dlg = wx.MessageDialog(parent = self.parent,
                                       message = _("No attribute table found.\n\n"
                                                   "Do you want to create a new attribute table "
                                                   "and defined a link to vector map <%s>?") % self.map,
                                       caption = _("Create table?"),
                                       style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)
                if dlg.ShowModal() == wx.ID_YES:
                    lmgr = self.parent.lmgr
                    lmgr.OnShowAttributeTable(event = None, selection = 'layers')
                
                dlg.Destroy()
            
            self.mapDBInfo = None
        
        wx.Dialog.__init__(self, parent = self.parent, id = wx.ID_ANY,
                           title = "", style = style, pos = pos)

        # dialog body
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        # notebook
        self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)

        self.closeDialog = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                       label = _("Close dialog on submit"))
        self.closeDialog.SetValue(True)
        if self.action == 'display':
            self.closeDialog.Enable(False)
        
        # feature id (text/choice for duplicates)
        self.fidMulti = wx.Choice(parent = self, id = wx.ID_ANY,
                                  size = (150, -1))
        self.fidMulti.Bind(wx.EVT_CHOICE, self.OnFeature)
        self.fidText = wx.StaticText(parent = self, id = wx.ID_ANY)

        self.noFoundMsg = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _("No attributes found"))
        
        self.UpdateDialog(query = query, cats = cats)

        # set title
        if self.action == "update":
            self.SetTitle(_("Update attributes"))
        elif self.action == "add":
            self.SetTitle(_("Define attributes"))
        else:
            self.SetTitle(_("Display attributes"))

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnReset  = wx.Button(self, wx.ID_UNDO, _("&Reload"))
        btnSubmit = wx.Button(self, wx.ID_OK, _("&Submit"))
        if self.action == 'display':
            btnSubmit.Enable(False)
        
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnReset)
        btnSizer.SetNegativeButton(btnReset)
        btnSubmit.SetDefault()
        btnSizer.AddButton(btnSubmit)
        btnSizer.Realize()

        mainSizer.Add(item = self.noFoundMsg, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = self.notebook, proportion = 1,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        fidSizer = wx.BoxSizer(wx.HORIZONTAL)
        fidSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _("Feature id:")),
                     proportion = 0, border = 5,
                     flag = wx.ALIGN_CENTER_VERTICAL)
        fidSizer.Add(item = self.fidMulti, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        fidSizer.Add(item = self.fidText, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        mainSizer.Add(item = fidSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)
        mainSizer.Add(item = self.closeDialog, proportion = 0, flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                      border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        # bindigs
        btnReset.Bind(wx.EVT_BUTTON, self.OnReset)
        btnSubmit.Bind(wx.EVT_BUTTON, self.OnSubmit)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

        # set min size for dialog
        w, h = self.GetBestSize()
        w += 50
        if h < 200:
            self.SetMinSize((w, 200))
        else:
            self.SetMinSize((w, h))
        
        if self.notebook.GetPageCount() == 0:
            Debug.msg(2, "DisplayAttributesDialog(): Nothing found!")
            ### self.mapDBInfo = None
        
    def OnSQLStatement(self, event):
        """!Update SQL statement"""
        pass

    def IsFound(self):
        """!Check for status

        @return True on attributes found
        @return False attributes not found
        """
        return bool(self.mapDBInfo and self.notebook.GetPageCount() > 0)
    
    def GetSQLString(self, updateValues = False):
        """!Create SQL statement string based on self.sqlStatement

        Show error message when invalid values are entered.
        
        If updateValues is True, update dataFrame according to values
        in textfields.
        """
        sqlCommands = []
        # find updated values for each layer/category
        for layer in self.mapDBInfo.layers.keys(): # for each layer
            table = self.mapDBInfo.GetTable(layer)
            key = self.mapDBInfo.GetKeyColumn(layer)
            columns = self.mapDBInfo.GetTableDesc(table)
            for idx in range(len(columns[key]['values'])): # for each category
                updatedColumns = []
                updatedValues = []
                for name in columns.keys():
                    if name == key:
                        cat = columns[name]['values'][idx]
                        continue
                    ctype  = columns[name]['ctype']
                    value = columns[name]['values'][idx]
                    id    = columns[name]['ids'][idx]
                    try:
                        newvalue = self.FindWindowById(id).GetValue()
                    except:
                        newvalue = self.FindWindowById(id).GetLabel()
                  
                    if newvalue:
                        try:
                            if ctype == int:
                                newvalue = int(newvalue)
                            elif ctype == float:
                                newvalue = float(newvalue)
                        except ValueError:
                            GError(parent = self,
                                   message = _("Column <%(col)s>: Value '%(value)s' needs to be entered as %(type)s.") % \
                                       {'col' : name,
                                        'value' : str(newvalue),
                                        'type' : columns[name]['type'].lower()},
                                   showTraceback = False)
                            sqlCommands.append(None)
                            continue
                    else:
                        if self.action == 'add':
                            continue
                    
                    if newvalue != value:
                        updatedColumns.append(name)
                        if not newvalue:
                            updatedValues.append('NULL')
                        else:
                            if ctype != str:
                                updatedValues.append(str(newvalue))
                            else:
                                updatedValues.append("'" + str(newvalue) + "'")
                        columns[name]['values'][idx] = newvalue

                if self.action != "add" and len(updatedValues) == 0:
                    continue

                if self.action == "add":
                    sqlString = "INSERT INTO %s (%s," % (table, key)
                else:
                    sqlString = "UPDATE %s SET " % table

                for idx in range(len(updatedColumns)):
                    name = updatedColumns[idx]
                    if self.action == "add":
                        sqlString += name + ","
                    else:
                        sqlString += name + "=" + updatedValues[idx] + ","

                sqlString = sqlString[:-1] # remove last comma

                if self.action == "add":
                    sqlString += ") VALUES (%s," % cat
                    for value in updatedValues:
                        sqlString += str(value) + ","
                    sqlString = sqlString[:-1] # remove last comma
                    sqlString += ")"
                else:
                    sqlString += " WHERE %s=%s" % (key, cat)
                sqlCommands.append(sqlString)
            # for each category
        # for each layer END

        Debug.msg(3, "DisplayAttributesDialog.GetSQLString(): %s" % sqlCommands)

        return sqlCommands

    def OnReset(self, event = None):
        """!Reset form"""
        for layer in self.mapDBInfo.layers.keys():
            table = self.mapDBInfo.layers[layer]["table"]
            key = self.mapDBInfo.layers[layer]["key"]
            columns = self.mapDBInfo.tables[table]
            for idx in range(len(columns[key]['values'])):
                for name in columns.keys():
                    type  = columns[name]['type']
                    value = columns[name]['values'][idx]
                    if value is None:
                        value = ''
                    try:
                        id = columns[name]['ids'][idx]
                    except IndexError:
                        id = wx.NOT_FOUND
                    
                    if name != key and id != wx.NOT_FOUND:
                        self.FindWindowById(id).SetValue(str(value))

    def OnCancel(self, event):
        """!Cancel button pressed
        """
        frame = self.parent.parent
        frame.dialogs['attributes'] = None
        if hasattr(self, "digit"):
            self.parent.digit.GetDisplay().SetSelected([])
            if frame.IsAutoRendered():
                self.parent.UpdateMap(render = False)
        elif frame.IsAutoRendered():
            frame.RemoveQueryLayer()
            self.parent.UpdateMap(render = True)

        self.Close()

    def OnSubmit(self, event):
        """!Submit records"""
        layer = 1
        close = True
        enc = UserSettings.Get(group = 'atm', key = 'encoding', subkey = 'value')
        if not enc and 'GRASS_DB_ENCODING' in os.environ:
            enc = os.environ['GRASS_DB_ENCODING']
        
        for sql in self.GetSQLString(updateValues = True):
            if not sql:
                close = False
                continue
            if enc:
                sql = sql.encode(enc)
            
            driver, database = self.mapDBInfo.GetDbSettings(layer)
            Debug.msg(1, "SQL: %s" % sql)
            RunCommand('db.execute',
                       parent = self,
                       quiet = True,
                       input = '-',
                       stdin = sql,
                       driver = driver,
                       database = database)
            
            layer += 1
        
        if close and self.closeDialog.IsChecked():
            self.OnCancel(event)

    def OnFeature(self, event):
        self.fid = int(event.GetString())
        self.UpdateDialog(cats = self.cats, fid = self.fid)
        
    def GetCats(self):
        """!Get id of selected vector object or 'None' if nothing selected

        @param id if true return ids otherwise cats
        """
        if self.fid < 0:
            return None
        
        return self.cats[self.fid]

    def GetFid(self):
        """!Get selected feature id"""
        return self.fid
    
    def UpdateDialog(self, map = None, query = None, cats = None, fid = -1,
                     action = None):
        """!Update dialog
        
        @param map name of vector map
        @param query
        @param cats
        @param fid feature id
        @param action add, update, display or None
        
        @return True if updated
        @return False
        """
        if action:
            self.action = action
            if action == 'display':
                enabled = False
            else:
                enabled = True
            self.closeDialog.Enable(enabled)
            self.FindWindowById(wx.ID_OK).Enable(enabled)
        
        if map:
            self.map = map
            # get layer/table/column information
            self.mapDBInfo = VectorDBInfo(self.map)
        
        if not self.mapDBInfo:
            return False
        
        self.mapDBInfo.Reset()
        
        layers = self.mapDBInfo.layers.keys() # get available layers
        
        # id of selected line
        if query: # select by position
            data = self.mapDBInfo.SelectByPoint(query[0],
                                                query[1])
            self.cats = {}
            if data and 'Layer' in data:
                idx = 0
                for layer in data['Layer']:
                    layer = int(layer)
                    if data['Id'][idx] is not None:
                        tfid = int(data['Id'][idx])
                    else:
                        tfid = 0 # Area / Volume
                    if not tfid in self.cats:
                        self.cats[tfid] = {}
                    if not layer in self.cats[tfid]:
                        self.cats[tfid][layer] = []
                    cat = int(data['Category'][idx])
                    self.cats[tfid][layer].append(cat)
                    idx += 1
        else:
            self.cats = cats
        
        if fid > 0:
            self.fid = fid
        elif len(self.cats.keys()) > 0:
            self.fid = self.cats.keys()[0]
        else:
            self.fid = -1
        
        if len(self.cats.keys()) == 1:
            self.fidMulti.Show(False)
            self.fidText.Show(True)
            if self.fid > 0:
                self.fidText.SetLabel("%d" % self.fid)
            else:
                self.fidText.SetLabel(_("Unknown"))
        else:
            self.fidMulti.Show(True)
            self.fidText.Show(False)
            choices = []
            for tfid in self.cats.keys():
                choices.append(str(tfid))
            self.fidMulti.SetItems(choices)
            self.fidMulti.SetStringSelection(str(self.fid))
        
        # reset notebook
        self.notebook.DeleteAllPages()
        
        for layer in layers: # for each layer
            if not query: # select by layer/cat
                if self.fid > 0 and layer in self.cats[self.fid]:
                    for cat in self.cats[self.fid][layer]:
                        nselected = self.mapDBInfo.SelectFromTable(layer,
                                                                   where = "%s=%d" % \
                                                                   (self.mapDBInfo.layers[layer]['key'],
                                                                    cat))
                else:
                    nselected = 0
            
            # if nselected <= 0 and self.action != "add":
            #    continue # nothing selected ...
            
            if self.action == "add":
                if nselected <= 0:
                    if layer in self.cats[self.fid]:
                        table = self.mapDBInfo.layers[layer]["table"]
                        key = self.mapDBInfo.layers[layer]["key"]
                        columns = self.mapDBInfo.tables[table]
                        for name in columns.keys():
                            if name == key:
                                for cat in self.cats[self.fid][layer]:
                                    self.mapDBInfo.tables[table][name]['values'].append(cat)
                            else:
                                self.mapDBInfo.tables[table][name]['values'].append(None)
                else: # change status 'add' -> 'update'
                    self.action = "update"
            
            table   = self.mapDBInfo.layers[layer]["table"]
            key   = self.mapDBInfo.layers[layer]["key"]
            columns = self.mapDBInfo.tables[table]
            
            for idx in range(len(columns[key]['values'])):
                for name in columns.keys():
                    if name == key:
                        cat = int(columns[name]['values'][idx])
                        break

                # use scrolled panel instead (and fix initial max height of the window to 480px)
                panel = scrolled.ScrolledPanel(parent = self.notebook, id = wx.ID_ANY,
                                               size = (-1, 150))
                panel.SetupScrolling(scroll_x = False)
                
                self.notebook.AddPage(page = panel, text = " %s %d / %s %d" % (_("Layer"), layer,
                                                                           _("Category"), cat))
                
                # notebook body
                border = wx.BoxSizer(wx.VERTICAL)
                
                flexSizer = wx.FlexGridSizer (cols = 3, hgap = 3, vgap = 3)
                flexSizer.AddGrowableCol(2)
                # columns (sorted by index)
                names = [''] * len(columns.keys())
                for name in columns.keys():
                    names[columns[name]['index']] = name
                
                for name in names:
                    if name == key: # skip key column (category)
                        continue
                    
                    vtype  = columns[name]['type'].lower()
                    ctype  = columns[name]['ctype']
                    
                    if columns[name]['values'][idx] is not None:
                        if columns[name]['ctype'] != types.StringType:
                            value = str(columns[name]['values'][idx])
                        else:
                            value = columns[name]['values'][idx]
                    else:
                        value = ''
                    
                    colName = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                            label = name)
                    colType = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                            label = "[%s]:" % vtype)
                    colValue = wx.TextCtrl(parent = panel, id = wx.ID_ANY, value = value)
                    colValue.SetName(name)
                    if ctype == int:
                        colValue.SetValidator(IntegerValidator())
                    elif ctype == float:
                        colValue.SetValidator(FloatValidator())
                    
                    self.Bind(wx.EVT_TEXT, self.OnSQLStatement, colValue)
                    if self.action == 'display':
                        colValue.SetWindowStyle(wx.TE_READONLY)
                    
                    flexSizer.Add(colName, proportion = 0,
                                  flag = wx.ALIGN_CENTER_VERTICAL)
                    flexSizer.Add(colType, proportion = 0,
                                  flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
                    flexSizer.Add(colValue, proportion = 1,
                                  flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
                    # add widget reference to self.columns
                    columns[name]['ids'].append(colValue.GetId()) # name, type, values, id
                # for each attribute (including category) END
                border.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
                panel.SetSizer(border)
            # for each category END
        # for each layer END
        
        if self.notebook.GetPageCount() == 0:
            self.noFoundMsg.Show(True)
        else:
            self.noFoundMsg.Show(False)
        
        self.Layout()
        
        return True

    def SetColumnValue(self, layer, column, value):
        """!Set attrbute value

        @param column column name
        @param value value
        """
        table = self.mapDBInfo.GetTable(layer)
        columns = self.mapDBInfo.GetTableDesc(table)
        
        for key, col in columns.iteritems():
            if key == column:
                col['values'] = [col['ctype'](value),]
                break
        
class ModifyTableRecord(wx.Dialog):
    def __init__(self, parent, title, data, keyEditable = (-1, True),
                 id = wx.ID_ANY, style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        """!Dialog for inserting/updating table record
        
        @param data a list: [(column, value)]
        @param KeyEditable (id, editable?) indicates if textarea for key column
        is editable(True) or not
        """
        # parent -> VDigitWindow
        wx.Dialog.__init__(self, parent, id, title, style = style)
        
        self.CenterOnParent()
        
        self.keyId = keyEditable[0]
        
        box = wx.StaticBox(parent = self, id = wx.ID_ANY)
        box.Hide()
        self.dataPanel = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY,
                                                style = wx.TAB_TRAVERSAL)
        self.dataPanel.SetupScrolling(scroll_x = False)
        
        # buttons
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnSubmit = wx.Button(self, wx.ID_OK, _("&Submit"))
        self.btnSubmit.SetDefault()
        
        # data area
        self.widgets = []
        cId = 0
        self.usebox = False
        self.cat = None
        winFocus = False
        
        for column, ctype, ctypeStr, value in data:
            if self.keyId == cId:
                self.cat = int(value)
                if not keyEditable[1]:
                    self.usebox = True
                    box.SetLabel(" %s %d " % (_("Category"), self.cat))
                    box.Show()
                    self.boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
                    cId += 1
                    continue
                else:
                    valueWin = wx.SpinCtrl(parent = self.dataPanel, id = wx.ID_ANY,
                                           value = value, min = -1e9, max = 1e9, size = (250, -1))
            else:
                valueWin = wx.TextCtrl(parent = self.dataPanel, id = wx.ID_ANY,
                                       value = value, size = (250, -1))
                if ctype == int:
                    valueWin.SetValidator(IntegerValidator())
                elif ctype == float:
                    valueWin.SetValidator(FloatValidator())
                if not winFocus:
                    wx.CallAfter(valueWin.SetFocus)
                    winFocus = True
            
            label = wx.StaticText(parent = self.dataPanel, id = wx.ID_ANY,
                                  label = column)
            ctype = wx.StaticText(parent = self.dataPanel, id = wx.ID_ANY,
                                  label = "[%s]:" % ctypeStr.lower())
            self.widgets.append((label.GetId(), ctype.GetId(), valueWin.GetId()))
            
            cId += 1
        
        self._layout()
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # data area
        dataSizer = wx.FlexGridSizer(cols = 3, hgap = 3, vgap = 3)
        dataSizer.AddGrowableCol(2)
        
        for labelId, ctypeId, valueId in self.widgets:
            label = self.FindWindowById(labelId)
            ctype = self.FindWindowById(ctypeId)
            value = self.FindWindowById(valueId)
            
            dataSizer.Add(label, proportion = 0,
                          flag = wx.ALIGN_CENTER_VERTICAL)
            dataSizer.Add(ctype, proportion = 0,
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
            dataSizer.Add(value, proportion = 0,
                          flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        
        self.dataPanel.SetAutoLayout(True)
        self.dataPanel.SetSizer(dataSizer)
        dataSizer.Fit(self.dataPanel)
        
        if self.usebox:
            self.boxSizer.Add(item = self.dataPanel, proportion = 1,
                              flag = wx.EXPAND | wx.ALL, border = 5)
            
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnSubmit)
        btnSizer.Realize()
        
        if not self.usebox:
            sizer.Add(item = self.dataPanel, proportion = 1,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        else:
            sizer.Add(item = self.boxSizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALL, border = 5)
        
        framewidth = self.GetBestSize()[0] + 25
        self.SetMinSize((framewidth, 250))

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)
        
        self.Layout()
        
    def GetValues(self, columns = None):
        """!Return list of values (casted to string).
        
        If columns is given (list), return only values of given columns.
        """
        valueList = list()
        for labelId, ctypeId, valueId in self.widgets:
            column = self.FindWindowById(labelId).GetLabel()
            if columns is None or column in columns:
                value = GetUnicodeValue(self.FindWindowById(valueId).GetValue())
                valueList.append(value)
        
        # add key value
        if self.usebox:
            valueList.insert(self.keyId, GetUnicodeValue(str(self.cat)))
        
        return valueList

class AddColumnDialog(wx.Dialog):
    def __init__(self, parent, title, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE  | wx.RESIZE_BORDER):
        """!Dialog for adding column into table
        """
        wx.Dialog.__init__(self, parent, id, title, style = style)
        
        self.CenterOnParent()

        self.data = {} 
        self.data['addColName'] = wx.TextCtrl(parent = self, id = wx.ID_ANY, value = '',
                                              size = (150, -1), style = wx.TE_PROCESS_ENTER)

           
        self.data['addColType'] = wx.Choice (parent = self, id = wx.ID_ANY,
                                             choices = ["integer",
                                                        "double",
                                                        "varchar",
                                                        "date"]) # FIXME
        self.data['addColType'].SetSelection(0)
        self.data['addColType'].Bind(wx.EVT_CHOICE, self.OnTableChangeType)
            
        self.data['addColLength'] = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (65, -1),
                                                initial = 250,
                                                min = 1, max = 1e6)
        self.data['addColLength'].Enable(False)


        # buttons
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnOk.SetDefault()

        self._layout()

    def _layout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)
        addSizer = wx.BoxSizer(wx.HORIZONTAL)

        addSizer.Add(item =  wx.StaticText(parent = self, id = wx.ID_ANY, label = _("Column")),
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)
        addSizer.Add(item = self.data['addColName'], proportion = 1,
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)

        addSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY, label = _("Type")), 
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)
        addSizer.Add(item = self.data['addColType'],
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)

        addSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY, label = _("Length")),
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)
        addSizer.Add(item = self.data['addColLength'],
                     flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border = 5)

        sizer.Add(item = addSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)

        self.SetSizer(sizer)

        self.Fit()

    def GetData(self):
        """!Get inserted data from dialog's widgets"""
        values = {}
        values['name'] = self.data['addColName'].GetValue()
        values['ctype'] = self.data['addColType'].GetStringSelection()
        values['length'] = int(self.data['addColLength'].GetValue())

        return values
  
    def OnTableChangeType(self, event):
        """!Data type for new column changed. Enable or disable
        data length widget"""
        if event.GetString() == "varchar":
            self.data['addColLength'].Enable(True)
        else:
            self.data['addColLength'].Enable(False)     
