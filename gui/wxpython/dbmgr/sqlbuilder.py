"""!
@package dbmgr.sqlbuilder

@brief GRASS SQL Select/Update Builder

Classes:
 - sqlbuilder::SQLBuilder
 - sqlbuilder::SQLBuilderSelect
 - sqlbuilder::SQLBuilderUpdate

Usage:
@code
python sqlbuilder.py select|update vector_map
@endcode

(C) 2007-2009, 2011-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Jachym Cepicky <jachym.cepicky gmail.com> (original author)
@author Martin Landa <landa.martin gmail.com>
@author Hamish Bowman <hamish_b yahoo com>
@author Refactoring, SQLBUilderUpdate by Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import os
import sys

if __name__ == "__main__":
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython'))
from core import globalvar
import wx

from core.gcmd   import RunCommand, GError, GMessage
from dbmgr.vinfo import createDbInfoDesc, VectorDBInfo

import grass.script as grass

class SQLBuilder(wx.Frame):
    """!SQLBuider class
    Base class for classes, which builds SQL statements.
    """
    def __init__(self, parent, title, vectmap, modeChoices, id = wx.ID_ANY,
                 layer = 1):
        wx.Frame.__init__(self, parent, id, title)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_sql.ico'),
                             wx.BITMAP_TYPE_ICO))
        
        self.parent     = parent
        
        # variables
        self.vectmap = vectmap # fullname
        if not "@" in self.vectmap:
            self.vectmap = grass.find_file(self.vectmap, element = 'vector')['fullname']
        self.mapname, self.mapset = self.vectmap.split("@", 1)
        
        # db info
        self.layer = layer
        self.dbInfo = VectorDBInfo(self.vectmap)
        self.tablename = self.dbInfo.GetTable(self.layer)
        self.driver, self.database = self.dbInfo.GetDbSettings(self.layer)
        
        self.colvalues = []     # array with unique values in selected column
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        # statusbar
        self.statusbar = self.CreateStatusBar(number=1)
        
        self._doLayout(modeChoices)

    def _doLayout(self, modeChoices):
        """!Do dialog layout"""
        
        self.pagesizer = wx.BoxSizer(wx.VERTICAL)

        
        # dbInfo
        databasebox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                   label = " %s " % _("Database connection"))
        databaseboxsizer = wx.StaticBoxSizer(databasebox, wx.VERTICAL)
        databaseboxsizer.Add(item=createDbInfoDesc(self.panel, self.dbInfo, layer = self.layer),
                             proportion=1,
                             flag=wx.EXPAND | wx.ALL,
                             border=3)

        #
        # text areas
        #
        # sql box
        sqlbox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                              label = " %s " % _("Query"))
        sqlboxsizer = wx.StaticBoxSizer(sqlbox, wx.VERTICAL)

        self.text_sql = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                    value = '', size = (-1, 50),
                                    style=wx.TE_MULTILINE)
        
        self.text_sql.SetInsertionPointEnd()
        wx.CallAfter(self.text_sql.SetFocus)

        sqlboxsizer.Add(item = self.text_sql, flag = wx.EXPAND)
        
        #
        # buttons
        #
        self.btn_clear  = wx.Button(parent = self.panel, id = wx.ID_CLEAR)
        self.btn_clear.SetToolTipString(_("Set SQL statement to default"))
        self.btn_apply  = wx.Button(parent = self.panel, id = wx.ID_APPLY)
        self.btn_apply.SetToolTipString(_("Apply SQL statement in Attribute Table Manager"))
        self.btn_close  = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close the dialog"))
        
        self.btn_logic = { 'is' : ['=', ],
                           'isnot' : ['!=', ],
                           'like'  : ['LIKE', ],
                           'gt'    : ['>', ],
                           'ge'    : ['>=', ],
                           'lt'    : ['<', ],
                           'le'    : ['<=', ],
                           'or'    : ['OR', ],
                           'not'   : ['NOT', ],
                           'and'   : ['AND', ],
                           'brac'  : ['()', ],
                           'prc'   : ['%', ] }
    
        self.btn_logicpanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)    
        for key, value in self.btn_logic.iteritems():
            btn = wx.Button(parent = self.btn_logicpanel, id = wx.ID_ANY,
                            label = value[0])
            self.btn_logic[key].append(btn.GetId())
            
        self.buttonsizer = wx.FlexGridSizer(cols = 4, hgap = 5, vgap = 5)
        self.buttonsizer.Add(item = self.btn_clear)
        self.buttonsizer.Add(item = self.btn_apply)
        self.buttonsizer.Add(item = self.btn_close)
        
        btn_logicsizer = wx.GridBagSizer(5, 5)
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['is'][1]), pos = (0,0))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['isnot'][1]), pos = (1,0))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['like'][1]), pos = (2, 0))

        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['gt'][1]), pos = (0, 1))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['ge'][1]), pos = (1, 1))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['or'][1]), pos = (2, 1))

        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['lt'][1]), pos = (0, 2))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['le'][1]), pos = (1, 2))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['not'][1]), pos = (2, 2))

        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['brac'][1]), pos = (0, 3))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['prc'][1]), pos = (1, 3))
        btn_logicsizer.Add(item = self.FindWindowById(self.btn_logic['and'][1]), pos = (2, 3))
    
        self.btn_logicpanel.SetSizer(btn_logicsizer)

        #
        # list boxes (columns, values)
        #
        self.hsizer = wx.BoxSizer(wx.HORIZONTAL)
        
        columnsbox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                  label = " %s " % _("Columns"))
        columnsizer = wx.StaticBoxSizer(columnsbox, wx.VERTICAL)
        self.list_columns = wx.ListBox(parent = self.panel, id = wx.ID_ANY,
                                       choices = self.dbInfo.GetColumns(self.tablename),
                                       style = wx.LB_MULTIPLE)
        columnsizer.Add(item = self.list_columns, proportion = 1,
                        flag = wx.EXPAND)

        modesizer = wx.BoxSizer(wx.VERTICAL)
        
        self.mode = wx.RadioBox(parent = self.panel, id = wx.ID_ANY,
                                label = " %s " % _("Interactive insertion"),
                                choices = modeChoices, 
                                style = wx.RA_SPECIFY_COLS,
                                majorDimension = 1)

        self.mode.SetSelection(1) # default 'values'
        modesizer.Add(item = self.mode, proportion = 1,
                      flag = wx.ALIGN_CENTER_HORIZONTAL | wx.EXPAND, border = 5)

        # self.list_columns.SetMinSize((-1,130))
        # self.list_values.SetMinSize((-1,100))

        self.valuespanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)
        valuesbox = wx.StaticBox(parent = self.valuespanel, id = wx.ID_ANY,
                                 label = " %s " % _("Values"))
        valuesizer = wx.StaticBoxSizer(valuesbox, wx.VERTICAL)
        self.list_values = wx.ListBox(parent = self.valuespanel, id = wx.ID_ANY,
                                      choices = self.colvalues,
                                      style = wx.LB_MULTIPLE)
        valuesizer.Add(item = self.list_values, proportion = 1,
                       flag = wx.EXPAND)
        self.valuespanel.SetSizer(valuesizer)

        self.btn_unique = wx.Button(parent = self.valuespanel, id = wx.ID_ANY,
                                    label = _("Get all values"))
        self.btn_unique.Enable(False)
        self.btn_uniquesample = wx.Button(parent = self.valuespanel, id = wx.ID_ANY,
                                          label = _("Get sample"))
        self.btn_uniquesample.Enable(False)

        buttonsizer3 = wx.BoxSizer(wx.HORIZONTAL)
        buttonsizer3.Add(item = self.btn_uniquesample, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL | wx.RIGHT, border = 5)
        buttonsizer3.Add(item = self.btn_unique, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL)

        valuesizer.Add(item = buttonsizer3, proportion = 0,
                       flag = wx.TOP, border = 5)
        
        # hsizer1.Add(wx.StaticText(self.panel,-1, "Unique values: "), border=0, proportion=1)
 
        self.hsizer.Add(item = columnsizer, proportion = 1,
                        flag = wx.EXPAND)
        self.hsizer.Add(item = self.valuespanel, proportion = 1,
                        flag = wx.EXPAND)

        self.close_onapply = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Close dialog on apply"))
        self.close_onapply.SetValue(True)
 
        self.pagesizer.Add(item = databaseboxsizer,
                           flag = wx.ALL | wx.EXPAND, border = 5)
        self.pagesizer.Add(item = modesizer, proportion = 0,
                           flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        self.pagesizer.Add(item = self.hsizer, proportion = 1,
                           flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        # self.pagesizer.Add(self.btn_uniqe,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        # self.pagesizer.Add(self.btn_uniqesample,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        self.pagesizer.Add(item = self.btn_logicpanel, proportion = 0,
                           flag = wx.ALIGN_CENTER_HORIZONTAL)
        self.pagesizer.Add(item = sqlboxsizer, proportion = 0,
                           flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)
        self.pagesizer.Add(item = self.buttonsizer, proportion = 0,
                           flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        self.pagesizer.Add(item = self.close_onapply, proportion = 0,
                           flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        #
        # bindings
        #
        self.mode.Bind(wx.EVT_RADIOBOX, self.OnMode)
        #self.text_sql.Bind(wx.EVT_ACTIVATE, self.OnTextSqlActivate)TODO

        self.btn_unique.Bind(wx.EVT_BUTTON,       self.OnUniqueValues)
        self.btn_uniquesample.Bind(wx.EVT_BUTTON, self.OnSampleValues)
        
        for key, value in self.btn_logic.iteritems():
            self.FindWindowById(value[1]).Bind(wx.EVT_BUTTON, self.OnAddMark)
                     
        self.btn_close.Bind(wx.EVT_BUTTON,       self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON,       self.OnClear)
        self.btn_apply.Bind(wx.EVT_BUTTON,       self.OnApply)

        self.list_columns.Bind(wx.EVT_LISTBOX,   self.OnAddColumn)
        self.list_values.Bind(wx.EVT_LISTBOX,    self.OnAddValue)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(self.pagesizer)
        self.pagesizer.Fit(self.panel)
        
        self.Layout()
        self.SetSize((400, 525))
        self.SetClientSize(self.panel.GetSize())
        self.CenterOnParent()
   
    def OnUniqueValues(self, event, justsample = False):
        """!Get unique values"""
        vals = []
        try:
            idx = self.list_columns.GetSelections()[0]
            column = self.list_columns.GetString(idx)
        except:
            self.list_values.Clear()
            return
        
        self.list_values.Clear()
        
        data = grass.db_select(sql = "SELECT %s FROM %s" % (column, self.tablename),
                               database = self.database,
                               driver = self.driver)
        if not data:
            return
        
        desc = self.dbInfo.GetTableDesc(self.dbInfo.GetTable(self.layer))[column]
        
        i = 0
        for item in sorted(set(map(lambda x: desc['ctype'](x[0]), data))):
            if justsample and i > 255:
                break
            
            if desc['type'] != 'character':
                item = str(item)
            self.list_values.Append(item)
            i += 1
        
    def OnSampleValues(self, event):
        """!Get sample values"""
        self.OnUniqueValues(None, True)

    def OnAddColumn(self, event):
        """!Add column name to the query"""
        idx = self.list_columns.GetSelections()
        for i in idx:
            column = self.list_columns.GetString(i)
            self._add(element = 'column', value = column)
        
        if not self.btn_uniquesample.IsEnabled():
            self.btn_uniquesample.Enable(True)
            self.btn_unique.Enable(True)
        
    def OnAddValue(self, event):
        """!Add value"""
        selection = self.list_values.GetSelections()
        if not selection:
            event.Skip()
            return

        idx = selection[0]
        value = self.list_values.GetString(idx)
        idx = self.list_columns.GetSelections()[0]
        column = self.list_columns.GetString(idx)
        
        ctype = self.dbInfo.GetTableDesc(self.dbInfo.GetTable(self.layer))[column]['type']
        
        if ctype == 'character':
            value = "'%s'" % value
        
        self._add(element = 'value', value = value)

    def OnAddMark(self, event):
        """!Add mark"""
        mark = None
        if self.btn_logicpanel and \
           self.btn_logicpanel.IsShown():
            btns = self.btn_logic
        elif self.btn_arithmeticpanel and \
             self.btn_arithmeticpanel.IsShown():
            btns = self.btn_arithmetic
             
        for key, value in btns.iteritems():
            if event.GetId() == value[1]:
                mark = value[0]
                break
        
        self._add(element = 'mark', value = mark)

    def GetSQLStatement(self):
        """!Return SQL statement"""
        return self.text_sql.GetValue().strip().replace("\n"," ")
        
    def OnClose(self, event):
        self.Destroy()
        event.Skip()
 
class SQLBuilderSelect(SQLBuilder):
    """!Class for building SELECT SQL statement"""
    def __init__(self, parent, vectmap, id = wx.ID_ANY,
                 layer = 1, evtHandler = None):

        self.evtHandler = evtHandler

        # set dialog title
        title = _("GRASS SQL Builder (%(type)s) - <%(map)s>") % \
                 { 'type' : "SELECT", 'map' : vectmap }

        modeChoices = [_("Column to show (SELECT clause)"), 
                       _("Constraint for query (WHERE clause)")]

        SQLBuilder.__init__(self, parent, title, vectmap, id = wx.ID_ANY,
                            modeChoices = modeChoices, layer = layer)


    def _doLayout(self, modeChoices):
        """!Do dialog layout"""

        SQLBuilder._doLayout(self, modeChoices)

        self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)
        self.text_sql.SetToolTipString(_("Example: %s") % "SELECT * FROM roadsmajor WHERE MULTILANE = 'no' OR OBJECTID < 10")

        self.btn_verify = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Verify"))
        self.btn_verify.SetToolTipString(_("Verify SQL statement"))

        self.buttonsizer.Insert(item = self.btn_verify, before = 1)

        self.text_sql.Bind(wx.EVT_TEXT, self.OnText)
        self.btn_verify.Bind(wx.EVT_BUTTON,      self.OnVerify)

        self.text_sql.SetInsertionPoint(self.text_sql.GetLastPosition())
        self.statusbar.SetStatusText(_("SQL statement not verified"), 0)

    def OnApply(self, event):
        """Apply button pressed"""
        if self.evtHandler:
            self.evtHandler(event = 'apply')
        
        if self.close_onapply.IsChecked():
            self.Destroy()
            
        event.Skip()

    def OnClear(self, event):
        """!Clear button pressed"""
        self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)

    def OnMode(self, event):
        """!Adjusts builder for chosen mode"""
        if self.mode.GetSelection() == 0:
            self.valuespanel.Hide()
            self.btn_logicpanel.Hide()
        elif self.mode.GetSelection() == 1:
             self.valuespanel.Show()
             self.btn_logicpanel.Show()
        self.pagesizer.Layout()

    def OnText(self, event):
        """Query string changed"""
        if len(self.text_sql.GetValue()) > 0:
            self.btn_verify.Enable(True)
        else:
            self.btn_verify.Enable(False)
            
    def OnVerify(self, event):
        """!Verify button pressed"""
        ret, msg = RunCommand('db.select',
                              getErrorMsg = True,
                              table = self.tablename,
                              sql = self.text_sql.GetValue(),
                              flags = 't',
                              driver = self.driver,
                              database = self.database)
        
        if ret != 0 and msg:
            self.statusbar.SetStatusText(_("SQL statement is not valid"), 0)
            GError(parent = self,
                   message = _("SQL statement is not valid.\n\n%s") % msg)
        else:
            self.statusbar.SetStatusText(_("SQL statement is valid"), 0)

    def _add(self, element, value):
        """!Add element to the query

        @param element element to add (column, value)
        """
        sqlstr = self.text_sql.GetValue()
        curspos = self.text_sql.GetInsertionPoint()
        newsqlstr = ''
        if element == 'column':
            if self.mode.GetSelection() == 0: # -> column
                idx1 = len('select')
                idx2 = sqlstr.lower().find('from')
                colstr = sqlstr[idx1:idx2].strip()
                if colstr == '*':
                    cols = []
                else:
                    cols = colstr.split(',')
                if value in cols:
                        cols.remove(value)
                else:
                    cols.append(value)
                
                if len(cols) < 1:
                    cols = ['*',]  
                newsqlstr = 'SELECT ' + ','.join(cols) + ' '
                curspos = len(newsqlstr)
                newsqlstr += sqlstr[idx2:]
            else: # -> where
                newsqlstr = ''
                if sqlstr.lower().find('where') < 0:
                    newsqlstr += ' WHERE'       
                newsqlstr += ' ' + value
                curspos = self.text_sql.GetLastPosition() + len(newsqlstr)
                newsqlstr = sqlstr + newsqlstr

        elif element in  ['value', 'mark']:
            addstr = ' ' + value + ' '
            newsqlstr = sqlstr[:curspos] + addstr + sqlstr[curspos:]
            curspos += len(addstr)
        
        if newsqlstr:
            self.text_sql.SetValue(newsqlstr)

        wx.CallAfter(self.text_sql.SetFocus)
        self.text_sql.SetInsertionPoint(curspos)

    def CloseOnApply(self):
        """!Return True if the dialog will be close on apply"""
        return self.close_onapply.IsChecked()

    def OnClose(self, event):
        """!Close button pressed"""
        if self.evtHandler:
            self.evtHandler(event = 'close')
    
        SQLBuilder.OnClose(self, event)

class SQLBuilderUpdate(SQLBuilder):
    """!Class for building UPDATE SQL statement"""
    def __init__(self, parent, vectmap, id = wx.ID_ANY,
                 layer = 1, column = None):

        self.column = column 
        # set dialog title
        title = _("GRASS SQL Builder (%(type)s) - <%(map)s>") % \
                 { 'type' : "UPDATE", 'map' : vectmap }

        modeChoices = [_("Column to set (SET clause)"), 
                       _("Constraint for query (WHERE clause)"), 
                       _("Calculate column value to set")]

        SQLBuilder.__init__(self, parent, title, vectmap, id = wx.ID_ANY,
                            modeChoices = modeChoices, layer = layer)

    def _doLayout(self, modeChoices):
        """!Do dialog layout"""

        SQLBuilder._doLayout(self, modeChoices)

        self.initText = "UPDATE %s SET" % self.tablename
        if self.column:
            self.initText += " %s = " % self.column 

        self.text_sql.SetValue(self.initText)

        self.btn_arithmetic = { 'eq'        : ['=', ],
                                'brac'      : ['()',],
                                'plus'      : ['+', ],
                                'minus'     : ['-', ],
                                'divide'    : ['/', ],
                                'multiply'  : ['*', ]} 

        self.btn_arithmeticpanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)

        for key, value in self.btn_arithmetic.iteritems():
            btn = wx.Button(parent = self.btn_arithmeticpanel, id = wx.ID_ANY,
                            label = value[0])
            self.btn_arithmetic[key].append(btn.GetId())

        btn_arithmeticsizer = wx.GridBagSizer(hgap = 5, vgap = 5)

        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['eq'][1]), pos = (0, 0))
        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['brac'][1]), pos = (1, 0))

        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['plus'][1]), pos = (0, 1))
        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['minus'][1]), pos = (1, 1))

        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['divide'][1]), pos = (0, 2))
        btn_arithmeticsizer.Add(item = self.FindWindowById(self.btn_arithmetic['multiply'][1]), pos = (1, 2))

        self.btn_arithmeticpanel.SetSizer(btn_arithmeticsizer)

        self.pagesizer.Insert(item = self.btn_arithmeticpanel, before = 3,
                              proportion = 0, flag = wx.ALIGN_CENTER_HORIZONTAL)       


        self.funcpanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)
        self._initSqlFunctions()
        funcsbox = wx.StaticBox(parent = self.funcpanel, id = wx.ID_ANY,
                                label = " %s " % _("Functions"))
        funcsizer = wx.StaticBoxSizer(funcsbox, wx.VERTICAL)
        self.list_func = wx.ListBox(parent = self.funcpanel, id = wx.ID_ANY,
                                    choices = self.sqlFuncs['sqlite'].keys(),
                                    style = wx.LB_SORT)

        funcsizer.Add(item = self.list_func, proportion = 1,
                      flag = wx.EXPAND)

        self.funcpanel.SetSizer(funcsizer)

        self.hsizer.Insert(item =  self.funcpanel, before = 2,
                          proportion = 1, flag = wx.EXPAND) 

        self.list_func.Bind(wx.EVT_LISTBOX,    self.OnAddFunc)
        for key, value in self.btn_arithmetic.iteritems():
            self.FindWindowById(value[1]).Bind(wx.EVT_BUTTON, self.OnAddMark)  
        self.mode.SetSelection(0)
        self.OnMode(None)
        self.text_sql.SetInsertionPoint(self.text_sql.GetLastPosition())

    def OnApply(self, event):
        """Apply button pressed"""

        ret, msg = RunCommand('db.execute',
                             getErrorMsg = True,
                             parent = self,
                             stdin = self.text_sql.GetValue(),
                             input = '-',
                             driver = self.driver,
                             database = self.database)

        if ret != 0 and msg:
            self.statusbar.SetStatusText(_("SQL statement was not applied"), 0)
            #GError(parent = self,
            #       message = _("SQL statement can not be applied.\n\n%s") % msg)
        else:
            self.statusbar.SetStatusText(_("SQL statement applied"), 0)

    def OnClear(self, event):
        """!Clear button pressed"""
        self.text_sql.SetValue(self.initText)

    def OnMode(self, event):
        """!Adjusts builder for chosen mode"""
        if self.mode.GetSelection() == 0:
            self.valuespanel.Hide()
            self.btn_logicpanel.Hide()
            self.btn_arithmeticpanel.Hide()
            self.funcpanel.Hide()
        elif self.mode.GetSelection() == 1:
             self.valuespanel.Show()
             self.btn_logicpanel.Show()
             self.btn_arithmeticpanel.Hide()
             self.funcpanel.Hide()       
        elif self.mode.GetSelection() == 2:
             self.valuespanel.Hide()
             self.btn_logicpanel.Hide()
             self.btn_arithmeticpanel.Show()
             self.funcpanel.Show()
        self.pagesizer.Layout()


    def OnAddFunc(self, event):
        """!Add function to the query"""

        if self.driver == 'dbf':
            GMessage(parent = self,
                     message = _("Dbf driver does not support usage of SQL functions."))
            return

        idx = self.list_func.GetSelections()
        for i in idx:
            func =  self.sqlFuncs['sqlite'][self.list_func.GetString(i)][0]
            self._add(element = 'func', value = func)
        

    def _add(self, element, value):
        """!Add element to the query

        @param element element to add (column, value)
        """
        sqlstr = self.text_sql.GetValue()
        curspos = self.text_sql.GetInsertionPoint()
        newsqlstr = ''

        if element in  ['value', 'mark', 'func'] or \
          (element == 'column' and self.mode.GetSelection() == 2):
            addstr = ' ' + value + ' '
            newsqlstr = sqlstr[:curspos] + addstr + sqlstr[curspos:]
            curspos += len(addstr)
        elif element == 'column':
            if self.mode.GetSelection() == 0: # -> column
                idx1 = sqlstr.lower().find('set') + len('set')
                idx2 = sqlstr.lower().find('where')

                if idx2 >= 0: 
                    colstr = sqlstr[idx1:idx2].strip()
                else:
                    colstr = sqlstr[idx1:].strip()
 
                cols =  [col.split('=')[0].strip() for col in colstr.split(',')]
                if unicode(value) in cols:
                    self.text_sql.SetInsertionPoint(curspos)
                    wx.CallAfter(self.text_sql.SetFocus)           
                    return
                if colstr:
                    colstr += ','
                colstr = ' ' + colstr
                colstr += ' ' + value + '= '
                newsqlstr = sqlstr[:idx1] + colstr
                if idx2 >= 0: 
                    newsqlstr += sqlstr[idx2:]
                curspos = idx1 + len(colstr)

            elif  self.mode.GetSelection() == 1: # -> where
                newsqlstr = ''
                if sqlstr.lower().find('where') < 0:
                    newsqlstr += ' WHERE'
                newsqlstr += ' ' + value
                curspos = self.text_sql.GetLastPosition() + len(newsqlstr)
                newsqlstr = sqlstr + newsqlstr              

        if newsqlstr:
            self.text_sql.SetValue(newsqlstr)

        wx.CallAfter(self.text_sql.SetFocus)
        self.text_sql.SetInsertionPoint(curspos)

    def _initSqlFunctions(self):

        self.sqlFuncs = {}
        # TODO add functions for other drivers
        self.sqlFuncs['sqlite'] = {
                                 'ABS'   : ['ABS()'],
                                 'LENGTH'   : ['LENGTH()'],
                                 'LOWER'   : ['LOWER()'],
                                 'LTRIM'   : ['LTRIM(,)'],
                                 'MAX'   : ['MAX()'],
                                 'MIN'   : ['MIN()'],
                                 'RTRIM'   : ['RTRIM(,)'],
                                 'SUBSTR' : ['SUBSTR (,[,])'],                         
                                 'TRIM' : ['TRIM (,)']                            
                                }

if __name__ == "__main__":
    if len(sys.argv) not in [3, 4]:
        print >>sys.stderr, __doc__
        sys.exit()
    
    if len(sys.argv) == 3:
        layer = 1
    else:
        layer = int(sys.argv[3])

    if sys.argv[1] == 'select':
        sqlBuilder = SQLBuilderSelect
    elif sys.argv[1] == 'update':
        sqlBuilder = SQLBuilderUpdate
    else:
        print >>sys.stderr, __doc__
        sys.exit()

    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

    app = wx.App(0)
    sqlb = sqlBuilder(parent = None, vectmap = sys.argv[2], layer = layer)
    sqlb.Show()
      
    app.MainLoop()
