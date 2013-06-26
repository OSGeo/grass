"""!
@package location_wizard.dialogs

@brief Location wizard - dialogs

Classes:
 - dialogs::RegionDef
 - dialogs::TransList
 - dialogs::SelectTransformDialog

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>   
"""
import os
import sys

import wx
import wx.lib.scrolledpanel as scrolled

if __name__ == '__main__':
    wxbase = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if wxbase not in sys.path:
        sys.path.append(wxbase)

from core                 import globalvar
from core.gcmd            import RunCommand
from location_wizard.base import BaseClass

from grass.script import core as grass

class RegionDef(BaseClass, wx.Dialog):
    """!Page for setting default region extents and resolution
    """
    def __init__(self, parent, id = wx.ID_ANY, size = (800, 600),
                 title = _("Set default region extent and resolution"), location = None):
        wx.Dialog.__init__(self, parent, id, title, size = size)
        panel = wx.Panel(self, id = wx.ID_ANY)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.parent = parent
        self.location = location
        
        #
        # default values
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
        self.tnorth = self.MakeTextCtrl(text = str(self.north), size = (150, -1), parent = panel)
        self.tsouth = self.MakeTextCtrl(str(self.south), size = (150, -1), parent = panel)
        self.twest = self.MakeTextCtrl(str(self.west), size = (150, -1), parent = panel)
        self.teast = self.MakeTextCtrl(str(self.east), size = (150, -1), parent = panel)
        self.tnsres = self.MakeTextCtrl(str(self.nsres), size = (150, -1), parent = panel)
        self.tewres = self.MakeTextCtrl(str(self.ewres), size = (150, -1), parent = panel)
        
        #
        # labels
        #
        self.lrows  = self.MakeLabel(parent = panel)
        self.lcols  = self.MakeLabel(parent = panel)
        self.lcells = self.MakeLabel(parent = panel)
        
        #
        # buttons
        #
        self.bset = self.MakeButton(text = _("&Set region"), id = wx.ID_OK, parent = panel)
        self.bcancel = wx.Button(panel, id = wx.ID_CANCEL)
        self.bset.SetDefault()
        
        #
        # image
        #
        self.img = wx.Image(os.path.join(globalvar.ETCIMGDIR, "qgis_world.png"),
                            wx.BITMAP_TYPE_PNG).ConvertToBitmap()
        
        #
        # set current working environment to PERMANENT mapset
        # in selected location in order to set default region (WIND)
        #
        envval = {}
        ret = RunCommand('g.gisenv',
                         read = True)
        if ret:
            for line in ret.splitlines():
                key, val = line.split('=')
                envval[key] = val
            self.currlocation = envval['LOCATION_NAME'].strip("';")
            self.currmapset = envval['MAPSET'].strip("';")
            if self.currlocation != self.location or self.currmapset != 'PERMANENT':
                RunCommand('g.gisenv',
                           set = 'LOCATION_NAME=%s' % self.location)
                RunCommand('g.gisenv',
                           set = 'MAPSET=PERMANENT')
        else:
            dlg = wx.MessageBox(parent = self,
                                message = _('Invalid location selected.'),
                                caption = _("Error"), style = wx.ID_OK | wx.ICON_ERROR)
            return
        
        #
        # get current region settings
        #
        region = {}
        ret = RunCommand('g.region',
                         read = True,
                         flags = 'gp3')
        if ret:
            for line in ret.splitlines():
                key, val = line.split('=')
                region[key] = float(val)
        else:
            dlg = wx.MessageBox(parent = self,
                                message = _("Invalid region"),
                                caption = _("Error"), style = wx.ID_OK | wx.ICON_ERROR)
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
        self.cells3 = int(region['cells3'])
        
        #
        # 3D box collapsable
        #
        self.infoCollapseLabelExp = _("Click here to show 3D settings")
        self.infoCollapseLabelCol = _("Click here to hide 3D settings")
        self.settings3D = wx.CollapsiblePane(parent = panel,
                                             label = self.infoCollapseLabelExp,
                                             style = wx.CP_DEFAULT_STYLE |
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
        self.lrows.SetLabel(_("Rows: %d") % self.rows)
        self.lcols.SetLabel(_("Cols: %d") % self.cols)
        self.lcells.SetLabel(_("Cells: %d") % self.cells)
        
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
        
        self.__DoLayout(panel)
        self.SetMinSize(self.GetBestSize())
        self.minWindowSize = self.GetMinSize()
        wx.CallAfter(self.settings3D.Collapse, True)
    
    def MakeSettings3DPaneContent(self, pane):
        """!Create 3D region settings pane"""
        border = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 0, hgap = 0)

        # inputs
        self.ttop = wx.TextCtrl(parent = pane, id = wx.ID_ANY, value = str(self.top),
                                size = (150, -1))
        self.tbottom = wx.TextCtrl(parent = pane, id = wx.ID_ANY, value = str(self.bottom),
                                size = (150, -1))
        self.ttbres = wx.TextCtrl(parent = pane, id = wx.ID_ANY, value = str(self.tbres),
                                size = (150, -1))
        #         self.tnsres3 = wx.TextCtrl(parent = pane, id = wx.ID_ANY, value = str(self.nsres3),
        #                                    size = (150, -1))
        #         self.tewres3  =  wx.TextCtrl(parent = pane, id = wx.ID_ANY, value = str(self.ewres3),
        #                                    size = (150, -1))

        #labels
        self.ldepth = wx.StaticText(parent = pane, label = _("Depth: %d") % self.depth)
        self.lcells3  =  wx.StaticText(parent = pane, label = _("3D Cells: %d") % self.cells3)

        # top
        gridSizer.Add(item = wx.StaticText(parent = pane, label = _("Top")),
                      flag = wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.TOP, border = 5,
                      pos = (0, 1))
        gridSizer.Add(item = self.ttop,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border = 5, pos = (1, 1))
        # bottom
        gridSizer.Add(item = wx.StaticText(parent = pane, label = _("Bottom")),
                      flag = wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.TOP, border = 5,
                      pos = (0, 2))
        gridSizer.Add(item = self.tbottom,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border = 5, pos = (1, 2))
        # tbres
        gridSizer.Add(item = wx.StaticText(parent = pane, label = _("T-B resolution")),
                      flag = wx.ALIGN_CENTER | 
                      wx.LEFT | wx.RIGHT | wx.TOP, border = 5,
                      pos = (0, 3))
        gridSizer.Add(item = self.ttbres,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALL, border = 5, pos = (1, 3))

        # res
        #         gridSizer.Add(item = wx.StaticText(parent = pane, label = _("3D N-S resolution")),
        #                       flag = wx.ALIGN_CENTER |
        #                       wx.LEFT | wx.RIGHT | wx.TOP, border = 5,
        #                       pos = (2, 1))
        #         gridSizer.Add(item = self.tnsres3,
        #                       flag = wx.ALIGN_CENTER_HORIZONTAL |
        #                       wx.ALL, border = 5, pos = (3, 1))
        #         gridSizer.Add(item = wx.StaticText(parent = pane, label = _("3D E-W resolution")),
        #                       flag = wx.ALIGN_CENTER |
        #                       wx.LEFT | wx.RIGHT | wx.TOP, border = 5,
        #                       pos = (2, 3))
        #         gridSizer.Add(item = self.tewres3,
        #                       flag = wx.ALIGN_CENTER_HORIZONTAL |
        #                       wx.ALL, border = 5, pos = (3, 3))

        # rows/cols/cells
        gridSizer.Add(item = self.ldepth,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border = 5, pos = (2, 1))

        gridSizer.Add(item = self.lcells3,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border = 5, pos = (2, 2))

        border.Add(item = gridSizer, proportion = 1,
                   flag = wx.ALL | wx.ALIGN_CENTER | wx.EXPAND, border = 5)

        pane.SetSizer(border)
        border.Fit(pane)

    def OnSettings3DPaneChanged(self, event):
        """!Collapse 3D settings box"""

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

    def __DoLayout(self, panel):
        """!Window layout"""
        frameSizer = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 0, hgap = 0)
        settings3DSizer = wx.BoxSizer(wx.VERTICAL)
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)

        # north
        gridSizer.Add(item = self.MakeLabel(text = _("North"), parent = panel),
                      flag = wx.ALIGN_BOTTOM | wx.ALIGN_CENTER_HORIZONTAL |
                      wx.TOP | wx.LEFT | wx.RIGHT, border = 5, pos = (0, 2))
        gridSizer.Add(item = self.tnorth,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5, pos = (1, 2))
        # west
        gridSizer.Add(item = self.MakeLabel(text = _("West"), parent = panel),
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.LEFT | wx.TOP | wx.BOTTOM, border = 5, pos = (2, 0))
        gridSizer.Add(item = self.twest,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5,  pos = (2, 1))

        gridSizer.Add(item = wx.StaticBitmap(panel, wx.ID_ANY, self.img, (-1, -1),
                                           (self.img.GetWidth(), self.img.GetHeight())),
                      flag = wx.ALIGN_CENTER |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5, pos = (2, 2))

        # east
        gridSizer.Add(item = self.teast,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5,  pos = (2, 3))
        gridSizer.Add(item = self.MakeLabel(text = _("East"), parent = panel),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.RIGHT | wx.TOP | wx.BOTTOM, border = 5, pos = (2, 4))
        # south
        gridSizer.Add(item = self.tsouth,
                      flag = wx.ALIGN_CENTER_HORIZONTAL |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5, pos = (3, 2))
        gridSizer.Add(item = self.MakeLabel(text = _("South"), parent = panel),
                      flag = wx.ALIGN_TOP | wx.ALIGN_CENTER_HORIZONTAL |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5, pos = (4, 2))
        # ns-res
        gridSizer.Add(item = self.MakeLabel(text = _("N-S resolution"), parent = panel),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.TOP | wx.LEFT | wx.RIGHT, border = 5, pos = (5, 1))
        gridSizer.Add(item = self.tnsres,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5,  pos = (6, 1))
        # ew-res
        gridSizer.Add(item = self.MakeLabel(text = _("E-W resolution"), parent = panel),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.TOP | wx.LEFT | wx.RIGHT, border = 5, pos = (5, 3))
        gridSizer.Add(item = self.tewres,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL |
                      wx.ALL, border = 5,  pos = (6, 3))
        # rows/cols/cells
        gridSizer.Add(item = self.lrows,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border = 5, pos = (7, 1))

        gridSizer.Add(item = self.lcells,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border = 5, pos = (7, 2))

        gridSizer.Add(item = self.lcols,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER |
                      wx.ALL, border = 5, pos = (7, 3))

        # 3D
        settings3DSizer.Add(item = self.settings3D,
                            flag = wx.ALL,
                            border = 5)

        # buttons
        buttonSizer.Add(item = self.bcancel, proportion = 1,
                        flag = wx.ALIGN_RIGHT |
                        wx.ALIGN_CENTER_VERTICAL |
                        wx.ALL, border = 10)
        buttonSizer.Add(item = self.bset, proportion = 1,
                        flag = wx.ALIGN_CENTER |
                        wx.ALIGN_CENTER_VERTICAL |
                        wx.ALL, border = 10)

        frameSizer.Add(item = gridSizer, proportion = 1,
                       flag = wx.ALL | wx.ALIGN_CENTER, border = 5)
        frameSizer.Add(item = settings3DSizer, proportion = 0,
                       flag = wx.ALL | wx.ALIGN_CENTER, border = 5)
        frameSizer.Add(item = buttonSizer, proportion = 0,
                       flag = wx.ALL | wx.ALIGN_RIGHT, border = 5)

        self.SetAutoLayout(True)
        panel.SetSizer(frameSizer)
        frameSizer.Fit(panel)
        self.Layout()

    def OnValue(self, event):
        """!Set given value"""
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
                dlg = wx.MessageBox(parent = self,
                                    message = _("Invalid value: %s") % e,
                                    caption = _("Error"),
                                    style = wx.OK | wx.ICON_ERROR)
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
        """!Update number of rows/cols/cells"""
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

    def OnSetButton(self, event = None):
        """!Set default region"""
        ret = RunCommand('g.region',
                         flags = 'sgpa',
                         n = self.north,
                         s = self.south,
                         e = self.east,
                         w = self.west,
                         nsres = self.nsres,
                         ewres = self.ewres,
                         t = self.top,
                         b = self.bottom,
                         tbres = self.tbres)
        if ret == 0:
            self.Destroy()

    def OnCancel(self, event):
        self.Destroy()

