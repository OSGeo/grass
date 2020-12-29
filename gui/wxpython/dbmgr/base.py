"""
@package dbmgr.base

@brief GRASS Attribute Table Manager base classes

List of classes:
 - base::Log
 - base::VirtualAttributeList
 - base::DbMgrBase
 - base::DbMgrNotebookBase
 - base::DbMgrBrowsePage
 - base::DbMgrTablesPage
 - base::DbMgrLayersPage
 - base::TableListCtrl
 - base::LayerListCtrl
 - base::LayerBook
 - base::FieldStatistics

.. todo::
    Implement giface class

(C) 2007-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Jachym Cepicky <jachym.cepicky gmail.com>
@author Martin Landa <landa.martin gmail.com>
@author Refactoring by Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import sys
import os
import locale
import tempfile
import copy
import math
import functools

from core import globalvar
import wx
import wx.lib.mixins.listctrl as listmix

if globalvar.wxPythonPhoenix:
    try:
        import agw.flatnotebook as FN
    except ImportError: # if it's not there locally, try the wxPython lib.
        import wx.lib.agw.flatnotebook as FN
else:
    import wx.lib.flatnotebook as FN
import wx.lib.scrolledpanel as scrolled

import grass.script as grass
from grass.script.utils import decode

from dbmgr.sqlbuilder import SQLBuilderSelect, SQLBuilderUpdate
from core.gcmd import RunCommand, GException, GError, GMessage, GWarning
from core.utils import ListOfCatsToRange
from gui_core.dialogs import CreateNewVector
from dbmgr.vinfo import VectorDBInfo, GetUnicodeValue, CreateDbInfoDesc, GetDbEncoding
from core.debug import Debug
from dbmgr.dialogs import ModifyTableRecord, AddColumnDialog
from core.settings import UserSettings
from gui_core.wrap import Button, CheckBox, ComboBox, ListCtrl, Menu, \
    NewId, SpinCtrl, StaticBox, StaticText, TextCtrl
from core.utils import cmp

if sys.version_info.major >= 3:
    unicode = str


class Log:
    """The log output SQL is redirected to the status bar of the
    containing frame.
    """

    def __init__(self, parent):
        self.parent = parent

    def write(self, text_string):
        """Update status bar"""
        if self.parent:
            self.parent.SetStatusText(text_string.strip())


class VirtualAttributeList(ListCtrl,
                           listmix.ListCtrlAutoWidthMixin,
                           listmix.ColumnSorterMixin):
    """Support virtual list class for Attribute Table Manager (browse page)
    """

    def __init__(self, parent, log, dbMgrData, layer, pages):
        # initialize variables
        self.parent = parent
        self.log = log
        self.dbMgrData = dbMgrData
        self.mapDBInfo = self.dbMgrData['mapDBInfo']
        self.layer = layer
        self.pages = pages

        self.fieldCalc = None
        self.fieldStats = None
        self.columns = {}  # <- LoadData()

        self.sqlFilter = {}

        ListCtrl.__init__(self, parent=parent, id=wx.ID_ANY,
                             style=wx.LC_REPORT | wx.LC_HRULES | wx.LC_VRULES |
                             wx.LC_VIRTUAL | wx.LC_SORT_ASCENDING)

        try:
            keyColumn = self.LoadData(layer)
        except GException as e:
            GError(parent=self,
                   message=e.value)
            return

        self.EnableAlternateRowColours()
        self.il = wx.ImageList(16, 16)
        self.sm_up = self.il.Add(
            wx.ArtProvider.GetBitmap(
                wx.ART_GO_UP, wx.ART_TOOLBAR, (16, 16)))
        self.sm_dn = self.il.Add(
            wx.ArtProvider.GetBitmap(
                wx.ART_GO_DOWN, wx.ART_TOOLBAR, (16, 16)))
        self.SetImageList(self.il, wx.IMAGE_LIST_SMALL)

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.ColumnSorterMixin.__init__(self, len(self.columns))

        # sort item by category (id)
        if keyColumn > -1:
            self.SortListItems(col=keyColumn, ascending=True)
        elif keyColumn:
            self.SortListItems(col=0, ascending=True)

        # events
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_DESELECTED, self.OnItemDeselected)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColumnSort)
        self.Bind(wx.EVT_LIST_COL_RIGHT_CLICK, self.OnColumnMenu)

    def Update(self, mapDBInfo=None):
        """Update list according new mapDBInfo description"""
        if mapDBInfo:
            self.mapDBInfo = mapDBInfo
            self.LoadData(self.layer)
        else:
            self.LoadData(self.layer, **self.sqlFilter)

    def LoadData(self, layer, columns=None, where=None, sql=None):
        """Load data into list

        :param layer: layer number
        :param columns: list of columns for output (-> v.db.select)
        :param where: where statement (-> v.db.select)
        :param sql: full sql statement (-> db.select)

        :return: id of key column
        :return: -1 if key column is not displayed
        """
        self.log.write(_("Loading data..."))

        tableName = self.mapDBInfo.layers[layer]['table']
        keyColumn = self.mapDBInfo.layers[layer]['key']
        try:
            self.columns = self.mapDBInfo.tables[tableName]
        except KeyError:
            raise GException(_("Attribute table <%s> not found. "
                               "For creating the table switch to "
                               "'Manage layers' tab.") % tableName)

        if not columns:
            columns = self.mapDBInfo.GetColumns(tableName)
        else:
            all = self.mapDBInfo.GetColumns(tableName)
            for col in columns:
                if col not in all:
                    GError(parent=self,
                           message=_("Column <%(column)s> not found in "
                                     "in the table <%(table)s>.") %
                           {'column': col, 'table': tableName})
                    return

        try:
            # for maps connected via v.external
            keyId = columns.index(keyColumn)
        except:
            keyId = -1

        # read data
        # FIXME: Max. number of rows, while the GUI is still usable

        # stdout can be very large, do not use PIPE, redirect to temp file
        # TODO: more effective way should be implemented...

        # split on field sep breaks if varchar() column contains the
        # values, so while sticking with ASCII we make it something
        # highly unlikely to exist naturally.
        fs = '{_sep_}'

        outFile = tempfile.NamedTemporaryFile(mode='w+b')

        cmdParams = dict(quiet=True,
                         parent=self,
                         flags='c',
                         separator=fs)

        if sql:
            cmdParams.update(dict(sql=sql,
                                  output=outFile.name,
                                  overwrite=True))
            ret = RunCommand('db.select',
                             **cmdParams)
            self.sqlFilter = {"sql": sql}
        else:
            cmdParams.update(dict(map=self.mapDBInfo.map,
                                  layer=layer,
                                  where=where,
                                  stdout=outFile))

            self.sqlFilter = {"where": where}

            if columns:
                cmdParams.update(dict(columns=','.join(columns)))

            ret = RunCommand('v.db.select',
                             **cmdParams)

        # These two should probably be passed to init more cleanly
        # setting the numbers of items = number of elements in the dictionary
        self.itemDataMap = {}
        self.itemIndexMap = []
        self.itemCatsMap = {}

        self.DeleteAllItems()

        # self.ClearAll()
        for i in range(self.GetColumnCount()):
            self.DeleteColumn(0)

        i = 0
        info = wx.ListItem()
        if globalvar.wxPythonPhoenix:
            info.Mask = wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE | wx.LIST_MASK_FORMAT
            info.Image = -1
            info.Format = 0
        else:
            info.m_mask = wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE | wx.LIST_MASK_FORMAT
            info.m_image = -1
            info.m_format = 0
        for column in columns:
            if globalvar.wxPythonPhoenix:
                info.Text = column
                self.InsertColumn(i, info)
            else:
                info.m_text = column
                self.InsertColumnInfo(i, info)
            i += 1
            if i >= 256:
                    self.log.write(_("Can display only 256 columns."))

        i = 0
        outFile.seek(0)

        enc = GetDbEncoding()
        first_wrong_encoding = True
        while True:
            # os.linesep doesn't work here (MSYS)
            # not sure what the replace is for?
            # but we need strip to get rid of the ending newline
            # which on windows leaves \r in a last empty attribute table cell
            # and causes error
            try:
                record = decode(outFile.readline(), encoding=enc).strip().replace('\n', '')
            except UnicodeDecodeError as e:
                record = outFile.readline().decode(encoding=enc, errors="replace").strip().replace('\n', '')
                if first_wrong_encoding:
                    first_wrong_encoding = False
                    GWarning(parent=self,
                             message=_("Incorrect encoding {enc} used. Set encoding in GUI Settings"
                                       " or set GRASS_DB_ENCODING variable.").format(enc=enc))

            if not record:
                break

            record = record.split(fs)
            if len(columns) != len(record):
                # Assuming there will be always at least one.
                last = record[-1]
                show_max = 3
                if len(record) > show_max:
                    record = record[:show_max]
                # TODO: The real fix here is to use JSON output from v.db.select or
                # proper CSV output and real CSV reader here (Python csv and json packages).
                raise GException(
                    _(
                        "Unable to read the table <{table}> from the database due"
                        " to seemingly inconsistent number of columns in the data transfer."
                        " Check row: {row}..."
                        " Likely, a newline character is present in the attribute value starting with: '{value}'"
                        " Use the v.db.select module to investigate."
                    ).format(table=tableName, row=" | ".join(record), value=last)
                )
                self.columns = {}  # because of IsEmpty method
                return None

            self.AddDataRow(i, record, columns, keyId)

            i += 1
            if i >= 100000:
                self.log.write(_("Viewing limit: 100000 records."))
                break

        self.SetItemCount(i)

        if where:
            item = -1
            while True:
                item = self.GetNextItem(item)
                if item == -1:
                    break
                self.SetItemState(
                    item,
                    wx.LIST_STATE_SELECTED,
                    wx.LIST_STATE_SELECTED)

        i = 0
        for col in columns:
            width = self.columns[col]['length'] * 6  # FIXME
            if width < 60:
                width = 60
            if width > 300:
                width = 300
            self.SetColumnWidth(col=i, width=width)
            i += 1

        self.SendSizeEvent()

        self.log.write(_("Number of loaded records: %d") %
                       self.GetItemCount())

        return keyId

    def AddDataRow(self, i, record, columns, keyId):
        """Add row to the data list"""
        self.itemDataMap[i] = []
        keyColumn = self.mapDBInfo.layers[self.layer]['key']
        j = 0
        cat = None

        if keyColumn == 'OGC_FID':
            self.itemDataMap[i].append(i + 1)
            j += 1
            cat = i + 1

        for value in record:
            if self.columns[columns[j]]['ctype'] != str:
                try:
                    # casting disabled (2009/03)
                    # self.itemDataMap[i].append(self.columns[columns[j]]['ctype'](value))
                    self.itemDataMap[i].append(value)
                except ValueError:
                    self.itemDataMap[i].append(_('Unknown value'))
            else:
                # encode string values
                try:
                    self.itemDataMap[i].append(GetUnicodeValue(value))
                except UnicodeDecodeError:
                    self.itemDataMap[i].append(_("Unable to decode value. "
                                                 "Set encoding in GUI preferences ('Attributes')."))

            if not cat and keyId > -1 and keyId == j:
                try:
                    cat = self.columns[columns[j]]['ctype'](value)
                except ValueError as e:
                    cat = -1
                    GError(
                        parent=self,
                        message=_(
                            "Error loading attribute data. "
                            "Record number: %(rec)d. Unable to convert value '%(val)s' in "
                            "key column (%(key)s) to integer.\n\n"
                            "Details: %(detail)s") % {
                            'rec': i + 1,
                            'val': value,
                            'key': keyColumn,
                            'detail': e})
            j += 1

        self.itemIndexMap.append(i)
        if keyId > -1:  # load cats only when LoadData() is called first time
            self.itemCatsMap[i] = cat

    def OnItemSelected(self, event):
        """Item selected. Add item to selected cats..."""
        #         cat = int(self.GetItemText(event.m_itemIndex))
        #         if cat not in self.selectedCats:
        #             self.selectedCats.append(cat)
        #             self.selectedCats.sort()

        event.Skip()

    def OnItemDeselected(self, event):
        """Item deselected. Remove item from selected cats..."""
        #         cat = int(self.GetItemText(event.m_itemIndex))
        #         if cat in self.selectedCats:
        #             self.selectedCats.remove(cat)
        #             self.selectedCats.sort()

        event.Skip()

    def GetSelectedItems(self):
        """Return list of selected items (category numbers)"""
        cats = []
        item = self.GetFirstSelected()
        while item != -1:
            cats.append(self.GetItemText(item))
            item = self.GetNextSelected(item)

        return cats

    def GetItems(self):
        """Return list of items (category numbers)"""
        cats = []
        for item in range(self.GetItemCount()):
            cats.append(self.GetItemText(item))

        return cats

    def GetColumnText(self, index, col):
        """Return column text"""
        item = self.GetItem(index, col)
        return item.GetText()

    def GetListCtrl(self):
        """Returt list"""
        return self

    def OnGetItemText(self, item, col):
        """Get item text"""
        index = self.itemIndexMap[item]
        s = self.itemDataMap[index][col]
        return str(s)

    def OnColumnMenu(self, event):
        """Column heading right mouse button -> pop-up menu"""
        self._col = event.GetColumn()

        popupMenu = Menu()

        if not hasattr(self, "popupID"):
            self.popupId = {'sortAsc': NewId(),
                            'sortDesc': NewId(),
                            'calculate': NewId(),
                            'area': NewId(),
                            'length': NewId(),
                            'compact': NewId(),
                            'fractal': NewId(),
                            'perimeter': NewId(),
                            'ncats': NewId(),
                            'slope': NewId(),
                            'lsin': NewId(),
                            'lazimuth': NewId(),
                            'calculator': NewId(),
                            'stats': NewId()}

        popupMenu.Append(self.popupId['sortAsc'], _("Sort ascending"))
        popupMenu.Append(self.popupId['sortDesc'], _("Sort descending"))
        popupMenu.AppendSeparator()
        subMenu = Menu()
        popupMenu.AppendMenu(self.popupId['calculate'], _(
            "Calculate (only numeric columns)"), subMenu)
        popupMenu.Append(
            self.popupId['calculator'],
            _("Field calculator"))
        popupMenu.AppendSeparator()
        popupMenu.Append(self.popupId['stats'], _("Statistics"))

        if not self.pages['manageTable']:
            popupMenu.AppendSeparator()
            self.popupId['addCol'] = NewId()
            popupMenu.Append(self.popupId['addCol'], _("Add column"))
            if not self.dbMgrData['editable']:
                popupMenu.Enable(self.popupId['addCol'], False)

        if not self.dbMgrData['editable']:
            popupMenu.Enable(self.popupId['calculator'], False)

        if not self.dbMgrData['editable'] or self.columns[
                self.GetColumn(self._col).GetText()]['ctype'] not in (
                int, float):
            popupMenu.Enable(self.popupId['calculate'], False)

        subMenu.Append(self.popupId['area'], _("Area size"))
        subMenu.Append(self.popupId['length'], _("Line length"))
        subMenu.Append(
            self.popupId['compact'],
            _("Compactness of an area"))
        subMenu.Append(self.popupId['fractal'], _(
            "Fractal dimension of boundary defining a polygon"))
        subMenu.Append(
            self.popupId['perimeter'],
            _("Perimeter length of an area"))
        subMenu.Append(self.popupId['ncats'], _(
            "Number of features for each category"))
        subMenu.Append(
            self.popupId['slope'],
            _("Slope steepness of 3D line"))
        subMenu.Append(self.popupId['lsin'], _("Line sinuousity"))
        subMenu.Append(self.popupId['lazimuth'], _("Line azimuth"))

        self.Bind(
            wx.EVT_MENU,
            self.OnColumnSortAsc,
            id=self.popupId['sortAsc'])
        self.Bind(
            wx.EVT_MENU,
            self.OnColumnSortDesc,
            id=self.popupId['sortDesc'])
        self.Bind(
            wx.EVT_MENU,
            self.OnFieldCalculator,
            id=self.popupId['calculator'])
        self.Bind(
            wx.EVT_MENU,
            self.OnFieldStatistics,
            id=self.popupId['stats'])
        if not self.pages['manageTable']:
            self.Bind(wx.EVT_MENU, self.OnAddColumn, id=self.popupId['addCol'])

        for id in (
                self.popupId['area'],
                self.popupId['length'],
                self.popupId['compact'],
                self.popupId['fractal'],
                self.popupId['perimeter'],
                self.popupId['ncats'],
                self.popupId['slope'],
                self.popupId['lsin'],
                self.popupId['lazimuth']):
            self.Bind(wx.EVT_MENU, self.OnColumnCompute, id=id)

        self.PopupMenu(popupMenu)
        popupMenu.Destroy()

    def OnColumnSort(self, event):
        """Column heading left mouse button -> sorting"""
        self._col = event.GetColumn()

        self.ColumnSort()

        event.Skip()

    def OnColumnSortAsc(self, event):
        """Sort values of selected column (ascending)"""
        self.SortListItems(col=self._col, ascending=True)
        event.Skip()

    def OnColumnSortDesc(self, event):
        """Sort values of selected column (descending)"""
        self.SortListItems(col=self._col, ascending=False)
        event.Skip()

    def OnColumnCompute(self, event):
        """Compute values of selected column"""
        id = event.GetId()

        option = None
        if id == self.popupId['area']:
            option = 'area'
        elif id == self.popupId['length']:
            option = 'length'
        elif id == self.popupId['compact']:
            option = 'compact'
        elif id == self.popupId['fractal']:
            option = 'fd'
        elif id == self.popupId['perimeter']:
            option = 'perimeter'
        elif id == self.popupId['ncats']:
            option = 'count'
        elif id == self.popupId['slope']:
            option = 'slope'
        elif id == self.popupId['lsin']:
            option = 'sinuous'
        elif id == self.popupId['lazimuth']:
            option = 'azimuth'

        if not option:
            return

        RunCommand('v.to.db',
                   parent=self.parent,
                   map=self.mapDBInfo.map,
                   layer=self.layer,
                   option=option,
                   columns=self.GetColumn(self._col).GetText())

        self.LoadData(self.layer)

    def ColumnSort(self):
        """Sort values of selected column (self._col)"""
        # remove duplicated arrow symbol from column header
        # FIXME: should be done automatically
        info = wx.ListItem()
        info.m_mask = wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE
        info.m_image = -1
        for column in range(self.GetColumnCount()):
            info.m_text = self.GetColumn(column).GetText()
            self.SetColumn(column, info)

    def OnFieldCalculator(self, event):
        """Calls SQLBuilderUpdate instance"""
        if not self.fieldCalc:
            self.fieldCalc = SQLBuilderUpdate(
                parent=self,
                id=wx.ID_ANY,
                vectmap=self.dbMgrData['vectName'],
                layer=self.layer,
                column=self.GetColumn(
                    self._col).GetText())
            self.fieldCalc.Show()
        else:
            self.fieldCalc.Raise()

    def OnFieldStatistics(self, event):
        """Calls FieldStatistics instance"""
        if not self.fieldStats:
            self.fieldStats = FieldStatistics(parent=self, id=wx.ID_ANY)
            self.fieldStats.Show()
        else:
            self.fieldStats.Raise()

        selLayer = self.dbMgrData['mapDBInfo'].layers[self.layer]
        self.fieldStats.Update(driver=selLayer['driver'],
                               database=selLayer['database'],
                               table=selLayer['table'],
                               column=self.GetColumn(self._col).GetText())

    def OnAddColumn(self, event):
        """Add column into table"""
        table = self.dbMgrData['mapDBInfo'].layers[self.layer]['table']
        dlg = AddColumnDialog(
            parent=self,
            title=_('Add column to table <%s>') %
            table)
        if not dlg:
            return
        if dlg.ShowModal() == wx.ID_OK:
            data = dlg.GetData()
            self.pages['browse'].AddColumn(name=data['name'],
                                           ctype=data['ctype'],
                                           length=data['length'])
        dlg.Destroy()

    def SortItems(self, sorter=cmp):
        """Sort items"""
        wx.BeginBusyCursor()
        items = list(self.itemDataMap.keys())
        items.sort(key=functools.cmp_to_key(self.Sorter))
        self.itemIndexMap = items

        # redraw the list
        self.Refresh()
        wx.EndBusyCursor()

    def Sorter(self, key1, key2):
        colName = self.GetColumn(self._col).GetText()
        ascending = self._colSortFlag[self._col]
        try:
            item1 = self.columns[colName]["ctype"](
                self.itemDataMap[key1][self._col])
            item2 = self.columns[colName]["ctype"](
                self.itemDataMap[key2][self._col])
        except ValueError:
            item1 = self.itemDataMap[key1][self._col]
            item2 = self.itemDataMap[key2][self._col]

        if isinstance(
                item1, str) or isinstance(
                item2, unicode):
            cmpVal = locale.strcoll(GetUnicodeValue(item1), GetUnicodeValue(item2))
        else:
            cmpVal = cmp(item1, item2)

        # If the items are equal then pick something else to make the sort
        # value unique
        if cmpVal == 0:
            cmpVal = cmp(*self.GetSecondarySortValues(self._col, key1, key2))

        if ascending:
            return cmpVal
        else:
            return -cmpVal

    def GetSortImages(self):
        """Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py"""
        return (self.sm_dn, self.sm_up)

    def OnGetItemImage(self, item):
        return -1

    def IsEmpty(self):
        """Check if list if empty"""
        if self.columns:
            return False

        return True


class DbMgrBase:

    def __init__(self, id=wx.ID_ANY, mapdisplay=None,
                 vectorName=None, item=None, giface=None,
                 statusbar=None,
                 **kwargs):
        """Base class, which enables usage of separate pages of Attribute Table Manager

        :param id: window id
        :param mapdisplay: MapFrame instance
        :param vectorName: name of vector map
        :param item: item from Layer Tree
        :param log: log window
        :param statusbar: widget with statusbar
        :param kwagrs: other wx.Frame's arguments
        """

        # stores all data, which are shared by pages
        self.dbMgrData = {}
        self.dbMgrData['vectName'] = vectorName
        self.dbMgrData['treeItem'] = item  # item in layer tree

        self.mapdisplay = mapdisplay

        if self.mapdisplay:
            self.map = mapdisplay.Map
        else:
            self.map = None

        if not self.mapdisplay:
            pass
        elif self.mapdisplay.tree and \
                self.dbMgrData['treeItem'] and not self.dbMgrData['vectName']:
            maptree = self.mapdisplay.tree
            name = maptree.GetLayerInfo(
                self.dbMgrData['treeItem'],
                key='maplayer').GetName()
            self.dbMgrData['vectName'] = name

        # vector attributes can be changed only if vector map is in
        # the current mapset
        mapInfo = None
        if self.dbMgrData['vectName']:
            mapInfo = grass.find_file(
                name=self.dbMgrData['vectName'],
                element='vector')
        if not mapInfo or mapInfo['mapset'] != grass.gisenv()['MAPSET']:
            self.dbMgrData['editable'] = False
        else:
            self.dbMgrData['editable'] = True

        self.giface = giface

        # status bar log class
        self.log = Log(statusbar)  # -> statusbar

        # -> layers / tables description
        self.dbMgrData['mapDBInfo'] = VectorDBInfo(self.dbMgrData['vectName'])

        # store information, which pages were initialized
        self.pages = {
            'browse': None,
            'manageTable': None,
            'manageLayer': None
        }

    def ChangeVectorMap(self, vectorName):
        """Change of vector map

        Does not import layers of new vector map into pages.
        For the import use methods addLayer in DbMgrBrowsePage and DbMgrTablesPage
        """
        if self.pages['browse']:
            self.pages['browse'].DeleteAllPages()
        if self.pages['manageTable']:
            self.pages['manageTable'].DeleteAllPages()

        self.dbMgrData['vectName'] = vectorName

        # fetch fresh db info
        self.dbMgrData['mapDBInfo'] = VectorDBInfo(self.dbMgrData['vectName'])

        # vector attributes can be changed only if vector map is in
        # the current mapset
        mapInfo = grass.find_file(
            name=self.dbMgrData['vectName'],
            element='vector')
        if not mapInfo or mapInfo['mapset'] != grass.gisenv()['MAPSET']:
            self.dbMgrData['editable'] = False
        else:
            self.dbMgrData['editable'] = True

        # 'manage layers page
        if self.pages['manageLayer']:
            self.pages['manageLayer'].UpdatePage()

    def CreateDbMgrPage(self, parent, pageName, onlyLayer=-1):
        """Creates chosen page

        :param pageName: can be 'browse' or 'manageTable' or
                         'manageLayer' which corresponds with pages in
                         Attribute Table Manager
        :return: created instance of page, if the page has been already
                 created returns the previously created instance
        :return: None  if wrong identifier was passed
        """
        if pageName == 'browse':
            if not self.pages['browse']:
                self.pages[pageName] = DbMgrBrowsePage(
                    parent=parent, parentDbMgrBase=self, onlyLayer=onlyLayer)
            return self.pages[pageName]
        if pageName == 'manageTable':
            if not self.pages['manageTable']:
                self.pages[pageName] = DbMgrTablesPage(
                    parent=parent, parentDbMgrBase=self, onlyLayer=onlyLayer)
            return self.pages[pageName]
        if pageName == 'manageLayer':
            if not self.pages['manageLayer']:
                self.pages[pageName] = DbMgrLayersPage(
                    parent=parent, parentDbMgrBase=self)
            return self.pages[pageName]
        return None

    def UpdateDialog(self, layer):
        """Updates dialog layout for given layer"""
        # delete page
        if layer in self.dbMgrData['mapDBInfo'].layers.keys():
            # delete page
            # draging pages disallowed
            # if self.browsePage.GetPageText(page).replace('Layer ', '').strip() == str(layer):
            # self.browsePage.DeletePage(page)
            # break
            if self.pages['browse']:
                self.pages['browse'].DeletePage(layer)
            if self.pages['manageTable']:
                self.pages['manageTable'].DeletePage(layer)

        # fetch fresh db info
        self.dbMgrData['mapDBInfo'] = VectorDBInfo(self.dbMgrData['vectName'])

        #
        # add new page
        #
        if layer in self.dbMgrData['mapDBInfo'].layers.keys():
            # 'browse data' page
            if self.pages['browse']:
                self.pages['browse'].AddLayer(layer)
            # 'manage tables' page
            if self.pages['manageTable']:
                self.pages['manageTable'].AddLayer(layer)

        # manage layers page
        if self.pages['manageLayer']:
            self.pages['manageLayer'].UpdatePage()

    def GetVectorName(self):
        """Get vector name"""
        return self.dbMgrData['vectName']

    def GetVectorLayers(self):
        """Get layers of vector map which have table"""
        return self.dbMgrData['mapDBInfo'].layers.keys()


class DbMgrNotebookBase(FN.FlatNotebook):

    def __init__(self, parent, parentDbMgrBase):
        """Base class for notebook with attribute tables in tabs

        :param parent: GUI parent
        :param parentDbMgrBase: instance of DbMgrBase class
        """

        self.parent = parent
        self.parentDbMgrBase = parentDbMgrBase

        self.log = self.parentDbMgrBase.log
        self.giface = self.parentDbMgrBase.giface

        self.map = self.parentDbMgrBase.map
        self.mapdisplay = self.parentDbMgrBase.mapdisplay

        # TODO no need to have it in class scope make it local?
        self.listOfCommands = []
        self.listOfSQLStatements = []

        # initializet pages
        self.pages = self.parentDbMgrBase.pages

        # shared data among pages
        self.dbMgrData = self.parentDbMgrBase.dbMgrData

        # set up virtual lists (each layer)
        # {layer: list, widgets...}
        self.layerPage = {}

        # currently selected layer
        self.selLayer = None

        # list which represents layers numbers in order of tabs
        self.layers = []

        if globalvar.hasAgw:
            dbmStyle = {'agwStyle': globalvar.FNPageStyle}
        else:
            dbmStyle = {'style': globalvar.FNPageStyle}

        FN.FlatNotebook.__init__(self, parent=self.parent, id=wx.ID_ANY,
                                 **dbmStyle)

        self.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnLayerPageChanged)

    def OnLayerPageChanged(self, event):
        """Layer tab changed"""

        # because of SQL Query notebook
        if event.GetEventObject() != self:
            return

        pageNum = self.GetSelection()
        self.selLayer = self.layers[pageNum]
        try:
            idCol = self.layerPage[self.selLayer]['whereColumn']
        except KeyError:
            idCol = None

        try:
            # update statusbar
            self.log.write(
                _("Number of loaded records: %d") %
                self.FindWindowById(
                    self.layerPage[
                        self.selLayer]['data']). GetItemCount())
        except:
            pass

        if idCol:
            winCol = self.FindWindowById(idCol)
            table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["table"]
            self.dbMgrData['mapDBInfo'].GetColumns(table)

    def ApplyCommands(self, listOfCommands, listOfSQLStatements):
        """Apply changes

        .. todo::
            this part should be _completely_ redesigned
        """
        # perform GRASS commands (e.g. v.db.addcolumn)
        wx.BeginBusyCursor()

        if len(listOfCommands) > 0:
            for cmd in listOfCommands:
                RunCommand(prog=cmd[0],
                           quiet=True,
                           parent=self,
                           **cmd[1])

            self.dbMgrData['mapDBInfo'] = VectorDBInfo(
                self.dbMgrData['vectName'])
            if self.pages['manageTable']:
                self.pages['manageTable'].UpdatePage(self.selLayer)

            if self.pages['browse']:
                self.pages['browse'].UpdatePage(self.selLayer)
            # reset list of commands
            listOfCommands = []

        # perform SQL non-select statements (e.g. 'delete from table where
        # cat=1')
        if len(listOfSQLStatements) > 0:
            enc = GetDbEncoding()
            fd, sqlFilePath = tempfile.mkstemp(text=True)
            with open(sqlFilePath, 'w', encoding=enc) as sqlFile:
                for sql in listOfSQLStatements:
                    sqlFile.write(sql + ';')
                    sqlFile.write('\n')

            driver = self.dbMgrData['mapDBInfo'].layers[
                self.selLayer]["driver"]
            database = self.dbMgrData['mapDBInfo'].layers[
                self.selLayer]["database"]

            Debug.msg(3, 'AttributeManger.ApplyCommands(): %s' %
                      ';'.join(["%s" % s for s in listOfSQLStatements]))

            RunCommand('db.execute',
                       parent=self,
                       input=sqlFilePath,
                       driver=driver,
                       database=database)

            os.close(fd)
            os.remove(sqlFilePath)
            # reset list of statements
            self.listOfSQLStatements = []

        wx.EndBusyCursor()

    def DeletePage(self, layer):
        """Removes layer page"""
        if layer not in self.layers:
            return False

        FN.FlatNotebook.DeletePage(self, self.layers.index(layer))

        self.layers.remove(layer)
        del self.layerPage[layer]

        if self.GetSelection() >= 0:
            self.selLayer = self.layers[self.GetSelection()]
        else:
            self.selLayer = None

        return True

    def DeleteAllPages(self):
        """Removes all layer pages"""
        FN.FlatNotebook.DeleteAllPages(self)
        self.layerPage = {}
        self.layers = []
        self.selLayer = None

    def AddColumn(self, name, ctype, length):
        """Add new column to the table"""
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']

        if not name:
            GError(parent=self,
                   message=_("Unable to add column to the table. "
                             "No column name defined."))
            return False

        # cast type if needed
        if ctype == 'double':
            ctype = 'double precision'
        if ctype != 'varchar':
            length = ''  # FIXME

        # check for duplicate items
        if name in self.dbMgrData['mapDBInfo'].GetColumns(table):
            GError(
                parent=self,
                message=_("Column <%(column)s> already exists in table <%(table)s>.") % {
                    'column': name,
                    'table': self.dbMgrData['mapDBInfo'].layers[
                        self.selLayer]["table"]})
            return False

        # add v.db.addcolumn command to the list
        if ctype == 'varchar':
            ctype += ' (%d)' % length
        self.listOfCommands.append(('v.db.addcolumn',
                                    {'map': self.dbMgrData['vectName'],
                                     'layer': self.selLayer,
                                     'columns': '%s %s' % (name, ctype)}
                                    ))
        # apply changes
        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        return True

    def GetAddedLayers(self):
        """Get list of added layers"""
        return self.layers[:]


class DbMgrBrowsePage(DbMgrNotebookBase):

    def __init__(self, parent, parentDbMgrBase, onlyLayer=-1):
        """Browse page class

        :param parent: GUI parent
        :param parentDbMgrBase: instance of DbMgrBase class
        :param onlyLayer: create only tab of given layer, if -1 creates
                          tabs of all layers
        """

        DbMgrNotebookBase.__init__(self, parent=parent,
                                   parentDbMgrBase=parentDbMgrBase)

        #   for Sql Query notebook adaptation on current width
        self.sqlBestSize = None

        for layer in self.dbMgrData['mapDBInfo'].layers.keys():
            if onlyLayer > 0 and layer != onlyLayer:
                continue
            self.AddLayer(layer)

        if self.layers:
            self.SetSelection(0)
            self.selLayer = self.layers[0]
            self.log.write(
                _("Number of loaded records: %d") %
                self.FindWindowById(
                    self.layerPage[
                        self.selLayer]['data']).GetItemCount())

        # query map layer (if parent (GMFrame) is given)
        self.qlayer = None

        # sqlbuilder
        self.builder = None

    def AddLayer(self, layer, pos=-1):
        """Adds tab which represents table and enables browse it

        :param layer: vector map layer conntected to table
        :param pos: position of tab, if -1 it is added to end

        :return: True if layer was added
        :return: False if layer was not added - layer has been already
                 added or has empty table or does not exist
        """
        if layer in self.layers or \
                layer not in self.parentDbMgrBase.GetVectorLayers():
            return False

        panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # IMPORTANT NOTE: wx.StaticBox MUST be defined BEFORE any of the
        #   controls that are placed IN the wx.StaticBox, or it will freeze
        #   on the Mac

        listBox = StaticBox(
            parent=panel, id=wx.ID_ANY, label=" %s " %
            _("Attribute data - right-click to edit/manage records"))
        listSizer = wx.StaticBoxSizer(listBox, wx.VERTICAL)

        win = VirtualAttributeList(panel, self.log,
                                   self.dbMgrData, layer, self.pages)
        if win.IsEmpty():
            panel.Destroy()
            return False

        self.layers.append(layer)

        win.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnDataItemActivated)

        self.layerPage[layer] = {'browsePage': panel.GetId()}

        label = _("Table")
        if not self.dbMgrData['editable']:
            label += _(" (read-only)")

        if pos == -1:
            pos = self.GetPageCount()
        self.InsertPage(
            pos, page=panel, text=" %d / %s %s" %
            (layer, label, self.dbMgrData['mapDBInfo'].layers[layer]['table']))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        sqlQueryPanel = wx.Panel(parent=panel, id=wx.ID_ANY)

        # attribute data
        sqlBox = StaticBox(parent=sqlQueryPanel, id=wx.ID_ANY,
                           label=" %s " % _("SQL Query"))

        sqlSizer = wx.StaticBoxSizer(sqlBox, wx.VERTICAL)

        win.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnDataRightUp)  # wxMSW
        win.Bind(wx.EVT_RIGHT_UP, self.OnDataRightUp)  # wxGTK
        if UserSettings.Get(group='atm', key='leftDbClick',
                            subkey='selection') == 0:
            win.Bind(wx.EVT_LEFT_DCLICK, self.OnDataItemEdit)
            win.Bind(wx.EVT_COMMAND_LEFT_DCLICK, self.OnDataItemEdit)
        else:
            win.Bind(wx.EVT_LEFT_DCLICK, self.OnDataDrawSelected)
            win.Bind(wx.EVT_COMMAND_LEFT_DCLICK, self.OnDataDrawSelected)

        listSizer.Add(win, proportion=1,
                      flag=wx.EXPAND | wx.ALL,
                      border=3)

        # sql statement box
        FNPageStyle = FN.FNB_NO_NAV_BUTTONS | \
            FN.FNB_NO_X_BUTTON | FN.FNB_NODRAG | FN.FNB_FANCY_TABS
        if globalvar.hasAgw:
            dbmStyle = {'agwStyle': FNPageStyle}
        else:
            dbmStyle = {'style': FNPageStyle}
        sqlNtb = FN.FlatNotebook(parent=sqlQueryPanel, id=wx.ID_ANY,
                                 **dbmStyle)
        # Simple tab
        simpleSqlPanel = wx.Panel(parent=sqlNtb, id=wx.ID_ANY)
        sqlNtb.AddPage(page=simpleSqlPanel,
                       text=_('Simple'))

        btnApply = Button(
            parent=simpleSqlPanel,
            id=wx.ID_APPLY,
            name='btnApply')
        btnApply.SetToolTip(
            _("Apply SELECT statement and reload data records"))
        btnApply.Bind(wx.EVT_BUTTON, self.OnApplySqlStatement)

        whereSimpleSqlPanel = wx.Panel(
            parent=simpleSqlPanel,
            id=wx.ID_ANY,
            name='wherePanel')
        sqlWhereColumn = ComboBox(
            parent=whereSimpleSqlPanel, id=wx.ID_ANY, size=(150, -1),
            style=wx.CB_READONLY,
            choices=self.dbMgrData['mapDBInfo'].GetColumns(
                self.dbMgrData['mapDBInfo'].layers[layer]['table']))
        sqlWhereColumn.SetSelection(0)
        sqlWhereCond = wx.Choice(parent=whereSimpleSqlPanel, id=wx.ID_ANY,
                                 size=(55, -1),
                                 choices=['=', '!=', '<', '<=', '>', '>='])
        sqlWhereCond.SetSelection(0)
        sqlWhereValue = TextCtrl(
            parent=whereSimpleSqlPanel,
            id=wx.ID_ANY,
            value="",
            style=wx.TE_PROCESS_ENTER)
        sqlWhereValue.SetToolTip(
            _("Example: %s") %
            "MULTILANE = 'no' AND OBJECTID < 10")

        sqlLabel = StaticText(
            parent=simpleSqlPanel,
            id=wx.ID_ANY,
            label="SELECT * FROM %s WHERE " %
            self.dbMgrData['mapDBInfo'].layers[layer]['table'])
        # Advanced tab
        advancedSqlPanel = wx.Panel(parent=sqlNtb, id=wx.ID_ANY)
        sqlNtb.AddPage(page=advancedSqlPanel,
                       text=_('Builder'))

        btnSqlBuilder = Button(
            parent=advancedSqlPanel,
            id=wx.ID_ANY,
            label=_("SQL Builder"))
        btnSqlBuilder.Bind(wx.EVT_BUTTON, self.OnBuilder)

        sqlStatement = TextCtrl(
            parent=advancedSqlPanel,
            id=wx.ID_ANY,
            value="SELECT * FROM %s" %
            self.dbMgrData['mapDBInfo'].layers[layer]['table'],
            style=wx.TE_PROCESS_ENTER)
        sqlStatement.SetToolTip(
            _("Example: %s") %
            "SELECT * FROM roadsmajor WHERE MULTILANE = 'no' AND OBJECTID < 10")
        sqlWhereValue.Bind(wx.EVT_TEXT_ENTER, self.OnApplySqlStatement)
        sqlStatement.Bind(wx.EVT_TEXT_ENTER, self.OnApplySqlStatement)

        # Simple tab layout
        simpleSqlSizer = wx.GridBagSizer(hgap=5, vgap=5)

        sqlSimpleWhereSizer = wx.BoxSizer(wx.HORIZONTAL)

        sqlSimpleWhereSizer.Add(
            sqlWhereColumn,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
            border=3)
        sqlSimpleWhereSizer.Add(
            sqlWhereCond,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
            border=3)
        sqlSimpleWhereSizer.Add(
            sqlWhereValue,
            proportion=1,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
            border=3)
        whereSimpleSqlPanel.SetSizer(sqlSimpleWhereSizer)
        simpleSqlSizer.Add(sqlLabel, border=5, pos=(0, 0),
                           flag=wx.ALIGN_CENTER_VERTICAL | wx.TOP | wx.LEFT)
        simpleSqlSizer.Add(whereSimpleSqlPanel, border=5, pos=(0, 1),
                           flag=wx.ALIGN_CENTER_VERTICAL | wx.TOP | wx.EXPAND)
        simpleSqlSizer.Add(btnApply, border=5, pos=(0, 2),
                           flag=wx.ALIGN_CENTER_VERTICAL | wx.TOP)
        simpleSqlSizer.AddGrowableCol(1)

        simpleSqlPanel.SetSizer(simpleSqlSizer)

        # Advanced tab layout
        advancedSqlSizer = wx.FlexGridSizer(cols=2, hgap=5, vgap=5)
        advancedSqlSizer.AddGrowableCol(0)

        advancedSqlSizer.Add(sqlStatement,
                             flag=wx.EXPAND | wx.ALL, border=5)
        advancedSqlSizer.Add(
            btnSqlBuilder,
            flag=wx.ALIGN_RIGHT | wx.TOP | wx.RIGHT | wx.BOTTOM,
            border=5)

        sqlSizer.Add(sqlNtb,
                     flag=wx.ALL | wx.EXPAND,
                     border=3)

        advancedSqlPanel.SetSizer(advancedSqlSizer)

        pageSizer.Add(listSizer,
                      proportion=1,
                      flag=wx.ALL | wx.EXPAND,
                      border=5)

        sqlQueryPanel.SetSizer(sqlSizer)

        pageSizer.Add(sqlQueryPanel,
                      proportion=0,
                      flag=wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.EXPAND,
                      border=5)

        panel.SetSizer(pageSizer)

        sqlNtb.Bind(wx.EVT_SIZE, self.OnSqlQuerySizeWrap(layer))

        self.layerPage[layer]['data'] = win.GetId()
        self.layerPage[layer]['sqlNtb'] = sqlNtb.GetId()
        self.layerPage[layer]['whereColumn'] = sqlWhereColumn.GetId()
        self.layerPage[layer]['whereOperator'] = sqlWhereCond.GetId()
        self.layerPage[layer]['where'] = sqlWhereValue.GetId()
        self.layerPage[layer]['builder'] = btnSqlBuilder.GetId()
        self.layerPage[layer]['statement'] = sqlStatement.GetId()
        # for SQL Query adaptation on width
        self.layerPage[layer]['sqlIsReduced'] = False

        return True

    def OnSqlQuerySizeWrap(self, layer):
        """Helper function"""
        return lambda event: self.OnSqlQuerySize(event, layer)

    def OnSqlQuerySize(self, event, layer):
        """Adapts SQL Query Simple tab on current width"""

        sqlNtb = event.GetEventObject()
        if not self.sqlBestSize:
            self.sqlBestSize = sqlNtb.GetBestSize()

        size = sqlNtb.GetSize()
        sqlReduce = self.sqlBestSize[0] > size[0]
        if (sqlReduce and self.layerPage[layer]['sqlIsReduced']) or \
           (not sqlReduce and not self.layerPage[layer]['sqlIsReduced']):
            event.Skip()
            return

        wherePanel = sqlNtb.FindWindowByName('wherePanel')
        btnApply = sqlNtb.FindWindowByName('btnApply')
        sqlSimpleSizer = btnApply.GetContainingSizer()

        if sqlReduce:
            self.layerPage[layer]['sqlIsReduced'] = True
            sqlSimpleSizer.AddGrowableCol(0)
            sqlSimpleSizer.RemoveGrowableCol(1)
            sqlSimpleSizer.SetItemPosition(wherePanel, (1, 0))
            sqlSimpleSizer.SetItemPosition(btnApply, (1, 1))
        else:
            self.layerPage[layer]['sqlIsReduced'] = False
            sqlSimpleSizer.AddGrowableCol(1)
            sqlSimpleSizer.RemoveGrowableCol(0)
            sqlSimpleSizer.SetItemPosition(wherePanel, (0, 1))
            sqlSimpleSizer.SetItemPosition(btnApply, (0, 2))

        event.Skip()

    def OnDataItemActivated(self, event):
        """Item activated, highlight selected item"""
        self.OnDataDrawSelected(event)

        event.Skip()

    def OnDataRightUp(self, event):
        """Table description area, context menu"""
        if not hasattr(self, "popupDataID1"):
            self.popupDataID1 = NewId()
            self.popupDataID2 = NewId()
            self.popupDataID3 = NewId()
            self.popupDataID4 = NewId()
            self.popupDataID5 = NewId()
            self.popupDataID6 = NewId()
            self.popupDataID7 = NewId()
            self.popupDataID8 = NewId()
            self.popupDataID9 = NewId()
            self.popupDataID10 = NewId()
            self.popupDataID11 = NewId()

            self.Bind(wx.EVT_MENU, self.OnDataItemEdit, id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnDataItemAdd, id=self.popupDataID2)
            self.Bind(wx.EVT_MENU, self.OnDataItemDelete, id=self.popupDataID3)
            self.Bind(
                wx.EVT_MENU,
                self.OnDataItemDeleteAll,
                id=self.popupDataID4)
            self.Bind(wx.EVT_MENU, self.OnDataSelectAll, id=self.popupDataID5)
            self.Bind(wx.EVT_MENU, self.OnDataSelectNone, id=self.popupDataID6)
            self.Bind(
                wx.EVT_MENU,
                self.OnDataDrawSelected,
                id=self.popupDataID7)
            self.Bind(
                wx.EVT_MENU,
                self.OnDataDrawSelectedZoom,
                id=self.popupDataID8)
            self.Bind(
                wx.EVT_MENU,
                self.OnExtractSelected,
                id=self.popupDataID9)
            self.Bind(
                wx.EVT_MENU,
                self.OnDeleteSelected,
                id=self.popupDataID11)
            self.Bind(wx.EVT_MENU, self.OnDataReload, id=self.popupDataID10)

        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        # generate popup-menu
        menu = Menu()
        menu.Append(self.popupDataID1, _("Edit selected record"))
        selected = tlist.GetFirstSelected()
        if not self.dbMgrData[
                'editable'] or selected == -1 or tlist.GetNextSelected(selected) != -1:
            menu.Enable(self.popupDataID1, False)
        menu.Append(self.popupDataID2, _("Insert new record"))
        menu.Append(self.popupDataID3, _("Delete selected record(s)"))
        menu.Append(self.popupDataID4, _("Delete all records"))
        if not self.dbMgrData['editable']:
            menu.Enable(self.popupDataID2, False)
            menu.Enable(self.popupDataID3, False)
            menu.Enable(self.popupDataID4, False)
        menu.AppendSeparator()
        menu.Append(self.popupDataID5, _("Select all"))
        menu.Append(self.popupDataID6, _("Deselect all"))
        menu.AppendSeparator()
        menu.Append(self.popupDataID7, _("Highlight selected features"))
        menu.Append(
            self.popupDataID8,
            _("Highlight selected features and zoom"))
        if not self.map or len(tlist.GetSelectedItems()) == 0:
            menu.Enable(self.popupDataID7, False)
            menu.Enable(self.popupDataID8, False)
        menu.Append(self.popupDataID9, _("Extract selected features"))
        menu.Append(self.popupDataID11, _("Delete selected features"))
        if not self.dbMgrData['editable']:
            menu.Enable(self.popupDataID11, False)
        if tlist.GetFirstSelected() == -1:
            menu.Enable(self.popupDataID3, False)
            menu.Enable(self.popupDataID9, False)
            menu.Enable(self.popupDataID11, False)
        menu.AppendSeparator()
        menu.Append(self.popupDataID10, _("Reload"))

        self.PopupMenu(menu)
        menu.Destroy()

        # update statusbar
        self.log.write(_("Number of loaded records: %d") %
                       tlist.GetItemCount())

    def OnDataItemEdit(self, event):
        """Edit selected record of the attribute table"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        item = tlist.GetFirstSelected()
        if item == -1:
            return
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        keyColumn = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['key']
        cat = tlist.itemCatsMap[tlist.itemIndexMap[item]]

        # (column name, value)
        data = []

        # collect names of all visible columns
        columnName = []
        for i in range(tlist.GetColumnCount()):
            columnName.append(tlist.GetColumn(i).GetText())

        # key column must be always presented
        if keyColumn not in columnName:
            # insert key column on first position
            columnName.insert(0, keyColumn)
            data.append((keyColumn, str(cat)))
            keyId = 0
            missingKey = True
        else:
            missingKey = False

        # add other visible columns
        for i in range(len(columnName)):
            ctype = self.dbMgrData['mapDBInfo'].tables[
                table][columnName[i]]['ctype']
            ctypeStr = self.dbMgrData['mapDBInfo'].tables[
                table][columnName[i]]['type']
            if columnName[i] == keyColumn:  # key
                if missingKey is False:
                    data.append((columnName[i], ctype, ctypeStr, str(cat)))
                    keyId = i
            else:
                if missingKey is True:
                    value = tlist.GetItem(item, i - 1).GetText()
                else:
                    value = tlist.GetItem(item, i).GetText()
                data.append((columnName[i], ctype, ctypeStr, value))

        dlg = ModifyTableRecord(parent=self,
                                title=_("Update existing record"),
                                data=data, keyEditable=(keyId, False))

        if dlg.ShowModal() == wx.ID_OK:
            values = dlg.GetValues()  # string
            updateList = list()
            try:
                for i in range(len(values)):
                    if i == keyId:  # skip key column
                        continue
                    if tlist.GetItem(item, i).GetText() == values[i]:
                        continue  # no change

                    column = tlist.columns[columnName[i]]
                    if len(values[i]) > 0:
                        try:
                            if missingKey is True:
                                idx = i - 1
                            else:
                                idx = i

                            if column['ctype'] != str:
                                tlist.itemDataMap[item][
                                    idx] = column['ctype'](values[i])
                            else:  # -> string
                                tlist.itemDataMap[item][idx] = values[i]
                        except ValueError:
                            raise ValueError(_("Value '%(value)s' needs to be entered as %(type)s.") %
                                             {'value': str(values[i]),
                                              'type': column['type']})

                        if column['ctype'] == str:
                            if "'" in values[i]:  # replace "'" -> "''"
                                values[i] = values[i].replace("'", "''")
                            updateList.append(
                                "%s='%s'" %
                                (columnName[i], values[i]))
                        else:
                            updateList.append(
                                "%s=%s" %
                                (columnName[i], values[i]))
                    else:  # -> NULL
                        updateList.append("%s=NULL" % (columnName[i]))
            except ValueError as err:
                GError(
                    parent=self,
                    message=_("Unable to update existing record.\n%s") %
                    err,
                    showTraceback=False)
                self.OnDataItemEdit(event)
                return

            if updateList:
                self.listOfSQLStatements.append(
                    'UPDATE %s SET %s WHERE %s=%d' %
                    (table, ','.join(updateList), keyColumn, cat))
                self.ApplyCommands(
                    self.listOfCommands,
                    self.listOfSQLStatements)

            tlist.Update()

    def OnDataItemAdd(self, event):
        """Add new record to the attribute table"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        keyColumn = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['key']

        # (column name, value)
        data = []

        # collect names of all visible columns
        columnName = []
        for i in range(tlist.GetColumnCount()):
            columnName.append(tlist.GetColumn(i).GetText())

        # maximal category number
        if len(tlist.itemCatsMap.values()) > 0:
            maxCat = max(tlist.itemCatsMap.values())
        else:
            maxCat = 0  # starting category '1'

        # key column must be always presented
        if keyColumn not in columnName:
            # insert key column on first position
            columnName.insert(0, keyColumn)
            data.append((keyColumn, str(maxCat + 1)))
            missingKey = True
        else:
            missingKey = False

        # add other visible columns
        colIdx = 0
        keyId = -1
        for col in columnName:
            ctype = self.dbMgrData['mapDBInfo'].tables[table][col]['ctype']
            ctypeStr = self.dbMgrData['mapDBInfo'].tables[table][col]['type']
            if col == keyColumn:  # key
                if missingKey is False:
                    data.append((col, ctype, ctypeStr, str(maxCat + 1)))
                    keyId = colIdx
            else:
                data.append((col, ctype, ctypeStr, ''))

            colIdx += 1

        dlg = ModifyTableRecord(parent=self,
                                title=_("Insert new record"),
                                data=data, keyEditable=(keyId, True))

        if dlg.ShowModal() == wx.ID_OK:
            try:  # get category number
                cat = int(dlg.GetValues(columns=[keyColumn])[0])
            except:
                cat = -1

            try:
                if cat in tlist.itemCatsMap.values():
                    raise ValueError(_("Record with category number %d "
                                       "already exists in the table.") % cat)

                values = dlg.GetValues()  # values (need to be casted)
                columnsString = ''
                valuesString = ''

                for i in range(len(values)):
                    if len(values[i]) == 0:  # NULL
                        if columnName[i] == keyColumn:
                            raise ValueError(_("Category number (column %s)"
                                               " is missing.") % keyColumn)
                        else:
                            continue

                    try:
                        if tlist.columns[columnName[i]]['ctype'] == int:
                            # values[i] is stored as text.
                            values[i] = int(float(values[i]))
                        elif tlist.columns[columnName[i]]['ctype'] == float:
                            values[i] = float(values[i])
                    except:
                        raise ValueError(_("Value '%(value)s' needs to be entered as %(type)s.") %
                                         {'value': values[i],
                                          'type': tlist.columns[columnName[i]]['type']})
                    columnsString += '%s,' % columnName[i]

                    if tlist.columns[columnName[i]]['ctype'] == str:
                        valuesString += "'%s'," % values[i].replace("'", "''")
                    else:
                        valuesString += "%s," % values[i]

            except ValueError as err:
                GError(parent=self,
                       message=_("Unable to insert new record.\n%s") % err,
                       showTraceback=False)
                self.OnDataItemAdd(event)
                return

            # remove category if need
            if missingKey is True:
                del values[0]

            # add new item to the tlist
            if len(tlist.itemIndexMap) > 0:
                index = max(tlist.itemIndexMap) + 1
            else:
                index = 0

            tlist.itemIndexMap.append(index)
            tlist.itemDataMap[index] = values
            tlist.itemCatsMap[index] = cat
            tlist.SetItemCount(tlist.GetItemCount() + 1)

            self.listOfSQLStatements.append('INSERT INTO %s (%s) VALUES(%s)' %
                                            (table,
                                             columnsString.rstrip(','),
                                             valuesString.rstrip(',')))

            self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

    def OnDataItemDelete(self, event):
        """Delete selected item(s) from the tlist (layer/category pair)"""
        dlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        item = dlist.GetFirstSelected()

        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["table"]
        key = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["key"]

        indices = []
        # collect SQL statements
        while item != -1:
            index = dlist.itemIndexMap[item]
            indices.append(index)

            cat = dlist.itemCatsMap[index]

            self.listOfSQLStatements.append('DELETE FROM %s WHERE %s=%d' %
                                            (table, key, cat))

            item = dlist.GetNextSelected(item)

        if UserSettings.Get(
                group='atm', key='askOnDeleteRec', subkey='enabled'):
            deleteDialog = wx.MessageBox(
                parent=self,
                message=_(
                    "Selected data records (%d) will be permanently deleted "
                    "from table. Do you want to delete them?") %
                (len(self.listOfSQLStatements)),
                caption=_("Delete records"),
                style=wx.YES_NO | wx.CENTRE)
            if deleteDialog != wx.YES:
                self.listOfSQLStatements = []
                return False

        # restore maps
        i = 0
        indexTemp = copy.copy(dlist.itemIndexMap)
        dlist.itemIndexMap = []
        dataTemp = copy.deepcopy(dlist.itemDataMap)
        dlist.itemDataMap = {}
        catsTemp = copy.deepcopy(dlist.itemCatsMap)
        dlist.itemCatsMap = {}

        i = 0
        for index in indexTemp:
            if index in indices:
                continue
            dlist.itemIndexMap.append(i)
            dlist.itemDataMap[i] = dataTemp[index]
            dlist.itemCatsMap[i] = catsTemp[index]

            i += 1

        dlist.SetItemCount(len(dlist.itemIndexMap))

        # deselect items
        item = dlist.GetFirstSelected()
        while item != -1:
            dlist.SetItemState(
                item, 0, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED)
            item = dlist.GetNextSelected(item)

        # submit SQL statements
        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        return True

    def OnDataItemDeleteAll(self, event):
        """Delete all items from the list"""
        dlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        if UserSettings.Get(
                group='atm', key='askOnDeleteRec', subkey='enabled'):
            deleteDialog = wx.MessageBox(
                parent=self,
                message=_(
                    "All data records (%d) will be permanently deleted "
                    "from table. Do you want to delete them?") %
                (len(dlist.itemIndexMap)),
                caption=_("Delete records"),
                style=wx.YES_NO | wx.CENTRE)
            if deleteDialog != wx.YES:
                return

        dlist.DeleteAllItems()
        dlist.itemDataMap = {}
        dlist.itemIndexMap = []
        dlist.SetItemCount(0)

        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["table"]
        self.listOfSQLStatements.append('DELETE FROM %s' % table)

        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        event.Skip()

    def _drawSelected(self, zoom, selectedOnly=True):
        """Highlight selected features"""
        if not self.map or not self.mapdisplay:
            return

        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        if selectedOnly:
            fn = tlist.GetSelectedItems
        else:
            fn = tlist.GetItems

        cats = list(map(int, fn()))

        digitToolbar = None
        if 'vdigit' in self.mapdisplay.toolbars:
            digitToolbar = self.mapdisplay.toolbars['vdigit']
        if digitToolbar and digitToolbar.GetLayer() and digitToolbar.GetLayer(
        ).GetName() == self.dbMgrData['vectName']:
            display = self.mapdisplay.GetMapWindow().GetDisplay()
            display.SetSelected(cats, layer=self.selLayer)
            if zoom:
                n, s, w, e = display.GetRegionSelected()
                self.mapdisplay.Map.GetRegion(n=n, s=s, w=w, e=e,
                                              update=True)
        else:
            # add map layer with higlighted vector features
            self.AddQueryMapLayer(selectedOnly)  # -> self.qlayer

            # set opacity based on queried layer
            if self.parent and self.mapdisplay.tree and \
                    self.dbMgrData['treeItem']:
                maptree = self.mapdisplay.tree  # TODO: giface
                opacity = maptree.GetLayerInfo(
                    self.dbMgrData['treeItem'],
                    key='maplayer').GetOpacity()
                self.qlayer.SetOpacity(opacity)
            if zoom:
                keyColumn = self.dbMgrData[
                    'mapDBInfo'].layers[self.selLayer]['key']
                where = ''
                for range in ListOfCatsToRange(cats).split(','):
                    if '-' in range:
                        min, max = range.split('-')
                        where += '%s >= %d and %s <= %d or ' % \
                            (keyColumn, int(min),
                             keyColumn, int(max))
                    else:
                        where += '%s = %d or ' % (keyColumn, int(range))
                where = where.rstrip('or ')

                select = RunCommand('v.db.select',
                                    parent=self,
                                    read=True,
                                    quiet=True,
                                    flags='r',
                                    map=self.dbMgrData['mapDBInfo'].map,
                                    layer=int(self.selLayer),
                                    where=where)

                region = {}
                for line in select.splitlines():
                    key, value = line.split('=')
                    region[key.strip()] = float(value.strip())

                nsdist = ewdist = 0
                renderer = self.mapdisplay.GetMap()
                nsdist = 10 * ((renderer.GetCurrentRegion()
                                ['n'] - renderer.GetCurrentRegion()['s']) / renderer.height)
                ewdist = 10 * ((renderer.GetCurrentRegion()
                                ['e'] - renderer.GetCurrentRegion()['w']) / renderer.width)
                north = region['n'] + nsdist
                south = region['s'] - nsdist
                west = region['w'] - ewdist
                east = region['e'] + ewdist
                renderer.GetRegion(
                    n=north, s=south, w=west, e=east, update=True)
                self.mapdisplay.GetMapWindow().ZoomHistory(n=north, s=south, w=west, e=east)

        if zoom:
            self.mapdisplay.Map.AdjustRegion()           # adjust resolution
            self.mapdisplay.Map.AlignExtentFromDisplay()  # adjust extent
            self.mapdisplay.MapWindow.UpdateMap(render=True, renderVector=True)
        else:
            self.mapdisplay.MapWindow.UpdateMap(
                render=False, renderVector=True)

    def AddQueryMapLayer(self, selectedOnly=True):
        """Redraw a map

        :return: True if map has been redrawn, False if no map is given
        """
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        if selectedOnly:
            fn = tlist.GetSelectedItems
        else:
            fn = tlist.GetItems

        cats = {self.selLayer: fn()}

        if self.mapdisplay.Map.GetLayerIndex(self.qlayer) < 0:
            self.qlayer = None

        if self.qlayer:
            self.qlayer.SetCmd(
                self.mapdisplay.AddTmpVectorMapLayer(
                    self.dbMgrData['vectName'],
                    cats, addLayer=False))
        else:
            self.qlayer = self.mapdisplay.AddTmpVectorMapLayer(
                self.dbMgrData['vectName'], cats)

        return self.qlayer

    def OnDataReload(self, event):
        """Reload tlist of records"""
        self.OnApplySqlStatement(None)
        self.listOfSQLStatements = []

    def OnDataSelectAll(self, event):
        """Select all items"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        item = -1

        while True:
            item = tlist.GetNextItem(item)
            if item == -1:
                break
            tlist.SetItemState(
                item,
                wx.LIST_STATE_SELECTED,
                wx.LIST_STATE_SELECTED)

        event.Skip()

    def OnDataSelectNone(self, event):
        """Deselect items"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        item = -1

        while True:
            item = tlist.GetNextItem(item, wx.LIST_STATE_SELECTED)
            if item == -1:
                break
            tlist.SetItemState(
                item, 0, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED)
        tlist.Focus(0)

        event.Skip()

    def OnDataDrawSelected(self, event):
        """Reload table description"""
        self._drawSelected(zoom=False)
        event.Skip()

    def OnDataDrawSelectedZoom(self, event):
        self._drawSelected(zoom=True)
        event.Skip()

    def OnExtractSelected(self, event):
        """Extract vector objects selected in attribute browse window
        to new vector map
        """
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        # cats = tlist.selectedCats[:]
        cats = tlist.GetSelectedItems()
        if len(cats) == 0:
            GMessage(parent=self,
                     message=_('Nothing to extract.'))
            return
        else:
            # dialog to get file name
            dlg = CreateNewVector(
                parent=self, title=_('Extract selected features'),
                giface=self.giface,
                cmd=(('v.extract',
                      {'input': self.dbMgrData['vectName'],
                       'cats': ListOfCatsToRange(cats)},
                      'output')),
                disableTable=True)
            if not dlg:
                return

            name = dlg.GetName(full=True)

            if not self.mapdisplay and self.mapdisplay.tree:
                pass
            elif name and dlg.IsChecked('add'):
                # add layer to map layer tree
                self.mapdisplay.tree.AddLayer(ltype='vector',
                                              lname=name,
                                              lcmd=['d.vect', 'map=%s' % name])
            dlg.Destroy()

    def OnDeleteSelected(self, event):
        """Delete vector objects selected in attribute browse window
        (attribures and geometry)
        """
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        cats = tlist.GetSelectedItems()
        if len(cats) == 0:
            GMessage(parent=self,
                     message=_('Nothing to delete.'))

            return

        display = None
        if not self.mapdisplay:
            pass
        elif 'vdigit' in self.mapdisplay.toolbars:
            digitToolbar = self.mapdisplay.toolbars['vdigit']
            if digitToolbar and digitToolbar.GetLayer() and digitToolbar.GetLayer(
            ).GetName() == self.dbMgrData['vectName']:
                display = self.mapdisplay.GetMapWindow().GetDisplay()
                display.SetSelected(list(map(int, cats)), layer=self.selLayer)
                self.mapdisplay.MapWindow.UpdateMap(
                    render=True, renderVector=True)

        if self.OnDataItemDelete(None) and self.mapdisplay:
            if display:
                self.mapdisplay.GetMapWindow().digit.DeleteSelectedLines()
            else:
                RunCommand('v.edit',
                           parent=self,
                           quiet=True,
                           map=self.dbMgrData['vectName'],
                           tool='delete',
                           cats=ListOfCatsToRange(cats))

            self.mapdisplay.MapWindow.UpdateMap(render=True, renderVector=True)

    def OnApplySqlStatement(self, event):
        """Apply simple/advanced sql statement"""
        keyColumn = -1  # index of key column
        listWin = self.FindWindowById(self.layerPage[self.selLayer]['data'])
        sql = None
        win = self.FindWindowById(self.layerPage[self.selLayer]['sqlNtb'])
        if not win:
            return

        showSelected = False
        wx.BeginBusyCursor()
        if win.GetSelection() == 0:
            # simple sql statement
            whereCol = self.FindWindowById(self.layerPage[self.selLayer][
                                           'whereColumn']).GetStringSelection()
            whereOpe = self.FindWindowById(self.layerPage[self.selLayer][
                                           'whereOperator']).GetStringSelection()
            whereWin = self.FindWindowById(
                self.layerPage[self.selLayer]['where'])
            whereVal = whereWin.GetValue().strip()
            table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["table"]
            if self.dbMgrData['mapDBInfo'].tables[
                    table][whereCol]['ctype'] == str:
                # string attribute, check for quotes
                whereVal = whereVal.replace('"', "'")
                if whereVal:
                    if not whereVal.startswith("'"):
                        whereVal = "'" + whereVal
                    if not whereVal.endswith("'"):
                        whereVal += "'"
                    whereWin.SetValue(whereVal)

            try:
                if len(whereVal) > 0:
                    showSelected = True
                    keyColumn = listWin.LoadData(
                        self.selLayer, where=whereCol + whereOpe + whereVal)
                else:
                    keyColumn = listWin.LoadData(self.selLayer)
            except GException as e:
                GError(
                    parent=self,
                    message=_("Loading attribute data failed.\n\n%s") %
                    e.value)
                self.FindWindowById(
                    self.layerPage[
                        self.selLayer]['where']).SetValue('')
        else:
            # advanced sql statement
            win = self.FindWindowById(
                self.layerPage[
                    self.selLayer]['statement'])
            try:
                cols, where = self.ValidateSelectStatement(win.GetValue())
                if cols is None and where is None:
                    sql = win.GetValue()
                if where:
                    showSelected = True
            except TypeError:
                GError(
                    parent=self, message=_(
                        "Loading attribute data failed.\n"
                        "Invalid SQL select statement.\n\n%s") %
                    win.GetValue())
                win.SetValue(
                    "SELECT * FROM %s" %
                    self.dbMgrData['mapDBInfo'].layers[
                        self.selLayer]['table'])
                cols = None
                where = None

            if cols or where or sql:
                try:
                    keyColumn = listWin.LoadData(self.selLayer, columns=cols,
                                                 where=where, sql=sql)
                except GException as e:
                    GError(
                        parent=self,
                        message=_("Loading attribute data failed.\n\n%s") %
                        e.value)
                    win.SetValue(
                        "SELECT * FROM %s" %
                        self.dbMgrData['mapDBInfo'].layers[
                            self.selLayer]['table'])

        # sort by key column
        if sql and 'order by' in sql.lower():
            pass  # don't order by key column
        else:
            if keyColumn > -1:
                listWin.SortListItems(col=keyColumn, ascending=True)
            else:
                listWin.SortListItems(col=0, ascending=True)

        wx.EndBusyCursor()

        # update statusbar
        self.log.write(
            _("Number of loaded records: %d") %
            self.FindWindowById(
                self.layerPage[
                    self.selLayer]['data']).GetItemCount())

        # update map display if needed
        if self.mapdisplay and \
                UserSettings.Get(group='atm', key='highlight', subkey='auto'):
            # TODO: replace by signals
            if showSelected:
                self._drawSelected(zoom=False, selectedOnly=False)
            else:
                self.mapdisplay.RemoveQueryLayer()
                self.mapdisplay.MapWindow.UpdateMap(
                    render=False)  # TODO: replace by signals

    def OnBuilder(self, event):
        """SQL Builder button pressed -> show the SQLBuilder dialog"""
        if not self.builder:
            self.builder = SQLBuilderSelect(parent=self, id=wx.ID_ANY,
                                            vectmap=self.dbMgrData['vectName'],
                                            layer=self.selLayer,
                                            evtHandler=self.OnBuilderEvt)
            self.builder.Show()
        else:
            self.builder.Raise()

    def OnBuilderEvt(self, event):
        if event == 'apply':
            sqlstr = self.builder.GetSQLStatement()
            self.FindWindowById(self.layerPage[self.selLayer][
                                'statement']).SetValue(sqlstr)
            # apply query
            # self.listOfSQLStatements.append(sqlstr) #TODO probably it was bug
            self.OnApplySqlStatement(None)
            # close builder on apply
            if self.builder.CloseOnApply():
                self.builder = None
        elif event == 'close':
            self.builder = None

    def ValidateSelectStatement(self, statement):
        """Validate SQL select statement

        :return: (columns, where)
        :return: None on error
        """
        if statement[0:7].lower() != 'select ':
            return None

        cols = ''
        index = 7
        for c in statement[index:]:
            if c == ' ':
                break
            cols += c
            index += 1
        if cols == '*':
            cols = None
        else:
            cols = cols.split(',')

        tablelen = len(
            self.dbMgrData['mapDBInfo'].layers[
                self.selLayer]['table'])

        if statement[index + 1:index + 6].lower() != 'from ' or \
                statement[index + 6:index + 6 + tablelen] != '%s' % \
                (self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']):
            return None

        if len(statement[index + 7 + tablelen:]) > 0:
            index = statement.lower().find('where ')
            if index > -1:
                where = statement[index + 6:]
            else:
                where = None
        else:
            where = None

        return (cols, where)

    def LoadData(self, layer, columns=None, where=None, sql=None):
        """Load data into list

        :param int layer: layer number
        :param list columns: list of columns for output
        :param str where: where statement
        :param str sql: full sql statement

        :return: id of key column
        :return: -1 if key column is not displayed
        """
        listWin = self.FindWindowById(self.layerPage[layer]['data'])
        return listWin.LoadData(layer, columns, where, sql)

    def UpdatePage(self, layer):
        # update data tlist
        if layer in self.layerPage.keys():
            tlist = self.FindWindowById(self.layerPage[layer]['data'])
            tlist.Update(self.dbMgrData['mapDBInfo'])

    def ResetPage(self, layer=None):
        if not layer:
            layer = self.selLayer
        if layer not in self.layerPage.keys():
            return
        win = self.FindWindowById(self.layerPage[self.selLayer]['sqlNtb'])
        if win.GetSelection() == 0:
            self.FindWindowById(
                self.layerPage[layer]['whereColumn']).SetSelection(0)
            self.FindWindowById(
                self.layerPage[layer]['whereOperator']).SetSelection(0)
            self.FindWindowById(self.layerPage[layer]['where']).SetValue('')
        else:
            sqlWin = self.FindWindowById(
                self.layerPage[self.selLayer]['statement'])
            sqlWin.SetValue("SELECT * FROM %s" %
                            self.dbMgrData['mapDBInfo'].layers[layer]['table'])

        self.UpdatePage(layer)


class DbMgrTablesPage(DbMgrNotebookBase):

    def __init__(self, parent, parentDbMgrBase, onlyLayer=-1):
        """Page for managing tables

        :param parent: GUI parent
        :param parentDbMgrBase: instance of DbMgrBase class
        :param onlyLayer: create only tab of given layer, if -1
                          creates tabs of all layers
        """

        DbMgrNotebookBase.__init__(self, parent=parent,
                                   parentDbMgrBase=parentDbMgrBase)

        for layer in self.dbMgrData['mapDBInfo'].layers.keys():
            if onlyLayer > 0 and layer != onlyLayer:
                continue
            self.AddLayer(layer)

        if self.layers:
            self.SetSelection(0)  # select first layer
            self.selLayer = self.layers[0]

    def AddLayer(self, layer, pos=-1):
        """Adds tab which represents table

        :param layer: vector map layer connected to table
        :param pos: position of tab, if -1 it is added to end

        :return: True if layer was added
        :return: False if layer was not added - layer has been already added or does not exist
        """
        if layer in self.layers or \
                layer not in self.parentDbMgrBase.GetVectorLayers():
            return False

        self.layers.append(layer)

        self.layerPage[layer] = {}
        panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.layerPage[layer]['tablePage'] = panel.GetId()
        label = _("Table")
        if not self.dbMgrData['editable']:
            label += _(" (read-only)")

        if pos == -1:
            pos = self.GetPageCount()
        self.InsertPage(
            pos, page=panel, text=" %d / %s %s" %
            (layer, label, self.dbMgrData['mapDBInfo'].layers[layer]['table']))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        #
        # dbInfo
        #
        dbBox = StaticBox(parent=panel, id=wx.ID_ANY,
                          label=" %s " % _("Database connection"))
        dbSizer = wx.StaticBoxSizer(dbBox, wx.VERTICAL)
        dbSizer.Add(
            CreateDbInfoDesc(
                panel,
                self.dbMgrData['mapDBInfo'],
                layer),
            proportion=1,
            flag=wx.EXPAND | wx.ALL,
            border=3)

        #
        # table description
        #
        table = self.dbMgrData['mapDBInfo'].layers[layer]['table']
        tableBox = StaticBox(
            parent=panel,
            id=wx.ID_ANY,
            label=" %s " %
            _("Table <%s> - right-click to delete column(s)") %
            table)

        tableSizer = wx.StaticBoxSizer(tableBox, wx.VERTICAL)

        tlist = self._createTableDesc(panel, table)
        tlist.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnTableRightUp)  # wxMSW
        tlist.Bind(wx.EVT_RIGHT_UP, self.OnTableRightUp)  # wxGTK
        self.layerPage[layer]['tableData'] = tlist.GetId()

        # manage columns (add)
        addBox = StaticBox(parent=panel, id=wx.ID_ANY,
                           label=" %s " % _("Add column"))
        addSizer = wx.StaticBoxSizer(addBox, wx.HORIZONTAL)

        column = TextCtrl(parent=panel, id=wx.ID_ANY, value='',
                          size=(150, -1), style=wx.TE_PROCESS_ENTER)
        column.Bind(wx.EVT_TEXT, self.OnTableAddColumnName)
        column.Bind(wx.EVT_TEXT_ENTER, self.OnTableItemAdd)
        self.layerPage[layer]['addColName'] = column.GetId()
        addSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Column")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
            border=5)
        addSizer.Add(column, proportion=1,
                     flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border=5)

        ctype = wx.Choice(parent=panel, id=wx.ID_ANY,
                          choices=["integer",
                                   "double",
                                   "varchar",
                                   "date"])  # FIXME
        ctype.SetSelection(0)
        ctype.Bind(wx.EVT_CHOICE, self.OnTableChangeType)
        self.layerPage[layer]['addColType'] = ctype.GetId()
        addSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Type")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
            border=5)
        addSizer.Add(ctype,
                     flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border=5)

        length = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                          initial=250,
                          min=1, max=1e6)
        length.Enable(False)
        self.layerPage[layer]['addColLength'] = length.GetId()
        addSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Length")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
            border=5)
        addSizer.Add(length,
                     flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                     border=5)

        btnAddCol = Button(parent=panel, id=wx.ID_ANY, label=_("Add"))
        btnAddCol.Bind(wx.EVT_BUTTON, self.OnTableItemAdd)
        btnAddCol.Enable(False)
        self.layerPage[layer]['addColButton'] = btnAddCol.GetId()
        addSizer.Add(btnAddCol, flag=wx.ALL | wx.EXPAND,
                     border=3)

        # manage columns (rename)
        renameBox = StaticBox(parent=panel, id=wx.ID_ANY,
                              label=" %s " % _("Rename column"))
        renameSizer = wx.StaticBoxSizer(renameBox, wx.HORIZONTAL)

        columnFrom = ComboBox(
            parent=panel, id=wx.ID_ANY, size=(150, -1),
            style=wx.CB_READONLY,
            choices=self.dbMgrData['mapDBInfo'].GetColumns(table))
        columnFrom.SetSelection(0)
        self.layerPage[layer]['renameCol'] = columnFrom.GetId()
        renameSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Column")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
            border=5)
        renameSizer.Add(columnFrom, proportion=1,
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                        border=5)

        columnTo = TextCtrl(parent=panel, id=wx.ID_ANY, value='',
                            size=(150, -1), style=wx.TE_PROCESS_ENTER)
        columnTo.Bind(wx.EVT_TEXT, self.OnTableRenameColumnName)
        columnTo.Bind(wx.EVT_TEXT_ENTER, self.OnTableItemChange)
        self.layerPage[layer]['renameColTo'] = columnTo.GetId()
        renameSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("To")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
            border=5)
        renameSizer.Add(columnTo, proportion=1,
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                        border=5)

        btnRenameCol = Button(
            parent=panel,
            id=wx.ID_ANY,
            label=_("&Rename"))
        btnRenameCol.Bind(wx.EVT_BUTTON, self.OnTableItemChange)
        btnRenameCol.Enable(False)
        self.layerPage[layer]['renameColButton'] = btnRenameCol.GetId()
        renameSizer.Add(
            btnRenameCol,
            flag=wx.ALL | wx.EXPAND,
            border=3)

        tableSizer.Add(tlist,
                       flag=wx.ALL | wx.EXPAND,
                       proportion=1,
                       border=3)

        pageSizer.Add(dbSizer,
                      flag=wx.ALL | wx.EXPAND,
                      proportion=0,
                      border=3)

        pageSizer.Add(tableSizer,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      proportion=1,
                      border=3)

        pageSizer.Add(addSizer,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      proportion=0,
                      border=3)
        pageSizer.Add(renameSizer,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      proportion=0,
                      border=3)

        panel.SetSizer(pageSizer)

        if not self.dbMgrData['editable']:
            for widget in [columnTo, columnFrom, length, ctype,
                           column, btnAddCol, btnRenameCol]:
                widget.Enable(False)

        return True

    def _createTableDesc(self, parent, table):
        """Create list with table description"""
        tlist = TableListCtrl(
            parent=parent,
            id=wx.ID_ANY,
            table=self.dbMgrData['mapDBInfo'].tables[table],
            columns=self.dbMgrData['mapDBInfo'].GetColumns(table))
        tlist.Populate()
        # sorter
        # itemDataMap = list.Populate()
        # listmix.ColumnSorterMixin.__init__(self, 2)

        return tlist

    def OnTableChangeType(self, event):
        """Data type for new column changed. Enable or disable
        data length widget"""
        win = self.FindWindowById(
            self.layerPage[
                self.selLayer]['addColLength'])
        if event.GetString() == "varchar":
            win.Enable(True)
        else:
            win.Enable(False)

    def OnTableRenameColumnName(self, event):
        """Editing column name to be added to the table"""
        btn = self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameColButton'])
        col = self.FindWindowById(self.layerPage[self.selLayer]['renameCol'])
        colTo = self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameColTo'])
        if len(col.GetValue()) > 0 and len(colTo.GetValue()) > 0:
            btn.Enable(True)
        else:
            btn.Enable(False)

        event.Skip()

    def OnTableAddColumnName(self, event):
        """Editing column name to be added to the table"""
        btn = self.FindWindowById(
            self.layerPage[
                self.selLayer]['addColButton'])
        if len(event.GetString()) > 0:
            btn.Enable(True)
        else:
            btn.Enable(False)

        event.Skip()

    def OnTableItemChange(self, event):
        """Rename column in the table"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['tableData'])
        name = self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameCol']).GetValue()
        nameTo = self.FindWindowById(self.layerPage[self.selLayer][
                                     'renameColTo']).GetValue()

        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]["table"]

        if not name or not nameTo:
            GError(parent=self,
                   message=_("Unable to rename column. "
                             "No column name defined."))
            return
        else:
            item = tlist.FindItem(start=-1, str=name)
            if item > -1:
                if tlist.FindItem(start=-1, str=nameTo) > -1:
                    GError(parent=self,
                           message=_("Unable to rename column <%(column)s> to "
                                     "<%(columnTo)s>. Column already exists "
                                     "in the table <%(table)s>.") %
                           {'column': name, 'columnTo': nameTo,
                            'table': table})
                    return
                else:
                    tlist.SetItemText(item, nameTo)

                    self.listOfCommands.append(('v.db.renamecolumn',
                                                {'map': self.dbMgrData['vectName'],
                                                 'layer': self.selLayer,
                                                 'column': '%s,%s' % (name, nameTo)}
                                                ))
            else:
                GError(
                    parent=self,
                    message=_(
                        "Unable to rename column. "
                        "Column <%(column)s> doesn't exist in the table <%(table)s>.") % {
                        'column': name,
                        'table': table})
                return

        # apply changes
        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        # update widgets
        self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameCol']).SetItems(
            self.dbMgrData['mapDBInfo'].GetColumns(table))
        self.FindWindowById(self.layerPage[self.selLayer][
                            'renameCol']).SetSelection(0)
        self.FindWindowById(self.layerPage[self.selLayer][
                            'renameColTo']).SetValue('')

        event.Skip()

    def OnTableRightUp(self, event):
        """Table description area, context menu"""
        if not hasattr(self, "popupTableID"):
            self.popupTableID1 = NewId()
            self.popupTableID2 = NewId()
            self.popupTableID3 = NewId()
            self.Bind(
                wx.EVT_MENU,
                self.OnTableItemDelete,
                id=self.popupTableID1)
            self.Bind(
                wx.EVT_MENU,
                self.OnTableItemDeleteAll,
                id=self.popupTableID2)
            self.Bind(wx.EVT_MENU, self.OnTableReload, id=self.popupTableID3)

        # generate popup-menu
        menu = Menu()
        menu.Append(self.popupTableID1, _("Drop selected column"))
        if self.FindWindowById(self.layerPage[self.selLayer][
                               'tableData']).GetFirstSelected() == -1:
            menu.Enable(self.popupTableID1, False)
        menu.Append(self.popupTableID2, _("Drop all columns"))
        menu.AppendSeparator()
        menu.Append(self.popupTableID3, _("Reload"))

        if not self.dbMgrData['editable']:
            menu.Enable(self.popupTableID1, False)
            menu.Enable(self.popupTableID2, False)

        self.PopupMenu(menu)
        menu.Destroy()

    def OnTableItemDelete(self, event):
        """Delete selected item(s) from the list"""
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['tableData'])

        item = tlist.GetFirstSelected()
        countSelected = tlist.GetSelectedItemCount()
        if UserSettings.Get(
                group='atm', key='askOnDeleteRec', subkey='enabled'):
            # if the user select more columns to delete, all the columns name
            # will appear the the warning dialog
            if tlist.GetSelectedItemCount() > 1:
                deleteColumns = "columns '%s'" % tlist.GetItemText(item)
                while item != -1:
                    item = tlist.GetNextSelected(item)
                    if item != -1:
                        deleteColumns += ", '%s'" % tlist.GetItemText(item)
            else:
                deleteColumns = "column '%s'" % tlist.GetItemText(item)
            deleteDialog = wx.MessageBox(
                parent=self,
                message=_(
                    "Selected %s will PERMANENTLY removed "
                    "from table. Do you want to drop the column?") %
                (deleteColumns),
                caption=_("Drop column(s)"),
                style=wx.YES_NO | wx.CENTRE)
            if deleteDialog != wx.YES:
                return False
        item = tlist.GetFirstSelected()
        while item != -1:
            self.listOfCommands.append(('v.db.dropcolumn',
                                        {'map': self.dbMgrData['vectName'],
                                         'layer': self.selLayer,
                                         'column': tlist.GetItemText(item)}
                                        ))
            tlist.DeleteItem(item)
            item = tlist.GetFirstSelected()

        # apply changes
        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        # update widgets
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameCol']).SetItems(
            self.dbMgrData['mapDBInfo'].GetColumns(table))
        self.FindWindowById(self.layerPage[self.selLayer][
                            'renameCol']).SetSelection(0)

        event.Skip()

    def OnTableItemDeleteAll(self, event):
        """Delete all items from the list"""
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        cols = self.dbMgrData['mapDBInfo'].GetColumns(table)
        keyColumn = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['key']
        if keyColumn in cols:
            cols.remove(keyColumn)

        if UserSettings.Get(
                group='atm', key='askOnDeleteRec', subkey='enabled'):
            deleteDialog = wx.MessageBox(
                parent=self,
                message=_(
                    "Selected columns\n%s\nwill PERMANENTLY removed "
                    "from table. Do you want to drop the columns?") %
                ('\n'.join(cols)),
                caption=_("Drop column(s)"),
                style=wx.YES_NO | wx.CENTRE)
            if deleteDialog != wx.YES:
                return False

        for col in cols:
            self.listOfCommands.append(('v.db.dropcolumn',
                                        {'map': self.dbMgrData['vectName'],
                                         'layer': self.selLayer,
                                         'column': col}
                                        ))
        self.FindWindowById(self.layerPage[self.selLayer][
                            'tableData']).DeleteAllItems()

        # apply changes
        self.ApplyCommands(self.listOfCommands, self.listOfSQLStatements)

        # update widgets
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameCol']).SetItems(
            self.dbMgrData['mapDBInfo'].GetColumns(table))
        self.FindWindowById(self.layerPage[self.selLayer][
                            'renameCol']).SetSelection(0)

        event.Skip()

    def OnTableReload(self, event=None):
        """Reload table description"""
        self.FindWindowById(
            self.layerPage[
                self.selLayer]['tableData']).Populate(
            update=True)
        self.listOfCommands = []

    def OnTableItemAdd(self, event):
        """Add new column to the table"""
        name = self.FindWindowById(
            self.layerPage[
                self.selLayer]['addColName']).GetValue()

        ctype = self.FindWindowById(self.layerPage[self.selLayer][
                                    'addColType']). GetStringSelection()

        length = int(
            self.FindWindowById(
                self.layerPage[
                    self.selLayer]['addColLength']). GetValue())

        # add item to the list of table columns
        tlist = self.FindWindowById(self.layerPage[self.selLayer]['tableData'])

        index = tlist.InsertItem(tlist.GetItemCount(), str(name))
        tlist.SetItem(index, 0, str(name))
        tlist.SetItem(index, 1, str(ctype))
        tlist.SetItem(index, 2, str(length))

        self.AddColumn(name, ctype, length)

        # update widgets
        table = self.dbMgrData['mapDBInfo'].layers[self.selLayer]['table']
        self.FindWindowById(self.layerPage[self.selLayer][
                            'addColName']).SetValue('')
        self.FindWindowById(
            self.layerPage[
                self.selLayer]['renameCol']).SetItems(
            self.dbMgrData['mapDBInfo'].GetColumns(table))
        self.FindWindowById(self.layerPage[self.selLayer][
                            'renameCol']).SetSelection(0)

        event.Skip()

    def UpdatePage(self, layer):

        if layer in self.layerPage.keys():
            table = self.dbMgrData['mapDBInfo'].layers[layer]['table']

            # update table description
            tlist = self.FindWindowById(self.layerPage[layer]['tableData'])
            tlist.Update(table=self.dbMgrData['mapDBInfo'].tables[table],
                         columns=self.dbMgrData['mapDBInfo'].GetColumns(table))
            self.OnTableReload(None)


