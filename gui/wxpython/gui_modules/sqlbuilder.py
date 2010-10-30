"""!
@package sqlbuilder.py

@brief GRASS SQL Builder

Classes:
 - SQLFrame

@todo Various updates are required...

Usage:
@code
python sqlbuilder.py vector_map
@endcode

(C) 2007-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Jachym Cepicky <jachym.cepicky gmail.com> (original author)
@author Martin Landa <landa.martin gmail.com>,
@author Hamish Bowman <hamish_b yahoo com>
"""

import os
import sys
import time

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()

import wx

import grass.script as grass

import gcmd
import dbm_base

class SQLFrame(wx.Frame):
    """!SQL Frame class"""
    def __init__(self, parent, title, vectmap, id = wx.ID_ANY,
                 layer = 1, qtype = "select", evtheader = None):
        
        wx.Frame.__init__(self, parent, id, title)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_sql.ico'),
                             wx.BITMAP_TYPE_ICO))
        
        self.parent = parent
        self.evtHeader = evtheader

        #
        # variables
        #
        self.vectmap = vectmap # fullname
        if not "@" in self.vectmap:
            self.vectmap = self.vectmap + "@" + grass.gisenv()['MAPSET']
        self.mapname, self.mapset = self.vectmap.split("@")
        
        # db info
        self.layer = layer
        self.dbInfo = dbm_base.VectorDBInfo(self.vectmap)
        self.tablename = self.dbInfo.GetTable(self.layer)
        self.driver, self.database = self.dbInfo.GetDbSettings(self.layer)
        
        self.qtype = qtype      # type of query: SELECT, UPDATE, DELETE, ...
        self.colvalues = []     # array with unique values in selected column

        # set dialog title
        self.SetTitle(_("GRASS SQL Builder (%(type)s): vector map <%(map)s>") % \
                          { 'type' : qtype.upper(), 'map' : self.vectmap })
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        # statusbar
        self.statusbar = self.CreateStatusBar(number=1)
        self.statusbar.SetStatusText(_("SQL statement not verified"), 0)

        #
        # buttons
        #
        self.btn_clear  = wx.Button(parent = self.panel, id = wx.ID_CLEAR)
        self.btn_clear.SetToolTipString(_("Set SQL statement to default"))
        self.btn_verify = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Verify"))
        self.btn_verify.SetToolTipString(_("Verify SQL statement"))
        # self.btn_help = wx.Button(self.panel, -1, "Help")
        # self.btn_load = wx.Button(self.panel, -1, "Load")
        # self.btn_save = wx.Button(self.panel, -1, "Save")
        self.btn_apply  = wx.Button(parent = self.panel, id = wx.ID_APPLY)
        self.btn_apply.SetToolTipString(_("Apply SQL statement and close the dialog"))
        self.btn_close  = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close the dialog"))
        self.btn_unique = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Get all values"))
        self.btn_unique.Enable(False)
        self.btn_uniquesample = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                          label = _("Get sample"))
        self.btn_uniquesample.Enable(False)
        
        self.btn_lv = { 'is'    : ['=', ],
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
        
        for key, value in self.btn_lv.iteritems():
            btn = wx.Button(parent = self.panel, id = wx.ID_ANY,
                            label = value[0])
            self.btn_lv[key].append(btn.GetId())
        
        #
        # text areas
        #
        self.text_sql = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                    value = '', size = (-1, 75),
                                    style=wx.TE_MULTILINE)
        if self.qtype.lower() == "select":
            self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)
        self.text_sql.SetInsertionPointEnd()
        wx.CallAfter(self.text_sql.SetFocus)

        #
        # list boxes (columns, values)
        #
        self.list_columns = wx.ListBox(parent = self.panel, id = wx.ID_ANY,
                                       choices = self.dbInfo.GetColumns(self.tablename),
                                       style = wx.LB_MULTIPLE)
        self.list_values = wx.ListBox(parent = self.panel, id = wx.ID_ANY,
                                      choices = self.colvalues,
                                      style = wx.LB_MULTIPLE)
        
        self.radio_cv = wx.RadioBox(parent = self.panel, id = wx.ID_ANY,
                                    label = " %s " % _("Add on double-click"),
                                    choices = [_("columns"), _("values")])
        self.radio_cv.SetSelection(1) # default 'values'

        self.close_onapply = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Close dialog on apply"))
        self.close_onapply.SetValue(True)
        
        #
        # bindings
        #
        self.btn_unique.Bind(wx.EVT_BUTTON,       self.OnUniqueValues)
        self.btn_uniquesample.Bind(wx.EVT_BUTTON, self.OnSampleValues)
        
        for key, value in self.btn_lv.iteritems():
            self.FindWindowById(value[1]).Bind(wx.EVT_BUTTON, self.OnAddMark)
        
        self.btn_close.Bind(wx.EVT_BUTTON,       self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON,       self.OnClear)
        self.btn_verify.Bind(wx.EVT_BUTTON,      self.OnVerify)
        self.btn_apply.Bind(wx.EVT_BUTTON,       self.OnApply)

        self.list_columns.Bind(wx.EVT_LISTBOX,   self.OnAddColumn)
        self.list_values.Bind(wx.EVT_LISTBOX,    self.OnAddValue)
        
        self.text_sql.Bind(wx.EVT_TEXT,          self.OnText)
        
        self.Bind(wx.EVT_CLOSE,                  self.OnClose)

        self.__doLayout()

    def __doLayout(self):
        """!Do dialog layout"""
      
        # dbInfo
        databasebox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                   label = " %s " % _("Database connection"))
        databaseboxsizer = wx.StaticBoxSizer(databasebox, wx.VERTICAL)
        databaseboxsizer.Add(item=dbm_base.createDbInfoDesc(self.panel, self.dbInfo, layer = self.layer),
                             proportion=1,
                             flag=wx.EXPAND | wx.ALL,
                             border=3)

        # sql box
        sqlbox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                              label = " %s " % _("Query"))
        sqlboxsizer = wx.StaticBoxSizer(sqlbox, wx.VERTICAL)
        sqlboxsizer.Add(item = self.text_sql, flag = wx.EXPAND)
        
        pagesizer = wx.BoxSizer(wx.VERTICAL)
        
        # buttons
        buttonsizer = wx.FlexGridSizer(cols = 4, hgap = 5, vgap = 5)
        buttonsizer.Add(item = self.btn_clear)
        buttonsizer.Add(item = self.btn_verify)
        # buttonsizer1.Add(self.btn_help,  (0,2))
        # buttonsizer1.Add(self.btn_load,  (0,2))
        # buttonsizer1.Add(self.btn_save,  (0,3))
        buttonsizer.Add(item = self.btn_apply)
        buttonsizer.Add(item = self.btn_close)
        
        buttonsizer2 = wx.GridBagSizer(5, 5)
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['is'][1]), pos = (0,0))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['isnot'][1]), pos = (1,0))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['like'][1]), pos = (2, 0))

        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['gt'][1]), pos = (0, 1))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['ge'][1]), pos = (1, 1))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['or'][1]), pos = (2, 1))

        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['lt'][1]), pos = (0, 2))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['le'][1]), pos = (1, 2))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['not'][1]), pos = (2, 2))

        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['brac'][1]), pos = (0, 3))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['prc'][1]), pos = (1, 3))
        buttonsizer2.Add(item = self.FindWindowById(self.btn_lv['and'][1]), pos = (2, 3))

        buttonsizer3 = wx.BoxSizer(wx.HORIZONTAL)
        buttonsizer3.Add(item = self.btn_uniquesample, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL | wx.RIGHT, border = 5)
        buttonsizer3.Add(item = self.btn_unique, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL)

        radiosizer = wx.BoxSizer(wx.HORIZONTAL)
        radiosizer.Add(item = self.radio_cv, proportion = 1,
                       flag = wx.ALIGN_CENTER_HORIZONTAL | wx.EXPAND, border = 5)
        
        hsizer = wx.BoxSizer(wx.HORIZONTAL)
        
        columnsbox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                  label = " %s " % _("Columns"))
        valuesbox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                 label = " %s " % _("Values"))
        # hsizer1.Add(wx.StaticText(self.panel,-1, "Unique values: "), border=0, proportion=1)
        columnsizer = wx.StaticBoxSizer(columnsbox, wx.VERTICAL)
        valuesizer = wx.StaticBoxSizer(valuesbox, wx.VERTICAL)
        columnsizer.Add(item = self.list_columns, proportion = 1,
                        flag = wx.EXPAND)
        columnsizer.Add(item = radiosizer, proportion = 0,
                        flag = wx.TOP | wx.EXPAND, border = 5)
        valuesizer.Add(item = self.list_values, proportion = 1,
                       flag = wx.EXPAND)
        # self.list_columns.SetMinSize((-1,130))
        # self.list_values.SetMinSize((-1,100))
        valuesizer.Add(item = buttonsizer3, proportion = 0,
                       flag = wx.TOP, border = 5)
        hsizer.Add(item = columnsizer, proportion = 1,
                   flag = wx.EXPAND)
        hsizer.Add(item = valuesizer, proportion = 1,
                   flag = wx.EXPAND)
        
        pagesizer.Add(item = databaseboxsizer,
                      flag = wx.ALL | wx.EXPAND, border = 5)
        pagesizer.Add(item = hsizer, proportion = 1,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        # pagesizer.Add(self.btn_uniqe,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        # pagesizer.Add(self.btn_uniqesample,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        pagesizer.Add(item = buttonsizer2, proportion = 0,
                      flag = wx.ALIGN_CENTER_HORIZONTAL | wx.BOTTOM, border = 5)
        pagesizer.Add(item = sqlboxsizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        pagesizer.Add(item = buttonsizer, proportion = 0,
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        pagesizer.Add(item = self.close_onapply, proportion = 0,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(pagesizer)
        pagesizer.Fit(self.panel)
        
        self.Layout()
        self.SetMinSize((660, 525))
        
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
        
        querystring = "SELECT %s FROM %s" % (column, self.tablename)
        
        data = grass.db_select(table = self.tablename,
                               sql = querystring,
                               database = self.database,
                               driver = self.driver)
        i = 0
        for line in data:
            if justsample and i < 256 or \
               not justsample:
                self.list_values.Append(line.strip())
            else:
                break
            i += 0
        
    def OnSampleValues(self, event):
        """!Get sample values"""
        self.OnUniqueValues(None, True)

    def OnAddColumn(self, event):
        """!Add column name to the query"""
        idx = self.list_columns.GetSelections()
        for i in idx:
            column = self.list_columns.GetString(i)
            self.__Add(element = 'column', value = column)
        
        if not self.btn_uniquesample.IsEnabled():
            self.btn_uniquesample.Enable(True)
            self.btn_unique.Enable(True)
        
    def OnAddValue(self, event):
        """!Add value"""
        idx = self.list_values.GetSelections()[0]
        value = self.list_values.GetString(idx)
        idx = self.list_columns.GetSelections()[0]
        column = self.list_columns.GetString(idx)
        
        ctype = self.dbInfo.GetTableDesc(self.dbInfo.GetTable(self.layer))[column]['type']
        
        if ctype == 'character':
            value = "'%s'" % value
        
        self.__Add(element = 'value', value = value)

    def OnAddMark(self, event):
        """!Add mark"""
        mark = None
        for key, value in self.btn_lv.iteritems():
            if event.GetId() == value[1]:
                mark = value[0]
                break
        
        self.__Add(element = 'mark', value = mark)

    def __Add(self, element, value):
        """!Add element to the query

        @param element element to add (column, value)
        """
        sqlstr = self.text_sql.GetValue()
        newsqlstr = ''
        if element == 'column':
            if self.radio_cv.GetSelection() == 0: # -> column
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
                
                newsqlstr = 'SELECT ' + ','.join(cols) + ' ' + sqlstr[idx2:]
            else: # -> where
                newsqlstr = sqlstr
                if sqlstr.lower().find('where') < 0:
                    newsqlstr += ' WHERE'
                
                newsqlstr += ' ' + value
        
        elif element == 'value':
            newsqlstr = sqlstr + ' ' + value
        elif element == 'mark':
            newsqlstr = sqlstr + ' ' + value
        
        if newsqlstr:
            self.text_sql.SetValue(newsqlstr)

    def GetSQLStatement(self):
        """!Return SQL statement"""
        return self.text_sql.GetValue().strip().replace("\n"," ")
    
    def CloseOnApply(self):
        """!Return True if the dialog will be close on apply"""
        return self.close_onapply.IsChecked()
    
    def OnText(self, event):
        """Query string changed"""
        if len(self.text_sql.GetValue()) > 0:
            self.btn_verify.Enable(True)
        else:
            self.btn_verify.Enable(False)
        
    def OnApply(self, event):
        """Apply button pressed"""
        if self.evtHeader:
            self.evtHeader(event = 'apply')

        if self.close_onapply.IsChecked():
            self.Destroy()
            
        event.Skip()
    
    def OnVerify(self, event):
        """!Verify button pressed"""
        ret, msg = gcmd.RunCommand('db.select',
                                   getErrorMsg = True,
                                   table = self.tablename,
                                   sql = self.text_sql.GetValue(),
                                   flags = 't',
                                   driver = self.driver,
                                   database = self.database)

        if ret != 0 and msg:
            self.statusbar.SetStatusText(_("SQL statement is not valid"), 0)
            wx.MessageBox(parent=self,
                          message=_("SQL statement is not valid.\n\n%s") % msg,
                          caption=_("Warning"), style=wx.OK | wx.ICON_WARNING | wx.CENTRE)
        else:
            self.statusbar.SetStatusText(_("SQL statement is valid"), 0)

    def OnClear(self, event):
        """!Clear button pressed"""
        if self.qtype.lower() == "select":
            self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)
        else:
            self.text_sql.SetValue("")
    
    def OnClose(self, event):
        """!Close button pressed"""
        if self.evtHeader:
            self.evtHeader(event = 'close')
        self.Destroy()

        event.Skip()
        
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print >>sys.stderr, __doc__
        sys.exit()
    
    app = wx.App(0)
    sqlb = SQLFrame(parent = None, title = _('SQL Builder'), vectmap = sys.argv[1])
    sqlb.Show()
    
    app.MainLoop()
