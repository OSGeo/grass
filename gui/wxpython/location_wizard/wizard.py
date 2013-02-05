"""!
@package location_wizard.wizard

@brief Location wizard - creates a new GRASS Location. User can choose
from multiple methods.

Classes:
 - wizard::TitledPage
 - wizard::DatabasePage
 - wizard::CoordinateSystemPage
 - wizard::ProjectionsPage
 - wizard::ItemList
 - wizard::ProjParamsPage
 - wizard::DatumPage
 - wizard::EllipsePage
 - wizard::GeoreferencedFilePage
 - wizard::WKTPage
 - wizard::EPSGPage
 - wizard::CustomPage
 - wizard::SummaryPage
 - wizard::LocationWizard
 - wizard::WizardWithHelpButton

(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>   
"""
import os
import locale

import wx
import wx.lib.mixins.listctrl as listmix
import wx.wizard as wiz
import wx.lib.scrolledpanel as scrolled

from core                    import globalvar
from core                    import utils
from core.gcmd               import RunCommand, GError, GMessage, GWarning
from location_wizard.base    import BaseClass
from location_wizard.dialogs import SelectTransformDialog

from grass.script import core as grass

global coordsys
global north
global south
global east
global west
global resolution
global wizerror
global translist

class TitledPage(BaseClass, wiz.WizardPageSimple):
    """!Class to make wizard pages. Generic methods to make labels,
    text entries, and buttons.
    """
    def __init__(self, parent, title):

        self.page = wiz.WizardPageSimple.__init__(self, parent)

        # page title
        self.title = wx.StaticText(parent = self, id = wx.ID_ANY, label = title)
        self.title.SetFont(wx.Font(13, wx.SWISS, wx.NORMAL, wx.BOLD))

        # main sizers
        self.pagesizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer = wx.GridBagSizer(vgap = 0, hgap = 0)
        
    def DoLayout(self):
        """!Do page layout"""
        self.pagesizer.Add(item = self.title, proportion = 0,
                           flag = wx.ALIGN_CENTRE | wx.ALL,
                           border = 5)
        self.pagesizer.Add(item = wx.StaticLine(self, -1), proportion = 0,
                           flag = wx.EXPAND | wx.ALL,
                           border = 0)
        self.pagesizer.Add(item = self.sizer, proportion = 1,
                           flag = wx.EXPAND)
        
        self.SetAutoLayout(True)
        self.SetSizer(self.pagesizer)
        self.Layout()