class DbMgrLayersPage(wx.Panel):

    def __init__(self, parent, parentDbMgrBase):
        """Create layer manage page"""
        self.parentDbMgrBase = parentDbMgrBase
        self.dbMgrData = self.parentDbMgrBase.dbMgrData

        wx.Panel.__init__(self, parent=parent)
        splitterWin = wx.SplitterWindow(parent=self, id=wx.ID_ANY)
        splitterWin.SetMinimumPaneSize(100)

        #
        # list of layers
        #
        panelList = wx.Panel(parent=splitterWin, id=wx.ID_ANY)

        panelListSizer = wx.BoxSizer(wx.VERTICAL)
        layerBox = StaticBox(parent=panelList, id=wx.ID_ANY,
                             label=" %s " % _("List of layers"))
        layerSizer = wx.StaticBoxSizer(layerBox, wx.VERTICAL)

        self.layerList = self._createLayerDesc(panelList)
        self.layerList.Bind(
            wx.EVT_COMMAND_RIGHT_CLICK,
            self.OnLayerRightUp)  # wxMSW
        self.layerList.Bind(wx.EVT_RIGHT_UP, self.OnLayerRightUp)  # wxGTK

        layerSizer.Add(self.layerList,
                       flag=wx.ALL | wx.EXPAND,
                       proportion=1,
                       border=3)

        panelListSizer.Add(layerSizer,
                           flag=wx.ALL | wx.EXPAND,
                           proportion=1,
                           border=3)

        panelList.SetSizer(panelListSizer)

        #
        # manage part
        #
        panelManage = wx.Panel(parent=splitterWin, id=wx.ID_ANY)

        manageSizer = wx.BoxSizer(wx.VERTICAL)

        self.manageLayerBook = LayerBook(parent=panelManage, id=wx.ID_ANY,
                                         parentDialog=self)
        if not self.dbMgrData['editable']:
            self.manageLayerBook.Enable(False)

        manageSizer.Add(self.manageLayerBook,
                        proportion=1,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                        border=5)

        panelSizer = wx.BoxSizer(wx.VERTICAL)
        panelSizer.Add(splitterWin,
                       proportion=1,
                       flag=wx.EXPAND)

        panelManage.SetSizer(manageSizer)
        splitterWin.SplitHorizontally(panelList, panelManage, 100)
        splitterWin.Fit()
        self.SetSizer(panelSizer)

    def _createLayerDesc(self, parent):
        """Create list of linked layers"""
        tlist = LayerListCtrl(parent=parent, id=wx.ID_ANY,
                              layers=self.dbMgrData['mapDBInfo'].layers)

        tlist.Populate()
        # sorter
        # itemDataMap = list.Populate()
        # listmix.ColumnSorterMixin.__init__(self, 2)

        return tlist

    def UpdatePage(self):
        #
        # 'manage layers' page
        #
        # update list of layers

        #self.dbMgrData['mapDBInfo'] = VectorDBInfo(self.dbMgrData['vectName'])

        self.layerList.Update(self.dbMgrData['mapDBInfo'].layers)
        self.layerList.Populate(update=True)
        # update selected widgets
        listOfLayers = list(map(str, self.dbMgrData['mapDBInfo'].layers.keys()))
        # delete layer page
        self.manageLayerBook.deleteLayer.SetItems(listOfLayers)
        if len(listOfLayers) > 0:
            self.manageLayerBook.deleteLayer.SetStringSelection(
                listOfLayers
                [0])
            tableName = self.dbMgrData['mapDBInfo'].layers[int(listOfLayers[0])][
                'table']
            maxLayer = max(self.dbMgrData['mapDBInfo'].layers.keys())
        else:
            tableName = ''
            maxLayer = 0
        self.manageLayerBook.deleteTable.SetLabel(
            _('Drop also linked attribute table (%s)') %
            tableName)
        # add layer page
        self.manageLayerBook.addLayerWidgets['layer'][1].SetValue(
            maxLayer + 1)
        # modify layer
        self.manageLayerBook.modifyLayerWidgets[
            'layer'][1].SetItems(listOfLayers)
        self.manageLayerBook.OnChangeLayer(event=None)

    def OnLayerRightUp(self, event):
        """Layer description area, context menu"""
        pass


