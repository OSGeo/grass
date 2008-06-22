"""
MODULE:    location_wizard.py

CLASSES:
    * BaseClass
    * TitledPage
    * DatabasePage
    * CoordinateSystemPage
    * ProjectionsPage
    * ItemList
    * ProjTypePage
    * DatumPage
    * EllipsePage
    * GeoreferencedFilePage
    * EPSGPage
    * CustomPage
    * SummaryPage
    * RegionDef
    * LocationWizard
    * SelectDatumDialog

PURPOSE:   Create a new GRASS Location. User can choose from multiple methods.

AUTHORS:   The GRASS Development Team
           Michael Barton
           Jachym Cepicky
           Martin Landa <landa.martin gmail.com>
           
COPYRIGHT: (C) 2006-2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""
import os
import shutil
import re
import string
import sys
import locale

import wx
import wx.lib.mixins.listctrl as listmix
import wx.wizard as wiz

import gcmd
import globalvar
import utils
try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR)
    sys.path.append(CompatPath)
    from compat import subprocess

global coordsys
global north
global south
global east
global west
global resolution
global wizerror

coordsys = ''
north = ''
south = ''
east = ''
west = ''
resolution = ''

class BaseClass(wx.Object):
    """Base class providing basic methods"""
    def __init__(self):
        pass

    def MakeLabel(self, text="", style=wx.ALIGN_LEFT):
        """Make aligned label"""
        return wx.StaticText(parent=self, id=wx.ID_ANY, label=text,
                             style=style)

    def MakeTextCtrl(self, text='', size=(100,-1), style=0):
        """Generic text control"""
        return wx.TextCtrl(parent=self, id=wx.ID_ANY, value=text,
                           size=size, style=style)

    def MakeButton(self, text, id=wx.ID_ANY, size=(-1,-1)):
        """Generic button"""
        return wx.Button(parent=self, id=id, label=text,
                         size=size)

class TitledPage(BaseClass, wiz.WizardPageSimple):
    """
    Class to make wizard pages. Generic methods to make
    labels, text entries, and buttons.
    """
    def __init__(self, parent, title):

        self.page = wiz.WizardPageSimple.__init__(self, parent)

        # page title
        self.title = wx.StaticText(parent=self, id=wx.ID_ANY, label=title)
        self.title.SetFont(wx.Font(13, wx.SWISS, wx.NORMAL, wx.BOLD))

        # main sizer
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        
    def DoLayout(self):
        """Do page layout"""
      
        tmpsizer = wx.BoxSizer(wx.VERTICAL)

        tmpsizer.Add(item=self.title, proportion=0,
                     flag=wx.ALIGN_CENTRE | wx.ALL,
                     border=5)
        tmpsizer.Add(item=wx.StaticLine(self, -1), proportion=0,
                     flag=wx.EXPAND | wx.ALL,
                     border=0)
        tmpsizer.Add(item=self.sizer, proportion=1,
                     flag=wx.EXPAND | wx.ALL,
                     border=5)

        self.SetAutoLayout(True)
        self.SetSizer(tmpsizer)
        # tmpsizer.Fit(self)
        self.Layout()

class DatabasePage(TitledPage):
    """
    Wizard page for setting GIS data directory and location name
    """
    def __init__(self, wizard, parent, grassdatabase):
        TitledPage.__init__(self, wizard, _("Define GRASS Database and Location Name"))

        self.grassdatabase = grassdatabase
        self.location = ''

        # buttons
        self.bbrowse = self.MakeButton(_("Browse"))

        # text controls
        self.tgisdbase = self.MakeTextCtrl(grassdatabase, size=(300, -1))
        self.tlocation = self.MakeTextCtrl("newLocation", size=(300, -1))
        
        # layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item=self.MakeLabel(_("GIS Data Directory:")),
                       flag=wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5,
                       pos=(1, 1))
        self.sizer.Add(item=self.tgisdbase,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5,
                       pos=(1, 2))
        self.sizer.Add(item=self.bbrowse,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5,
                       pos=(1, 3))
        #
        self.sizer.Add(item=self.MakeLabel("%s:" % _("Project Location")),
                       flag=wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5,
                       pos=(2, 1))
        self.sizer.Add(item=self.tlocation,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5,
                       pos=(2, 2))
        # self.sizer.Add(item=self.MakeLabel(_("(projection/coordinate system)")),
        #                flag=wx.ALIGN_LEFT |
        #                wx.ALIGN_CENTER_VERTICAL |
        #                wx.ALL, border=5,
        #                pos=(2, 4))

        # bindings
        self.Bind(wx.EVT_BUTTON,                self.OnBrowse, self.bbrowse)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED,  self.OnEnterPage)
        self.tgisdbase.Bind(wx.EVT_TEXT,        self.OnChangeName)
        self.tlocation.Bind(wx.EVT_TEXT,        self.OnChangeName)

        # do page layout
        # self.DoLayout()

    def OnChangeName(self, event):
        """Name for new location was changed"""
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(event.GetString()) > 0:
            if not nextButton.IsEnabled():
                nextButton.Enable()
        else:
            nextButton.Disable()

        event.Skip()

    def OnBrowse(self, event):
        dlg = wx.DirDialog(self, _("Choose GRASS data directory:"),
                           os.getcwd(), wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            self.grassdatabase = dlg.GetPath()
            self.tgisdbase.SetValue(self.grassdatabase)
            
        dlg.Destroy()

    def OnPageChanging(self,event=None):
        error = ''
        if os.path.isdir(os.path.join(self.tgisdbase.GetValue(), self.tlocation.GetValue())):
            error = _("Location already exists in GRASS Database.")

        if error != '':
            dlg = wx.MessageDialog(parent=self, message="%s <%s>.%s%s" % (_("Unable to create location"),
                                                                          str(self.tlocation.GetValue()),
                                                                          os.linesep,
                                                                          error),
                                   caption=_("Error"),  style=wx.OK | wx.ICON_ERROR)
            
            dlg.ShowModal()
            dlg.Destroy()
            event.Veto()
            return

        self.location = self.tlocation.GetValue()
        self.grassdatabase = self.tgisdbase.GetValue()

    def OnEnterPage(self, event):
        """Wizard page changed"""
        self.grassdatabase = self.tgisdbase.GetValue()
        self.location = self.tlocation.GetValue()

        event.Skip()
        
class CoordinateSystemPage(TitledPage):
    """
    Wizard page for choosing method for location creation
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose method for creating a new location"))

        self.parent = parent
        global coordsys

        # toggles
        self.radio1 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Select coordinate system"),
                                     style = wx.RB_GROUP)
        self.radio2 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Select EPSG code of coordinate system"))
        self.radio3 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Use coordinate system of selected "
                                             "georeferenced file"))
        self.radio4 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Use coordinate system of selected "
                                             "WKT or PRJ file"))
        self.radio5 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Create custom PROJ.4 parameters "
                                             "string for coordinate system"))
        self.radio6 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Use arbitrary non-earth "
                                             "coordinate system (XY)"))
        # layout
        self.sizer.AddGrowableCol(1)
        self.sizer.Add(item=self.radio1,
                       flag=wx.ALIGN_LEFT, pos=(1, 1))
        self.sizer.Add(item=self.radio2,
                       flag=wx.ALIGN_LEFT, pos=(2, 1))
        self.sizer.Add(item=self.radio3,
                       flag=wx.ALIGN_LEFT, pos=(3, 1))
        self.sizer.Add(item=self.radio4,
                       flag=wx.ALIGN_LEFT, pos=(4, 1))
        self.sizer.Add(item=self.radio5,
                       flag=wx.ALIGN_LEFT, pos=(5, 1))
        self.sizer.Add(item=self.radio6,
                       flag=wx.ALIGN_LEFT, pos=(6, 1))

        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio1.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio2.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio3.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio4.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio5.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio6.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED,  self.OnEnterPage)
        
        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event):
        global coordsys
        if event.GetDirection() and not coordsys:
            coordsys = "proj"
            self.SetNext(self.parent.projpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
            
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()
    
    def SetVal(self, event):
        """Choose method"""
        global coordsys
        if event.GetId() == self.radio1.GetId():
            coordsys = "proj"
            self.SetNext(self.parent.projpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
        elif event.GetId() == self.radio2.GetId():
            coordsys = "epsg"
            self.SetNext(self.parent.epsgpage)
            self.parent.sumpage.SetPrev(self.parent.epsgpage)
        elif event.GetId() == self.radio3.GetId():
            coordsys = "file"
            self.SetNext(self.parent.filepage)
            self.parent.sumpage.SetPrev(self.parent.filepage)
        elif event.GetId() == self.radio4.GetId():
            coordsys = "wkt"
            self.SetNext(self.parent.wktpage)
            self.parent.sumpage.SetPrev(self.parent.wktpage)
        elif event.GetId() == self.radio5.GetId():
            coordsys = "custom"
            self.SetNext(self.parent.custompage)
            self.parent.sumpage.SetPrev(self.parent.custompage)
        elif event.GetId() == self.radio6.GetId():
            coordsys = "xy"
            self.SetNext(self.parent.sumpage)
            self.parent.sumpage.SetPrev(self.parent.csystemspage)

class ProjectionsPage(TitledPage):
    """
    Wizard page for selecting projection (select coordinate system option)
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose projection"))

        self.parent = parent
        self.proj = ''
        self.projdesc = ''

        # text input
        self.tproj = self.MakeTextCtrl("", size=(200,-1))
        
        # search box
        self.searchb = wx.SearchCtrl(self, size=(200,-1),
                                     style=wx.TE_PROCESS_ENTER)

        # projection list
        self.projlist = ItemList(self, data=self.parent.projections.items(),
                                 columns=[_('Code'), _('Description')])
        # layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item=self.MakeLabel(_("Projection code:")),
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tproj,
                       flag=wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL,
                       border=5, pos=(1, 2))

        self.sizer.Add(item=self.MakeLabel(_("Search in description:")),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                       border=5, pos=(2, 1))
        self.sizer.Add(item=self.searchb,
                       flag=wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL,
                       border=5, pos=(2, 2))
        
        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item=self.projlist,
                       flag=wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(3, 1), span=(1, 3))

        # events
        self.tproj.Bind(wx.EVT_TEXT, self.OnText)
        self.tproj.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)
        self.projlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED,  self.OnEnterPage)

        # do layout
        # self.Layout()

    def OnPageChanging(self,event):
        if event.GetDirection() and self.proj not in self.parent.projections:
            event.Veto()
        if self.proj == 'utm':
            self.parent.projtypepage.text_utm.SetEditable(True)
            self.parent.projtypepage.hemischoices = ['north','south']
        else:
            self.parent.projtypepage.text_utm.SetValue('')
            self.parent.projtypepage.text_utm.SetEditable(False)
            self.parent.projtypepage.hemischoices = []

    def OnText(self, event):
        """Projection name changed"""
        self.proj = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.proj) == 0 and nextButton.IsEnabled():
            nextButton.Enable(False)
        
        if self.proj in self.parent.projections:
            self.projdesc = self.parent.projections[self.proj]
            if not nextButton.IsEnabled():
                nextButton.Enable()

    def OnEnterPage(self, event):
        if len(self.proj) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()
    
    def OnSearch(self, event):
        """Search projection by desc"""
        str = event.GetString()
        try:
            self.proj, self.projdesc = self.projlist.Search(index=1, str=event.GetString())
        except:
            self.proj = self.projdesc = ''

        self.tproj.SetValue(self.proj)

        event.Skip()

    def OnItemSelected(self, event):
        """Projection selected"""
        index = event.m_itemIndex

        # set values
        self.proj = self.projlist.GetItem(index, 0).GetText()
        self.projdesc = self.projlist.GetItem(index, 0).GetText()
        self.tproj.SetValue(self.proj)

class ItemList(wx.ListCtrl,
               listmix.ListCtrlAutoWidthMixin,
               listmix.ColumnSorterMixin):
    """Generic list (for projections, ellipsoids, etc.)"""

    def __init__(self, parent, columns, data=None):
        wx.ListCtrl.__init__(self, parent=parent, id=wx.ID_ANY,
                             style=wx.LC_REPORT |
                             wx.LC_VIRTUAL | 
                             wx.LC_HRULES |
                             wx.LC_VRULES |
                             wx.LC_SINGLE_SEL |
                             wx.LC_SORT_ASCENDING, size=(400, 100))

        # original data or None
        self.sourceData = data
        
        #
        # insert columns
        #
        i = 0
        for column in columns:
            self.InsertColumn(i, column)
            i += 1

        if self.sourceData:
            self.Populate()
        else:
            for i in range(self.GetColumnCount()):
                self.SetColumnWidth(i, wx.LIST_AUTOSIZE_USEHEADER)

        #
        # listmix
        #
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.ColumnSorterMixin.__init__(self, self.GetColumnCount())
            
        #
        # add some attributes
        #
        self.attr1 = wx.ListItemAttr()
        self.attr1.SetBackgroundColour(wx.Colour(238,238,238))
        self.attr2 = wx.ListItemAttr()
        self.attr2.SetBackgroundColour("white")
        self.il = wx.ImageList(16, 16)
        self.sm_up = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_GO_UP,   wx.ART_TOOLBAR,
                                                          (16,16)))
        self.sm_dn = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_GO_DOWN, wx.ART_TOOLBAR,
                                                          (16,16)))
        self.SetImageList(self.il, wx.IMAGE_LIST_SMALL)

        #
        # sort by first column
        #
        if self.sourceData:
            self.SortListItems(col=0, ascending=True)

        #
        # bindings
        #
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColumnClick)

    def Populate(self, data=None, update=False):
        """Populate list"""
        self.itemDataMap  = {}
        self.itemIndexMap = []
        
        if data is None:
            data = self.sourceData
        elif update:
            self.sourceData = data

        try:
            data.sort()
            self.DeleteAllItems()
            row = 0
            for value in data:
                # index = self.InsertStringItem(sys.maxint, str(value[0]))
                self.itemDataMap[row] = [value[0]]
                for i in range(1, len(value)):
                    # try:
                    # self.SetStringItem(index, i, str(value[i]))
                    # except:
                    # self.SetStringItem(index, i, unicode(str(value[i]), 'latin1'))
                    self.itemDataMap[row].append(value[i])
                # self.SetItemData(index, row)
                self.itemIndexMap.append(row)
                row += 1

            self.SetItemCount(row)
            
            # set column width
            # for i in range(self.GetColumnCount()):
            # self.SetColumnWidth(i, wx.LIST_AUTOSIZE)
            # for i in range(self.GetColumnCount()):
            # if self.GetColumnWidth(i) < 80:
            # self.SetColumnWidth(i, 80)
            self.SetColumnWidth(0, 80)
            self.SetColumnWidth(1, 300)
            
            self.SendSizeEvent()
            
        except StandardError, e:
            wx.MessageBox(parent=self,
                          message=_("Unable to read list: %s") % e,
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR)

    def OnColumnClick(self, event):
        """Sort by column"""
        self._col = event.GetColumn()

        # remove duplicated arrow symbol from column header
        # FIXME: should be done automatically
        info = wx.ListItem()
        info.m_mask = wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE
        info.m_image = -1
        for column in range(self.GetColumnCount()):
            info.m_text = self.GetColumn(column).GetText()
            self.SetColumn(column, info)

        event.Skip()

    def GetSortImages(self):
        """Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py"""
        return (self.sm_dn, self.sm_up)

    def OnGetItemText(self, item, col):
        """Get item text"""
        index = self.itemIndexMap[item]
        s = str(self.itemDataMap[index][col])
        return s

    def OnGetItemAttr(self, item):
        """Get item attributes"""
        index = self.itemIndexMap[item]
        if ( index % 2) == 0:
            return self.attr2
        else:
            return self.attr1

    def SortItems(self, sorter=cmp):
        """Sort items"""
        items = list(self.itemDataMap.keys())
        items.sort(self.Sorter)
        self.itemIndexMap = items

        # redraw the list
        self.Refresh()
        
    def Sorter(self, key1, key2):
        colName = self.GetColumn(self._col).GetText()
        ascending = self._colSortFlag[self._col]
        # convert always string
        item1 = self.itemDataMap[key1][self._col]
        item2 = self.itemDataMap[key2][self._col]

        if type(item1) == type('') or type(item2) == type(''):
            cmpVal = locale.strcoll(str(item1), str(item2))
        else:
            cmpVal = cmp(item1, item2)


        # If the items are equal then pick something else to make the sort value unique
        if cmpVal == 0:
            cmpVal = apply(cmp, self.GetSecondarySortValues(self._col, key1, key2))

        if ascending:
            return cmpVal
        else:
            return -cmpVal

    def GetListCtrl(self):
        """Used by listmix.ColumnSorterMixin"""
        return self

    def Search (self, index, str):
        """Search projection by description

        Return first found item or None
        """
        if str == '':
            self.Populate(self.sourceData)
            return None

        data = []
        for i in range(len(self.sourceData)):
            value = self.sourceData[i][index]
            if str.lower() in value.lower():
                data.append(self.sourceData[i])

        self.Populate(data)

        if len(data) > 0:
            return data[0]
        else:
            return None

class ProjTypePage(TitledPage):
    """
    Wizard page for selecting method of setting coordinate system parameters
    (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose method of specifying georeferencing parameters"))
        global coordsys

        self.utmzone = ''
        self.utmhemisphere = ''
        self.hemischoices = ["north","south"]
        self.parent = parent

        self.radio1 = wx.RadioButton(parent=self, id=wx.ID_ANY, label=_("Select datum with associated ellipsoid"),
                                     style = wx.RB_GROUP)
        self.radio2 = wx.RadioButton(parent=self, id=wx.ID_ANY,
                                     label=_("Select ellipsoid"))
        self.title_utm = self.MakeLabel(_("Set zone for UTM projection:"))
        self.text_utm = self.MakeTextCtrl(size=(100,-1))
        self.label_utm = self.MakeLabel(_("Zone:"))
        self.hemisphere = wx.Choice(parent=self, id=wx.ID_ANY, size=(100, -1),
                                    choices=self.hemischoices)
        self.label_hemisphere = self.MakeLabel(_("Hemisphere for zone:"))

        # layout
        self.sizer.AddGrowableCol(2)
        self.sizer.Add(item=self.radio1,
                       flag=wx.ALIGN_LEFT, pos=(1, 1), span=(1, 2))
        self.sizer.Add(item=self.radio2,
                       flag=wx.ALIGN_LEFT, pos=(2, 1), span=(1, 2))
        self.sizer.Add(item=self.title_utm,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(4, 1), span=(1, 2))
        self.sizer.Add(item=self.label_utm,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                       border=5, pos=(5, 1))
        self.sizer.Add(item=self.text_utm,
                       flag=wx.ALIGN_LEFT | wx.ALL, border=5,
                       pos=(5, 2))
        self.sizer.Add(item=self.label_hemisphere,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                       border=5, pos=(6, 1))
        self.sizer.Add(item=self.hemisphere,
                       flag=wx.ALIGN_LEFT|wx.ALL,
                       border=5, pos=(6, 2))

        self.title_utm.Hide()
        self.text_utm.Hide()
        self.label_utm.Hide()
        self.hemisphere.Hide()
        self.label_hemisphere.Hide()

        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio1.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio2.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChange)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnPageChange(self,event=None):
        if event.GetDirection() and self.parent.projpage.proj == 'utm' and self.utmzone == '':
            wx.MessageBox('You must set a zone for a UTM projection')
            event.Veto()
        self.title_utm.Hide()
        self.text_utm.Hide()
        self.label_utm.Hide()
        self.hemisphere.Hide()
        self.label_hemisphere.Hide()

    def OnEnterPage(self,event):
        if self.parent.projpage.proj == 'utm':
            self.title_utm.Show()
            self.text_utm.Show()
            self.label_utm.Show()
            self.hemisphere.Show()
            self.label_hemisphere.Show()
            
            self.Bind(wx.EVT_CHOICE, self.OnHemisphere, self.hemisphere)
            self.Bind(wx.EVT_TEXT, self.GetUTM, self.text_utm)

        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()

        event.Skip()

    def SetVal(self, event):
        global coordsys
        if event.GetId() == self.radio1.GetId():
            self.SetNext(self.parent.datumpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
        elif event.GetId() == self.radio2.GetId():
            self.SetNext(self.parent.ellipsepage)
            self.parent.sumpage.SetPrev(self.parent.ellipsepage)

    def GetUTM(self, event):
        self.utmzone = event.GetString()

    def OnHemisphere(self, event):
        self.utmhemisphere = event.GetString()


class DatumPage(TitledPage):
    """
    Wizard page for selecting datum (with associated ellipsoid)
    and datum transformation parameters (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify geodetic datum"))

        self.parent = parent
        self.datum = ''
        self.datumdesc = ''
        self.ellipsoid = ''
        self.datumparams = ''
        self.transform = ''
        self.transregion = ''
        self.transparams = ''
        self.hastransform = False
        self.proj4params = ''

        # text input
        self.tdatum = self.MakeTextCtrl("", size=(200,-1))
        self.ttrans = self.MakeTextCtrl("", size=(200,-1))

        # search box
        self.searchb = wx.SearchCtrl(self, size=(200,-1),
                                     style=wx.TE_PROCESS_ENTER)

        # create list control for datum/elipsoid list
        data = []
        for key in self.parent.datums.keys():
            data.append([key, self.parent.datums[key][0], self.parent.datums[key][1]])
        self.datumlist = ItemList(self,
                                  data=data,
                                  columns=[_('Code'), _('Description'), _('Ellipsoid')])

        # create list control for datum transformation parameters list
        data = []
        for key in self.parent.transforms.keys():
            data.append([key, self.parent.transforms[key][0], self.parent.transforms[key][1]])
        self.transformlist = ItemList(self,
                                      data=None,
                                      columns=[_('Code'), _('Datum'), _('Description')])
        self.transformlist.sourceData = data
        
        # layout
        self.sizer.AddGrowableCol(4)
        self.sizer.Add(item=self.MakeLabel(_("Datum code:")),
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tdatum,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 2))

        self.sizer.Add(item=self.MakeLabel(_("Search in description:")),
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 1))
        self.sizer.Add(item=self.searchb,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 2))

        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item=self.datumlist,
                       flag=wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(3, 1), span=(1, 4))

        self.sizer.Add(item=self.MakeLabel(_("Transformation parameters:")),
                       flag=wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(5, 1))
        self.sizer.Add(item=self.ttrans,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(5, 2))

        self.sizer.Add(item=self.transformlist,
                       flag=wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(6, 1), span=(1, 4))

        # events
        self.datumlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnDatumSelected)
        self.transformlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnTransformSelected)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnDSearch)
        self.tdatum.Bind(wx.EVT_TEXT, self.OnDText)
        self.tdatum.Bind(wx.EVT_TEXT_ENTER, self.OnDText)
        self.ttrans.Bind(wx.EVT_TEXT, self.OnTText)
        self.ttrans.Bind(wx.EVT_TEXT_ENTER, self.OnTText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnPageChanging(self, event):
        self.proj4params = ''
        if event.GetDirection() and self.datum not in self.parent.datums:
            event.Veto()
        if self.hastransform == True and self.transform == '':
            event.Veto()
        self.GetNext().SetPrev(self)
        self.parent.ellipsepage.ellipseparams = self.parent.ellipsoids[self.ellipsoid][1]

    def OnEnterPage(self,event):
        if len(self.datum) == 0 or \
                (self.hastransform == True and self.transform == ''):
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnDText(self, event):
        self.datum = event.GetString()
        self.transformlist.Search(index=1, str=self.datum)
        if self.transformlist.GetItemCount() > 0:
            self.hastransform = True
        else:
            self.hastransform = False

        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.datum) == 0 and nextButton.IsEnabled():
            nextButton.Enable(False)
        elif self.datum in self.parent.datums:
            self.datumdesc = self.parent.datums[self.datum][0]
            self.ellipsoid = self.parent.datums[self.datum][1]
            self.datumparams = self.parent.datums[self.datum][2]
            if self.hastransform == False or \
                    (self.hastransform == True and self.transform != ''):
                if not nextButton.IsEnabled():
                    nextButton.Enable(True)
            else:
                if nextButton.IsEnabled():
                    nextButton.Enable(False)
            
        event.Skip()

    def OnTText(self, event):
        if self.hastransform == False:
            event.Skip()
            return

        self.transform = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)

        if len(self.transform) == 0 and nextButton.IsEnabled():
            nextButton.Enable(False)
        elif self.transform in self.parent.transforms:
            self.transdatum = self.parent.transforms[self.transform][0]
            self.transregion = self.parent.transforms[self.transform][1]
            self.transparams = self.parent.transforms[self.transform][2]
            if not nextButton.IsEnabled():
                nextButton.Enable()

    def OnDSearch(self, event):
        str =  self.searchb.GetValue()
        try:
            self.datum, self.datumdesc, self.ellipsoid = self.datumlist.Search(index=1, str=str)
            self.transformlist.Search(index=1, str=self.datum)
        except:
            self.datum = self.datumdesc = self.ellipsoid = ''

        if str == '' or self.datum == '':
            self.transformlist.DeleteAllItems()
            self.transformlist.Refresh()
        self.tdatum.SetValue(self.datum)

        event.Skip()
        
    def OnTransformSelected(self,event):
        index = event.m_itemIndex
        item = event.GetItem()

        self.transform = self.transformlist.GetItem(index, 0).GetText()
        self.transdatum = self.parent.transforms[self.transform][0]
        self.transregion = self.parent.transforms[self.transform][1]
        self.transparams = self.parent.transforms[self.transform][2]

        self.ttrans.SetValue(str(self.transform))

    def OnDatumSelected(self,event):
        index = event.m_itemIndex
        item = event.GetItem()

        self.datum = self.datumlist.GetItem(index, 0).GetText()
        self.datumdesc = self.parent.datums[self.datum][0]
        self.ellipsoid = self.parent.datums[self.datum][1]
        self.datumparams = self.parent.datums[self.datum][2]

        self.tdatum.SetValue(self.datum)
        event.Skip()

class EllipsePage(TitledPage):
    """
    Wizard page for selecting ellipsoid (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify ellipsoid"))

        self.parent = parent
        
        self.ellipse = ''
        self.ellipsedesc = ''
        self.ellipseparams = ''
        self.proj4params = ''

        # text input
        self.tellipse = self.MakeTextCtrl("", size=(200,-1))

        # search box
        self.searchb = wx.SearchCtrl(self, size=(200,-1),
                                     style=wx.TE_PROCESS_ENTER)

        # create list control for ellipse list
        data = []
        # extract code, desc
        for key in self.parent.ellipsoids.keys():
            data.append([key, self.parent.ellipsoids[key][0]])

        self.ellipselist = ItemList(self, data=data,
                                    columns=[_('Code'), _('Description')])

        # layout
        self.sizer.AddGrowableCol(4)
        self.sizer.Add(item=self.MakeLabel(_("Ellipsoid code:")),
                       flag=wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tellipse,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 2))
        self.sizer.Add(item=self.MakeLabel(_("Search in description:")),
                       flag=wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 1))
        self.sizer.Add(item=self.searchb,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 2))

        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item=self.ellipselist,
                       flag=wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(3, 1), span=(1, 4))

        # events
        self.ellipselist.Bind(wx.EVT_LIST_ITEM_SELECTED,    self.OnItemSelected)
        self.tellipse.Bind(wx.EVT_TEXT, self.OnText)
        self.tellipse.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT_ENTER,    self.OnSearch)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self,event):
        if len(self.ellipse) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and self.ellipse not in self.parent.ellipsoids:
            event.Veto()

        self.proj4params = ''
        self.GetNext().SetPrev(self)
        self.parent.datumpage.datumparams = ''
        self.parent.datumpage.transparams = ''
        # self.GetNext().SetPrev(self) (???)

    def OnText(self, event):
        """Ellipspoid code changed"""
        self.ellipse = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.ellipse) == 0 and nextButton.IsEnabled():
            nextButton.Enable(False)
            self.ellipsedesc = ''
            self.ellipseparams = ''
            self.proj4params = ''
        elif self.ellipse in self.parent.ellipsoids:
            self.ellipsedesc = self.parent.ellipsoids[self.ellipse][0]
            self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            if not nextButton.IsEnabled():
                nextButton.Enable()

    def OnSearch(self, event):
        """Search ellipsoid by desc"""
        str =  event.GetString()
        try:
            self.ellipse, self.ellipsedesc = \
                self.ellipselist.Search(index=1, str=event.GetString())
            self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            self.proj4params = self.parent.ellipsoids[self.ellipse][2]
        except:
            self.ellipse = self.ellipsedesc = ''
            self.ellipseparams = self.proj4params = ''

        self.tellipse.SetValue(self.ellipse)

        event.Skip()

    def OnItemSelected(self,event):
        index = event.m_itemIndex
        item = event.GetItem()

        self.ellipse = self.ellipselist.GetItem(index, 0).GetText()
        self.ellipsedesc = self.parent.ellipsoids[self.ellipse][0]
        self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]

        self.tellipse.SetValue(self.ellipse)