class DatabasePage(TitledPage):
    """!Wizard page for setting GIS data directory and location name"""
    def __init__(self, wizard, parent, grassdatabase):
        TitledPage.__init__(self, wizard, _("Define GRASS Database and Location Name"))

        self.grassdatabase  = grassdatabase
        self.location       = ''
        self.locTitle       = ''
        
        # buttons
        self.bbrowse = self.MakeButton(_("Browse"))

        # text controls
        self.tgisdbase = self.MakeTextCtrl(grassdatabase, size = (300, -1))
        self.tlocation = self.MakeTextCtrl("newLocation", size = (300, -1))
        self.tlocTitle = self.MakeTextCtrl(size = (400, -1))
        
        # layout
        self.sizer.Add(item = self.MakeLabel(_("GIS Data Directory:")),
                       flag = wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (1, 1))
        self.sizer.Add(item = self.tgisdbase,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (1, 2))
        self.sizer.Add(item = self.bbrowse,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (1, 3))
        
        self.sizer.Add(item = self.MakeLabel("%s:" % _("Project Location"),
                                             tooltip = _("Name of location directory in GIS Data Directory")),
                       flag = wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (2, 1))
        self.sizer.Add(item = self.tlocation,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (2, 2))

        self.sizer.Add(item = self.MakeLabel("%s:" % _("Optional Location Title"),
                                             tooltip = _("Optional location title, "
                                                         "you can leave this field blank.")),
                       flag = wx.ALIGN_RIGHT |
                       wx.ALIGN_TOP | wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (3, 1))
        self.sizer.Add(item = self.tlocTitle,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5,
                       pos = (3, 2), span  =  (1, 2))
        self.sizer.AddGrowableCol(3)
        
        # bindings
        self.Bind(wx.EVT_BUTTON,                self.OnBrowse, self.bbrowse)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.tgisdbase.Bind(wx.EVT_TEXT,        self.OnChangeName)
        self.tlocation.Bind(wx.EVT_TEXT,        self.OnChangeName)
        
    def OnChangeName(self, event):
        """!Name for new location was changed"""
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(event.GetString()) > 0:
            if not nextButton.IsEnabled():
                nextButton.Enable()
        else:
            nextButton.Disable()

        event.Skip()

    def OnBrowse(self, event):
        """!Choose GRASS data directory"""
        dlg = wx.DirDialog(self, _("Choose GRASS data directory:"),
                           os.getcwd(), wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            self.grassdatabase = dlg.GetPath()
            self.tgisdbase.SetValue(self.grassdatabase)
            
        dlg.Destroy()

    def OnPageChanging(self, event = None):
        error = None
        if os.path.isdir(os.path.join(self.tgisdbase.GetValue(), self.tlocation.GetValue())):
            error = _("Location already exists in GRASS Database.")

        if error:
            GError(parent = self,
                   message="%s <%s>.%s%s" % (_("Unable to create location"),
                                             str(self.tlocation.GetValue()),
                                             os.linesep,
                                             error))
            event.Veto()
            return

        self.location      = self.tlocation.GetValue()
        self.grassdatabase = self.tgisdbase.GetValue()
        self.locTitle      = self.tlocTitle.GetValue()
        if os.linesep in self.locTitle or \
                len(self.locTitle) > 255:
            GWarning(parent = self,
                     message = _("Title of the location is limited only to one line and "
                                 "256 characters. The rest of the text will be ignored."))
            self.locTitle = self.locTitle.split(os.linesep)[0][:255]
            
class CoordinateSystemPage(TitledPage):
    """!Wizard page for choosing method for location creation"""
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose method for creating a new location"))
        
        self.parent = parent
        global coordsys
        
        # toggles
        self.radio1 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Select coordinate system parameters from a list"),
                                     style  =  wx.RB_GROUP)
        self.radio2 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Select EPSG code of spatial reference system"))
        self.radio3 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Read projection and datum terms from a "
                                             "georeferenced data file"))
        self.radio4 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Read projection and datum terms from a "
                                             "WKT or PRJ file"))
        self.radio5 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Specify projection and datum terms using custom "
                                             "PROJ.4 parameters"))
        self.radio6 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Create a generic Cartesian coordinate system (XY)"))
        
        # layout
        self.sizer.AddGrowableCol(1)
        self.sizer.SetVGap(10)
        self.sizer.Add(item = self.radio1,
                       flag = wx.ALIGN_LEFT, pos = (1, 1))
        self.sizer.Add(item = self.radio2,
                       flag = wx.ALIGN_LEFT, pos = (2, 1))
        self.sizer.Add(item = self.radio3,
                       flag = wx.ALIGN_LEFT, pos = (3, 1))
        self.sizer.Add(item = self.radio4,
                       flag = wx.ALIGN_LEFT, pos = (4, 1))
        self.sizer.Add(item = self.radio5,
                       flag = wx.ALIGN_LEFT, pos = (5, 1))
        self.sizer.Add(item = self.radio6,
                       flag = wx.ALIGN_LEFT, pos = (6, 1))

        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio1.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio2.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio3.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio4.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio5.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio6.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED,  self.OnEnterPage)
        
    def OnEnterPage(self, event):
        global coordsys
        
        if not coordsys:
            coordsys = "proj"
            self.radio1.SetValue(True)
        else:
            if coordsys == 'proj':
                self.radio1.SetValue(True)
            if coordsys == "epsg":
                self.radio2.SetValue(True)
            if coordsys == "file":
                self.radio3.SetValue(True)
            if coordsys == "wkt":
                self.radio4.SetValue(True)
            if coordsys == "custom":
                self.radio5.SetValue(True)
            if coordsys == "xy":
                self.radio6.SetValue(True)
        
        if event.GetDirection():
            if coordsys == 'proj':
                self.SetNext(self.parent.projpage)
                self.parent.sumpage.SetPrev(self.parent.datumpage)
            if coordsys == "epsg":
                self.SetNext(self.parent.epsgpage)
                self.parent.sumpage.SetPrev(self.parent.epsgpage)
            if coordsys == "file":
                self.SetNext(self.parent.filepage)
                self.parent.sumpage.SetPrev(self.parent.filepage)
            if coordsys == "wkt":
                self.SetNext(self.parent.wktpage)
                self.parent.sumpage.SetPrev(self.parent.wktpage)
            if coordsys == "custom":
                self.SetNext(self.parent.custompage)
                self.parent.sumpage.SetPrev(self.parent.custompage)
            if coordsys == "xy":
                self.SetNext(self.parent.sumpage)
                self.parent.sumpage.SetPrev(self.parent.csystemspage)
        
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()
    
    def SetVal(self, event):
        """!Choose method"""
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
    """!Wizard page for selecting projection (select coordinate system option)"""
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose projection"))

        self.parent = parent
        self.proj = ''
        self.projdesc = ''
        self.p4proj = ''

        # text input
        self.tproj = self.MakeTextCtrl("", size = (200,-1))
        
        # search box
        self.searchb = wx.SearchCtrl(self, size = (200,-1),
                                     style = wx.TE_PROCESS_ENTER)

        # projection list
        self.projlist = ItemList(self, data = self.parent.projdesc.items(),
                                 columns = [_('Code'), _('Description')])
        self.projlist.resizeLastColumn(30) 

        # layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item = self.MakeLabel(_("Projection code:")),
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1))
        self.sizer.Add(item = self.tproj,
                       flag = wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL,
                       border = 5, pos = (1, 2))

        self.sizer.Add(item = self.MakeLabel(_("Search in description:")),
                       flag = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                       border = 5, pos = (2, 1))
        self.sizer.Add(item = self.searchb,
                       flag = wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL,
                       border = 5, pos = (2, 2))
        
        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item = self.projlist,
                       flag = wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border = 5, pos = (3, 1), span = (1, 3))

        # events
        self.tproj.Bind(wx.EVT_TEXT, self.OnText)
        self.tproj.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)
        self.projlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED,  self.OnEnterPage)
        
    def OnPageChanging(self,event):
        if event.GetDirection() and self.proj not in self.parent.projections.keys():
            event.Veto()

    def OnText(self, event):
        """!Projection name changed"""
        self.proj = event.GetString().lower()
        self.p4proj = ''
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if self.proj not in self.parent.projections.keys() and nextButton.IsEnabled():
            nextButton.Enable(False)
        
        if self.proj in self.parent.projections.keys():
            if self.proj == 'stp':
                wx.MessageBox('Currently State Plane projections must be selected using the '
                              'text-based setup (g.setproj), or entered by EPSG code or '
                              'custom PROJ.4 terms.',
                              'Warning', wx.ICON_WARNING)
                self.proj = ''
                self.tproj.SetValue(self.proj)
                nextButton.Enable(False)
                return
            elif self.proj.lower() == 'll':
                self.p4proj = '+proj=longlat'
            else:
                self.p4proj = '+proj=' + self.proj.lower()
            self.projdesc = self.parent.projections[self.proj][0]
            nextButton.Enable()

    def OnEnterPage(self, event):
        if len(self.proj) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()
    
    def OnSearch(self, event):
        """!Search projection by desc"""
        str = event.GetString()
        try:
            self.proj, self.projdesc = self.projlist.Search(index = [0,1], pattern = event.GetString())
        except:
            self.proj = self.projdesc = ''
            
        event.Skip()

    def OnItemSelected(self, event):
        """!Projection selected"""
        index = event.m_itemIndex

        # set values
        self.proj = self.projlist.GetItem(index, 0).GetText().lower()
        self.tproj.SetValue(self.proj)
        
        event.Skip()

class ItemList(wx.ListCtrl,
               listmix.ListCtrlAutoWidthMixin,
               listmix.ColumnSorterMixin):
    """!Generic list (for projections, ellipsoids, etc.)"""

    def __init__(self, parent, columns, data = None):
        wx.ListCtrl.__init__(self, parent = parent, id = wx.ID_ANY,
                             style = wx.LC_REPORT |
                             wx.LC_VIRTUAL | 
                             wx.LC_HRULES |
                             wx.LC_VRULES |
                             wx.LC_SINGLE_SEL |
                             wx.LC_SORT_ASCENDING, size = (550, 125))

        # original data or None
        self.sourceData = data
        
        #
        # insert columns
        #
        i = 0
        for column in columns:
            self.InsertColumn(i, column)
            i += 1
        #
        # add some attributes
        #
        self.attr1 = wx.ListItemAttr()
        self.attr1.SetBackgroundColour(wx.Colour(238,238,238))
        self.attr2 = wx.ListItemAttr()
        self.attr2.SetBackgroundColour("white")

        if self.sourceData:
            self.Populate()
        
        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE_USEHEADER)
            if self.GetColumnWidth(i) < 80:
                self.SetColumnWidth(i, 80)
        
        #
        # listmix
        #
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.ColumnSorterMixin.__init__(self, self.GetColumnCount())
            
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
            self.SortListItems(col = 0, ascending = True)

        #
        # bindings
        #
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColumnClick)

    def Populate(self, data = None, update = False):
        """!Populate list"""
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
                self.itemDataMap[row] = [value[0]]
                for i in range(1, len(value)):
                     self.itemDataMap[row].append(value[i])
                self.itemIndexMap.append(row)
                row += 1

            self.SetItemCount(row)
            
            # set column width
            self.SetColumnWidth(0, 80)
            self.SetColumnWidth(1, 300)
            
            self.SendSizeEvent()
            
        except StandardError, e:
            wx.MessageBox(parent = self,
                          message = _("Unable to read list: %s") % e,
                          caption = _("Error"), style = wx.OK | wx.ICON_ERROR)

    def OnColumnClick(self, event):
        """!Sort by column"""
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
        """!Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py"""
        return (self.sm_dn, self.sm_up)

    def OnGetItemText(self, item, col):
        """!Get item text"""
        index = self.itemIndexMap[item]
        s = str(self.itemDataMap[index][col])
        return s

    def OnGetItemAttr(self, item):
        """!Get item attributes"""
        index = self.itemIndexMap[item]
        if ( index % 2) == 0:
            return self.attr2
        else:
            return self.attr1

    def SortItems(self, sorter = cmp):
        """!Sort items"""
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
        """!Used by listmix.ColumnSorterMixin"""
        return self

    def Search (self, index, pattern):
        """!Search projection by description
        Return first found item or None
        """
        if pattern == '':
            self.Populate(self.sourceData)
            return []

        data = []
        pattern = pattern.lower()
        for i in range(len(self.sourceData)):
            for idx in index:
                try:
                    value = str(self.sourceData[i][idx]).lower()
                    if pattern in value:
                        data.append(self.sourceData[i])
                        break
                except UnicodeDecodeError:
                    # osgeo4w problem (should be fixed)
                    pass

        self.Populate(data)
        if len(data) > 0:
            return data[0]
        else:
            return []