class TableListCtrl(ListCtrl,
                    listmix.ListCtrlAutoWidthMixin):
    #                    listmix.TextEditMixin):
    """Table description list"""

    def __init__(self, parent, id, table, columns, pos=wx.DefaultPosition,
                 size=wx.DefaultSize):

        self.parent = parent
        self.table = table
        self.columns = columns
        ListCtrl.__init__(self, parent, id, pos, size,
                          style=wx.LC_REPORT | wx.LC_HRULES | wx.LC_VRULES |
                          wx.BORDER_NONE)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        # listmix.TextEditMixin.__init__(self)

    def Update(self, table, columns):
        """Update column description"""
        self.table = table
        self.columns = columns

    def Populate(self, update=False):
        """Populate the list"""
        itemData = {}  # requested by sorter

        if not update:
            headings = [_("Column name"), _("Data type"), _("Data length")]
            i = 0
            for h in headings:
                self.InsertColumn(col=i, heading=h)
                i += 1
            self.SetColumnWidth(col=0, width=350)
            self.SetColumnWidth(col=1, width=175)
        else:
            self.DeleteAllItems()

        i = 0
        for column in self.columns:
            index = self.InsertItem(i, str(column))
            self.SetItem(index, 0, str(column))
            self.SetItem(index, 1, str(self.table[column]['type']))
            self.SetItem(index, 2, str(self.table[column]['length']))
            self.SetItemData(index, i)
            itemData[i] = (str(column),
                           str(self.table[column]['type']),
                           int(self.table[column]['length']))
            i = i + 1

        self.SendSizeEvent()

        return itemData