class GeoreferencedFilePage(TitledPage):
    """
    Wizard page for selecting georeferenced file to use
    for setting coordinate system parameters
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select georeferenced file"))

        self.georeffile = ''

        # create controls
        self.lfile= self.MakeLabel(_("Georeferenced file:"))
        self.tfile = self.MakeTextCtrl(size=(300,-1))
        self.bbrowse = self.MakeButton(_("Browse"))

        # do layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item=self.lfile, flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tfile, flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border=5, pos=(1, 2))
        self.sizer.Add(item=self.bbrowse, flag=wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(1, 3))

        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event):
        if len(self.georeffile) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and self.georeffile == '':
            event.Veto()
        self.GetNext().SetPrev(self)

        event.Skip()

    def OnText(self, event):
        self.georeffile = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.georeffile) > 0 and os.path.isfile(self.georeffile):
            if not nextButton.IsEnabled():
                nextButton.Enable(True)
        else:
            if nextButton.IsEnabled():
                nextButton.Enable(False)

        event.Skip()

    def OnBrowse(self, event):
        """Choose file"""
        dlg = wx.FileDialog(self,
                            _("Select georeferenced file"),
                            os.getcwd(), "", "*.*", wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
        dlg.Destroy()

        event.Skip()

    def OnCreate(self, event):
        pass

class WKTPage(TitledPage):
    """
    Wizard page for selecting WKT file to use
    for setting coordinate system parameters
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select WKT file"))

        self.wktfile = ''

        # create controls
        self.lfile= self.MakeLabel(_("WKT file:"))
        self.tfile = self.MakeTextCtrl(size=(300,-1))
        self.bbrowse = self.MakeButton(_("Browse"))

        # do layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item=self.lfile, flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tfile, flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border=5, pos=(1, 2))
        self.sizer.Add(item=self.bbrowse, flag=wx.ALIGN_LEFT |
                       wx.ALL, border=5, pos=(1, 3))

        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event):
        if len(self.wktfile) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and self.wktfile == '':
            event.Veto()
        self.GetNext().SetPrev(self)

        event.Skip()

    def OnText(self, event):
        self.wktfile = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.wktfile) > 0 and os.path.isfile(self.wktfile):
            if not nextButton.IsEnabled():
                nextButton.Enable(True)
        else:
            if nextButton.IsEnabled():
                nextButton.Enable(False)

        event.Skip()

    def OnBrowse(self, event):
        """Choose file"""
        dlg = wx.FileDialog(self,
                            _("Select WKT file"),
                            os.getcwd(), "", "*.*", wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
        dlg.Destroy()

        event.Skip()

    def OnCreate(self, event):
        pass

class EPSGPage(TitledPage):
    """
    Wizard page for selecting EPSG code for
    setting coordinate system parameters
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose EPSG Code"))
        self.parent = parent
        self.epsgCodeDict = {}
        self.epsgcode = None
        self.epsgdesc = ''
        self.epsgparams = ''

        # labels
        self.lfile= self.MakeLabel(_("Path to the EPSG-codes file:"),
                                    style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        self.lcode= self.MakeLabel(_("EPSG code:"),
                                    style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        self.lsearch = self.MakeLabel(_("Search in description:"),
                                       style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)

        # text input
        epsgdir = os.path.join(os.environ["GRASS_PROJSHARE"], 'epsg')
        self.tfile = self.MakeTextCtrl(text=epsgdir, size=(200,-1))
        self.tcode = self.MakeTextCtrl(size=(200,-1))

        # buttons
        self.bbrowse = self.MakeButton(_("Browse"))
        self.bbcodes = self.MakeButton(_("Browse EPSG Codes"))

        # search box
        self.searchb = wx.SearchCtrl(self, size=(200,-1),
                                     style=wx.TE_PROCESS_ENTER)

        self.epsglist = ItemList(self, data=None,
                                 columns=[_('Code'), _('Description'), _('Parameters')])

        # layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item=self.lfile,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 1))
        self.sizer.Add(item=self.tfile,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 2))
        self.sizer.Add(item=self.bbrowse,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(1, 3))

        self.sizer.Add(item=self.lcode,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 1))
        self.sizer.Add(item=self.tcode,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(2, 2))

        self.sizer.Add(item=self.lsearch,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(3, 1))
        self.sizer.Add(item=self.searchb,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(3, 2))
        self.sizer.Add(item=self.bbcodes,
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border=5, pos=(3, 3))

        self.sizer.AddGrowableRow(4)
        self.sizer.Add(item=self.epsglist,
                       flag=wx.ALIGN_LEFT | wx.EXPAND, pos=(4, 1),
                       span=(1, 3))

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.bbcodes.Bind(wx.EVT_BUTTON, self.OnBrowseCodes)
        self.tcode.Bind(wx.EVT_TEXT, self.OnText)
        self.tcode.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.epsglist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event):
        if not self.epsgcode:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        # load default epsg database file
        self.OnBrowseCodes(None)
        
        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and not self.epsgcode:
            event.Veto()
        self.GetNext().SetPrev(self)

    def OnText(self, event):
        self.epsgcode = event.GetString()
        try:
            self.epsgcode = int(self.epsgcode)
        except:
            self.epsgcode = None
            
        nextButton = wx.FindWindowById(wx.ID_FORWARD)

        if self.epsgcode and self.epsgcode in self.epsgCodeDict.keys():
            self.epsgdesc = self.epsgCodeDict[self.epsgcode][0]
            self.epsgparams = self.epsgCodeDict[self.epsgcode][1]
            if not nextButton.IsEnabled():
                nextButton.Enable(True)
        else:
            if nextButton.IsEnabled():
                nextButton.Enable(False)
            self.epsgdesc = self.epsgparams = ''

    def OnSearch(self, event):
        str =  self.searchb.GetValue()
        if self.epsglist.GetItemCount() == 0:
            event.Skip()
            return
        
        try:
            self.epsgcode = self.epsglist.Search(index=1, str=str)[0]
            self.tcode.SetValue(str(self.epsgcode))
        except:
            self.epsgcode = None
            self.tcode.SetValue('')

        event.Skip()
        
    def OnBrowse(self, event):
        """Define path for EPSG code file"""
        path = os.path.dirname(self.tfile.GetValue())
        if not path:
            path = os.getcwd()

        dlg = wx.FileDialog(parent=self, message=_("Choose EPSG codes file"),
                            defaultDir=path, defaultFile="", wildcard="*", style=wx.OPEN)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
            self.OnBrowseCodes(None)

        dlg.Destroy()

        event.Skip()

    def OnItemSelected(self, event):
        """EPSG code selected from the list"""
        index = event.m_itemIndex
        item = event.GetItem()

        self.epsgcode = int(self.epsglist.GetItem(index, 0).GetText())
        self.epsgdesc = self.epsglist.GetItem(index, 1).GetText()
        self.tcode.SetValue(str(self.epsgcode))

        event.Skip()
        
    def OnBrowseCodes(self, event, search=None):
        """Browse EPSG codes"""
        try:
            data = []
            self.epsgCodeDict = {}
            f = open(self.tfile.GetValue(), "r")
            i = 0
            code = None
            for line in f.readlines():

                line = line.strip()

                if line[0] == '#':
                    descr = line[1:].strip()
                elif line[0] == '<':
                    code, params = line.split(" ", 1)
                    code = int(code.replace('<', '').replace('>', ''))

                if code is not None:
                    data.append((code, descr, params))
                    self.epsgCodeDict[code] = (descr, params)
                    code = None
                i += 1
            f.close()

            self.epsglist.Populate(data, update=True)
        except StandardError, e:
            wx.MessageBox(parent=self,
                          message=_("Unable to read EPGS codes: %s") % e,
                          caption=_("Error"),  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            self.epsglist.Populate([], update=True)

class CustomPage(TitledPage):
    """
    Wizard page for entering custom PROJ.4 string
    for setting coordinate system parameters
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard,
                            _("Choose method of specifying georeferencing parameters"))
        global coordsys
        self.customstring = ''
        self.parent = parent

        # widgets
        self.text_proj4string = self.MakeTextCtrl(size=(400, 200),
                                                  style=wx.TE_MULTILINE)
        self.label_proj4string = self.MakeLabel(_("Enter PROJ.4 parameters string:"))

        # layout
        self.sizer.AddGrowableCol(2)
        self.sizer.Add(self.label_proj4string,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(1, 1))
        self.sizer.AddGrowableRow(2)
        self.sizer.Add(self.text_proj4string,
                       flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND, 
                       border=5, pos=(2, 1), span=(1, 2))

        self.text_proj4string.Bind(wx.EVT_TEXT, self.GetProjstring)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event):
        if len(self.customstring) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and not self.customstring:
            event.Veto()
        self.GetNext().SetPrev(self)

    def GetProjstring(self, event):
        """Change proj string"""
        # TODO: check PROJ.4 syntax
        self.customstring = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.customstring) == 0:
            if nextButton.IsEnabled():
                nextButton.Enable(False)
        else:
            if not nextButton.IsEnabled():
                nextButton.Enable()