class TransList(wx.VListBox):
    """!Creates a multiline listbox for selecting datum transforms"""
        
    def OnDrawItem(self, dc, rect, n):
        if self.GetSelection() == n:
            c = wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHTTEXT)
        else:
            c = self.GetForegroundColour()
        dc.SetFont(self.GetFont())
        dc.SetTextForeground(c)
        dc.DrawLabel(self._getItemText(n), rect,
                     wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)

    def OnMeasureItem(self, n):
        height = 0
        if self._getItemText(n) == None:
            return
        for line in self._getItemText(n).splitlines():
            w, h = self.GetTextExtent(line)
            height += h
        return height + 5

    def _getItemText(self, item):
        global transformlist
        transitem = transformlist[item]
        if transitem.strip() !='':
            return transitem

class SelectTransformDialog(wx.Dialog):
    """!Dialog for selecting datum transformations"""
    def __init__(self, parent, transforms, title = _("Select datum transformation"),
                 pos = wx.DefaultPosition, size = wx.DefaultSize, 
                 style = wx.DEFAULT_DIALOG_STYLE|wx.RESIZE_BORDER):

        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)

        global transformlist
        self.CentreOnParent()
        
        # default transform number
        self.transnum = 0
        
        panel = scrolled.ScrolledPanel(self, wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)

        #
        # set panel sizer
        #
        panel.SetSizer(sizer)
        panel.SetupScrolling()

        #
        # dialog body
        #
        bodyBox = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                               label = " %s " % _("Select from list of datum transformations"))
        bodySizer = wx.StaticBoxSizer(bodyBox)       
        
        # add no transform option
        transforms = '---\n\n0\nDo not apply any datum transformations\n\n' + transforms
        
        transformlist = transforms.split('---')
        tlistlen = len(transformlist)
        
        # calculate size for transform list
        height = 0
        width = 0
        for line in transforms.splitlines():
            w, h = self.GetTextExtent(line)
            height += h
            width = max(width, w)
            
        height = height + 5
        if height > 400: height = 400
        width = width + 5
        if width > 400: width = 400

        #
        # VListBox for displaying and selecting transformations
        #
        self.translist = TransList(panel, id = -1, size = (width, height), style = wx.SUNKEN_BORDER)
        self.translist.SetItemCount(tlistlen)
        self.translist.SetSelection(2)
        self.translist.SetFocus()
        
        self.Bind(wx.EVT_LISTBOX, self.ClickTrans, self.translist)

        bodySizer.Add(item = self.translist, proportion = 1, flag = wx.ALIGN_CENTER|wx.ALL|wx.EXPAND)

        #
        # buttons
        #
        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent = panel, id = wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent = panel, id = wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item = bodySizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        sizer.Add(item = btnsizer, proportion = 0,
                  flag =  wx.ALL | wx.ALIGN_RIGHT, border = 5)

        sizer.Fit(panel)

        self.SetSize(self.GetBestSize())
        self.Layout()
        
    def ClickTrans(self, event):
        """!Get the number of the datum transform to use in g.proj"""
        self.transnum = event.GetSelection()
        self.transnum = self.transnum - 1
    
    def GetTransform(self):
        """!Get the number of the datum transform to use in g.proj"""
        self.transnum = self.translist.GetSelection()
        self.transnum = self.transnum - 1
        return self.transnum

def testRegionDef():
    import sys
    import gettext
    import wx.lib.inspection
    import grass.script as grass
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

    app = wx.PySimpleApp()

    dlg = RegionDef(None, location = grass.gisenv()["LOCATION_NAME"])
    dlg.Show()
    wx.lib.inspection.InspectionTool().Show()
    app.MainLoop()
if __name__ == '__main__':
    testRegionDef()
