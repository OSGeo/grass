"""
MODULE:    sqlbuilder.py

CLASSES:
    * SQLFrame

PURPOSE:   GRASS SQL Builder

           Usage:
           sqlbuilder.py table_name

AUTHOR(S): GRASS Development Team
           Original author: Jachym Cepicky <jachym.cepicky gmail.com>
           Various updates: Martin Landa <landa.martin gmail.com>

COPYRIGHT: (C) 2007 by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import wx
import os,sys
import time

import grassenv
import gcmd
import globalvar

imagePath = os.path.join(globalvar.ETCWXDIR)
sys.path.append(imagePath)
import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

class SQLFrame(wx.Frame):
    """SQL Frame class"""
    def __init__(self, parent, id, title, vectmap, qtype="select"):

        wx.Frame.__init__(self, parent, id, title)

        self.SetTitle(_("GRASS SQL Builder: %s") % (qtype.upper()))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_sql.ico'), wx.BITMAP_TYPE_ICO))

        #
        # variables
        #
        self.vectmap = vectmap
        if not "@" in self.vectmap:
            self.vectmap = self.vectmap + "@" + grassenv.GetGRASSVariable ("MAPSET")
        self.mapname, self.mapset = self.vectmap.split("@")
        self.layer,self.tablename, self.column, self.database, self.driver =\
                 os.popen("v.db.connect -g map=%s" %\
                (self.vectmap)).readlines()[0].strip().split()

        self.qtype = qtype        # type of the uqery: SELECT, UPDATE, DELETE, ...
        self.column_names = []       # array with column names
        self.columns = {}       # array with colum properties
        self.colvalues = []     # arrya with uniqe values in selected column
        self.heading = ""
        self.parent = parent

        if self.qtype.lower()=="select":
            self.heading = "SELECT * FROM %s WHERE" % self.tablename


        # Init
        self.GetColumns()


        #
        # Buttons
        #
        self.btn_clear = wx.Button(self, -1, "Clear")
        self.btn_verify = wx.Button(self, -1, "Verify")
        #self.btn_help = wx.Button(self, -1, "Help")
        # self.btn_load = wx.Button(self, -1, "Load")
        # self.btn_save = wx.Button(self, -1, "Save")
        self.btn_apply = wx.Button(self, -1, "Apply")
        self.btn_close = wx.Button(self, -1, "Close")
        self.btn_uniqe = wx.Button(self, -1, "Get all values")
        self.btn_uniqesample = wx.Button(self, -1, "Get sample")

        self.btn_is = wx.Button(self, -1, "=")
        self.btn_isnot = wx.Button(self, -1, "!=")
        self.btn_like = wx.Button(self, -1, "LIKE")
        self.btn_gt = wx.Button(self, -1, ">=")
        self.btn_gtis = wx.Button(self, -1, ">")
        self.btn_lt = wx.Button(self, -1, "<=")
        self.btn_ltis = wx.Button(self, -1, "<")
        self.btn_or = wx.Button(self, -1, "OR")
        self.btn_not = wx.Button(self, -1, "NOT")
        self.btn_and = wx.Button(self, -1, "AND")
        self.btn_brackets = wx.Button(self, -1, "()")
        self.btn_prc = wx.Button(self, -1, "%")

        #
        # Text labels
        #
        #self.label_headding = wx.StaticText(self, -1, '')

        #
        # Textareas
        #
        self.text_sql = wx.TextCtrl(self, -1, '', size=(-1,75),style=wx.TE_MULTILINE)

        #
        # List Boxes
        #
        self.list_columns = wx.ListBox(self, -1, wx.DefaultPosition, (-1, -1), self.columns.keys(), wx.LB_MULTIPLE|wx.LB_SORT)
        self.list_values = wx.ListBox(self, -1, wx.DefaultPosition, (-1, -1), self.colvalues, wx.LB_MULTIPLE|wx.LB_SORT)

        #
        # Bindings
        #
        self.btn_uniqe.Bind(wx.EVT_BUTTON, self.GetUniqueValues)
        self.btn_uniqesample.Bind(wx.EVT_BUTTON, self.GetSampleValues)
        self.btn_is.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_isnot.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_like.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_gt.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_gtis.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_or.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_lt.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_ltis.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_not.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_brackets.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_prc.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_and.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btn_verify.Bind(wx.EVT_BUTTON, self.OnVerify)
        self.btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)

        self.list_columns.Bind(wx.EVT_LISTBOX, self.AddColumnName)
        self.list_values.Bind(wx.EVT_LISTBOX, self.AddValue)

        self.__doLayout()

    def __doLayout(self):
        databasebox = wx.StaticBox(self, -1, "Database connection")
        databaseboxsizer = wx.StaticBoxSizer(databasebox,wx.VERTICAL)
        dbstr = "Database: %s\n" % (self.database)
        dbstr += "Driver: %s\n" % (self.driver)
        dbstr += "Table: %s" % (self.tablename)
        databaseboxsizer.Add(wx.StaticText(self,-1,dbstr), flag=wx.EXPAND)

        sqlbox = wx.StaticBox(self, -1, "%s" % self.heading)
        sqlboxsizer = wx.StaticBoxSizer(sqlbox,wx.VERTICAL)
        sqlboxsizer.Add(self.text_sql, flag=wx.EXPAND)


        pagesizer = wx.BoxSizer(wx.VERTICAL)

        buttonsizer1 = wx.GridBagSizer(2,2)
        buttonsizer1.Add(self.btn_clear, (0,0))
        buttonsizer1.Add(self.btn_verify, (0,1))
        #buttonsizer1.Add(self.btn_help,  (0,2))
        #buttonsizer1.Add(self.btn_load,  (0,2))
        # buttonsizer1.Add(self.btn_save,  (0,3))
        buttonsizer1.Add(self.btn_apply, (0,2))

        buttonsizer2 = wx.GridBagSizer(2, 2)
        buttonsizer2.Add(self.btn_is, (0,0))
        buttonsizer2.Add(self.btn_isnot, (1,0))
        buttonsizer2.Add(self.btn_like, (2, 0))

        buttonsizer2.Add(self.btn_gt, (0, 1))
        buttonsizer2.Add(self.btn_gtis, (1, 1))
        buttonsizer2.Add(self.btn_or, (2, 1))

        buttonsizer2.Add(self.btn_lt, (0, 2))
        buttonsizer2.Add(self.btn_ltis, (1, 2))
        buttonsizer2.Add(self.btn_not, (2, 2))

        buttonsizer2.Add(self.btn_brackets, (0, 3))
        buttonsizer2.Add(self.btn_prc, (1, 3))
        buttonsizer2.Add(self.btn_and, (2, 3))

        buttonsizer3 = wx.GridSizer(4, 3, 3, 3)
        buttonsizer3.Add(self.btn_apply,0,wx.RIGHT,5)
        buttonsizer3.Add(self.btn_close,0,wx.RIGHT,5)

        buttonsizer4 = wx.BoxSizer(wx.HORIZONTAL)
        buttonsizer4.Add(self.btn_uniqesample,0,flag=wx.ALIGN_CENTER_HORIZONTAL,border=5)
        buttonsizer4.Add(self.btn_uniqe,0,flag=wx.ALIGN_CENTER_HORIZONTAL,border=5)

        hsizer1 = wx.BoxSizer(wx.HORIZONTAL)
        #hsizer2 = wx.BoxSizer(wx.HORIZONTAL)

        columnsbox = wx.StaticBox(self,-1,"Columns: ")
        valuesbox = wx.StaticBox(self,-1,"Values: ")
        #hsizer1.Add(wx.StaticText(self,-1, "Unique values: "), border=0, proportion=1)
        columnsizer = wx.StaticBoxSizer(columnsbox,wx.VERTICAL)
        valuesizer = wx.StaticBoxSizer(valuesbox,wx.VERTICAL)
        columnsizer.Add(self.list_columns,  flag=wx.EXPAND,)
        valuesizer.Add(self.list_values,  flag=wx.EXPAND)
        self.list_columns.SetMinSize((-1,130))
        self.list_values.SetMinSize((-1,100))
        valuesizer.Add(buttonsizer4)
        hsizer1.Add(columnsizer,border=0, proportion=1)
        hsizer1.Add(valuesizer,border=0, proportion=1)

        pagesizer.Add(databaseboxsizer,flag=wx.EXPAND,border=5)
        pagesizer.Add(hsizer1, 1,flag=wx.EXPAND,border=5)
        #pagesizer.Add(self.btn_uniqe,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        #pagesizer.Add(self.btn_uniqesample,0,wx.ALIGN_LEFT|wx.TOP,border=5)
        pagesizer.Add(buttonsizer2, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.TOP, border=5)
        pagesizer.Add(sqlboxsizer, flag=wx.EXPAND,border=5)
        pagesizer.Add(buttonsizer1, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.TOP, border=5)
        pagesizer.Add(buttonsizer3, proportion=0, flag=wx.TOP, border=5)
        self.SetAutoLayout(True)
        self.SetSizer(pagesizer)
        pagesizer.Fit(self)
        #pagesizer.SetSizeHints(self)
        self.Layout()
        self.Show(True)

    def GetColumns(self):
        """Get columns"""
        dbDescribe = gcmd.Command(['db.describe',
                                   '-c', '--q',
                                   'table=%s' % self.tablename])

        # skip ncols and nrows lines
        for line in dbDescribe.ReadStdOutput()[2:]:
            num, name, ctype, length = line.strip().split(":")
            name.strip()
            #self.columns_names.append(name)
            self.columns[name] = {'type' : ctype}


    def GetUniqueValues(self,event,justsample=False):
        vals = []
        try:
            idx = self.list_columns.GetSelections()[0]
        except:
            return
        self.list_values.Clear()
        column = self.list_columns.GetString(idx)
        i = 0
        for line in os.popen("""db.select -c database=%s driver=%s sql="SELECT %s FROM %s" """ %\
                (self.database,self.driver,column,self.tablename)):
                if justsample and i < 256 or \
                   not justsample:
                    self.list_values.Insert(line.strip(),0)
                else:
                    break
                i += 1

    def GetSampleValues(self,event):
        self.GetUniqueValues(None,True)

    def AddColumnName(self,event):
        idx = self.list_columns.GetSelections()[0]
        column = self.list_columns.GetString(idx)
        self.__addSomething(column)

    def AddValue(self,event):
        idx = self.list_values.GetSelections()[0]
        value = self.list_values.GetString(idx)
        idx = self.list_columns.GetSelections()[0]
        column = self.list_columns.GetString(idx)

        if self.columns[column]['type'].lower().find("chara") > -1:
            value = "'%s'" % value
        self.__addSomething(value)

    def AddMark(self,event):


        if event.GetId() == self.btn_is.GetId(): mark = "="
        elif event.GetId() == self.btn_isnot.GetId(): mark = "!="
        elif event.GetId() == self.btn_like.GetId(): mark = "LIKE"
        elif event.GetId() == self.btn_gt.GetId(): mark = ">"
        elif event.GetId() == self.btn_gtis.GetId(): mark = ">="
        elif event.GetId() == self.btn_lt.GetId(): mark = "<"
        elif event.GetId() == self.btn_ltis.GetId(): mark =  "<="
        elif event.GetId() == self.btn_or.GetId(): mark =  "OR"
        elif event.GetId() == self.btn_not.GetId(): mark = "NOT"
        elif event.GetId() == self.btn_and.GetId(): mark = "AND"
        elif event.GetId() == self.btn_brackets.GetId(): mark = "()"
        elif event.GetId() == self.btn_prc.GetId(): mark = "%"
        self.__addSomething(mark)


    def __addSomething(self,what):
        sqlstr = self.text_sql.GetValue()
        newsqlstr = ''
        position = self.text_sql.GetPosition()[0]
        selection = self.text_sql.GetSelection()

        newsqlstr = sqlstr[:position]
        try:
            if newsqlstr[-1] != " ":
                newsqlstr += " "
        except:
            pass
        newsqlstr += what
        newsqlstr += " "+sqlstr[position:]

        self.text_sql.SetValue(newsqlstr)
        self.text_sql.SetInsertionPoint(position)

    def OnApply(self,event):
        if self.parent:
            try:
                self.parent.text_query.SetValue= self.text_sql.GetValue().strip().replace("\n"," ")
            except:
                pass
    def OnVerify(self,event):
        if self.text_sql.GetValue():
            if os.system("""db.select -t driver=%s database=%s sql="SELECT * FROM %s WHERE %s" """ % \
                    (self.driver, self.database,self.tablename,
                        self.text_sql.GetValue().strip().replace("\n"," "))):
                # FIXME: LOG
                # print self.text_sql.GetValue().strip().replace("\n"," "), "not correct!"
                pass

    def OnClear(self, event):
        self.text_sql.SetValue("")

    def OnClose(self,event):
        self.Destroy()

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print >>sys.stderr, __doc__
        sys.exit()

    app = wx.App(0)
    sqlb = SQLFrame(None, -1, 'SQL Builder',sys.argv[1])
    app.MainLoop()