class ProjParamsPage(TitledPage):
    """!Wizard page for selecting method of setting coordinate system
    parameters (select coordinate system option)
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose projection parameters"))
        global coordsys
        
        self.parent = parent
        self.panel = None
        self.prjParamSizer = None
        
        self.pparam = dict()
        
        self.p4projparams = ''
        self.projdesc = ''

        self.sizer.AddGrowableCol(1)
        self.sizer.AddGrowableRow(1)

        radioSBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                 label = " %s " % _("Select datum or ellipsoid (next page)"))
        radioSBSizer = wx.StaticBoxSizer(radioSBox)
        self.sizer.Add(item = radioSBSizer, pos = (0, 1),
                       flag = wx.EXPAND | wx.ALIGN_TOP | wx.TOP, border = 10)
        
        self.radio1 = wx.RadioButton(parent = self, id = wx.ID_ANY, 
                                     label = _("Datum with associated ellipsoid"),
                                     style  =  wx.RB_GROUP)
        self.radio2 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                                     label = _("Ellipsoid only"))   
        
        # default button setting
        if self.radio1.GetValue() == False and self.radio2.GetValue() == False:
            self.radio1.SetValue(True)
            self.SetNext(self.parent.datumpage)
            #            self.parent.sumpage.SetPrev(self.parent.datumpage)  
        
        radioSBSizer.Add(item = self.radio1,
                         flag = wx.ALIGN_LEFT | wx.RIGHT, border = 20)
        radioSBSizer.Add(item = self.radio2,
                         flag = wx.ALIGN_LEFT)
        
        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio1.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio2.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChange)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        
    def OnParamEntry(self, event):
        """!Parameter value changed"""
        id  = event.GetId()
        val = event.GetString()
        
        if id not in self.pparam:
            event.Skip()
            return

        param = self.pparam[id]
        win = self.FindWindowById(id)
        if param['type'] == 'zone':
            val = event.GetInt()
            if val < 1:
                win.SetValue(1)
            elif val > 60:
                    win.SetValue(60)
        
        if param['type'] == 'bool':
            param['value'] = event.GetSelection()
        else:
            param['value'] = val
        
        event.Skip()
        
    def OnPageChange(self,event=None):
        """!Go to next page"""
        if event.GetDirection():
            self.p4projparams = ''
            for id, param in self.pparam.iteritems():
                if param['type'] == 'bool':
                    if param['value'] == False:
                        continue
                    else:
                        self.p4projparams += (' +' + param['proj4'])
                else:
                    if param['value'] is None:
                        wx.MessageBox(parent = self,
                                      message = _('You must enter a value for %s') % param['desc'],
                                      caption = _('Error'), style = wx.ICON_ERROR | wx.CENTRE)
                        event.Veto()
                    else:
                        self.p4projparams += (' +' + param['proj4'] + '=' + str(param['value']))

    def OnEnterPage(self,event):
        """!Page entered"""
        self.projdesc = self.parent.projections[self.parent.projpage.proj][0]
        if self.prjParamSizer is None:
            # entering page for the first time
            self.paramSBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                     label = _(" Enter parameters for %s projection ") % self.projdesc)
            paramSBSizer = wx.StaticBoxSizer(self.paramSBox)
            
            self.panel = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY)
            self.panel.SetupScrolling()
            
            self.prjParamSizer = wx.GridBagSizer(vgap = 0, hgap = 0) 
            
            self.sizer.Add(item = paramSBSizer, pos = (1, 1),
                           flag = wx.EXPAND)
            paramSBSizer.Add(item = self.panel, proportion = 1, 
                             flag = wx.ALIGN_CENTER | wx.EXPAND)
            
            paramSBSizer.Fit(self.panel)
            self.panel.SetSizer(self.prjParamSizer)
                    
        if event.GetDirection(): 
            self.prjParamSizer.Clear(True)
            self.paramSBox.SetLabel(_(" Enter parameters for %s projection ") % self.projdesc)
            self.pparam = dict()
            row = 0
            for paramgrp in self.parent.projections[self.parent.projpage.proj][1]:
                # get parameters
                id = wx.NewId()
                param = self.pparam[id] = { 'type' : self.parent.paramdesc[paramgrp[0]][0],
                                            'proj4': self.parent.paramdesc[paramgrp[0]][1],
                                            'desc' : self.parent.paramdesc[paramgrp[0]][2] }
                
                # default values
                if param['type'] == 'bool':
                    param['value'] = 0
                elif param['type'] == 'zone': 
                    param['value'] = 30 
                    param['desc'] += ' (1-60)'
                else:
                    param['value'] = paramgrp[2]
                
                label = wx.StaticText(parent = self.panel, id = wx.ID_ANY, label = param['desc'], 
                                      style = wx.ALIGN_RIGHT | wx.ST_NO_AUTORESIZE)
                if param['type'] == 'bool':
                    win = wx.Choice(parent = self.panel, id = id, size = (100,-1), 
                                    choices = [_('No'), _('Yes')])  
                    win.SetSelection(param['value'])
                    win.Bind(wx.EVT_CHOICE, self.OnParamEntry)
                elif param['type'] == 'zone':
                    win = wx.SpinCtrl(parent = self.panel, id = id,
                                      size = (100, -1), 
                                      style = wx.SP_ARROW_KEYS | wx.SP_WRAP,
                                      min = 1, max = 60)
                    win.SetValue(param['value'])
                    win.Bind(wx.EVT_SPINCTRL, self.OnParamEntry)
                    win.Bind(wx.EVT_TEXT, self.OnParamEntry)
                else:
                    win = wx.TextCtrl(parent = self.panel, id = id,
                                      value = param['value'],
                                      size=(100, -1))
                    win.Bind(wx.EVT_TEXT, self.OnParamEntry)
                    if paramgrp[1] == 'noask':
                        win.Enable(False)
                    
                self.prjParamSizer.Add(item = label, pos = (row, 1),
                                       flag = wx.ALIGN_RIGHT | 
                                       wx.ALIGN_CENTER_VERTICAL |
                                       wx.RIGHT, border = 5)
                self.prjParamSizer.Add(item = win, pos = (row, 2),
                                       flag = wx.ALIGN_LEFT | 
                                       wx.ALIGN_CENTER_VERTICAL |
                                       wx.LEFT, border = 5)           
                row += 1
        
        self.panel.SetSize(self.panel.GetBestSize())
        self.panel.Layout()
        self.Layout()
        self.Update()
        
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()
        
        event.Skip()

    def SetVal(self, event):
        """!Set value"""
        if event.GetId() == self.radio1.GetId():
            self.SetNext(self.parent.datumpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
        elif event.GetId() == self.radio2.GetId():
            self.SetNext(self.parent.ellipsepage)
            self.parent.sumpage.SetPrev(self.parent.ellipsepage)
    
class DatumPage(TitledPage):
    """!Wizard page for selecting datum (with associated ellipsoid)
    and datum transformation parameters (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify geodetic datum"))

        self.parent = parent
        self.datum = ''
        self.datumdesc = ''
        self.ellipse = ''
        self.datumparams = ''
        self.proj4params = ''

        # text input
        self.tdatum = self.MakeTextCtrl("", size = (200,-1))

        # search box
        self.searchb = wx.SearchCtrl(self, size = (200,-1),
                                     style = wx.TE_PROCESS_ENTER)

        # create list control for datum/elipsoid list
        data = []
        for key in self.parent.datums.keys():
            data.append([key, self.parent.datums[key][0], self.parent.datums[key][1]])
        self.datumlist = ItemList(self,
                                  data = data,
                                  columns = [_('Code'), _('Ellipsoid'), _('Description')])
        self.datumlist.resizeLastColumn(10) 
        
        # layout
        self.sizer.AddGrowableCol(4)
        self.sizer.Add(item = self.MakeLabel(_("Datum code:")),
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1))
        self.sizer.Add(item = self.tdatum,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 2))

        self.sizer.Add(item = self.MakeLabel(_("Search in description:")),
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 1))
        self.sizer.Add(item = self.searchb,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 2))

        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item = self.datumlist,
                       flag = wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border = 5, pos = (3, 1), span = (1, 4))

        # events
        self.datumlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnDatumSelected)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnDSearch)
        self.tdatum.Bind(wx.EVT_TEXT, self.OnDText)
        self.tdatum.Bind(wx.EVT_TEXT_ENTER, self.OnDText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnPageChanging(self, event):
        self.proj4params = ''
        proj = self.parent.projpage.p4proj
                
        if event.GetDirection():
            if self.datum not in self.parent.datums:
                event.Veto()
            else:
                # check for datum tranforms            
#                proj4string = self.parent.CreateProj4String() + ' +datum=%s' % self.datum
                ret = RunCommand('g.proj',
                                 read = True,
                                 proj4 = '%s' % proj,
                                 datum = '%s' % self.datum, 
                                 datum_trans = '-1')
#                wx.Messagebox('here')
                if ret != '':
                    dtrans = ''
                    # open a dialog to select datum transform number
                    dlg = SelectTransformDialog(self.parent.parent, transforms=ret)
                    
                    if dlg.ShowModal() == wx.ID_OK:
                        dtrans = dlg.GetTransform()
                        if dtrans == '':
                            dlg.Destroy()
                            event.Veto()
                            return 'Datum transform is required.'
                    else:
                        dlg.Destroy()
                        event.Veto()
                        return 'Datum transform is required.'
                    
                    self.parent.datum_trans = dtrans
                
            self.GetNext().SetPrev(self)
            self.parent.ellipsepage.ellipse = self.ellipse
            self.parent.ellipsepage.ellipseparams = self.parent.ellipsoids[self.ellipse][1]

    def OnEnterPage(self,event):
        self.parent.datum_trans = None
        if event.GetDirection():
            if len(self.datum) == 0:
                # disable 'next' button by default when entering from previous page
                wx.FindWindowById(wx.ID_FORWARD).Enable(False)
            else:
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnDText(self, event):
        """!Datum code changed"""
        self.datum = event.GetString()

        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.datum) == 0 or self.datum not in self.parent.datums:
            nextButton.Enable(False)
        else:
            self.ellipse = self.parent.datums[self.datum][0]
            self.datumdesc = self.parent.datums[self.datum][1]
            self.datumparams = self.parent.datums[self.datum][2]
            try:
                self.datumparams.remove('dx=0.0')
            except:
                pass
            try:
                self.datumparams.remove('dy=0.0')
            except:
                pass
            try:
                self.datumparams.remove('dz=0.0')
            except:
                pass
            
            nextButton.Enable(True)
            
        self.Update()    
        event.Skip()

    def OnDSearch(self, event):
        """!Search geodetic datum by desc"""
        str =  self.searchb.GetValue()
        try:
            self.datum, self.ellipsoid, self.datumdesc = self.datumlist.Search(index = [0,1,2], pattern = str)
        except:
            self.datum = self.datumdesc = self.ellipsoid = ''

        event.Skip()

    def OnDatumSelected(self, event):
        """!Datum selected"""
        index = event.m_itemIndex
        item = event.GetItem()

        self.datum = self.datumlist.GetItem(index, 0).GetText()
        self.tdatum.SetValue(self.datum)
        
        event.Skip()