class LayerListCtrl(ListCtrl,
                    listmix.ListCtrlAutoWidthMixin):
                    # listmix.ColumnSorterMixin):
                    # listmix.TextEditMixin):
    """Layer description list"""

    def __init__(self, parent, id, layers,
                 pos=wx.DefaultPosition,
                 size=wx.DefaultSize):

        self.parent = parent
        self.layers = layers
        ListCtrl.__init__(self, parent, id, pos, size,
                          style=wx.LC_REPORT | wx.LC_HRULES | wx.LC_VRULES |
                          wx.BORDER_NONE)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        # listmix.TextEditMixin.__init__(self)

    def Update(self, layers):
        """Update description"""
        self.layers = layers

    def Populate(self, update=False):
        """Populate the list"""
        itemData = {}  # requested by sorter

        if not update:
            headings = [
                _("Layer"),
                _("Driver"),
                _("Database"),
                _("Table"),
                _("Key")]
            i = 0
            for h in headings:
                self.InsertColumn(col=i, heading=h)
                i += 1
        else:
            self.DeleteAllItems()

        i = 0
        for layer in self.layers.keys():
            index = self.InsertItem(i, str(layer))
            self.SetItem(index, 0, str(layer))
            database = str(self.layers[layer]['database'])
            driver = str(self.layers[layer]['driver'])
            table = str(self.layers[layer]['table'])
            key = str(self.layers[layer]['key'])
            self.SetItem(index, 1, driver)
            self.SetItem(index, 2, database)
            self.SetItem(index, 3, table)
            self.SetItem(index, 4, key)
            self.SetItemData(index, i)
            itemData[i] = (str(layer),
                           driver,
                           database,
                           table,
                           key)
            i += 1

        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(col=i, width=wx.LIST_AUTOSIZE)
            if self.GetColumnWidth(col=i) < 60:
                self.SetColumnWidth(col=i, width=60)

        self.SendSizeEvent()

        return itemData