class SummaryPage(TitledPage):
    """
    Shows summary result of choosing coordinate system parameters
    prior to creating location
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))

        self.parent = parent

        # labels
        self.ldatabase  = self.MakeLabel("")
        self.llocation  = self.MakeLabel("")
        self.lprojection = self.MakeLabel("")
        self.lproj4string = self.MakeLabel("")
        self.lproj4stringLabel = self.MakeLabel("")
        
        self.lprojection.Wrap(400)
        
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        # self.Bind(wx.EVT_BUTTON, self.OnFinish, wx.ID_FINISH)

        # do sub-page layout
        self.__DoLayout()
        
    def __DoLayout(self):
        """Do page layout"""
        self.sizer.AddGrowableCol(1)
        self.sizer.Add(item=self.MakeLabel(_("GRASS Database:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(1, 0))
        self.sizer.Add(item=self.ldatabase, 
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(1, 1))
        self.sizer.Add(item=self.MakeLabel(_("Location Name:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(2, 0))
        self.sizer.Add(item=self.llocation,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(2, 1))
        self.sizer.Add(item=self.MakeLabel(_("Projection:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(3, 0))
        self.sizer.Add(item=self.lprojection,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(3, 1))
        self.sizer.Add(item=self.lproj4stringLabel,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(4, 0))
        self.sizer.Add(item=self.lproj4string,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(4, 1))
        self.sizer.Add(item=(10,20),
                       flag=wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                       border=5, pos=(5, 0), span=(1, 2))
        # self.sizer.AddGrowableRow(6)
        #self.sizer.Add(item=self.MakeLabel(_("You can set the default extents "
        #                                     "and resolution after creating new location%s"
        #                                     "or you can set them during a working session.") % os.linesep,
        #                                   style=wx.ALIGN_CENTER),
        #               flag=wx.ALIGN_CENTRE | wx.ALL, border=5, pos=(6, 0),
        #               span=(1, 2))

    def OnEnterPage(self,event):
        """
        Insert values into text controls for summary of location creation options
        """

        database = self.parent.startpage.grassdatabase
        location = self.parent.startpage.location

        global coordsys
        if coordsys not in ['proj', 'epsg']:
            self.lproj4stringLabel.Hide()
            self.lproj4string.Hide()
            self.lproj4stringLabel.SetLabel('')
            self.lproj4string.SetLabel('')
        else:
            self.lproj4string.Show()
            self.lproj4stringLabel.SetLabel(_("PROJ.4 definition:"))
            if coordsys == 'proj':
                self.lproj4string.SetLabel(self.parent.CreateProj4String())
            else:
                self.lproj4string.SetLabel(self.parent.epsgpage.epsgCodeDict[self.parent.epsgpage.epsgcode][1])
            self.lproj4string.Wrap(400)
            
        projection = self.parent.projpage.proj
        projdesc = self.parent.projpage.projdesc
        utmzone = self.parent.projtypepage.utmzone
        utmhemisphere = self.parent.projtypepage.utmhemisphere
        ellipse = self.parent.ellipsepage.ellipse
        ellipsedesc = self.parent.ellipsepage.ellipsedesc
        datum = self.parent.datumpage.datum
        datumdesc = self.parent.datumpage.datumdesc
        ellipsoid = self.parent.datumpage.ellipsoid
        datumparams = self.parent.datumpage.datumparams
        transform = self.parent.datumpage.transform
        transregion = self.parent.datumpage.transregion
        transparams = self.parent.datumpage.transparams

        self.ldatabase.SetLabel(str(database))
        self.llocation.SetLabel(str(location))
        label = ''
        if coordsys == 'epsg':
            label = 'EPSG code %s (%s)' % (self.parent.epsgpage.epsgcode, self.parent.epsgpage.epsgdesc)
            self.lprojection.SetLabel(label)
        elif coordsys == 'file':
            label = 'matches file %s' % self.parent.filepage.georeffile
            self.lprojection.SetLabel(label)
        elif coordsys == 'wkt':
            label = 'matches file %s' % self.parent.wktpage.wktfile
            self.lprojection.SetLabel(label)
        elif coordsys == 'proj':
            label = ('%s, %s%s' % (projdesc, datumdesc, ellipsedesc))
            self.lprojection.SetLabel(label)
        elif coordsys == 'xy':
            label = ('XY coordinate system (not projected).')
            self.lprojection.SetLabel(label)
        elif coordsys == 'custom':
            label = ('%s' % self.parent.custompage.customstring)
            self.lprojection.SetLabel(label)

    def OnFinish(self, event):
        dlg = wx.MessageDialog(parent=self.wizard,
                               message=_("Do you want to create GRASS location <%s>?") % location,
                               caption=_("Create new location?"),
                               style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_NO:
            dlg.Destroy()
            event.Veto()
        else:
            dlg.Destroy()
            event.Skip()

class LocationWizard(wx.Object):
    """
    Start wizard here and finish wizard here
    """
    def __init__(self, parent, grassdatabase):
        global coordsys
        self.parent = parent

        #
        # define wizard image
        #
        # file = "loc_wizard.png"
        file = "loc_wizard_qgis.png"
        imagePath = os.path.join(globalvar.ETCWXDIR, "images", file)
        wizbmp = wx.Image(imagePath, wx.BITMAP_TYPE_PNG)
        # wizbmp.Rescale(250,600)
        wizbmp = wizbmp.ConvertToBitmap()

        #
        # get georeferencing information from tables in $GISBASE/etc
        #
        self.__readData()

        #
        # define wizard pages
        #
        self.wizard = wiz.Wizard(parent, id=wx.ID_ANY, title=_("Define new GRASS Location"),
                                 bitmap=wizbmp)
        self.startpage = DatabasePage(self.wizard, self, grassdatabase)
        self.csystemspage = CoordinateSystemPage(self.wizard, self)
        self.projpage = ProjectionsPage(self.wizard, self)
        self.datumpage = DatumPage(self.wizard, self)
        self.projtypepage = ProjTypePage(self.wizard,self)
        self.epsgpage = EPSGPage(self.wizard, self)
        self.filepage = GeoreferencedFilePage(self.wizard, self)
        self.wktpage = WKTPage(self.wizard, self)
        self.ellipsepage = EllipsePage(self.wizard, self)
        self.custompage = CustomPage(self.wizard, self)
        self.sumpage = SummaryPage(self.wizard, self)

        #
        # set the initial order of the pages
        # (should follow the epsg line)
        #
        self.startpage.SetNext(self.csystemspage)

        self.csystemspage.SetPrev(self.startpage)
        self.csystemspage.SetNext(self.sumpage)

        self.projpage.SetPrev(self.csystemspage)
        self.projpage.SetNext(self.projtypepage)

        self.projtypepage.SetPrev(self.projpage)
        self.projtypepage.SetNext(self.datumpage)

        self.datumpage.SetPrev(self.projtypepage)
        self.datumpage.SetNext(self.sumpage)

        self.ellipsepage.SetPrev(self.projtypepage)
        self.ellipsepage.SetNext(self.sumpage)

        self.epsgpage.SetPrev(self.csystemspage)
        self.epsgpage.SetNext(self.sumpage)

        self.filepage.SetPrev(self.csystemspage)
        self.filepage.SetNext(self.sumpage)

        self.wktpage.SetPrev(self.csystemspage)
        self.wktpage.SetNext(self.sumpage)

        self.custompage.SetPrev(self.csystemspage)
        self.custompage.SetNext(self.sumpage)

        self.sumpage.SetPrev(self.csystemspage)

        #
        # do pages layout
        #
        self.startpage.DoLayout()
        self.csystemspage.DoLayout()
        self.projpage.DoLayout()
        self.datumpage.DoLayout()
        self.projtypepage.DoLayout()
        self.epsgpage.DoLayout()
        self.filepage.DoLayout()
        self.wktpage.DoLayout()
        self.ellipsepage.DoLayout()
        self.custompage.DoLayout()
        self.sumpage.DoLayout()
        self.wizard.FitToPage(self.datumpage)

        # new location created?
        self.location = None 
        success = False

        #
        # run wizard...
        #
        if self.wizard.RunWizard(self.startpage):
            success = self.OnWizFinished()
            if success == True:
                self.wizard.Destroy()
                self.location = self.startpage.location
                dlg = wx.MessageDialog(parent=self.parent,
                                       message=_("Do you want to set the default "
                                                 "region extents and resolution now?"),
                                       caption=_("Location <%s> created") % self.location,
                                       style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
                dlg.CenterOnScreen()
                if dlg.ShowModal() == wx.ID_YES:
                    dlg.Destroy()
                    defineRegion = RegionDef(self.parent, location=self.location)
                    defineRegion.Centre()
                    defineRegion.Show()
                else:
                    dlg.Destroy()

            elif success == False:
                self.wizard.Destroy()
                wx.MessageBox(parent=self.parent,
                              message="%s" % _("Unable to create new location. "
                                               "Location <%s> not created.") % self.startpage.location,
                              caption=_("Location wizard"),
                              style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            else: # None
                pass
        else:
            win = wx.MessageBox(parent=self.parent,
                                message=_("Location wizard canceled. "
                                          "Location not created."),
                                caption=_("Location wizard"))

    def __readData(self):
        """Get georeferencing information from tables in $GISBASE/etc"""
        # read projection definitions
        f = open(os.path.join(globalvar.ETCDIR, "projections"), "r")
        self.projections = {}
        for line in f.readlines():
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            proj, projdesc = line.split(":", 1)
            self.projections[proj.strip()] = projdesc.strip()
        f.close()

        # read datum definitions
        f = open(os.path.join(globalvar.ETCDIR, "datum.table"), "r")
        self.datums = {}
        paramslist = []
        for line in f.readlines():
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            datum, info = line.split(" ", 1)
            info = info.strip()
            datumdesc, params = info.split(" ", 1)
            datumdesc = datumdesc.strip('"')
            paramlist = params.split()
            ellipsoid = paramlist.pop(0)
            self.datums[datum] = (datumdesc.replace('_', ' '), ellipsoid, paramlist)
        f.close()

        # read datum transforms parameters
        f = open(os.path.join(globalvar.ETCDIR, "datumtransform.table"), "r")
        self.transforms = {}
        j = 1
        for line in f.readlines():
            if j < 10:
                transcode = 'T0' + str(j)
            else:
               transcode = 'T' + str(j)
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            datum, rest = line.split(" ", 1)
            rest = rest.strip('" ')
            params, rest = rest.split('"', 1)
            params = params.strip()
            rest = rest.strip('" ')
            try:
                region, info = rest.split('"', 1)
                info = info.strip('" ')
                info = region + ': ' + info
            except:
                info = rest
            self.transforms[transcode] = (datum, info, params)
            j += 1
        f.close()

        # read ellipsiod definitions
        f = open(os.path.join(globalvar.ETCDIR, "ellipse.table"), "r")
        self.ellipsoids = {}
        for line in f.readlines():
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            ellipse, rest = line.split(" ", 1)
            rest = rest.strip('" ')
            desc, params = rest.split('"', 1)
            desc = desc.strip('" ')
            paramslist = params.split()
            self.ellipsoids[ellipse] = (desc, paramslist)
        f.close()

    def OnWizFinished(self):
        database = self.startpage.grassdatabase
        location = self.startpage.location
        global coordsys
        success = False
        
        # location already exists?
        if os.path.isdir(os.path.join(database,location)):
            dlg = wx.MessageDialog(parent=self.wizard,
                                   message="%s <%s>: %s" % \
                                       (_("Unable to create new location"),
                                        os.path.join(database, location),
                                        _("Location already exists in GRASS Database.")),
                                   caption=_("Error"),
                                   style=wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

        if coordsys == "xy":
            success = self.XYCreate()
        elif coordsys == "latlong":
            rows = int(round((float(north) - float(south)) / float(resolution)))
            cols = int(round((float(east) - float(west)) / float(resolution)))
            cells = int(rows * cols)
            success = self.LatlongCreate()
        elif coordsys == "proj":
            proj4string = self.CreateProj4String()
            success = self.Proj4Create(proj4string)
        elif coordsys == 'custom':
            success = self.CustomCreate()
        elif coordsys == "epsg":
            success = self.EPSGCreate()
        elif coordsys == "file":
            success = self.FileCreate()
        elif coordsys == "wkt":
            success = self.WKTCreate()

        return success

    def XYCreate(self):
        """Create an XY location"""
        database = self.startpage.grassdatabase
        location = self.startpage.location

        # create location directory and PERMANENT mapset
        try:
            os.mkdir(os.path.join(database, location))
            os.mkdir(os.path.join(database, location, 'PERMANENT'))
            # create DEFAULT_WIND and WIND files
            regioninfo =   ['proj:       0',
                            'zone:       0',
                            'north:      1',
                            'south:      0',
                            'east:       1',
                            'west:       0',
                            'cols:       1',
                            'rows:       1',
                            'e-w resol:  1',
                            'n-s resol:  1',
                            'top:        1',
                            'bottom:     0',
                            'cols3:      1',
                            'rows3:      1',
                            'depths:     1',
                            'e-w resol3: 1',
                            'n-s resol3: 1',
                            't-b resol:  1']
            
            defwind = open(os.path.join(database, location, 
                                        "PERMANENT", "DEFAULT_WIND"), 'w')
            for param in regioninfo:
                defwind.write(param + '%s' % os.linesep)
            defwind.close()

            shutil.copy(os.path.join(database, location, "PERMANENT", "DEFAULT_WIND"),
                        os.path.join(database, location, "PERMANENT", "WIND"))

            # create MYNAME file
            myname = open(os.path.join(database, location, "PERMANENT",
                                       "MYNAME"), 'w')
            myname.write('%s' % os.linesep)
            myname.close()

            return True

        except OSError, e:
            dlg = wx.MessageDialog(parent=self.wizard,
                                   message="%s: %s" % (_("Unable to create new location"), e),
                                   caption=_("Error"),
                                   style=wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

    def CreateProj4String(self):
        """Constract PROJ.4 string"""
        location = self.startpage.location
        proj = self.projpage.proj
        projdesc = self.projpage.projdesc

        utmzone = self.projtypepage.utmzone
        utmhemisphere = self.projtypepage.utmhemisphere

        datum = self.datumpage.datum
        if self.datumpage.datumdesc:
            datumdesc = self.datumpage.datumdesc +' - ' + self.datumpage.ellipsoid
        else:
            datumdesc = ''
        datumparams = self.datumpage.datumparams
        transparams = self.datumpage.transparams

        ellipse = self.ellipsepage.ellipse
        ellipsedesc = self.ellipsepage.ellipsedesc
        ellipseparams = self.ellipsepage.ellipseparams

        #
        # creating PROJ.4 string
        #
        if proj == 'll':
            proj = 'longlat'
        elif proj == 'utm':
            proj4string = '+proj=%s +zone=%s' % (proj, utmzone)
            if utmhemisphere == 'south':
                proj4string += '+south'
        else:
            proj4string = '+proj=%s ' % (proj)

        proj4params = ''
        # set ellipsoid parameters
        for item in ellipseparams:
            if item[:4] == 'f=1/':
                item = '+rf='+item[4:]
            else:
                item = '+'+item
            proj4params = '%s %s' % (proj4params, item)
        # set datum and transform parameters if relevant
        if datumparams:
            for item in datumparams:
                proj4params = '%s +%s' % (proj4params,item)
            if transparams:
                proj4params = '%s +no_defs +%s' % (proj4params,transparams)
            else:
                proj4params = '%s +no_defs' % proj4params
        else:
            proj4params = '%s +no_defs' % proj4params

        return '%s %s' % (proj4string, proj4params)
        
    def Proj4Create(self, proj4string):
        """
        Create a new location for selected projection
        """
        # creating location from PROJ.4 string passed to g.proj
        cmdlist = ['g.proj', '-c',
                   'proj4=%s' % proj4string,
                   'location=%s' % self.startpage.location]

        p = gcmd.Command(cmdlist, stderr=None)

        if p.returncode == 0:
            return True

        return False

    def CustomCreate(self):
        """Create a new location based on given proj4 string"""
        proj4string = self.custompage.customstring
        location = self.startpage.location

        cmdlist = ['g.proj', '-c',
                   'proj4=%s' % proj4string,
                   'location=%s' % location]

        p = gcmd.Command(cmdlist, stderr=None)

        if p.returncode == 0:
            return True

        return False

    def EPSGCreate(self):
        """
        Create a new location from an EPSG code.
        """
        epsgcode = self.epsgpage.epsgcode
        epsgdesc = self.epsgpage.epsgdesc
        location = self.startpage.location
        cmdlist = []

        # should not happend
        if epsgcode == '':
            wx.MessageBox(parent=self,
                          message="%s: %s" % (_("Unable to create new location"), _("EPSG code missing.")),
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR)
            return False
        
        # creating location
        cmdlist = ['g.proj',
                   'epsg=%s' % epsgcode,
                   'datumtrans=-1']

        p = gcmd.Command(cmdlist)

        dtoptions = {}
        try:
            line = p.ReadStdOutput()
            i = 0
            while i < len(line):
                if line[i] == '---':
                    for j in range(3):
                        dtoptions[line[i+1]] = (line[i+2],
                                                line[i+3],
                                                line[i+4])
                    i += 5
        except:
            pass

        if dtoptions != {}:
            dtrans = ''
            # open a dialog to select datum transform number
            dlg = SelectDatumDialog(self.parent, datums=dtoptions)
            
            if dlg.ShowModal() == wx.ID_OK:
                dtrans = dlg.GetDatum()
                if dtrans == '':
                    wx.MessageBox(parent=self.parent,
                                  message=_('Datum transform is required.'),
                                  caption=_("Error"),
                                  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
                    dlg.Destroy()
                    return False
                dlg.Destroy()
            else:
                dlg.Destroy()
                return False

            cmdlist = ['g.proj', '-c',
                       'epsg=%s' % epsgcode,
                       'location=%s' % location,
                       'datumtrans=%s' % dtrans]
        else:
            cmdlist = ['g.proj','-c',
                       'epsg=%s' % epsgcode,
                       'location=%s' % location,
                       'datumtrans=1']

        p = gcmd.Command(cmdlist, stderr=None)

        if p.returncode == 0:
            return True

        return False

    def FileCreate(self):
        """
        Create a new location from a georeferenced file
        """
        georeffile = self.filepage.georeffile
        location = self.startpage.location

        cmdlist = []

        # this should not happen
        if not georeffile or not os.path.isfile(georeffile):
            dlg = wx.MessageBox(parent=self.wizard,
                                message="%s: %s ('%s')" % \
                                    (_("Unable to create new location"),
                                     _("file not found"),
                                     georeffile),
                                caption=("Error"), style=wx.OK | wx.ICON_ERROR)
            return False

        # creating location
        cmdlist = ['g.proj', '-c',
                   'georef=%s' % georeffile,
                   'location=%s' % location]

        p = gcmd.Command(cmdlist, stderr=None)

        if p.returncode == 0:
            return True

        return False

    def WKTCreate(self):
        """
        Create a new location from a WKT file
        """
        wktfile = self.wktpage.wktfile
        location = self.startpage.location

        cmdlist = []

        # this should not happen
        if not wktfile or not os.path.isfile(wktfile):
            dlg = wx.MessageBox(parent=self.wizard,
                                message="%s: %s ('%s')" % \
                                    (_("Unable to create new location"),
                                     _("file not found"),
                                     wktfile),
                                caption=("Error"), style=wx.OK | wx.ICON_ERROR)
            return False

        # creating location
        cmdlist = ['g.proj', '-c',
                   'wkt=%s' % wktfile,
                   'location=%s' % location]

        p = gcmd.Command(cmdlist, stderr=None)

        if p.returncode == 0:
            return True

        return False

class RegionDef(BaseClass, wx.Frame):
    """
    Page for setting default region extents and resolution
    """

    def __init__(self, parent, id=wx.ID_ANY,
                 title=_("Set default region extent and resolution"), location=None):
        wx.Frame.__init__(self, parent, id, title, size=(650,300))

        self.parent = parent
        self.location = location

        #
        # values
        #
        # 2D
        self.north = 1.0
        self.south = 0.0
        self.east = 1.0
        self.west = 0.0
        self.nsres = 1.0
        self.ewres = 1.0
        # 3D
        self.top = 1.0
        self.bottom = 0.0
        #         self.nsres3 = 1.0
        #         self.ewres3 = 1.0
        self.tbres  = 1.0

        #
        # inputs
        #
        # 2D
        self.tnorth = self.MakeTextCtrl(str(self.north), size=(150, -1))
        self.tsouth = self.MakeTextCtrl(str(self.south), size=(150, -1))
        self.twest = self.MakeTextCtrl(str(self.west), size=(150, -1))
        self.teast = self.MakeTextCtrl(str(self.east), size=(150, -1))
        self.tnsres = self.MakeTextCtrl(str(self.nsres), size=(150, -1))
        self.tewres = self.MakeTextCtrl(str(self.ewres), size=(150, -1))

        #
        # labels
        #
        self.lrows = self.MakeLabel("")
        self.lcols = self.MakeLabel("")
        self.lcells = self.MakeLabel("")

        #
        # buttons
        #
        self.bset = self.MakeButton(_("&Set region"), id=wx.ID_OK)
        self.bcancel = wx.Button(self, id=wx.ID_CANCEL)
        self.bset.SetDefault()

        #
        # image
        #
        self.img = wx.Image(os.path.join(globalvar.ETCWXDIR, "images",
                                         "qgis_world.png"), wx.BITMAP_TYPE_PNG).ConvertToBitmap()

        #
        # set current working environment to PERMANENT mapset
        # in selected location in order to set default region (WIND)
        #
        envval = {}
        cmdlist = ['g.gisenv']
        p = gcmd.Command(cmdlist)
        if p.returncode == 0:
            output = p.ReadStdOutput()
            for line in output:
                line = line.strip()
                if '=' in line:
                    key, val = line.split('=')
                envval[key] = val
            self.currlocation = envval['LOCATION_NAME'].strip("';")
            self.currmapset = envval['MAPSET'].strip("';")
            if self.currlocation != self.location or self.currmapset != 'PERMANENT':
                # cmdlist = ['g.mapset', 'location=%s' % self.location, 'mapset=PERMANENT']
                # gcmd.Command(cmdlist
                gcmd.Command(["g.gisenv",
                              "set=LOCATION_NAME=%s" % self.location])
                gcmd.Command(["g.gisenv",
                              "set=MAPSET=PERMANENT"])

        else:
            dlg = wx.MessageBox(parent=self,
                                message=_('Invalid location selected.'),
                                caption=_("Error"), style=wx.ID_OK | wx.ICON_ERROR)
            return

        #
        # get current region settings
        #
        region = {}
        cmdlist = ['g.region', '-gp3']
        p = gcmd.Command(cmdlist)
        if p.returncode == 0:
            output = p.ReadStdOutput()
            for line in output:
                line = line.strip()
                if '=' in line:
                    key, val = line.split('=')
                region[key] = float(val)
        else:
            dlg = wx.MessageBox(parent=self,
                                message=_("Invalid region"),
                                caption=_("Error"), style=wx.ID_OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return

        #
        # update values
        # 2D
        self.north = float(region['n'])
        self.south = float(region['s'])
        self.east = float(region['e'])
        self.west = float(region['w'])
        self.nsres = float(region['nsres'])
        self.ewres = float(region['ewres'])
        self.rows = int(region['rows'])
        self.cols = int(region['cols'])
        self.cells = int(region['cells'])
        # 3D
        self.top = float(region['t'])
        self.bottom = float(region['b'])
        #         self.nsres3 = float(region['nsres3'])
        #         self.ewres3 = float(region['ewres3'])
        self.tbres = float(region['tbres'])
        self.depth = int(region['depths'])
        self.cells3 = int(region['3dcells'])

        #
        # 3D box collapsable
        #
        self.infoCollapseLabelExp = _("Click here to show 3D settings")
        self.infoCollapseLabelCol = _("Click here to hide 3D settings")
        self.settings3D = wx.CollapsiblePane(parent=self,
                                             label=self.infoCollapseLabelExp,
                                             style=wx.CP_DEFAULT_STYLE |
                                             wx.CP_NO_TLW_RESIZE | wx.EXPAND)
        self.MakeSettings3DPaneContent(self.settings3D.GetPane())
        self.settings3D.Collapse(False) # FIXME
        self.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnSettings3DPaneChanged, self.settings3D)

        #
        # set current region settings
        #
        self.tnorth.SetValue(str(self.north))
        self.tsouth.SetValue(str(self.south))
        self.twest.SetValue(str(self.west))
        self.teast.SetValue(str(self.east))
        self.tnsres.SetValue(str(self.nsres))
        self.tewres.SetValue(str(self.ewres))
        self.ttop.SetValue(str(self.top))
        self.tbottom.SetValue(str(self.bottom))
        #         self.tnsres3.SetValue(str(self.nsres3))
        #         self.tewres3.SetValue(str(self.ewres3))
        self.ttbres.SetValue(str(self.tbres))
        self.lrows.SetLabel(_("Rows: %d" % self.rows))
        self.lcols.SetLabel(_("Cols: %d" % self.cols))
        self.lcells.SetLabel(_("Cells: %d" % self.cells))

        #
        # bindings
        #
        self.Bind(wx.EVT_BUTTON, self.OnSetButton, self.bset)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.bcancel)
        self.tnorth.Bind(wx.EVT_TEXT,   self.OnValue)
        self.tsouth.Bind(wx.EVT_TEXT,   self.OnValue)
        self.teast.Bind(wx.EVT_TEXT,    self.OnValue)
        self.twest.Bind(wx.EVT_TEXT,    self.OnValue)
        self.tnsres.Bind(wx.EVT_TEXT,   self.OnValue)
        self.tewres.Bind(wx.EVT_TEXT,   self.OnValue)
        self.ttop.Bind(wx.EVT_TEXT,     self.OnValue)
        self.tbottom.Bind(wx.EVT_TEXT,  self.OnValue)
        #         self.tnsres3.Bind(wx.EVT_TEXT,  self.OnValue)
        #         self.tewres3.Bind(wx.EVT_TEXT,  self.OnValue)
        self.ttbres.Bind(wx.EVT_TEXT,   self.OnValue)

        self.__DoLayout()
        self.SetMinSize(self.GetBestSize())
        self.minWindowSize = self.GetMinSize()

    def MakeSettings3DPaneContent(self, pane):
        """Create 3D region settings pane"""
        border = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=0, hgap=0)

        # inputs
        self.ttop = wx.TextCtrl(parent=pane, id=wx.ID_ANY, value=str(self.top),
                                size=(150, -1))
        self.tbottom = wx.TextCtrl(parent=pane, id=wx.ID_ANY, value=str(self.bottom),
                                size=(150, -1))
        self.ttbres = wx.TextCtrl(parent=pane, id=wx.ID_ANY, value=str(self.tbres),
                                size=(150, -1))
        #         self.tnsres3 = wx.TextCtrl(parent=pane, id=wx.ID_ANY, value=str(self.nsres3),
        #                                    size=(150, -1))
        #         self.tewres3 = wx.TextCtrl(parent=pane, id=wx.ID_ANY, value=str(self.ewres3),
        #                                    size=(150, -1))

        #labels
        self.ldepth = wx.StaticText(parent=pane, label=_("Depth: %d") % self.depth)
        self.lcells3 = wx.StaticText(parent=pane, label=_("3D Cells: %d") % self.cells3)

        # top
        gridSizer.Add(item=wx.StaticText(parent=pane, label=_("Top")),
                      flag=wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.TOP, border=5,
                      pos=(0, 1))
        gridSizer.Add(item=self.ttop,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border=5, pos=(1, 1))
        # bottom
        gridSizer.Add(item=wx.StaticText(parent=pane, label=_("Bottom")),
                      flag=wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.TOP, border=5,
                      pos=(0, 2))
        gridSizer.Add(item=self.tbottom,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border=5, pos=(1, 2))
        # tbres
        gridSizer.Add(item=wx.StaticText(parent=pane, label=_("T-B resolution")),
                      flag=wx.ALIGN_CENTER | 
                      wx.LEFT | wx.RIGHT | wx.TOP, border=5,
                      pos=(0, 3))
        gridSizer.Add(item=self.ttbres,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border=5, pos=(1, 3))

        # res
        #         gridSizer.Add(item=wx.StaticText(parent=pane, label=_("3D N-S resolution")),
        #                       flag=wx.ALIGN_CENTER |
        #                       wx.LEFT | wx.RIGHT | wx.TOP, border=5,
        #                       pos=(2, 1))
        #         gridSizer.Add(item=self.tnsres3,
        #                       flag=wx.ALIGN_CENTER_HORIZONTAL |
        #                       wx.ALL, border=5, pos=(3, 1))
        #         gridSizer.Add(item=wx.StaticText(parent=pane, label=_("3D E-W resolution")),
        #                       flag=wx.ALIGN_CENTER |
        #                       wx.LEFT | wx.RIGHT | wx.TOP, border=5,
        #                       pos=(2, 3))
        #         gridSizer.Add(item=self.tewres3,
        #                       flag=wx.ALIGN_CENTER_HORIZONTAL |
        #                       wx.ALL, border=5, pos=(3, 3))

        # rows/cols/cells
        gridSizer.Add(item=self.ldepth,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border=5, pos=(2, 1))

        gridSizer.Add(item=self.lcells3,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border=5, pos=(2, 2))

        border.Add(item=gridSizer, proportion=1,
                   flag=wx.ALL | wx.ALIGN_CENTER | wx.EXPAND, border=5)

        pane.SetSizer(border)
        border.Fit(pane)

    def OnSettings3DPaneChanged(self, event):
        """Collapse 3D settings box"""

        if self.settings3D.IsExpanded():
            self.settings3D.SetLabel(self.infoCollapseLabelCol)
            self.Layout()
            self.SetSize(self.GetBestSize())
            self.SetMinSize(self.GetSize())
        else:
            self.settings3D.SetLabel(self.infoCollapseLabelExp)
            self.Layout()
            self.SetSize(self.minWindowSize)
            self.SetMinSize(self.minWindowSize)

        self.SendSizeEvent()

    def __DoLayout(self):
        """Window layout"""
        frameSizer = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=0, hgap=0)
        settings3DSizer = wx.BoxSizer(wx.VERTICAL)
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)

        # north
        gridSizer.Add(item=self.MakeLabel(_("North")),
                      flag=wx.ALIGN_BOTTOM | wx.ALIGN_CENTER_HORIZONTAL |
                      wx.TOP | wx.LEFT | wx.RIGHT, border=5, pos=(0, 2))
        gridSizer.Add(item=self.tnorth,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5, pos=(1, 2))
        # west
        gridSizer.Add(item=self.MakeLabel(_("West")),
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.LEFT | wx.TOP | wx.BOTTOM, border=5, pos=(2, 0))
        gridSizer.Add(item=self.twest,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5,  pos=(2, 1))

        gridSizer.Add(item=wx.StaticBitmap(self, wx.ID_ANY, self.img, (-1, -1),
                                           (self.img.GetWidth(), self.img.GetHeight())),
                      flag=wx.ALIGN_CENTER |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5, pos=(2, 2))

        # east
        gridSizer.Add(item=self.teast,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5,  pos=(2, 3))
        gridSizer.Add(item=self.MakeLabel(_("East")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.RIGHT | wx.TOP | wx.BOTTOM, border=5, pos=(2, 4))
        # south
        gridSizer.Add(item=self.tsouth,
                      flag=wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5, pos=(3, 2))
        gridSizer.Add(item=self.MakeLabel(_("South")),
                      flag=wx.ALIGN_TOP | wx.ALIGN_CENTER_HORIZONTAL |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5, pos=(4, 2))
        # ns-res
        gridSizer.Add(item=self.MakeLabel(_("N-S resolution")),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.TOP | wx.LEFT | wx.RIGHT, border=5, pos=(5, 1))
        gridSizer.Add(item=self.tnsres,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5,  pos=(6, 1))
        # ew-res
        gridSizer.Add(item=self.MakeLabel(_("E-W resolution")),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.TOP | wx.LEFT | wx.RIGHT, border=5, pos=(5, 3))
        gridSizer.Add(item=self.tewres,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border=5,  pos=(6, 3))
        # rows/cols/cells
        gridSizer.Add(item=self.lrows,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border=5, pos=(7, 1))

        gridSizer.Add(item=self.lcells,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border=5, pos=(7, 2))

        gridSizer.Add(item=self.lcols,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border=5, pos=(7, 3))

        # 3D
        settings3DSizer.Add(item=self.settings3D,
                            flag=wx.ALL,
                            border=5)

        # buttons
        buttonSizer.Add(item=self.bcancel, proportion=1,
                        flag=wx.ALIGN_RIGHT |
                        wx.ALIGN_CENTER_VERTICAL |
                        wx.ALL, border=10)
        buttonSizer.Add(item=self.bset, proportion=1,
                        flag=wx.ALIGN_CENTER |
                        wx.ALIGN_CENTER_VERTICAL |
                        wx.ALL, border=10)

        frameSizer.Add(item=gridSizer, proportion=1,
                       flag=wx.ALL | wx.ALIGN_CENTER, border=5)
        frameSizer.Add(item=settings3DSizer, proportion=0,
                       flag=wx.ALL | wx.ALIGN_CENTER, border=5)
        frameSizer.Add(item=buttonSizer, proportion=0,
                       flag=wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.SetAutoLayout(True)
        self.SetSizer(frameSizer)
        frameSizer.Fit(self)
        self.Layout()

    def OnValue(self, event):
        """Set given value"""
        try:
            if event.GetId() == self.tnorth.GetId():
                self.north = float(event.GetString())
            elif event.GetId() == self.tsouth.GetId():
                self.south = float(event.GetString())
            elif event.GetId() == self.teast.GetId():
                self.east = float(event.GetString())
            elif event.GetId() == self.twest.GetId():
                self.west = float(event.GetString())
            elif event.GetId() == self.tnsres.GetId():
                self.nsres = float(event.GetString())
            elif event.GetId() == self.tewres.GetId():
                self.ewres = float(event.GetString())
            elif event.GetId() == self.ttop.GetId():
                self.top = float(event.GetString())
            elif event.GetId() == self.tbottom.GetId():
                self.bottom = float(event.GetString())
            #             elif event.GetId() == self.tnsres3.GetId():
            #                 self.nsres3 = float(event.GetString())
            #             elif event.GetId() == self.tewres3.GetId():
            #                 self.ewres3 = float(event.GetString())
            elif event.GetId() == self.ttbres.GetId():
                self.tbres = float(event.GetString())

            self.__UpdateInfo()

        except ValueError, e:
            if len(event.GetString()) > 0 and event.GetString() != '-':
                dlg = wx.MessageBox(parent=self,
                                    message=_("Invalid value: %s") % e,
                                    caption=_("Error"),
                                    style=wx.OK | wx.ICON_ERROR)
                # reset values
                self.tnorth.SetValue(str(self.north))
                self.tsouth.SetValue(str(self.south))
                self.teast.SetValue(str(self.east))
                self.twest.SetValue(str(self.west))
                self.tnsres.SetValue(str(self.nsres))
                self.tewres.SetValue(str(self.ewres))
                self.ttop.SetValue(str(self.top))
                self.tbottom.SetValue(str(self.bottom))
                self.ttbres.SetValue(str(self.tbres))
                # self.tnsres3.SetValue(str(self.nsres3))
                # self.tewres3.SetValue(str(self.ewres3))

        event.Skip()

    def __UpdateInfo(self):
        """Update number of rows/cols/cells"""
        self.rows = int((self.north - self.south) / self.nsres)
        self.cols = int((self.east - self.west) / self.ewres)
        self.cells = self.rows * self.cols

        self.depth = int((self.top - self.bottom) / self.tbres)
        self.cells3 = self.rows * self.cols * self.depth

        # 2D
        self.lrows.SetLabel(_("Rows: %d") % self.rows)
        self.lcols.SetLabel(_("Cols: %d") % self.cols)
        self.lcells.SetLabel(_("Cells: %d") % self.cells)
        # 3D
        self.ldepth.SetLabel(_("Depth: %d" % self.depth))
        self.lcells3.SetLabel(_("3D Cells: %d" % self.cells3))

    def OnSetButton(self, event=None):
        """Set default region"""
        cmdlist = ['g.region', '-sgpa',
                   'n=%f' % self.north,
                   's=%f' % self.south,
                   'e=%f' % self.east,
                   'w=%f' % self.west,
                   'nsres=%f' % self.nsres,
                   'ewres=%f' % self.ewres,
                   't=%f' % self.top,
                   'b=%f' % self.bottom,
                   'tbres=%f' % self.tbres]

        p = gcmd.Command(cmdlist)
        if p.returncode == 0:
            self.Destroy()

    def OnCancel(self, event):
        self.Destroy()

class SelectDatumDialog(wx.Dialog):
    """Dialog for selecting datum transformations"""
    def __init__(self, parent, datums, title=_("Select datum transformation"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_DIALOG_STYLE):

        self.datums = datums
        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)

        panel = wx.Panel(self, wx.ID_ANY)

        sizer = wx.BoxSizer(wx.VERTICAL)

        #
        # dialog body
        #
        bodyBox = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                               label=" %s " % _("List of datum transformations"))
        bodySizer = wx.StaticBoxSizer(bodyBox, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        
        gridSizer.Add(item=wx.StaticText(parent=panel, label=_("Datums:")),
                      flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))

        items = self.datums.keys()
        utils.ListSortLower(items)
        self.cdatums = wx.ComboBox(parent=panel, id=wx.ID_ANY,
                              style=wx.CB_SIMPLE | wx.CB_READONLY,
                              choices=items,
                              size=(300,-1))
        self.cdatums.SetSelection(0)
        self.cdatums.Bind(wx.EVT_COMBOBOX, self.OnChangeDatum)
        gridSizer.Add(item=self.cdatums, pos=(0, 1))

        self.textWidth = self.GetSize()[0]
        self.datumDesc = wx.StaticText(parent=panel,
                                       label='\n'.join(self.datums[self.cdatums.GetStringSelection()]))
        self.datumDesc.Wrap(self.textWidth)

        gridSizer.Add(item=self.datumDesc, flag=wx.EXPAND,
                      pos=(1, 0), span=(1, 2))

        bodySizer.Add(item=gridSizer, proportion=1,
                      flag=wx.ALL | wx.ALIGN_CENTER | wx.EXPAND, border=5)

        #
        # buttons
        #
        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent=panel, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent=panel, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item=bodySizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        #
        # set panel sizer
        #
        panel.SetSizer(sizer)
        sizer.Fit(panel)
        self.SetSize(self.GetBestSize())

    def OnChangeDatum(self, event):
        """Datum changed, update description text"""
        self.datumDesc.SetLabel('\n'.join(self.datums[event.GetString()]))
        self.datumDesc.Wrap(self.textWidth)

        event.Skip()

    def GetDatum(self):
        """Get selected datum"""
        return self.cdatums.GetStringSelection()

if __name__ == "__main__":
    app = wx.PySimpleApp()
    # gWizard = LocationWizard(None, "")
    gWizard = RegionDef(None)
    gWizzard.Show()
    app.MainLoop()