class EllipsePage(TitledPage):
    """!Wizard page for selecting ellipsoid (select coordinate system option)"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify ellipsoid"))

        self.parent = parent
        
        self.ellipse = ''
        self.ellipsedesc = ''
        self.ellipseparams = ''
        self.proj4params = ''

        # text input
        self.tellipse = self.MakeTextCtrl("", size = (200,-1))

        # search box
        self.searchb = wx.SearchCtrl(self, size = (200,-1),
                                     style = wx.TE_PROCESS_ENTER)
        # radio buttons
        self.radio1 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                        label = _("Earth based"),
                        style  =  wx.RB_GROUP)
        self.radio2 = wx.RadioButton(parent = self, id = wx.ID_ANY,
                        label = _("Planetary bodies"))

        # create list control for ellipse list
        data = []
        # extract code, desc
        for key in self.parent.ellipsoids.keys():
            data.append([key, self.parent.ellipsoids[key][0]])

        self.ellipselist = ItemList(self, data = data,
                                    columns = [_('Code'), _('Description')])
        self.ellipselist.resizeLastColumn(30)                             

        # layout
        self.sizer.AddGrowableCol(4)

        self.sizer.Add(item = self.MakeLabel(_("Ellipsoid code:")),
                       flag = wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1))
        self.sizer.Add(item = self.tellipse,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 2))
        self.sizer.Add(item = self.radio1,
                       flag = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 
                       border = 25, pos = (1, 3))

        self.sizer.Add(item = self.MakeLabel(_("Search in description:")),
                       flag = wx.ALIGN_RIGHT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 1))
        self.sizer.Add(item = self.searchb,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 2))
        self.sizer.Add(item = self.radio2,
                       flag = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                       border = 25, pos = (2, 3))

        self.sizer.AddGrowableRow(3)
        self.sizer.Add(item = self.ellipselist,
                       flag = wx.EXPAND |
                       wx.ALIGN_LEFT |
                       wx.ALL, border = 5, pos = (3, 1), span = (1, 4))

        # events
        self.ellipselist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.tellipse.Bind(wx.EVT_TEXT, self.OnText)
        self.tellipse.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)

        self.radio1.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio1.GetId())
        self.radio2.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id = self.radio2.GetId())

        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)

    def OnEnterPage(self,event):
        if len(self.ellipse) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        self.scope = 'earth'
        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() \
          and self.ellipse not in self.parent.ellipsoids \
          and self.ellipse not in self.parent.planetary_ellipsoids:
            event.Veto()

        self.proj4params = ''
        self.GetNext().SetPrev(self)
        self.parent.datumpage.datumparams = ''
        # self.GetNext().SetPrev(self) (???)

    #FIXME: index number doesn't translate when you've given a valid name from the other list
    def OnText(self, event):
        """!Ellipspoid code changed"""
        self.ellipse = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.ellipse) == 0 or \
          (self.ellipse not in self.parent.ellipsoids and \
           self.ellipse not in self.parent.planetary_ellipsoids):
            nextButton.Enable(False)
            self.ellipsedesc = ''
            self.ellipseparams = ''
            self.proj4params = ''
        elif self.ellipse in self.parent.ellipsoids:
            self.ellipsedesc = self.parent.ellipsoids[self.ellipse][0]
            self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            nextButton.Enable(True)
        elif self.ellipse in self.parent.planetary_ellipsoids:
            self.ellipsedesc = self.parent.planetary_ellipsoids[self.ellipse][0]
            self.ellipseparams = self.parent.planetary_ellipsoids[self.ellipse][1]
            nextButton.Enable(True)

    def OnSearch(self, event):
        """!Search ellipsoid by desc"""
        try:
            self.ellipse, self.ellipsedesc = \
                self.ellipselist.Search(index=[0,1], pattern=event.GetString())
            if self.scope is 'earth':
                self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            else:
                self.ellipseparams = self.parent.planetary_ellipsoids[self.ellipse][1]
        except:
            self.ellipse = self.ellipsedesc = self.ellipseparams = ''

        event.Skip()

    def OnItemSelected(self,event):
        """!Ellipsoid selected"""
        index = event.m_itemIndex
        item = event.GetItem()

        self.ellipse = self.ellipselist.GetItem(index, 0).GetText()
        self.tellipse.SetValue(self.ellipse)
        
        event.Skip()

    def SetVal(self, event):
        """!Choose table to use"""
        self.ellipselist.DeleteAllItems()
        data = []
        if event.GetId() == self.radio1.GetId():
            self.scope = 'earth'
            for key in self.parent.ellipsoids.keys():
                data.append([key, self.parent.ellipsoids[key][0]])
        elif event.GetId() == self.radio2.GetId():
            self.scope = 'planetary'
            for key in self.parent.planetary_ellipsoids.keys():
                data.append([key, self.parent.planetary_ellipsoids[key][0]])

        self.ellipselist.Populate(data = data, update = True)


class GeoreferencedFilePage(TitledPage):
    """!Wizard page for selecting georeferenced file to use
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select georeferenced file"))

        self.georeffile = ''

        # create controls
        self.lfile= self.MakeLabel(_("Georeferenced file:"))
        self.tfile = self.MakeTextCtrl(size = (300,-1))
        self.bbrowse = self.MakeButton(_("Browse"))

        # do layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item = self.lfile, flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1))
        self.sizer.Add(item = self.tfile, flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 2))
        self.sizer.Add(item = self.bbrowse, flag = wx.ALIGN_LEFT |
                       wx.ALL, border = 5, pos = (1, 3))

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
        if event.GetDirection() and not os.path.isfile(self.georeffile):
            event.Veto()
        self.GetNext().SetPrev(self)

        event.Skip()

    def OnText(self, event):
        """!File changed"""
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
        """!Choose file"""
        dlg = wx.FileDialog(self,
                            _("Select georeferenced file"),
                            os.getcwd(), "", "*.*", wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
        dlg.Destroy()

        event.Skip()

class WKTPage(TitledPage):
    """!Wizard page for selecting WKT file to use
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select WKT or PRJ file"))

        self.wktfile = ''

        # create controls
        self.lfile= self.MakeLabel(_("WKT or PRJ file:"))
        self.tfile = self.MakeTextCtrl(size = (300,-1))
        self.bbrowse = self.MakeButton(_("Browse"))

        # do layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item = self.lfile, flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1))
        self.sizer.Add(item = self.tfile, flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTRE_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 2))
        self.sizer.Add(item = self.bbrowse, flag = wx.ALIGN_LEFT |
                       wx.ALL, border = 5, pos = (1, 3))

        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event):
        if len(self.wktfile) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection() and not os.path.isfile(self.wktfile):
            event.Veto()
        self.GetNext().SetPrev(self)

        event.Skip()

    def OnText(self, event):
        """!File changed"""
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
        """!Choose file"""
        dlg = wx.FileDialog(parent = self, message = _("Select WKT or PRJ file"),
                            defaultDir = os.getcwd(), 
                            wildcard = "PRJ files (*.prj)|*.prj|Files (*.*)|*.*", style = wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
        dlg.Destroy()

        event.Skip()

class EPSGPage(TitledPage):
    """!Wizard page for selecting EPSG code for
    setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose EPSG Code"))
        self.parent = parent
        self.epsgCodeDict = {}
        self.epsgcode = None
        self.epsgdesc = ''
        self.epsgparams = ''

        # labels
        self.lfile = self.MakeLabel(_("Path to the EPSG-codes file:"),
                                    style = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        self.lcode = self.MakeLabel(_("EPSG code:"),
                                    style = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        # text input
        epsgdir = utils.PathJoin(os.environ["GRASS_PROJSHARE"], 'epsg')
        self.tfile = self.MakeTextCtrl(text = epsgdir, size = (200,-1),
                                       style = wx.TE_PROCESS_ENTER)
        self.tcode = self.MakeTextCtrl(size = (200,-1))

        # buttons
        self.bbrowse = self.MakeButton(_("Browse"))
        
        # search box
        self.searchb = wx.SearchCtrl(self, size = (200,-1),
                                     style = wx.TE_PROCESS_ENTER)

        self.epsglist = ItemList(self, data = None,
                                 columns = [_('Code'), _('Description'), _('Parameters')])

        # layout
        self.sizer.AddGrowableCol(3)
        self.sizer.Add(item = self.lfile,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 1), span = (1, 2))
        self.sizer.Add(item = self.tfile,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 3))
        self.sizer.Add(item = self.bbrowse,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (1, 4))
        self.sizer.Add(item = self.lcode,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 1), span = (1, 2))
        self.sizer.Add(item = self.tcode,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (2, 3))
        self.sizer.Add(item = self.searchb,
                       flag = wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL, border = 5, pos = (3, 3))
        
        self.sizer.AddGrowableRow(4)
        self.sizer.Add(item = self.epsglist,
                       flag = wx.ALIGN_LEFT | wx.EXPAND, pos = (4, 1),
                       span = (1, 4))

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT_ENTER, self.OnBrowseCodes)
        self.tcode.Bind(wx.EVT_TEXT, self.OnText)
        self.tcode.Bind(wx.EVT_TEXT_ENTER, self.OnText)
        self.epsglist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.searchb.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event):
        self.parent.datum_trans = None
        if event.GetDirection():
            if not self.epsgcode:
                # disable 'next' button by default
                wx.FindWindowById(wx.ID_FORWARD).Enable(False)
            else:
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        # load default epsg database file
        self.OnBrowseCodes(None)
        
        event.Skip()

    def OnPageChanging(self, event):
        if event.GetDirection():
            if not self.epsgcode:
                event.Veto()
                return
            else:              
                # check for datum transforms
                ret = RunCommand('g.proj',
                                 read = True,
                                 epsg = self.epsgcode,
                                 datum_trans = '-1')
                
                if ret != '':
                    dtrans = ''
                    # open a dialog to select datum transform number
                    dlg = SelectTransformDialog(self.parent.parent, transforms = ret)
                    
                    if dlg.ShowModal() == wx.ID_OK:
                        dtrans = dlg.GetTransform()
                        if dtrans == '':
                            dlg.Destroy()
                            event.Veto()
                            return 'Datum transform is required.'
                    else:
                        dlg.Destroy()
                        event.Veto()
                        return 'Datum transform is required.'
                    
                    self.parent.datum_trans = dtrans
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
            self.epsgcode = None # not found
            if nextButton.IsEnabled():
                nextButton.Enable(False)
            self.epsgdesc = self.epsgparams = ''
        
    def OnSearch(self, event):
        value =  self.searchb.GetValue()
        
        if value == '':
            self.epsgcode = None
            self.epsgdesc = self.epsgparams = ''
            self.tcode.SetValue('')
            self.searchb.SetValue('')
            self.OnBrowseCodes(None)
        else:    
            try:
                self.epsgcode, self.epsgdesc, self.epsgparams = \
                        self.epsglist.Search(index=[0,1,2], pattern=value)
            except (IndexError, ValueError): # -> no item found
                self.epsgcode = None
                self.epsgdesc = self.epsgparams = ''
                self.tcode.SetValue('')
                self.searchb.SetValue('')

        event.Skip()
        
    def OnBrowse(self, event):
        """!Define path for EPSG code file"""
        path = os.path.dirname(self.tfile.GetValue())
        if not path:
            path = os.getcwd()
        
        dlg = wx.FileDialog(parent = self, message = _("Choose EPSG codes file"),
                            defaultDir = path, defaultFile = "", wildcard = "*", style = wx.OPEN)
        
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
            self.OnBrowseCodes(None)
        
        dlg.Destroy()

        event.Skip()

    def OnItemSelected(self, event):
        """!EPSG code selected from the list"""
        index = event.m_itemIndex
        item = event.GetItem()

        self.epsgcode = int(self.epsglist.GetItem(index, 0).GetText())
        self.epsgdesc = self.epsglist.GetItem(index, 1).GetText()
        self.tcode.SetValue(str(self.epsgcode))

        event.Skip()
        
    def OnBrowseCodes(self, event, search = None):
        """!Browse EPSG codes"""
        self.epsgCodeDict = utils.ReadEpsgCodes(self.tfile.GetValue())

        if type(self.epsgCodeDict) != dict:
            wx.MessageBox(parent = self,
                          message = _("Unable to read EPGS codes: %s") % self.epsgCodeDict,
                          caption = _("Error"),  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            self.epsglist.Populate(list(), update = True)
            return
        
        data = list()
        for code, val in self.epsgCodeDict.iteritems():
            if code is not None:
                data.append((code, val[0], val[1]))
        
        self.epsglist.Populate(data, update = True)
        
class CustomPage(TitledPage):
    """!Wizard page for entering custom PROJ.4 string
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard,
                            _("Choose method of specifying georeferencing parameters"))
        global coordsys
        self.customstring = ''
        self.parent = parent

        # widgets
        self.text_proj4string = self.MakeTextCtrl(size = (400, 200),
                                                  style = wx.TE_MULTILINE)
        self.label_proj4string = self.MakeLabel(_("Enter PROJ.4 parameters string:"))

        # layout
        self.sizer.AddGrowableCol(2)
        self.sizer.Add(self.label_proj4string,
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (1, 1))
        self.sizer.AddGrowableRow(2)
        self.sizer.Add(self.text_proj4string,
                       flag = wx.ALIGN_LEFT | wx.ALL | wx.EXPAND, 
                       border = 5, pos = (2, 1), span = (1, 2))

        self.text_proj4string.Bind(wx.EVT_TEXT, self.GetProjstring)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event):
        if len(self.customstring) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnPageChanging(self, event):
        if event.GetDirection():
        # check for datum tranforms            
            ret, out, err = RunCommand('g.proj',
                                       read = True, getErrorMsg = True,
                                       proj4 = self.customstring, 
                                       datum_trans = '-1')
            if ret != 0:
                wx.MessageBox(parent = self,
                              message = err,
                              caption = _("Error"),
                              style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
                event.Veto()
                return
            
            if out:
                dtrans = ''
                # open a dialog to select datum transform number
                dlg = SelectTransformDialog(self.parent.parent, transforms = out)
                
                if dlg.ShowModal() == wx.ID_OK:
                    dtrans = dlg.GetTransform()
                    if len(dtrans) == 0:
                        dlg.Destroy()
                        event.Veto()
                        return _('Datum transform is required.')
                else:
                    dlg.Destroy()
                    event.Veto()
                    return _('Datum transform is required.')
                
                self.parent.datum_trans = dtrans
        
        self.GetNext().SetPrev(self)
            
    def GetProjstring(self, event):
        """!Change proj string"""
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
    """!Shows summary result of choosing coordinate system parameters
    prior to creating location"""
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))
        self.parent = parent

        self.panelTitle = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY)
        self.panelProj4string = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY)
        self.panelProj = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY)

        # labels
        self.ldatabase    = self.MakeLabel()
        self.llocation    = self.MakeLabel()
        self.llocTitle    = self.MakeLabel(parent = self.panelTitle)
        self.lprojection  = self.MakeLabel(parent = self.panelProj)
        self.lproj4string = self.MakeLabel(parent = self.panelProj4string)
        
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        
        # do sub-page layout
        self._doLayout()
        
    def _doLayout(self):
        """!Do page layout"""
        self.sizer.AddGrowableCol(1)
        self.sizer.AddGrowableRow(3, 1)
        self.sizer.AddGrowableRow(4, 1)
        self.sizer.AddGrowableRow(5, 5)

        titleSizer = wx.BoxSizer(wx.VERTICAL)
        titleSizer.Add(item = self.llocTitle, proportion = 1,
                       flag = wx.EXPAND | wx.ALL, border = 5)
        self.panelTitle.SetSizer(titleSizer)        
        
        projSizer = wx.BoxSizer(wx.VERTICAL)
        projSizer.Add(item = self.lprojection, proportion = 1,
                       flag = wx.EXPAND | wx.ALL, border = 5)
        self.panelProj.SetSizer(projSizer)        
        
        proj4stringSizer = wx.BoxSizer(wx.VERTICAL)
        proj4stringSizer.Add(item = self.lproj4string, proportion = 1,
                       flag = wx.EXPAND | wx.ALL, border = 5)
        self.panelProj4string.SetSizer(proj4stringSizer)

        self.panelProj4string.SetupScrolling()
        self.panelProj.SetupScrolling(scroll_y = False)
        self.panelTitle.SetupScrolling(scroll_y = False)

        self.sizer.Add(item = self.MakeLabel(_("GRASS Database:")),
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (1, 0))
        self.sizer.Add(item = self.ldatabase, 
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (1, 1))
        self.sizer.Add(item = self.MakeLabel(_("Location Name:")),
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (2, 0))
        self.sizer.Add(item = self.llocation,
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (2, 1))
        self.sizer.Add(item = self.MakeLabel(_("Location Title:")),
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (3, 0))
        self.sizer.Add(item = self.panelTitle,
                       flag = wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
                       border = 0, pos = (3, 1))
        self.sizer.Add(item = self.MakeLabel(_("Projection:")),
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (4, 0))
        self.sizer.Add(item = self.panelProj,
                       flag = wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
                       border = 0, pos = (4, 1))
        self.sizer.Add(item = self.MakeLabel(_("PROJ.4 definition:")),
                       flag = wx.ALIGN_LEFT | wx.ALL,
                       border = 5, pos = (5, 0))
        self.sizer.Add(item = self.panelProj4string,
                       flag = wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
                       border = 0, pos = (5, 1))
   
    def OnEnterPage(self, event):
        """!Insert values into text controls for summary of location
        creation options
        """
        database = self.parent.startpage.grassdatabase
        location = self.parent.startpage.location
        proj4string = self.parent.CreateProj4String()
        epsgcode = self.parent.epsgpage.epsgcode
        datum = self.parent.datumpage.datum
        dtrans = self.parent.datum_trans
        
        global coordsys
        if coordsys in ('proj', 'epsg'):
            if coordsys == 'proj':
                ret, projlabel, err = RunCommand('g.proj',
                                                 flags = 'jf',
                                                 proj4 = proj4string,
                                                 datum = datum,
                                                 datum_trans = dtrans,
                                                 location = location,
                                                 getErrorMsg = True,
                                                 read = True)
            elif coordsys == 'epsg':
                ret, projlabel, err = RunCommand('g.proj',
                                                 flags = 'jf',
                                                 epsg = epsgcode,
                                                 datum_trans = dtrans,
                                                 location = location,
                                                 getErrorMsg = True,
                                                 read = True)

            finishButton = wx.FindWindowById(wx.ID_FORWARD)
            if ret == 0:
                if datum != '':
                    projlabel = projlabel + ' ' + 'datum=%s' % datum
                self.lproj4string.SetLabel(projlabel.replace(' ', os.linesep))
                finishButton.Enable(True)
            else:
                GError(err, parent = self)
                self.lproj4string.SetLabel('')
                finishButton.Enable(False)
        
        projdesc = self.parent.projpage.projdesc
        ellipsedesc = self.parent.ellipsepage.ellipsedesc
        datumdesc = self.parent.datumpage.datumdesc
        self.ldatabase.SetLabel(database)
        self.llocation.SetLabel(location)
        self.llocTitle.SetLabel(self.parent.startpage.locTitle)
        
        label = ''
        if coordsys == 'epsg':
            label = 'EPSG code %s (%s)' % (self.parent.epsgpage.epsgcode, self.parent.epsgpage.epsgdesc)
        elif coordsys == 'file':
            label = 'matches file %s' % self.parent.filepage.georeffile
            self.lproj4string.SetLabel("")
        elif coordsys == 'wkt':
            label = 'matches file %s' % self.parent.wktpage.wktfile
            self.lproj4string.SetLabel("")
        elif coordsys == 'proj':
            label = ('%s, %s %s' % (projdesc, datumdesc, ellipsedesc))
        elif coordsys == 'xy':
            label = ('XY coordinate system (not projected).')
            self.lproj4string.SetLabel("")
        elif coordsys == 'custom':
            label = _("custom")
            self.lproj4string.SetLabel(('%s' % self.parent.custompage.customstring.replace(' ', os.linesep)))
        self.lprojection.SetLabel(label)
        
    def OnFinish(self, event):
        dlg = wx.MessageDialog(parent = self.wizard,
                               message = _("Do you want to create GRASS location <%s>?") % location,
                               caption = _("Create new location?"),
                               style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
        
        if dlg.ShowModal() == wx.ID_NO:
            dlg.Destroy()
            event.Veto()
        else:
            dlg.Destroy()
            event.Skip()

class LocationWizard(wx.Object):
    """!Start wizard here and finish wizard here
    """
    def __init__(self, parent, grassdatabase):
        self.__cleanUp()
        
        global coordsys
        self.parent = parent
        
        #
        # define wizard image
        #
        imagePath = os.path.join(globalvar.ETCIMGDIR, "loc_wizard_qgis.png")
        wizbmp = wx.Image(imagePath, wx.BITMAP_TYPE_PNG)
        wizbmp = wizbmp.ConvertToBitmap()
        
        #
        # get georeferencing information from tables in $GISBASE/etc
        #
        self.__readData()
        
        #
        # datum transform number and list of datum transforms
        #
        self.datum_trans = None
        self.proj4string = ''

        # file from which new location is created
        self.georeffile = None

        #
        # define wizard pages
        #
        self.wizard = WizardWithHelpButton(parent, id = wx.ID_ANY, title = _("Define new GRASS Location"),
                                           bitmap = wizbmp)
        self.wizard.Bind(wiz.EVT_WIZARD_HELP, self.OnHelp)

        self.startpage = DatabasePage(self.wizard, self, grassdatabase)
        self.csystemspage = CoordinateSystemPage(self.wizard, self)
        self.projpage = ProjectionsPage(self.wizard, self)
        self.datumpage = DatumPage(self.wizard, self)
        self.paramspage = ProjParamsPage(self.wizard,self)
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
        self.projpage.SetNext(self.paramspage)

        self.paramspage.SetPrev(self.projpage)
        self.paramspage.SetNext(self.datumpage)

        self.datumpage.SetPrev(self.paramspage)
        self.datumpage.SetNext(self.sumpage)

        self.ellipsepage.SetPrev(self.paramspage)
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
        self.paramspage.DoLayout()
        self.epsgpage.DoLayout()
        self.filepage.DoLayout()
        self.wktpage.DoLayout()
        self.ellipsepage.DoLayout()
        self.custompage.DoLayout()
        self.sumpage.DoLayout()
        self.wizard.FitToPage(self.datumpage)
        size = self.wizard.GetPageSize()
        self.wizard.SetPageSize((size[0], size[1] + 75))
        
        # new location created?
        self.location = None 
        success = False
        
        # location created in different GIS database?
        self.altdb = False

        #
        # run wizard...
        #
        if self.wizard.RunWizard(self.startpage):
            msg = self.OnWizFinished()
            if not msg:
                self.wizard.Destroy()
                self.location = self.startpage.location
                self.grassdatabase = self.startpage.grassdatabase
                self.georeffile = self.filepage.georeffile
                # FIXME here was code for setting default region, what for is this if:
                # if self.altdb == False: 
                    
            else: # -> error
                self.wizard.Destroy()
                GError(parent = self.parent,
                       message = "%s" % _("Unable to create new location. "
                                          "Location <%(loc)s> not created.\n\n"
                                          "Details: %(err)s") % \
                           { 'loc' : self.startpage.location,
                             'err' : msg })
        else: # -> canceled
            self.wizard.Destroy()
            GMessage(parent = self.parent,
                     message = _("Location wizard canceled. "
                                 "Location not created."))
            
        self.__cleanUp()
        
    def __cleanUp(self):
        global coordsys
        global north
        global south
        global east
        global west
        global resolution
        global wizerror
        global translist
        
        coordsys = None
        north = None
        south = None
        east = None
        west = None
        resolution = None
        transformlist = list()

    def __readData(self):
        """!Get georeferencing information from tables in $GISBASE/etc/proj"""

        # read projection and parameters
        f = open(os.path.join(globalvar.ETCDIR, "proj", "parms.table"), "r")
        self.projections = {}
        self.projdesc = {}
        for line in f.readlines():
            line = line.strip()
            try:
                proj, projdesc, params = line.split(':')
                paramslist = params.split(';')
                plist = []
                for p in paramslist:
                    if p == '': continue
                    p1, pdefault = p.split(',')
                    pterm, pask = p1.split('=')
                    p = [pterm.strip(), pask.strip(), pdefault.strip()]
                    plist.append(p)
                self.projections[proj.lower().strip()] = (projdesc.strip(), plist)
                self.projdesc[proj.lower().strip()] = projdesc.strip()
            except:
                continue
        f.close()

        # read datum definitions
        f = open(os.path.join(globalvar.ETCDIR, "proj", "datum.table"), "r")
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
            self.datums[datum] = (ellipsoid, datumdesc.replace('_', ' '), paramlist)
        f.close()

        # read Earth-based ellipsiod definitions
        f = open(os.path.join(globalvar.ETCDIR, "proj", "ellipse.table"), "r")
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

        # read Planetary ellipsiod definitions
        f = open(os.path.join(globalvar.ETCDIR, "proj", "ellipse.table.solar.system"), "r")
        self.planetary_ellipsoids = {}
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
            self.planetary_ellipsoids[ellipse] = (desc, paramslist)
        f.close()

        # read projection parameter description and parsing table
        f = open(os.path.join(globalvar.ETCDIR, "proj", "desc.table"), "r")
        self.paramdesc = {}
        for line in f.readlines():
            line = line.strip()
            try:
                pparam, datatype, proj4term, desc = line.split(':')
                self.paramdesc[pparam] = (datatype, proj4term, desc)
            except:
                continue
        f.close()

    def OnWizFinished(self):
        """!Wizard finished, create new location

        @return error message on error
        @return None on success
        """
        database = self.startpage.grassdatabase
        location = self.startpage.location
        
        # location already exists?
        if os.path.isdir(os.path.join(database,location)):
            GError(parent = self.wizard,
                   message = "%s <%s>: %s" % \
                       (_("Unable to create new location"),
                        os.path.join(database, location),
                        _("Location already exists in GRASS Database.")))
            return None
        
        # current GISDbase or a new one?
        current_gdb = grass.gisenv()['GISDBASE']
        if current_gdb != database:
            # change to new GISDbase or create new one
            if os.path.isdir(database) != True:
                # create new directory
                os.mkdir(database)
            
            # change to new GISDbase directory
            RunCommand('g.gisenv',
                       parent = self.wizard,
                       set = 'GISDBASE=%s' % database)
            
            wx.MessageBox(parent = self.wizard,
                          message = _("Location <%(loc)s> will be created "
                                    "in GIS data directory <%(dir)s>. "
                                    "You will need to change the default GIS "
                                    "data directory in the GRASS startup screen.") % \
                              { 'loc' : location, 'dir' : database},
                          caption = _("New GIS data directory"), 
                          style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            
            # location created in alternate GISDbase
            self.altdb = True
        
        global coordsys
        try:
            if coordsys == "xy":
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      desc = self.startpage.locTitle)
            elif coordsys == "proj":
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      proj4 = self.CreateProj4String(),
                                      datum = self.datumpage.datum,
                                      datum_trans = self.datum_trans,
                                      desc = self.startpage.locTitle)
            elif coordsys == 'custom':
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      proj4 = self.custompage.customstring,
                                      desc = self.startpage.locTitle)
            elif coordsys == "epsg":
                if not self.epsgpage.epsgcode:
                    return _('EPSG code missing.')
                
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      epsg = self.epsgpage.epsgcode,
                                      datum = self.datumpage.datum,
                                      datum_trans = self.datum_trans,
                                      desc = self.startpage.locTitle)
            elif coordsys == "file":
                if not self.filepage.georeffile or \
                        not os.path.isfile(self.filepage.georeffile):
                    return _("File <%s> not found." % self.filepage.georeffile)
                
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      filename = self.filepage.georeffile,
                                      desc = self.startpage.locTitle)
            elif coordsys == "wkt":
                if not self.wktpage.wktfile or \
                        not os.path.isfile(self.wktpage.wktfile):
                    return _("File <%s> not found." % self.wktpage.wktfile)
                
                grass.create_location(dbase = self.startpage.grassdatabase,
                                      location = self.startpage.location,
                                      wkt = self.wktpage.wktfile,
                                      desc = self.startpage.locTitle)
        
        except grass.ScriptError, e:
            return e.value
        
        return None
    
    def CreateProj4String(self):
        """!Constract PROJ.4 string"""
        location = self.startpage.location
        proj = self.projpage.p4proj
        projdesc = self.projpage.projdesc
        proj4params = self.paramspage.p4projparams
                
#        datum = self.datumpage.datum
        if self.datumpage.datumdesc:
            datumdesc = self.datumpage.datumdesc +' - ' + self.datumpage.ellipse
        else:
            datumdesc = ''
        datumparams = self.datumpage.datumparams        
        ellipse = self.ellipsepage.ellipse
        ellipsedesc = self.ellipsepage.ellipsedesc
        ellipseparams = self.ellipsepage.ellipseparams
                
        #
        # creating PROJ.4 string
        #
        proj4string = '%s %s' % (proj, proj4params)
                            
        # set ellipsoid parameters
        if ellipse != '':
            proj4string = '%s +ellps=%s' % (proj4string, ellipse)
        for item in ellipseparams:
            if item[:4] == 'f=1/':
                item = ' +rf=' + item[4:]
            else:
                item = ' +' + item
            proj4string = '%s %s' % (proj4string, item)
            
        # set datum transform parameters if relevant
        if datumparams:
            for item in datumparams:
                proj4string = '%s +%s' % (proj4string,item)

        proj4string = '%s +no_defs' % proj4string
        
        return proj4string

    def OnHelp(self, event):
        """'Help' button clicked"""

        # help text in lib/init/helptext.html
        RunCommand('g.manual', entry = 'helptext')

class WizardWithHelpButton(wiz.Wizard):
    def __init__(self, parent, id, title, bitmap):
        pre = wiz.PreWizard()
        pre.SetExtraStyle(wx.wizard.WIZARD_EX_HELPBUTTON)
        pre.Create(parent = parent, id = id, title = title, bitmap = bitmap)
        self.PostCreate(pre)