class LayerBook(wx.Notebook):
    """Manage layers (add, delete, modify)"""

    def __init__(self, parent, id,
                 parentDialog,
                 style=wx.BK_DEFAULT):
        wx.Notebook.__init__(self, parent, id, style=style)

        self.parent = parent
        self.parentDialog = parentDialog
        self.mapDBInfo = self.parentDialog.dbMgrData['mapDBInfo']

        #
        # drivers
        #
        drivers = RunCommand('db.drivers',
                             quiet=True,
                             read=True,
                             flags='p')

        self.listOfDrivers = []
        for drv in drivers.splitlines():
            self.listOfDrivers.append(drv.strip())

        #
        # get default values
        #
        self.defaultConnect = {}
        connect = RunCommand('db.connect',
                             flags='p',
                             read=True,
                             quiet=True)

        for line in connect.splitlines():
            item, value = line.split(':', 1)
            self.defaultConnect[item.strip()] = value.strip()

        # really needed?
        # if len(self.defaultConnect['driver']) == 0 or \
        #        len(self.defaultConnect['database']) == 0:
        #     GWarning(parent = self.parent,
        #              message = _("Unknown default DB connection. "
        #                          "Please define DB connection using db.connect module."))

        self.defaultTables = self._getTables(self.defaultConnect['driver'],
                                             self.defaultConnect['database'])
        try:
            self.defaultColumns = self._getColumns(
                self.defaultConnect['driver'],
                self.defaultConnect['database'],
                self.defaultTables[0])
        except IndexError:
            self.defaultColumns = []

        self._createAddPage()
        self._createDeletePage()
        self._createModifyPage()

    def _createAddPage(self):
        """Add new layer"""
        self.addPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.AddPage(page=self.addPanel, text=_("Add layer"))

        try:
            maxLayer = max(self.mapDBInfo.layers.keys())
        except ValueError:
            maxLayer = 0

        # layer description

        layerBox = StaticBox(parent=self.addPanel, id=wx.ID_ANY,
                             label=" %s " % (_("Layer description")))
        layerSizer = wx.StaticBoxSizer(layerBox, wx.VERTICAL)

        #
        # list of layer widgets (label, value)
        #
        self.addLayerWidgets = {'layer':
                                (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                            label='%s:' % _("Layer")),
                                 SpinCtrl(parent=self.addPanel, id=wx.ID_ANY, size=(65, -1),
                                          initial=maxLayer + 1,
                                          min=1, max=1e6)),
                                'driver':
                                    (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                label='%s:' % _("Driver")),
                                     wx.Choice(parent=self.addPanel, id=wx.ID_ANY, size=(200, -1),
                                               choices=self.listOfDrivers)),
                                'database':
                                    (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                label='%s:' % _("Database")),
                                     TextCtrl(parent=self.addPanel, id=wx.ID_ANY,
                                              value='',
                                              style=wx.TE_PROCESS_ENTER)),
                                'table':
                                    (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                label='%s:' % _("Table")),
                                     wx.Choice(parent=self.addPanel, id=wx.ID_ANY, size=(200, -1),
                                               choices=self.defaultTables)),
                                'key':
                                    (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                label='%s:' % _("Key column")),
                                     wx.Choice(parent=self.addPanel, id=wx.ID_ANY, size=(200, -1),
                                               choices=self.defaultColumns)),
                                'addCat':
                                    (CheckBox(parent=self.addPanel, id=wx.ID_ANY,
                                              label=_("Insert record for each category into table")),
                                     None),
                                }

        # set default values for widgets
        self.addLayerWidgets['driver'][1].SetStringSelection(
            self.defaultConnect['driver'])
        self.addLayerWidgets['database'][1].SetValue(
            self.defaultConnect['database'])
        self.addLayerWidgets['table'][1].SetSelection(0)
        self.addLayerWidgets['key'][1].SetSelection(0)
        self.addLayerWidgets['addCat'][0].SetValue(True)
        # events
        self.addLayerWidgets['driver'][1].Bind(
            wx.EVT_CHOICE, self.OnDriverChanged)
        self.addLayerWidgets['database'][1].Bind(
            wx.EVT_TEXT_ENTER, self.OnDatabaseChanged)
        self.addLayerWidgets['table'][1].Bind(
            wx.EVT_CHOICE, self.OnTableChanged)

        # tooltips
        self.addLayerWidgets['addCat'][0].SetToolTip(
            _("You need to add categories " "by v.category module."))

        # table description
        tableBox = StaticBox(parent=self.addPanel, id=wx.ID_ANY,
                             label=" %s " % (_("Table description")))
        tableSizer = wx.StaticBoxSizer(tableBox, wx.VERTICAL)

        #
        # list of table widgets
        #
        keyCol = UserSettings.Get(group='atm', key='keycolumn', subkey='value')
        self.tableWidgets = {'table': (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                  label='%s:' % _("Table name")),
                                       TextCtrl(parent=self.addPanel, id=wx.ID_ANY,
                                                value='',
                                                style=wx.TE_PROCESS_ENTER)),
                             'key': (StaticText(parent=self.addPanel, id=wx.ID_ANY,
                                                label='%s:' % _("Key column")),
                                     TextCtrl(parent=self.addPanel, id=wx.ID_ANY,
                                              value=keyCol,
                                              style=wx.TE_PROCESS_ENTER))}
        # events
        self.tableWidgets['table'][1].Bind(
            wx.EVT_TEXT_ENTER, self.OnCreateTable)
        self.tableWidgets['key'][1].Bind(wx.EVT_TEXT_ENTER, self.OnCreateTable)

        btnTable = Button(self.addPanel, wx.ID_ANY, _("&Create table"),
                             size=(125, -1))
        btnTable.Bind(wx.EVT_BUTTON, self.OnCreateTable)

        btnLayer = Button(self.addPanel, wx.ID_ANY, _("&Add layer"),
                             size=(125, -1))
        btnLayer.Bind(wx.EVT_BUTTON, self.OnAddLayer)

        btnDefault = Button(self.addPanel, wx.ID_ANY, _("&Set default"),
                               size=(125, -1))
        btnDefault.Bind(wx.EVT_BUTTON, self.OnSetDefault)

        # do layout

        pageSizer = wx.BoxSizer(wx.HORIZONTAL)

        # data area
        dataSizer = wx.GridBagSizer(hgap=5, vgap=5)
        row = 0
        for key in ('layer', 'driver', 'database', 'table', 'key', 'addCat'):
            label, value = self.addLayerWidgets[key]
            if not value:
                span = (1, 2)
            else:
                span = (1, 1)
            dataSizer.Add(label,
                          flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0),
                          span=span)

            if not value:
                row += 1
                continue

            if key == 'layer':
                style = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT
            else:
                style = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND

            dataSizer.Add(value,
                          flag=style, pos=(row, 1))

            row += 1

        dataSizer.AddGrowableCol(1)
        layerSizer.Add(dataSizer,
                       proportion=1,
                       flag=wx.ALL | wx.EXPAND,
                       border=5)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(btnDefault,
                     proportion=0,
                     flag=wx.ALL | wx.ALIGN_LEFT,
                     border=5)

        btnSizer.Add((5, 5),
                     proportion=1,
                     flag=wx.ALL | wx.EXPAND,
                     border=5)

        btnSizer.Add(btnLayer,
                     proportion=0,
                     flag=wx.ALL,
                     border=5)

        layerSizer.Add(btnSizer,
                       proportion=0,
                       flag=wx.ALL | wx.EXPAND,
                       border=0)

        # data area
        dataSizer = wx.FlexGridSizer(cols=2, hgap=5, vgap=5)
        dataSizer.AddGrowableCol(1)
        for key in ['table', 'key']:
            label, value = self.tableWidgets[key]
            dataSizer.Add(label,
                          flag=wx.ALIGN_CENTER_VERTICAL)
            dataSizer.Add(value,
                          flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)

        tableSizer.Add(dataSizer,
                       proportion=1,
                       flag=wx.ALL | wx.EXPAND,
                       border=5)

        tableSizer.Add(btnTable,
                       proportion=0,
                       flag=wx.ALL | wx.ALIGN_RIGHT,
                       border=5)

        pageSizer.Add(layerSizer,
                      proportion=3,
                      flag=wx.ALL | wx.EXPAND,
                      border=3)

        pageSizer.Add(tableSizer,
                      proportion=2,
                      flag=wx.TOP | wx.BOTTOM | wx.RIGHT | wx.EXPAND,
                      border=3)
        layerSizer.FitInside(self.addPanel)

        self.addPanel.SetAutoLayout(True)
        self.addPanel.SetSizer(pageSizer)
        pageSizer.Fit(self.addPanel)

    def _createDeletePage(self):
        """Delete layer"""
        self.deletePanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.AddPage(page=self.deletePanel, text=_("Remove layer"))

        label = StaticText(parent=self.deletePanel, id=wx.ID_ANY,
                           label='%s:' % _("Layer to remove"))

        self.deleteLayer = ComboBox(
            parent=self.deletePanel, id=wx.ID_ANY, size=(100, -1),
            style=wx.CB_READONLY,
            choices=list(map(str, self.mapDBInfo.layers.keys())))
        self.deleteLayer.SetSelection(0)
        self.deleteLayer.Bind(wx.EVT_COMBOBOX, self.OnChangeLayer)

        try:
            tableName = self.mapDBInfo.layers[
                int(self.deleteLayer.GetStringSelection())]['table']
        except ValueError:
            tableName = ''

        self.deleteTable = CheckBox(
            parent=self.deletePanel,
            id=wx.ID_ANY,
            label=_('Drop also linked attribute table (%s)') %
            tableName)

        if tableName == '':
            self.deleteLayer.Enable(False)
            self.deleteTable.Enable(False)

        btnDelete = Button(
            self.deletePanel, wx.ID_DELETE, _("&Remove layer"),
            size=(125, -1))
        btnDelete.Bind(wx.EVT_BUTTON, self.OnDeleteLayer)

        #
        # do layout
        #
        pageSizer = wx.BoxSizer(wx.VERTICAL)

        dataSizer = wx.BoxSizer(wx.VERTICAL)

        flexSizer = wx.FlexGridSizer(cols=2, hgap=5, vgap=5)

        flexSizer.Add(label,
                      flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.deleteLayer,
                      flag=wx.ALIGN_CENTER_VERTICAL)

        dataSizer.Add(flexSizer,
                      proportion=0,
                      flag=wx.ALL | wx.EXPAND,
                      border=1)

        dataSizer.Add(self.deleteTable,
                      proportion=0,
                      flag=wx.ALL | wx.EXPAND,
                      border=1)

        pageSizer.Add(dataSizer,
                      proportion=1,
                      flag=wx.ALL | wx.EXPAND,
                      border=5)

        pageSizer.Add(btnDelete,
                      proportion=0,
                      flag=wx.ALL | wx.ALIGN_RIGHT,
                      border=5)

        self.deletePanel.SetSizer(pageSizer)

    def _createModifyPage(self):
        """Modify layer"""
        self.modifyPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.AddPage(page=self.modifyPanel, text=_("Modify layer"))

        #
        # list of layer widgets (label, value)
        #
        self.modifyLayerWidgets = {'layer':
                                   (StaticText(parent=self.modifyPanel, id=wx.ID_ANY,
                                               label='%s:' % _("Layer")),
                                    ComboBox(parent=self.modifyPanel, id=wx.ID_ANY,
                                                size=(100, -1),
                                                style=wx.CB_READONLY,
                                                choices=list(map(str,
                                                            self.mapDBInfo.layers.keys())))),
                                   'driver':
                                       (StaticText(parent=self.modifyPanel, id=wx.ID_ANY,
                                                   label='%s:' % _("Driver")),
                                        wx.Choice(parent=self.modifyPanel, id=wx.ID_ANY,
                                                  size=(200, -1),
                                                  choices=self.listOfDrivers)),
                                   'database':
                                       (StaticText(parent=self.modifyPanel, id=wx.ID_ANY,
                                                   label='%s:' % _("Database")),
                                        TextCtrl(parent=self.modifyPanel, id=wx.ID_ANY,
                                                 value='', size=(350, -1),
                                                 style=wx.TE_PROCESS_ENTER)),
                                   'table':
                                       (StaticText(parent=self.modifyPanel, id=wx.ID_ANY,
                                                   label='%s:' % _("Table")),
                                        wx.Choice(parent=self.modifyPanel, id=wx.ID_ANY,
                                                  size=(200, -1),
                                                  choices=self.defaultTables)),
                                   'key':
                                       (StaticText(parent=self.modifyPanel, id=wx.ID_ANY,
                                                   label='%s:' % _("Key column")),
                                        wx.Choice(parent=self.modifyPanel, id=wx.ID_ANY,
                                                  size=(200, -1),
                                                  choices=self.defaultColumns))}

        # set default values for widgets
        self.modifyLayerWidgets['layer'][1].SetSelection(0)
        try:
            layer = int(self.modifyLayerWidgets[
                        'layer'][1].GetStringSelection())
        except ValueError:
            layer = None
            for label in self.modifyLayerWidgets.keys():
                self.modifyLayerWidgets[label][1].Enable(False)

        if layer:
            driver = self.mapDBInfo.layers[layer]['driver']
            database = self.mapDBInfo.layers[layer]['database']
            table = self.mapDBInfo.layers[layer]['table']

            listOfColumns = self._getColumns(driver, database, table)
            self.modifyLayerWidgets['driver'][1].SetStringSelection(driver)
            self.modifyLayerWidgets['database'][1].SetValue(database)
            if table in self.modifyLayerWidgets['table'][1].GetItems():
                self.modifyLayerWidgets['table'][1].SetStringSelection(table)
            else:
                if self.defaultConnect['schema'] != '':
                    # try with default schema
                    table = self.defaultConnect['schema'] + table
                else:
                    table = 'public.' + table  # try with 'public' schema
                self.modifyLayerWidgets['table'][1].SetStringSelection(table)
            self.modifyLayerWidgets['key'][1].SetItems(listOfColumns)
            self.modifyLayerWidgets['key'][1].SetSelection(0)

        # events
        self.modifyLayerWidgets['layer'][1].Bind(
            wx.EVT_COMBOBOX, self.OnChangeLayer)
        # self.modifyLayerWidgets['driver'][1].Bind(wx.EVT_CHOICE, self.OnDriverChanged)
        # self.modifyLayerWidgets['database'][1].Bind(wx.EVT_TEXT_ENTER, self.OnDatabaseChanged)
        # self.modifyLayerWidgets['table'][1].Bind(wx.EVT_CHOICE, self.OnTableChanged)

        btnModify = Button(
            self.modifyPanel, wx.ID_DELETE, _("&Modify layer"),
            size=(125, -1))
        btnModify.Bind(wx.EVT_BUTTON, self.OnModifyLayer)

        #
        # do layout
        #
        pageSizer = wx.BoxSizer(wx.VERTICAL)

        # data area
        dataSizer = wx.FlexGridSizer(cols=2, hgap=5, vgap=5)
        dataSizer.AddGrowableCol(1)
        for key in ('layer', 'driver', 'database', 'table', 'key'):
            label, value = self.modifyLayerWidgets[key]
            dataSizer.Add(label,
                          flag=wx.ALIGN_CENTER_VERTICAL)
            if key == 'layer':
                dataSizer.Add(value,
                              flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
            else:
                dataSizer.Add(value,
                              flag=wx.ALIGN_CENTER_VERTICAL)

        pageSizer.Add(dataSizer,
                      proportion=1,
                      flag=wx.ALL | wx.EXPAND,
                      border=5)

        pageSizer.Add(btnModify,
                      proportion=0,
                      flag=wx.ALL | wx.ALIGN_RIGHT,
                      border=5)

        self.modifyPanel.SetSizer(pageSizer)

    def _getTables(self, driver, database):
        """Get list of tables for given driver and database"""
        tables = []

        ret = RunCommand('db.tables',
                         parent=self,
                         read=True,
                         flags='p',
                         driver=driver,
                         database=database)

        if ret is None:
            GError(
                parent=self, message=_(
                    "Unable to get list of tables.\n"
                    "Please use db.connect to set database parameters."))

            return tables

        for table in ret.splitlines():
            tables.append(table)

        return tables

    def _getColumns(self, driver, database, table):
        """Get list of column of given table"""
        columns = []

        ret = RunCommand('db.columns',
                         parent=self,
                         quiet=True,
                         read=True,
                         driver=driver,
                         database=database,
                         table=table)

        if ret is None:
            return columns

        for column in ret.splitlines():
            columns.append(column)

        return columns

    def OnDriverChanged(self, event):
        """Driver selection changed, update list of tables"""
        driver = event.GetString()
        database = self.addLayerWidgets['database'][1].GetValue()

        winTable = self.addLayerWidgets['table'][1]
        winKey = self.addLayerWidgets['key'][1]
        tables = self._getTables(driver, database)

        winTable.SetItems(tables)
        winTable.SetSelection(0)

        if len(tables) == 0:
            winKey.SetItems([])

        event.Skip()

    def OnDatabaseChanged(self, event):
        """Database selection changed, update list of tables"""
        event.Skip()

    def OnTableChanged(self, event):
        """Table name changed, update list of columns"""
        driver = self.addLayerWidgets['driver'][1].GetStringSelection()
        database = self.addLayerWidgets['database'][1].GetValue()
        table = event.GetString()

        win = self.addLayerWidgets['key'][1]
        cols = self._getColumns(driver, database, table)
        win.SetItems(cols)
        win.SetSelection(0)

        event.Skip()

    def OnSetDefault(self, event):
        """Set default values"""
        driver = self.addLayerWidgets['driver'][1]
        database = self.addLayerWidgets['database'][1]
        table = self.addLayerWidgets['table'][1]
        key = self.addLayerWidgets['key'][1]

        driver.SetStringSelection(self.defaultConnect['driver'])
        database.SetValue(self.defaultConnect['database'])
        tables = self._getTables(self.defaultConnect['driver'],
                                 self.defaultConnect['database'])
        table.SetItems(tables)
        table.SetSelection(0)
        if len(tables) == 0:
            key.SetItems([])
        else:
            cols = self._getColumns(self.defaultConnect['driver'],
                                    self.defaultConnect['database'],
                                    tables[0])
            key.SetItems(cols)
            key.SetSelection(0)

        event.Skip()

    def OnCreateTable(self, event):
        """Create new table (name and key column given)"""
        driver = self.addLayerWidgets['driver'][1].GetStringSelection()
        database = self.addLayerWidgets['database'][1].GetValue()
        table = self.tableWidgets['table'][1].GetValue()
        key = self.tableWidgets['key'][1].GetValue()

        if not table or not key:
            GError(parent=self,
                   message=_("Unable to create new table. "
                             "Table name or key column name is missing."))
            return

        if table in self.addLayerWidgets['table'][1].GetItems():
            GError(
                parent=self, message=_(
                    "Unable to create new table. "
                    "Table <%s> already exists in the database.") %
                table)
            return

        # create table
        sql = 'CREATE TABLE %s (%s INTEGER)' % (table, key)

        RunCommand('db.execute',
                   quiet=True,
                   parent=self,
                   stdin=sql,
                   input='-',
                   driver=driver,
                   database=database)

        # update list of tables
        tableList = self.addLayerWidgets['table'][1]
        tableList.SetItems(self._getTables(driver, database))
        tableList.SetStringSelection(table)

        # update key column selection
        keyList = self.addLayerWidgets['key'][1]
        keyList.SetItems(self._getColumns(driver, database, table))
        keyList.SetStringSelection(key)

        event.Skip()

    def OnAddLayer(self, event):
        """Add new layer to vector map"""
        layer = int(self.addLayerWidgets['layer'][1].GetValue())
        layerWin = self.addLayerWidgets['layer'][1]
        driver = self.addLayerWidgets['driver'][1].GetStringSelection()
        database = self.addLayerWidgets['database'][1].GetValue()
        table = self.addLayerWidgets['table'][1].GetStringSelection()
        key = self.addLayerWidgets['key'][1].GetStringSelection()

        if layer in self.mapDBInfo.layers.keys():
            GError(
                parent=self,
                message=_(
                    "Unable to add new layer to vector map <%(vector)s>. "
                    "Layer %(layer)d already exists.") %
                {'vector': self.mapDBInfo.map, 'layer': layer})
            return

        # add new layer
        ret = RunCommand('v.db.connect',
                         parent=self,
                         quiet=True,
                         map=self.mapDBInfo.map,
                         driver=driver,
                         database=database,
                         table=table,
                         key=key,
                         layer=layer)

        # insert records into table if required
        if self.addLayerWidgets['addCat'][0].IsChecked():
            RunCommand('v.to.db',
                       parent=self,
                       quiet=True,
                       map=self.mapDBInfo.map,
                       layer=layer,
                       qlayer=layer,
                       option='cat',
                       columns=key)

        if ret == 0:
            # update dialog (only for new layer)
            self.parentDialog.parentDbMgrBase.UpdateDialog(layer=layer)
            # update db info
            self.mapDBInfo = self.parentDialog.dbMgrData['mapDBInfo']
            # increase layer number
            layerWin.SetValue(layer + 1)

        if len(self.mapDBInfo.layers.keys()) == 1:
            # first layer add --- enable previously disabled widgets
            self.deleteLayer.Enable()
            self.deleteTable.Enable()
            for label in self.modifyLayerWidgets.keys():
                self.modifyLayerWidgets[label][1].Enable()

    def OnDeleteLayer(self, event):
        """Delete layer"""
        try:
            layer = int(self.deleteLayer.GetValue())
        except:
            return

        RunCommand('v.db.connect',
                   parent=self,
                   flags='d',
                   map=self.mapDBInfo.map,
                   layer=layer)

        # drop also table linked to layer which is deleted
        if self.deleteTable.IsChecked():
            driver = self.addLayerWidgets['driver'][1].GetStringSelection()
            database = self.addLayerWidgets['database'][1].GetValue()
            table = self.mapDBInfo.layers[layer]['table']
            sql = 'DROP TABLE %s' % (table)

            RunCommand('db.execute',
                       input='-',
                       parent=self,
                       stdin=sql,
                       quiet=True,
                       driver=driver,
                       database=database)

            # update list of tables
            tableList = self.addLayerWidgets['table'][1]
            tableList.SetItems(self._getTables(driver, database))
            tableList.SetStringSelection(table)

        # update dialog
        self.parentDialog.parentDbMgrBase.UpdateDialog(layer=layer)
        # update db info
        self.mapDBInfo = self.parentDialog.dbMgrData['mapDBInfo']

        if len(self.mapDBInfo.layers.keys()) == 0:
            # disable selected widgets
            self.deleteLayer.Enable(False)
            self.deleteTable.Enable(False)
            for label in self.modifyLayerWidgets.keys():
                self.modifyLayerWidgets[label][1].Enable(False)

        event.Skip()

    def OnChangeLayer(self, event):
        """Layer number of layer to be deleted is changed"""
        try:
            layer = int(event.GetString())
        except:
            try:
                layer = self.mapDBInfo.layers.keys()[0]
            except:
                return

        if self.GetCurrentPage() == self.modifyPanel:
            driver = self.mapDBInfo.layers[layer]['driver']
            database = self.mapDBInfo.layers[layer]['database']
            table = self.mapDBInfo.layers[layer]['table']
            listOfColumns = self._getColumns(driver, database, table)
            self.modifyLayerWidgets['driver'][1].SetStringSelection(driver)
            self.modifyLayerWidgets['database'][1].SetValue(database)
            self.modifyLayerWidgets['table'][1].SetStringSelection(table)
            self.modifyLayerWidgets['key'][1].SetItems(listOfColumns)
            self.modifyLayerWidgets['key'][1].SetSelection(0)
        else:
            self.deleteTable.SetLabel(
                _('Drop also linked attribute table (%s)') %
                self.mapDBInfo.layers[layer]['table'])
        if event:
            event.Skip()

    def OnModifyLayer(self, event):
        """Modify layer connection settings"""

        layer = int(self.modifyLayerWidgets['layer'][1].GetStringSelection())

        modify = False
        if self.modifyLayerWidgets['driver'][1].GetStringSelection() != \
                self.mapDBInfo.layers[layer]['driver'] or \
                self.modifyLayerWidgets['database'][1].GetStringSelection() != \
                self.mapDBInfo.layers[layer]['database'] or \
                self.modifyLayerWidgets['table'][1].GetStringSelection() != \
                self.mapDBInfo.layers[layer]['table'] or \
                self.modifyLayerWidgets['key'][1].GetStringSelection() != \
                self.mapDBInfo.layers[layer]['key']:
            modify = True

        if modify:
            # delete layer
            RunCommand('v.db.connect',
                       parent=self,
                       quiet=True,
                       flags='d',
                       map=self.mapDBInfo.map,
                       layer=layer)

            # add modified layer
            RunCommand(
                'v.db.connect',
                quiet=True,
                map=self.mapDBInfo.map,
                driver=self.modifyLayerWidgets['driver'][1].GetStringSelection(),
                database=self.modifyLayerWidgets['database'][1].GetValue(),
                table=self.modifyLayerWidgets['table'][1].GetStringSelection(),
                key=self.modifyLayerWidgets['key'][1].GetStringSelection(),
                layer=int(layer))

            # update dialog (only for new layer)
            self.parentDialog.parentDbMgrBase.UpdateDialog(layer=layer)
            # update db info
            self.mapDBInfo = self.parentDialog.dbMgrData['mapDBInfo']

        event.Skip()


class FieldStatistics(wx.Frame):

    def __init__(self, parent, id=wx.ID_ANY,
                 style=wx.DEFAULT_FRAME_STYLE, **kwargs):
        """Dialog to display and save statistics of field stats
        """
        self.parent = parent
        wx.Frame.__init__(self, parent, id, style=style, **kwargs)

        self.SetTitle(_("Field statistics"))
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    globalvar.ICONDIR,
                    'grass_sql.ico'),
                wx.BITMAP_TYPE_ICO))

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.sp = scrolled.ScrolledPanel(parent=self.panel, id=wx.ID_ANY, size=(
            250, 150), style=wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER, name="Statistics")
        self.text = TextCtrl(
            parent=self.sp,
            id=wx.ID_ANY,
            style=wx.TE_MULTILINE | wx.TE_READONLY)
        self.text.SetBackgroundColour("white")

        # buttons
        self.btnClipboard = Button(parent=self.panel, id=wx.ID_COPY)
        self.btnClipboard.SetToolTip(
            _("Copy statistics the clipboard (Ctrl+C)"))
        self.btnCancel = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnCancel.SetDefault()

        # bindings
        self.btnCancel.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btnClipboard.Bind(wx.EVT_BUTTON, self.OnCopy)

        self._layout()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        txtSizer = wx.BoxSizer(wx.VERTICAL)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)

        txtSizer.Add(self.text, proportion=1, flag=wx.EXPAND |
                     wx.ALL, border=5)

        self.sp.SetSizer(txtSizer)
        self.sp.SetAutoLayout(True)
        self.sp.SetupScrolling()

        sizer.Add(self.sp, proportion=1, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT | wx.BOTTOM, border=3)

        line = wx.StaticLine(parent=self.panel, id=wx.ID_ANY,
                             size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=3)

        # buttons
        btnSizer.Add(self.btnClipboard, proportion=0,
                     flag=wx.ALL, border=5)
        btnSizer.Add(self.btnCancel, proportion=0,
                     flag=wx.ALL, border=5)
        sizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.ALIGN_RIGHT | wx.ALL,
            border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

    def OnCopy(self, event):
        """!Copy the statistics to the clipboard
        """
        stats = self.text.GetValue()
        rdata = wx.TextDataObject()
        rdata.SetText(stats)

        if wx.TheClipboard.Open():
            wx.TheClipboard.SetData(rdata)
            wx.TheClipboard.Close()

    def OnClose(self, event):
        """!Button 'Close' pressed
        """
        self.Close(True)

    def Update(self, driver, database, table, column):
        """!Update statistics for given column

        :param: column column name
        """
        if driver == 'dbf':
            GError(parent=self,
                   message=_("Statistics is not support for DBF tables."))
            self.Close()
            return

        fd, sqlFilePath = tempfile.mkstemp(text=True)
        sqlFile = open(sqlFilePath, 'w')
        stats = ['count', 'min', 'max', 'avg', 'sum', 'null']
        for fn in stats:
            if fn == 'null':
                sqlFile.write(
                    'select count(*) from %s where %s is null;%s' %
                    (table, column, '\n'))
            else:
                sqlFile.write(
                    'select %s(%s) from %s;%s' %
                    (fn, column, table, '\n'))
        sqlFile.close()

        dataStr = RunCommand('db.select',
                             parent=self.parent,
                             read=True,
                             flags='c',
                             input=sqlFilePath,
                             driver=driver,
                             database=database)
        if not dataStr:
            GError(parent=self.parent,
                   message=_("Unable to calculte statistics."))
            self.Close()
            return

        dataLines = dataStr.splitlines()
        if len(dataLines) != len(stats):
            GError(
                parent=self.parent, message=_(
                    "Unable to calculte statistics. "
                    "Invalid number of lines %d (should be %d).") %
                (len(dataLines), len(stats)))
            self.Close()
            return

        # calculate stddev
        avg = float(dataLines[stats.index('avg')])
        count = float(dataLines[stats.index('count')])
        sql = "select (%(column)s - %(avg)f)*(%(column)s - %(avg)f) from %(table)s" % {
            'column': column, 'avg': avg, 'table': table}
        dataVar = RunCommand('db.select',
                             parent=self.parent,
                             read=True,
                             flags='c',
                             sql=sql,
                             driver=driver,
                             database=database)
        if not dataVar:
            GWarning(parent=self.parent,
                     message=_("Unable to calculte standard deviation."))
        varSum = 0
        for var in decode(dataVar).splitlines():
            varSum += float(var)
        stddev = math.sqrt(varSum / count)

        self.SetTitle(_("Field statistics <%s>") % column)
        self.text.Clear()
        for idx in range(len(stats)):
            self.text.AppendText('%s: %s\n' % (stats[idx], dataLines[idx]))
        self.text.AppendText('stddev: %f\n' % stddev)
