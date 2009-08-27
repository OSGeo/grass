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
grass.set_fatal_exit(False)

import gcmd
import dbm_base

class SQLFrame(wx.Frame):
    """!SQL Frame class"""
    def __init__(self, parent, title, vectmap, id = wx.ID_ANY,
                 layer = 1, qtype = "select"):
        
        wx.Frame.__init__(self, parent, id, title)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_sql.ico'),
                             wx.BITMAP_TYPE_ICO))
        
        self.parent = parent

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
        
        self.qtype = qtype      # type of the uqery: SELECT, UPDATE, DELETE, ...
        self.colvalues = []     # array with unique values in selected column

        # set dialog title
        self.SetTitle(_("GRASS SQL Builder (%s): vector map <%s>") % \
                          (qtype.upper(), self.vectmap))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        #
        # buttons
        #
        self.btn_clear  = wx.Button(parent = self.panel, id = wx.ID_CLEAR)
        self.btn_verify = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Verify"))
        # self.btn_help = wx.Button(self.panel, -1, "Help")
        # self.btn_load = wx.Button(self.panel, -1, "Load")
        # self.btn_save = wx.Button(self.panel, -1, "Save")
        self.btn_apply  = wx.Button(parent = self.panel, id = wx.ID_APPLY)
        self.btn_close  = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_unique = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Get all values"))
        self.btn_unique.Enable(False)
        self.btn_uniquesample = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                          label = _("Get sample"))
        self.btn_uniquesample.Enable(False)
        self.btn_is    = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "=")
        self.btn_isnot = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "!=")
        self.btn_like  = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "LIKE")
        self.btn_gt    = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">")
        self.btn_gtis  = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">=")
        self.btn_lt    = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "<")
        self.btn_ltis  = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "<=")
        self.btn_or    = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "OR")
        self.btn_not   = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "NOT")
        self.btn_and   = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "AND")
        self.btn_brackets = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "()")
        self.btn_prc   = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "%")
        
        #
        # Text labels
        #
        # self.label_headding = wx.StaticText(self.panel, -1, '')
        
        #
        # Textareas
        #
        self.text_sql = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                    value = '', size = (-1, 75),
                                    style=wx.TE_MULTILINE)
        if self.qtype.lower() == "select":
            self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)
        
        #
        # List Boxes
        #
        self.list_columns = wx.ListBox(parent = self.panel, id = wx.ID_ANY,
                                       choices = self.dbInfo.GetColumns(self.tablename),
                                       style = wx.LB_MULTIPLE)
        self.list_values = wx.ListBox(parent = self.panel, id = wx.ID_ANY,
                                      choices = self.colvalues,
                                      style = wx.LB_MULTIPLE)
        
        #
        # Bindings
        #
        self.btn_unique.Bind(wx.EVT_BUTTON,       self.GetUniqueValues)
        self.btn_uniquesample.Bind(wx.EVT_BUTTON, self.GetSampleValues)
        self.btn_is.Bind(wx.EVT_BUTTON,          self.AddMark)
        self.btn_isnot.Bind(wx.EVT_BUTTON,       self.AddMark)
        self.btn_like.Bind(wx.EVT_BUTTON,        self.AddMark)
        self.btn_gt.Bind(wx.EVT_BUTTON,          self.AddMark)
        self.btn_gtis.Bind(wx.EVT_BUTTON,        self.AddMark)
        self.btn_or.Bind(wx.EVT_BUTTON,          self.AddMark)
        self.btn_lt.Bind(wx.EVT_BUTTON,          self.AddMark)
        self.btn_ltis.Bind(wx.EVT_BUTTON,        self.AddMark)
        self.btn_not.Bind(wx.EVT_BUTTON,         self.AddMark)
        self.btn_brackets.Bind(wx.EVT_BUTTON,    self.AddMark)
        self.btn_prc.Bind(wx.EVT_BUTTON,         self.AddMark)
        self.btn_and.Bind(wx.EVT_BUTTON,         self.AddMark)
        self.btn_close.Bind(wx.EVT_BUTTON,       self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON,       self.OnClear)
        self.btn_verify.Bind(wx.EVT_BUTTON,      self.OnVerify)
        self.btn_apply.Bind(wx.EVT_BUTTON,       self.OnApply)

        self.list_columns.Bind(wx.EVT_LISTBOX,   self.AddColumnName)
        self.list_values.Bind(wx.EVT_LISTBOX,    self.AddValue)
        
        self.text_sql.Bind(wx.EVT_TEXT,          self.OnText)
        
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
        buttonsizer2.Add(item = self.btn_is, pos = (0,0))
        buttonsizer2.Add(item = self.btn_isnot, pos = (1,0))
        buttonsizer2.Add(item = self.btn_like, pos = (2, 0))

        buttonsizer2.Add(item = self.btn_gt, pos = (0, 1))
        buttonsizer2.Add(item = self.btn_gtis, pos = (1, 1))
        buttonsizer2.Add(item = self.btn_or, pos = (2, 1))

        buttonsizer2.Add(item = self.btn_lt, pos = (0, 2))
        buttonsizer2.Add(item = self.btn_ltis, pos = (1, 2))
        buttonsizer2.Add(item = self.btn_not, pos = (2, 2))

        buttonsizer2.Add(item = self.btn_brackets, pos = (0, 3))
        buttonsizer2.Add(item = self.btn_prc, pos = (1, 3))
        buttonsizer2.Add(item = self.btn_and, pos = (2, 3))

        buttonsizer4 = wx.BoxSizer(wx.HORIZONTAL)
        buttonsizer4.Add(item = self.btn_uniquesample, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL | wx.RIGHT, border = 5)
        buttonsizer4.Add(item = self.btn_unique, proportion = 0,
                         flag = wx.ALIGN_CENTER_HORIZONTAL)
        
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
        valuesizer.Add(item = self.list_values, proportion = 1,
                       flag = wx.EXPAND)
        # self.list_columns.SetMinSize((-1,130))
        # self.list_values.SetMinSize((-1,100))
        valuesizer.Add(item = buttonsizer4, proportion = 0,
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
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(pagesizer)
        pagesizer.Fit(self.panel)
        
        self.Layout()
        self.SetMinSize((660, 480))
        
    def GetUniqueValues(self, event, justsample = False):
        """!Get unique values"""
        vals = []
        try:
            idx = self.list_columns.GetSelections()[0]
            column = self.list_columns.GetString(idx)
        except:
            self.list_values.Clear()
            return
        
        self.list_values.Clear()
        
        i = 0
        querystring = "SELECT %s FROM %s" % (column, self.tablename)
        
        data = grass.db_select(table = self.tablename,
                               sql = querystring,
                               database = self.database,
                               driver = self.driver)
        
        for line in data:
            if justsample and i < 256 or \
               not justsample:
                self.list_values.Append(line.strip())
            else:
                break
            i += 1
    
    def GetSampleValues(self, event):
        """!Get sample values"""
        self.GetUniqueValues(None, True)

    def AddColumnName(self, event):
        """!Add column name to the query"""
        idx = self.list_columns.GetSelections()
        for i in idx:
            column = self.list_columns.GetString(i)
            self.__Add(element = 'column', value = column)
        
        if not self.btn_uniquesample.IsEnabled():
            self.btn_uniquesample.Enable()
            self.btn_unique.Enable()
        
    def AddValue(self,event):
        """!Add value"""
        idx = self.list_values.GetSelections()[0]
        value = self.list_values.GetString(idx)
        idx = self.list_columns.GetSelections()[0]
        column = self.list_columns.GetString(idx)
        
        if self.columns[column]['type'].lower().find("chara") > -1:
            value = "'%s'" % value
        self.__Add(value)

    def AddMark(self,event):
        """!Add mark"""
        if event.GetId() == self.btn_is.GetId():
            mark = "="
        elif event.GetId() == self.btn_isnot.GetId():
            mark = "!="
        elif event.GetId() == self.btn_like.GetId():
            mark = "LIKE"
        elif event.GetId() == self.btn_gt.GetId():
            mark = ">"
        elif event.GetId() == self.btn_gtis.GetId():
            mark = ">="
        elif event.GetId() == self.btn_lt.GetId():
            mark = "<"
        elif event.GetId() == self.btn_ltis.GetId():
            mark =  "<="
        elif event.GetId() == self.btn_or.GetId():
            mark =  "OR"
        elif event.GetId() == self.btn_not.GetId():
            mark = "NOT"
        elif event.GetId() == self.btn_and.GetId():
            mark = "AND"
        elif event.GetId() == self.btn_brackets.GetId():
            mark = "()"
        elif event.GetId() == self.btn_prc.GetId():
            mark = "%"
        self.__Add(mark)


    def __Add(self, element, value):
        """!Add element to the query

        @param what what to add
        """
        sqlstr = self.text_sql.GetValue()
        newsqlstr = ''
        if element == 'column':
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
            
        if newsqlstr:
            self.text_sql.SetValue(newsqlstr)
        
    def OnText(self, event):
        """Query string changed"""
        if len(self.text_sql.GetValue()) > 0:
            self.btn_verify.Enable(True)
        else:
            self.btn_verify.Enable(False)
        
    def OnApply(self, event):
        """Apply button pressed"""
        if self.parent:
            try:
                self.parent.text_query.SetValue= self.text_sql.GetValue().strip().replace("\n"," ")
            except:
                pass
        
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
            wx.MessageBox(parent=self,
                          message=_("SQL statement is not valid.\n\n%s") % msg,
                          caption=_("Warning"), style=wx.OK | wx.ICON_WARNING | wx.CENTRE)
        
    def OnClear(self, event):
        """!Clear button pressed"""
        if self.qtype.lower() == "select":
            self.text_sql.SetValue("SELECT * FROM %s" % self.tablename)
        else:
            self.text_sql.SetValue("")
    
    def OnClose(self, event):
        """!Close button pressed"""
        self.Destroy()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print >>sys.stderr, __doc__
        sys.exit()
    
    app = wx.App(0)
    sqlb = SQLFrame(parent = None, title = _('SQL Builder'), vectmap = sys.argv[1])
    sqlb.Show()
    
    app.MainLoop()
