"""!
@package nviz_tools.py

@brief Nviz (3D view) tools window

Classes:
 - NvizToolWindow
 - PositionWindow
 - ViewPositionWindow
 - LightPositionWindow

(C) 2008-2010 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton@asu.edu>
"""

import os
import sys
import copy

import wx
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as SP
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN

import globalvar
import gselect
import gcmd
from preferences import globalSettings as UserSettings
from nviz_mapdisp import wxUpdateView, wxUpdateLight, wxUpdateProperties
from debug import Debug

import wxnviz

class NvizToolWindow(FN.FlatNotebook):
    """!Nviz (3D view) tools panel
    """
    def __init__(self, parent, display, id = wx.ID_ANY,
                 style = globalvar.FNPageStyle, **kwargs):
        self.parent     = parent # GMFrame
        self.mapDisplay = display
        self.mapWindow  = display.GetWindow()
        self._display   = self.mapWindow.GetDisplay()
        
        if globalvar.hasAgw:
            kwargs['agwStyle'] = style
        else:
            kwargs['style'] = style
        FN.FlatNotebook.__init__(self, parent, id, **kwargs)
        self.SetTabAreaColour(globalvar.FNPageColor)
        
        self.win  = {} # window ids
        self.page = {} # page ids

        # view page
        self.AddPage(page = self._createViewPage(),
                     text = " %s " % _("View"))

        # data page
        self.AddPage(page = self._createDataPage(),
                     text = " %s " % _("Data"))

        # appearance page
        self.AddPage(page = self._createAppearancePage(),
                     text = " %s " % _("Appearance"))
        
        self.UpdateSettings()
        self.pageChanging = False
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
        # bindings
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        
        self.Update()
        wx.CallAfter(self.SetPage, 'view')
        
    def OnPageChanged(self, event):
        new = event.GetSelection()
        # self.ChangeSelection(new)
    
    def PostViewEvent(self, zExag = False):
        """!Change view settings"""
        event = wxUpdateView(zExag = zExag)
        wx.PostEvent(self.mapWindow, event)

    def _createViewPage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['view'] = { 'id' : 0 }
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Control View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        self.win['view'] = {}
        
        # position
        posSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("W")),
                     pos = (1, 0), flag = wx.ALIGN_CENTER)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("N")),
                     pos = (0, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_BOTTOM)
        view = ViewPositionWindow(panel, size = (175, 175),
                                  mapwindow = self.mapWindow)
        self.win['view']['position'] = view.GetId()
        posSizer.Add(item = view,
                     pos = (1, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("S")),
                     pos = (2, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_TOP)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("E")),
                     pos = (1, 2), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = posSizer, pos = (0, 0))
                  
        # perspective
        # set initial defaults here (or perhaps in a default values file), not in user settings
        self._createControl(panel, data = self.win['view'], name = 'persp',
                            range = (1,100),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Perspective:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = self.FindWindowById(self.win['view']['persp']['slider']), pos = (2, 0))
        gridSizer.Add(item = self.FindWindowById(self.win['view']['persp']['spin']), pos = (3, 0),
                      flag = wx.ALIGN_CENTER)        
        
        # twist
        self._createControl(panel, data = self.win['view'], name = 'twist',
                            range = (-180,180),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Twist:")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = self.FindWindowById(self.win['view']['twist']['slider']), pos = (2, 1))
        gridSizer.Add(item = self.FindWindowById(self.win['view']['twist']['spin']), pos = (3, 1),
                      flag = wx.ALIGN_CENTER)        
        
        # height + z-exag
        self._createControl(panel, data = self.win['view'], name = 'height', sliderHor = False,
                            range = (0, 1),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        
        self._createControl(panel, data = self.win['view'], name = 'z-exag', sliderHor = False,
                            range = (0, 5),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        self.FindWindowById(self.win['view']['z-exag']['slider']).SetValue(1)
        self.FindWindowById(self.win['view']['z-exag']['spin']).SetValue(1)
        
        heightSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:")),
                      pos = (0, 0), flag = wx.ALIGN_LEFT, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['height']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 0))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['height']['spin']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos = (1, 1))
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Z-exag:")),
                      pos = (0, 2), flag = wx.ALIGN_LEFT, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['z-exag']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['z-exag']['spin']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos = (1, 3))
        
        gridSizer.Add(item = heightSizer, pos = (0, 1), flag = wx.ALIGN_RIGHT)
        
        # view setup + reset
        viewSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        viewSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY,
                                           label = _("Look at:")),
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
        
        viewType = wx.Choice (parent = panel, id = wx.ID_ANY, size = (125, -1),
                              choices = [_("top"),
                                         _("north"),
                                         _("south"),
                                         _("east"),
                                         _("west"),
                                         _("north-west"),
                                         _("north-east"),
                                         _("south-east"),
                                         _("south-west")])
        viewType.SetSelection(0)
        viewType.Bind(wx.EVT_CHOICE, self.OnLookAt)
        # self.win['lookAt'] = viewType.GetId()
        viewSizer.Add(item = viewType,
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
        
        reset = wx.Button(panel, id = wx.ID_ANY, label = _("Reset"))
        reset.SetToolTipString(_("Reset to default view"))
        # self.win['reset'] = reset.GetId()
        reset.Bind(wx.EVT_BUTTON, self.OnResetView)
        
        viewSizer.Add(item = wx.Size(-1, -1), proportion = 1,
                      flag = wx.EXPAND)
        viewSizer.Add(item = reset, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_RIGHT,
                      border = 5)
        
        gridSizer.AddGrowableCol(2)
        gridSizer.Add(item = viewSizer, pos = (4, 0), span = (1, 2),
                      flag = wx.EXPAND)
        
        # body
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 2)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        box = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                           label = " %s " % (_("Image Appearance")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        gridSizer.AddGrowableCol(0)
        
        # background color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Background color:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'view',
                                                            subkey = ['background', 'color']),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        self.win['view']['bgcolor'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnBgColor)
        gridSizer.Add(item = color, pos = (0, 1))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel

    def _createDataPage(self):
        """!Create data (surface, vector, volume) settings page"""
        if globalvar.hasAgw:
            self.notebookData = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                                agwStyle = globalvar.FNPageDStyle)
        else:
            self.notebookData = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                                style = globalvar.FNPageDStyle)
        
        # surface page
        self.notebookData.AddPage(page = self._createSurfacePage(),
                                  text = " %s " % _("Surface"))
        # vector page
        self.notebookData.AddPage(page = self._createVectorPage(),
                                  text = " %s " % _("Vector"))
        # volume page
        self.notebookData.AddPage(page = self._createVolumePage(),
                                  text = " %s " % _("Volume"))

        return self.notebookData
    
    def _createAppearancePage(self):
        """!Create data (surface, vector, volume) settings page"""
        if globalvar.hasAgw:
            self.notebookAppearance = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                                      agwStyle = globalvar.FNPageDStyle)
        else:
            self.notebookAppearance = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                                      style = globalvar.FNPageDStyle)
        
        # light page
        self.notebookAppearance.AddPage(page = self._createLightPage(),
                                        text = " %s " % _("Lighting"))
    
        # fringe page
        self.notebookAppearance.AddPage(page = self._createFringePage(),
                                        text = " %s " % _("Fringe"))
        
        return self.notebookAppearance
    
    def _createSurfacePage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['surface'] = {}
        self.page['surface']['id'] = 1
        self.page['surface']['panel'] = panel.GetId()
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['surface'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Raster map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        rmaps = gselect.Select(parent = panel, type = 'raster',
                               onPopup = self.GselectOnPopup)
        rmaps.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnSetRaster)
        self.win['surface']['map'] = rmaps.GetId()
        desc = wx.StaticText(parent = panel, id = wx.ID_ANY)
        self.win['surface']['desc'] = desc.GetId()
        boxSizer.Add(item = rmaps, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        boxSizer.Add(item = desc, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # surface attributes
        #
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Surface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # type 
        self.win['surface']['attr'] = {}
        row = 0
        for code, attrb in (('topo', _("Topography")),
                           ('color', _("Color")),
                           ('mask', _("Mask")),
                           ('transp', _("Transparency")),
                           ('shine', _("Shininess")),
                           ('emit', _("Emission"))):
            self.win['surface'][code] = {} 
            gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                             label = attrb + ':'),
                          pos = (row, 0), flag = wx.ALIGN_CENTER_VERTICAL)
            use = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                             choices = [_("map")])
            if code not in ('topo', 'color', 'shine'):
                use.Insert(item = _("unset"), pos = 0)
                self.win['surface'][code]['required'] = False
            else:
                self.win['surface'][code]['required'] = True
            if code != 'mask':
                use.Append(item = _('constant'))
            self.win['surface'][code]['use'] = use.GetId()
            use.Bind(wx.EVT_CHOICE, self.OnMapObjUse)
            gridSizer.Add(item = use, flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 1))
            
            map = gselect.Select(parent = panel, id = wx.ID_ANY,
                                 # size = globalvar.DIALOG_GSELECT_SIZE,
                                 size = (200, -1),
                                 type = "raster")
            self.win['surface'][code]['map'] = map.GetId() - 1 # FIXME
            map.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            # changing map topography not allowed
            if code == 'topo':
                map.Enable(False)
            gridSizer.Add(item = map, flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 2))
            
            if code == 'color':
                value = csel.ColourSelect(panel, id = wx.ID_ANY,
                                          colour = (0,0,0),
                                          size = globalvar.DIALOG_COLOR_SIZE)
                value.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceMap)
            elif code == 'mask':
                value = None
            else:
                value = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                                    initial = 0)
                if code == 'topo':
                    value.SetRange(minVal = -1e9, maxVal = 1e9)
                elif code in ('shine', 'transp', 'emit'):
                    value.SetRange(minVal = 0, maxVal = 255)
                else:
                    value.SetRange(minVal = 0, maxVal = 100)
                value.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            
            if value:
                self.win['surface'][code]['const'] = value.GetId()
                value.Enable(False)
                gridSizer.Add(item = value, flag = wx.ALIGN_CENTER_VERTICAL,
                              pos = (row, 3))
            else:
                self.win['surface'][code]['const'] = None
            
            self.SetMapObjUseMap(nvizType = 'surface',
                                 attrb = code) # -> enable map / disable constant
                
            row += 1
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        #
        # draw
        #
        self.win['surface']['draw'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        gridSizer.AddGrowableCol(6)
        
        # mode
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Mode:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent = panel, id = wx.ID_ANY, size = (-1, -1),
                          choices = [_("coarse"),
                                     _("fine"),
                                     _("both")])
        mode.SetSelection(0)
        mode.SetName("selection")
        mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['surface']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item = mode, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 1))
        
        # shading
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Shading:")),
                      pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        shade = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['surface']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item = shade, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 3))
        
        # set to all
        all = wx.Button(panel, id = wx.ID_ANY, label = _("Set to all"))
        all.SetToolTipString(_("Use draw settings for all loaded surfaces"))
        all.Bind(wx.EVT_BUTTON, self.OnSurfaceModeAll)
        gridSizer.Add(item = all, flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (0, 4), span = (1,2), border = 3 )
        
        # resolution coarse
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Coarse:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        resSizer = wx.BoxSizer(wx.HORIZONTAL)
        resSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                        label = _("res.")),
                     flag = wx.ALL | wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL, 
                     border = 3)
        resC = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = 6,
                           min = 1,
                           max = 100)
        resC.SetName("value")
        resC.SetValue(6)
        
        self.win['surface']['draw']['res-coarse'] = resC.GetId()
        resC.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item = resC, flag = wx.ALL | wx.ALIGN_LEFT | 
                      wx.ALIGN_CENTER_VERTICAL, border = 3)
        gridSizer.Add(item = resSizer, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        # Coarse style
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("style")),
                      pos = (1, 2), flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
        style = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                          choices = [_("wire"),
                                     _("surface")])
        style.SetName("selection")
        self.win['surface']['draw']['style'] = style.GetId()
        style.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item = style, flag = wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 3))
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("wire color")),
                      pos = (1, 4), flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_RIGHT | wx.LEFT, border = 3)
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetColour((136,136,136))
        color.SetName("colour")
        color.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceWireColor)
        self.win['surface']['draw']['wire-color'] = color.GetId()
        gridSizer.Add(item = color, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos = (1, 5))
        
        # resolution fine
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Fine:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        resSizer = wx.BoxSizer(wx.HORIZONTAL)
        resSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                        label = _("res.")),
                     flag = wx.ALL | wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL, 
                     border = 3)
        resF = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = 3,
                           min = 1,
                           max = 100)
        resF.SetName("value")
        resF.SetValue(3)
        self.win['surface']['draw']['res-fine'] = resF.GetId()
        resF.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item = resF, flag = wx.ALL | wx.ALIGN_LEFT | 
                      wx.ALIGN_CENTER_VERTICAL, border = 3)
        gridSizer.Add(item = resSizer, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # mask
        #
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Mask")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Mask zeros:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        elev = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                           label = _("by elevation"))
        elev.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item = elev, pos = (0, 1))
        
        color = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                           label = _("by color"))
        color.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item = color, pos = (0, 2))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # position
        #
        self.win['surface']['position'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        # position
        self._createControl(panel, data = self.win['surface'], name = 'position',
                            range = (-10000, 10000),
                            bind = (self.OnSurfacePosition, self.OnSurfacePosition, self.OnSurfacePosition))
        
        axis = wx.Choice (parent = panel, id = wx.ID_ANY, size = (75, -1),
                          choices = ["X",
                                     "Y",
                                     "Z"])
        
        self.win['surface']['position']['axis'] = axis.GetId()
        axis.SetSelection(0)
        axis.Bind(wx.EVT_CHOICE, self.OnSurfaceAxis)
        
        pslide = self.FindWindowById(self.win['surface']['position']['slider'])
        pspin = self.FindWindowById(self.win['surface']['position']['spin'])
        
        gridSizer.Add(item = axis, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 0))
        gridSizer.Add(item = pslide, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 1))
        gridSizer.Add(item = pspin, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 2))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        box.SetSizer(boxSizer)
        box.Layout()
        
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel

    def _createVectorPage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['vector'] = {}
        self.page['vector']['id'] = 2
        self.page['vector']['panel'] = panel.GetId()
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['vector'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        vmaps = gselect.Select(parent = panel, type = 'vector',
                               onPopup = self.GselectOnPopup)
        vmaps.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnSetVector)
        self.win['vector']['map'] = vmaps.GetId()
        desc = wx.StaticText(parent = panel, id = wx.ID_ANY)
        self.win['vector']['desc'] = desc.GetId()
        boxSizer.Add(item = vmaps, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        boxSizer.Add(item = desc, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # vector lines
        #
        self.win['vector']['lines'] = {}
        
        showLines = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = _("Show vector lines"))
        showLines.SetValue(True)
        
        self.win['vector']['lines']['show'] = showLines.GetId()
        showLines.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)
        
        pageSizer.Add(item = showLines, proportion = 0,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        # width
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Line:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("width")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_RIGHT)
        
        width = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 1,
                            min = 0,
                            max = 100)
        width.SetValue(1)
        self.win['vector']['lines']['width'] = width.GetId()
        width.Bind(wx.EVT_SPINCTRL, self.OnVectorLines)
        gridSizer.Add(item = width, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("color")),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = (0,0,0),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        self.win['vector']['lines']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnVectorLines)

        gridSizer.Add(item = color, pos = (0, 4), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_LEFT)
        
        # display
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("display")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        display = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                             choices = [_("on surface"),
                                        _("flat")])
        self.win['vector']['lines']['flat'] = display.GetId()
        display.Bind(wx.EVT_CHOICE, self.OnVectorDisplay)
        
        gridSizer.Add(item = display, flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_LEFT, pos = (1, 2), span = (1,2))
        
        # height
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height above surface:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL,
                      span = (1, 3))
        
        surface = wx.ComboBox(parent = panel, id = wx.ID_ANY, size = (250, -1),
                              style = wx.CB_SIMPLE | wx.CB_READONLY,
                              choices = [])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['lines']['surface'] = surface.GetId()
        gridSizer.Add(item = surface, 
                      pos = (2, 3), span = (1, 6),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        self._createControl(panel, data = self.win['vector']['lines'], name = 'height', size = 300,
                            range = (0, 1000),
                            bind = (self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))
        self.FindWindowById(self.win['vector']['lines']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['lines']['height']['spin']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['lines']['height']['slider']),
                      pos = (3, 0), span = (1, 7))
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['lines']['height']['spin']),
                      pos = (3, 7),
                      flag = wx.ALIGN_CENTER)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # vector points
        #
        self.win['vector']['points'] = {}
        
        showPoints = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Show vector points"))
        showPoints.SetValue(True)
        self.win['vector']['points']['show'] = showPoints.GetId()
        showPoints.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)
        
        pageSizer.Add(item = showPoints, proportion = 0,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        # icon size
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Icon:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("size")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        isize = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 1,
                            min = 1,
                            max = 1e6)
        isize.SetName('value')
        isize.SetValue(100)
        self.win['vector']['points']['size'] = isize.GetId()
        isize.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        isize.Bind(wx.EVT_TEXT, self.OnVectorPoints)
        gridSizer.Add(item = isize, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        
        # icon color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("color")),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        icolor = csel.ColourSelect(panel, id = wx.ID_ANY,
                                   size = globalvar.DIALOG_COLOR_SIZE)
        icolor.SetName("color")
        icolor.SetColour((0,0,255))
        self.win['vector']['points']['color'] = icolor.GetId()
        icolor.Bind(csel.EVT_COLOURSELECT, self.OnVectorPoints)
        gridSizer.Add(item = icolor, flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_LEFT,
                      pos = (0, 4))

        # icon width
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("width")),
                      pos = (0, 5), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        iwidth = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                             initial = 1,
                             min = 1,
                             max = 1e6)
        iwidth.SetName('value')
        iwidth.SetValue(100)
        self.win['vector']['points']['width'] = iwidth.GetId()
        iwidth.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        iwidth.Bind(wx.EVT_TEXT, self.OnVectorPoints)
        gridSizer.Add(item = iwidth, pos = (0, 6),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        
        # icon symbol
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("symbol")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                          choices = UserSettings.Get(group = 'nviz', key = 'vector',
                                                   subkey = ['points', 'marker'], internal = True))
        isym.SetName("selection")
        self.win['vector']['points']['marker'] = isym.GetId()
        isym.Bind(wx.EVT_CHOICE, self.OnVectorPoints)
        gridSizer.Add(item = isym, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 2), span = (1,2))
        
        # high
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height above surface:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL,
                      span = (1, 3))
        
        surface = wx.ComboBox(parent = panel, id = wx.ID_ANY, size = (250, -1),
                              style = wx.CB_SIMPLE | wx.CB_READONLY,
                              choices = [])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['points']['surface'] = surface.GetId()
        gridSizer.Add(item = surface, 
                      pos = (2, 3), span = (1, 5),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        self._createControl(panel, data = self.win['vector']['points'], name = 'height', size = 300,
                            range = (0, 1000),
                            bind = (self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))
        
        self.FindWindowById(self.win['vector']['points']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['points']['height']['spin']).SetValue(0)
        
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['points']['height']['slider']),
                      pos = (3, 0), span = (1, 7))
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['points']['height']['spin']),
                      pos = (3, 7),
                      flag = wx.ALIGN_CENTER)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)

        return panel

    def GselectOnPopup(self, ltype, exclude = False):
        """Update gselect.Select() items"""
        maps = list()
        for layer in self.mapWindow.Map.GetListOfLayers(l_type = ltype):
            maps.append(layer.GetName())
        return maps, exclude
    
    def _createVolumePage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['volume'] = {}
        self.page['volume']['id'] = 3
        self.page['volume']['panel'] = panel.GetId()
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['volume'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("3D Raster map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        rmaps = gselect.Select(parent = panel, type = 'raster3d',
                               onPopup = self.GselectOnPopup)
        rmaps.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnSetRaster3D)
        self.win['volume']['map'] = rmaps.GetId()
        desc = wx.StaticText(parent = panel, id = wx.ID_ANY)
        self.win['volume']['desc'] = desc.GetId()
        boxSizer.Add(item = rmaps, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        boxSizer.Add(item = desc, proportion = 0,
                     flag = wx.ALL,
                     border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
                
        #
        # draw
        #
        self.win['volume']['draw'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        gridSizer.AddGrowableCol(4)
        
        # mode
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Mode:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent = panel, id = wx.ID_ANY, size = (150, -1),
                          choices = [_("isosurfaces"),
                                     _("slides")])
        mode.SetSelection(0)
        mode.SetName("selection")
        # mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['volume']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item = mode, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 1))
        
        # shading
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Shading:")),
                      pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        shade = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['volume']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnVolumeIsosurfMode)
        gridSizer.Add(item = shade, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 3))
        
        # resolution (mode)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Resolution:")),
                      pos = (0, 4), flag = wx.ALIGN_CENTER_VERTICAL)
        resol = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 1,
                            min = 1,
                            max = 100)
        resol.SetName("value")
        self.win['volume']['draw']['resolution'] = resol.GetId()
        resol.Bind(wx.EVT_SPINCTRL, self.OnVolumeIsosurfResolution)
        resol.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfResolution)
        gridSizer.Add(item = resol, pos = (0, 5))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        #
        # manage isosurfaces
        #
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("List of isosurfaces")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # list
        isolevel = wx.CheckListBox(parent = panel, id = wx.ID_ANY,
                                   size = (300, 150))
        self.Bind(wx.EVT_CHECKLISTBOX, self.OnVolumeIsosurfCheck, isolevel)
        self.Bind(wx.EVT_LISTBOX, self.OnVolumeIsosurfSelect, isolevel)
        
        self.win['volume']['isosurfs'] = isolevel.GetId()
        gridSizer.Add(item = isolevel, pos = (0, 0), span = (4, 1))
        
        # buttons (add, delete, move up, move down)
        btnAdd = wx.Button(parent = panel, id = wx.ID_ADD)
        self.win['volume']['btnIsosurfAdd'] = btnAdd.GetId()
        btnAdd.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfAdd)
        gridSizer.Add(item = btnAdd,
                      pos = (0, 1))
        btnDelete = wx.Button(parent = panel, id = wx.ID_DELETE)
        self.win['volume']['btnIsosurfDelete'] = btnDelete.GetId()
        btnDelete.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfDelete)
        btnDelete.Enable(False)
        gridSizer.Add(item = btnDelete,
                      pos = (1, 1))
        btnMoveUp = wx.Button(parent = panel, id = wx.ID_UP)
        self.win['volume']['btnIsosurfMoveUp'] = btnMoveUp.GetId()
        btnMoveUp.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfMoveUp)
        btnMoveUp.Enable(False)
        gridSizer.Add(item = btnMoveUp,
                      pos = (2, 1))
        btnMoveDown = wx.Button(parent = panel, id = wx.ID_DOWN)
        self.win['volume']['btnIsosurfMoveDown'] = btnMoveDown.GetId()
        btnMoveDown.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfMoveDown)
        btnMoveDown.Enable(False)
        gridSizer.Add(item = btnMoveDown,
                      pos = (3, 1))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        #
        # isosurface attributes
        #
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Isosurface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        self.win['volume']['attr'] = {}
        row = 0
        for code, attrb in (('topo', _("Topography level")),
                            ('color', _("Color")),
                            ('mask', _("Mask")),
                            ('transp', _("Transparency")),
                            ('shine', _("Shininess")),
                            ('emit', _("Emission"))):
            self.win['volume'][code] = {} 
            # label
            gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                             label = attrb + ':'),
                          pos = (row, 0), flag = wx.ALIGN_CENTER_VERTICAL)
            if code != 'topo':
                use = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                                 choices = [_("map")])
            else:
                use = None
            # check for required properties
            if code not in ('topo', 'color', 'shine'):
                use.Insert(item = _("unset"), pos = 0)
                self.win['volume'][code]['required'] = False
            else:
                self.win['volume'][code]['required'] = True
            if use and code != 'mask':
                use.Append(item = _('constant'))
            if use:
                self.win['volume'][code]['use'] = use.GetId()
                use.Bind(wx.EVT_CHOICE, self.OnMapObjUse)
                gridSizer.Add(item = use, flag = wx.ALIGN_CENTER_VERTICAL,
                              pos = (row, 1))
            
            if code != 'topo':
                map = gselect.Select(parent = panel, id = wx.ID_ANY,
                                     # size = globalvar.DIALOG_GSELECT_SIZE,
                                     size = (200, -1),
                                     type = "grid3")
                self.win['volume'][code]['map'] = map.GetId() - 1 # FIXME
                map.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfMap)
                gridSizer.Add(item = map, flag = wx.ALIGN_CENTER_VERTICAL,
                              pos = (row, 2))
            else:
                map = None
            
            if code == 'color':
                value = csel.ColourSelect(panel, id = wx.ID_ANY,
                                          colour = (0,0,0),
                                          size = globalvar.DIALOG_COLOR_SIZE)
                value.Bind(csel.EVT_COLOURSELECT, self.OnVolumeIsosurfMap)
            elif code == 'mask':
                value = None
            else:
                if code == 'topo':
                    size = (200, -1)
                else:
                    size = (65, -1)
                value = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = size,
                                    initial = 0)
                if code == 'topo':
                    value.SetRange(minVal = -1e9, maxVal = 1e9)
                elif code in ('shine', 'transp', 'emit'):
                    value.SetRange(minVal = 0, maxVal = 255)
                else:
                    value.SetRange(minVal = 0, maxVal = 100)
                value.Bind(wx.EVT_SPINCTRL, self.OnVolumeIsosurfMap)
                value.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfMap)
            
            if value:
                self.win['volume'][code]['const'] = value.GetId()
                if code == 'topo':
                    gridSizer.Add(item = value, flag = wx.ALIGN_CENTER_VERTICAL,
                                  pos = (row, 2))
                else:
                    value.Enable(False)
                    gridSizer.Add(item = value, flag = wx.ALIGN_CENTER_VERTICAL,
                                  pos = (row, 3))
            else:
                self.win['volume'][code]['const'] = None
            
            if code != 'topo':
                self.SetMapObjUseMap(nvizType = 'volume',
                                     attrb = code) # -> enable map / disable constant
            
            row += 1
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
    
    def _createLightPage(self):
        """!Create light page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        
        self.page['light'] = { 'id' : 4 } 
        self.win['light'] = {}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        show = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                           label = _("Show light model"))
        show.Bind(wx.EVT_CHECKBOX, self.OnShowLightModel)
        pageSizer.Add(item = show, proportion = 0,
                      flag = wx.ALL, border = 3)
        surface = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                              label = _("Follow source viewpoint"))
        pageSizer.Add(item = surface, proportion = 0,
                      flag = wx.ALL, border = 3)
        
        # position
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Light source position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        posSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("W")),
                     pos = (1, 0), flag = wx.ALIGN_CENTER)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("N")),
                     pos = (0, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_BOTTOM)
        pos = LightPositionWindow(panel, id = wx.ID_ANY, size = (175, 175),
                                  mapwindow = self.mapWindow)
        self.win['light']['position'] = pos.GetId()
        posSizer.Add(item = pos,
                     pos = (1, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("S")),
                     pos = (2, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_TOP)
        posSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("E")),
                     pos = (1, 2), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = posSizer, pos = (0, 0))
        
        # height
        self._createControl(panel, data = self.win['light'], name = 'z', sliderHor = False,
                            range = (0, 100),
                            bind = (self.OnLightChange, None, self.OnLightChange))
        
        heightSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:")),
                      pos = (0, 0), flag = wx.ALIGN_LEFT, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['light']['z']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 0))
        heightSizer.Add(item = self.FindWindowById(self.win['light']['z']['spin']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos = (1, 1))
        
        gridSizer.Add(item = heightSizer, pos = (0, 1), flag = wx.ALIGN_RIGHT)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 2)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        # position
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Light color and intensity")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)

        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Color:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'light',
                                                            subkey = 'color'),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.Bind(csel.EVT_COLOURSELECT, self.OnLightColor)
        gridSizer.Add(item = color, pos = (0, 2))

        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Brightness:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        self._createControl(panel, data = self.win['light'], name = 'bright', size = 300,
                            range = (0, 100),
                            bind = (self.OnLightValue, None, self.OnLightValue))
        gridSizer.Add(item = self.FindWindowById(self.win['light']['bright']['slider']),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.FindWindowById(self.win['light']['bright']['spin']),
                      pos = (1, 2),
                      flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Ambient:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        self._createControl(panel, data = self.win['light'], name = 'ambient', size = 300,
                            range = (0, 100),
                            bind = (self.OnLightValue, None, self.OnLightValue))
        gridSizer.Add(item = self.FindWindowById(self.win['light']['ambient']['slider']),
                      pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.FindWindowById(self.win['light']['ambient']['spin']),
                      pos = (2, 2),
                      flag = wx.ALIGN_CENTER)

        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 2)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        # reset = wx.Button(panel, id = wx.ID_ANY, label = _("Reset"))
        # reset.SetToolTipString(_("Reset to default view"))
        # # self.win['reset'] = reset.GetId()
        # reset.Bind(wx.EVT_BUTTON, self.OnResetView)
        
        # viewSizer.Add(item = reset, proportion = 1,
        #               flag = wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT,
        #               border = 5)
        
        # gridSizer.AddGrowableCol(3)
        # gridSizer.Add(item = viewSizer, pos = (4, 0), span = (1, 2),
        #               flag = wx.EXPAND)
        
        panel.SetSizer(pageSizer)
        
        return panel

    def _createFringePage(self):
        """!Create fringe page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        
        self.page['fringe'] = { 'id' : 2 } 
        self.win['fringe'] = {}

        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        # selection
        rbox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("Surface")))
        rboxSizer = wx.StaticBoxSizer(rbox, wx.VERTICAL)
        rmaps = gselect.Select(parent = panel, type = 'raster',
                               onPopup = self.GselectOnPopup)
        self.win['fringe']['surface'] = rmaps.GetId()
        rboxSizer.Add(item = rmaps, proportion = 0,
                      flag = wx.ALL,
                      border = 3)
        pageSizer.Add(item = rboxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        ebox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("Edges with fringe")))
        eboxSizer = wx.StaticBoxSizer(ebox, wx.HORIZONTAL)
        for edge in [(_("N && W"), "nw"),
                     (_("N && E"), "ne"),
                     (_("S && W"), "sw"),
                     (_("S && E"), "se")]:
            chkbox = wx.CheckBox(parent = panel,
                                 label = edge[0],
                                 name = edge[1])
            self.win['fringe'][edge[1]] = chkbox.GetId()
            eboxSizer.Add(item = chkbox, proportion = 0,
                         flag = wx.ADJUST_MINSIZE | wx.LEFT | wx.RIGHT, border = 5)
            chkbox.Bind(wx.EVT_CHECKBOX, self.OnFringe)
        
        pageSizer.Add(item = eboxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)

        sbox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("Settings")))
        sboxSizer = wx.StaticBoxSizer(sbox, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        # elevation
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Elevation of fringe from bottom:")),
                      pos = (0, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        spin = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                           size = (65, -1), min = -1e6, max = 1e6)
        spin.SetValue(UserSettings.Get(group = 'nviz', key = 'fringe', subkey = 'elev'))
        spin.Bind(wx.EVT_SPINCTRL, self.OnFringe)
        self.win['fringe']['elev'] = spin.GetId()
        gridSizer.Add(item = spin, pos = (0, 1))
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Color:")),
                      pos = (1, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetColour(UserSettings.Get(group = 'nviz', key = 'fringe',
                                         subkey = 'color'))
        color.Bind(csel.EVT_COLOURSELECT, self.OnFringe)
        self.win['fringe']['color'] = color.GetId()
        gridSizer.Add(item = color, pos = (1, 1))
        
        sboxSizer.Add(item = gridSizer, proportion = 1,
                      flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = sboxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)

        return panel
    
    def OnFringe(self, event):
        """!Show/hide fringe"""
        enabled = event.IsChecked()
        win = self.FindWindowById(event.GetId())
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['surface']
        sid = data['object']['id']
        elev = self.FindWindowById(self.win['fringe']['elev']).GetValue()
        color = self.FindWindowById(self.win['fringe']['color']).GetValue()
        
        self._display.SetFringe(sid, color, elev,
                                self.FindWindowById(self.win['fringe']['nw']).IsChecked(),
                                self.FindWindowById(self.win['fringe']['ne']).IsChecked(),
                                self.FindWindowById(self.win['fringe']['sw']).IsChecked(),
                                self.FindWindowById(self.win['fringe']['se']).IsChecked())
        self.mapWindow.Refresh(False)
        
    def OnScroll(self, event, win, data):
        """!Generic scrolling handler"""
        winName = self.__GetWindowName(win, event.GetId())
        if not winName:
            return
        data[winName] = event.GetInt()
        for w in win[winName].itervalues():
            self.FindWindowById(w).SetValue(data[winName])
        
        event.Skip()
        
    def _createControl(self, parent, data, name, range, bind = (None, None, None),
                       sliderHor = True, size = 200):
        """!Add control (Slider + SpinCtrl)"""
        data[name] = dict()
        if sliderHor:
            style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                wx.SL_BOTTOM
            sizeW = (size, -1)
        else:
            style = wx.SL_VERTICAL | wx.SL_AUTOTICKS | \
                wx.SL_INVERSE
            sizeW = (-1, size)
        
        slider = wx.Slider(parent = parent, id = wx.ID_ANY,
                           minValue = range[0],
                           maxValue = range[1],
                           style = style,
                           size = sizeW)
        slider.SetName('slider')
        if bind[0]:
            slider.Bind(wx.EVT_SCROLL, bind[0])
        
        # slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, bind[1])
        if bind[1]:
            slider.Bind(wx.EVT_SCROLL_CHANGED, bind[1]) # this only works in MSW
        data[name]['slider'] = slider.GetId()
        
        spin = wx.SpinCtrl(parent = parent, id = wx.ID_ANY, size = (65, -1),
                           min = range[0],
                           max = range[1])
        
        # no 'changed' event ... (FIXME)
        spin.SetName('spin')
        if bind[2]:
            spin.Bind(wx.EVT_SPINCTRL, bind[2])
        
        data[name]['spin'] = spin.GetId()
        
    def __GetWindowName(self, data, id):
        for name in data.iterkeys():
            if type(data[name]) is type({}):
                for win in data[name].itervalues():
                    if win == id:
                        return name
            else:
                if data[name] == id:
                    return name
        
        return None

    def UpdateSettings(self):
        """!Update view from settings values 
        stored in self.mapWindow.view dictionary"""
        for control in ('height',
                        'persp',
                        'twist',
                        'z-exag'):
            for win in self.win['view'][control].itervalues():                
                if control == 'height':
                    value = UserSettings.Get(group = 'nviz', key = 'view',
                                             subkey = ['height', 'value'], internal = True)
                else:
                    try:
                        value = self.mapWindow.view[control]['value']
                    except KeyError:
                        value = -1
                
                self.FindWindowById(win).SetValue(value)
        
        viewWin = self.FindWindowById(self.win['view']['position'])
        x, y = viewWin.UpdatePos(self.mapWindow.view['position']['x'],
                                 self.mapWindow.view['position']['y'])
        viewWin.Draw(pos = (x, y), scale = True)
        viewWin.Refresh(False)
        
        # bgcolor = self.FindWindowById(self.win['settings']['general']['bgcolor']).GetColour()
        # self.OnBgColor(event = bgcolor)
        self.Update()
        
        self.mapWindow.Refresh(eraseBackground = False)
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
    def OnShowLightModel(self, event):
        """!Show light model"""
        self._display.showLight = event.IsChecked()
        self._display.DrawLightingModel()
        
    def OnLightChange(self, event):
        """!Position of the light changed"""
        winName = self.__GetWindowName(self.win['light'], event.GetId())
        if not winName:
            return
        
        val = event.GetInt()
        self.mapWindow.light['position']['z'] = val / 100.
        for win in self.win['light'][winName].itervalues():
            self.FindWindowById(win).SetValue(val)
        
        event = wxUpdateLight()
        wx.PostEvent(self.mapWindow, event)
        
        event.Skip()

    def OnLightColor(self, event):
        """!Color of the light changed"""
        self.mapWindow.light['color'] = event.GetValue()
        
        event = wxUpdateLight()
        wx.PostEvent(self.mapWindow, event)
        
        event.Skip()
        
    def OnLightValue(self, event):
        """!Light brightness changed"""
        data = self.mapWindow.light
        self.OnScroll(event, self.win['light'], data)
        
        event = wxUpdateLight()
        wx.PostEvent(self.mapWindow, event)
        
        event.Skip()
        
    def OnBgColor(self, event):
        """!Background color changed"""
        color = event.GetValue()
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
        
        self._display.SetBgColor(str(color))
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnSetRaster(self, event):
        """!Raster map selected, update surface page"""
        name = event.GetString()
        try:
            layer = self.mapWindow.Map.GetListOfLayers(l_type = 'raster', l_name = name)[0]
        except IndexError:
            return
        
        data = self.mapWindow.GetLayerData(type = 'raster', name = name)['surface']
        self.UpdateSurfacePage(layer, data, updateName = False)

    def OnSetVector(self, event):
        """!Vector map selected, update surface page"""
        name = event.GetString()
        try:
            layer = self.mapWindow.Map.GetListOfLayers(l_type = 'vector', l_name = name)[0]
        except IndexError:
            return
        
        data = self.mapWindow.GetLayerData(type = 'vector', name = name)['vector']
        self.UpdateVectorPage(layer, data, updateName = False)

    def OnSetRaster3D(self, event):
        """!3D Raster map selected, update surface page"""
        name = event.GetString()
        try:
            layer = self.mapWindow.Map.GetListOfLayers(l_type = 'raster3d', l_name = name)[0]
        except IndexError:
            return
        
        data = self.mapWindow.GetLayerData(type = 'raster3d', name = name)
        self.UpdateVolumePage(layer, data, updateName = False)
        
    def OnViewChange(self, event):
        """!Change view, render in quick mode"""
        # find control
        winName = self.__GetWindowName(self.win['view'], event.GetId())
        if not winName:
            return
        
        if winName == 'height':
            view = self.mapWindow.iview # internal
        else:
            view = self.mapWindow.view
        
        if winName == 'z-exag' and event.GetInt() >= 0:
            self.PostViewEvent(zExag = True)
        else:
            self.PostViewEvent(zExag = False)
        
        view[winName]['value'] = event.GetInt()
        
        for win in self.win['view'][winName].itervalues():
            self.FindWindowById(win).SetValue(view[winName]['value'])
                
        self.mapWindow.render['quick'] = True
        self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnViewChanged(self, event):
        """!View changed, render in full resolution"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
        self.UpdateSettings()
        
    def OnViewChangedSpin(self, event):
        """!View changed, render in full resolution"""
        self.mapWindow.render['quick'] = False
        self.OnViewChange(event)
        self.OnViewChanged(None)
        self.Update()
        
        event.Skip()
        
    def OnResetView(self, event):
        """!Reset to default view (view page)"""
        self.mapWindow.ResetView()
        self.UpdateSettings()
        self.mapWindow.Refresh(False)
        
    def OnLookAt(self, event):
        """!Look at (view page)"""
        sel = event.GetSelection()
        if sel == 0: # top
            self.mapWindow.view['position']['x'] = 0.5
            self.mapWindow.view['position']['y'] = 0.5
        elif sel == 1: # north
            self.mapWindow.view['position']['x'] = 0.5
            self.mapWindow.view['position']['y'] = 0.0
        elif sel == 2: # south
            self.mapWindow.view['position']['x'] = 0.5
            self.mapWindow.view['position']['y'] = 1.0
        elif sel == 3: # east
            self.mapWindow.view['position']['x'] = 1.0
            self.mapWindow.view['position']['y'] = 0.5
        elif sel == 4: # west
            self.mapWindow.view['position']['x'] = 0.0
            self.mapWindow.view['position']['y'] = 0.5
        elif sel == 5: # north-west
            self.mapWindow.view['position']['x'] = 0.0
            self.mapWindow.view['position']['y'] = 0.0
        elif sel == 6: # north-east
            self.mapWindow.view['position']['x'] = 1.0
            self.mapWindow.view['position']['y'] = 0.0
        elif sel == 7: # south-east
            self.mapWindow.view['position']['x'] = 1.0
            self.mapWindow.view['position']['y'] = 1.0
        elif sel == 8: # south-west
            self.mapWindow.view['position']['x'] = 0.0
            self.mapWindow.view['position']['y'] = 1.0
        
        self.PostViewEvent(zExag = True)
        
        self.UpdateSettings()
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

    def OnClose(self, event):
        """!Close button pressed
        
        Close dialog
        """
        self.Hide()
        
    def OnMapObjUse(self, event):
        """!Set surface attribute -- use -- map/constant"""
        if not self.mapWindow.init:
            return
        
        wx.Yield()
        
        # find attribute row
        attrb = self.__GetWindowName(self.win['surface'], event.GetId())
        if not attrb:
            attrb = self.__GetWindowName(self.win['volume'], event.GetId())
            nvizType = 'volume'
        else:
            nvizType = 'surface'
        
        selection = event.GetSelection()
        if self.win[nvizType][attrb]['required']: # no 'unset'
            selection += 1
        if selection == 0: # unset
            useMap = None
            value = ''
        elif selection == 1: # map
            useMap = True
            value = self.FindWindowById(self.win[nvizType][attrb]['map']).GetValue()
        elif selection == 2: # constant
            useMap = False
            if attrb == 'color':
                value = self.FindWindowById(self.win[nvizType][attrb]['const']).GetColour()
                value = self._getColorString(value)
            else:
                value = self.FindWindowById(self.win[nvizType][attrb]['const']).GetValue()
        
        self.SetMapObjUseMap(nvizType = nvizType,
                             attrb = attrb, map = useMap)
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        if nvizType == 'surface':
            data[nvizType]['attribute'][attrb] = { 'map' : useMap,
                                                   'value' : str(value),
                                                   'update' : None }
        else: # volume / isosurface
            list = self.FindWindowById(self.win['volume']['isosurfs'])
            id = list.GetSelection()
            data[nvizType]['isosurface'][id][attrb] = { 'map' : useMap,
                                                        'value' : str(value),
                                                        'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def SetMapObjUseMap(self, nvizType, attrb, map = None):
        """!Update dialog widgets when attribute type changed"""
        if attrb in ('topo', 'color', 'shine'):
            incSel = -1 # decrement selection (no 'unset')
        else:
            incSel = 0
        
        if map is True: # map
            if attrb != 'topo': # changing map topography not allowed
                # not sure why, but here must be disabled both ids, should be fixed!
                self.FindWindowById(self.win[nvizType][attrb]['map'] + 1).Enable(True)
            if self.win[nvizType][attrb]['const']:
                self.FindWindowById(self.win[nvizType][attrb]['const']).Enable(False)
            self.FindWindowById(self.win[nvizType][attrb]['use']).SetSelection(1 + incSel)
        elif map is False: # const
            self.FindWindowById(self.win[nvizType][attrb]['map'] + 1).Enable(False)
            if self.win[nvizType][attrb]['const']:
                self.FindWindowById(self.win[nvizType][attrb]['const']).Enable(True)
            self.FindWindowById(self.win[nvizType][attrb]['use']).SetSelection(2 + incSel)
        else: # unset
            self.FindWindowById(self.win[nvizType][attrb]['map'] + 1).Enable(False)
            if self.win[nvizType][attrb]['const']:
                self.FindWindowById(self.win[nvizType][attrb]['const']).Enable(False)
            self.FindWindowById(self.win[nvizType][attrb]['use']).SetSelection(0)
        
    def OnSurfaceMap(self, event):
        """!Set surface attribute"""
        self.SetMapObjAttrb(nvizType = 'surface', winId = event.GetId())
        
    def SetMapObjAttrb(self, nvizType, winId):
        """!Set map object (surface/isosurface) attribute (map/constant)"""
        if not self.mapWindow.init:
            return
        
        attrb = self.__GetWindowName(self.win[nvizType], winId) 
        if not attrb:
            return
        
        if nvizType == 'volume' and attrb == 'topo':
            return
        
        selection = self.FindWindowById(self.win[nvizType][attrb]['use']).GetSelection()
        if self.win[nvizType][attrb]['required']:
            selection += 1
        
        if selection == 0: # unset
            useMap = None
            value = ''
        elif selection == 1: # map
            value = self.FindWindowById(self.win[nvizType][attrb]['map']).GetValue()
            useMap = True
        else: # constant
            if attrb == 'color':
                value = self.FindWindowById(self.win[nvizType][attrb]['const']).GetColour()
                # tuple to string
                value = self._getColorString(value)
            else:
                value = self.FindWindowById(self.win[nvizType][attrb]['const']).GetValue()
            useMap = False
        
        if not self.pageChanging:
            data = self.mapWindow.GetSelectedLayer(type = 'nviz')
            if nvizType == 'surface':
                data[nvizType]['attribute'][attrb] = { 'map' : useMap,
                                                       'value' : str(value),
                                                       'update' : None }
            else: # volume / isosurface
                list = self.FindWindowById(self.win['volume']['isosurfs'])
                id = list.GetSelection()
                data[nvizType]['isosurface'][id][attrb] = { 'map' : useMap,
                                                            'value' : str(value),
                                                            'update' : None }
            
            # update properties
            event = wxUpdateProperties(data = data)
            wx.PostEvent(self.mapWindow, event)
            
            if self.mapDisplay.statusbarWin['render'].IsChecked():
                self.mapWindow.Refresh(False)
        
    def OnSurfaceResolution(self, event):
        """!Draw resolution changed"""
        self.SetSurfaceResolution()
        
        if apply and self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def SetSurfaceResolution(self):
        """!Set draw resolution"""
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        data['surface']['draw']['resolution'] = { 'coarse' : coarse,
                                                  'fine' : fine,
                                                  'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
    def SetSurfaceMode(self):
        """!Set draw mode

        @param apply allow auto-rendering
        """
        mode = self.FindWindowById(self.win['surface']['draw']['mode']).GetSelection()
        if mode == 0: # coarse
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(True)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(False)
        elif mode == 1: # fine
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(False)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(True)
        else: # both
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(True)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(True)
        
        style = self.FindWindowById(self.win['surface']['draw']['style']).GetSelection()
        
        shade = self.FindWindowById(self.win['surface']['draw']['shading']).GetSelection()
        
        value, desc = self.mapWindow.nvizDefault.GetDrawMode(mode, style, shade)
        
        return value, desc

    def OnSurfaceMode(self, event):
        """!Set draw mode"""
        value, desc = self.SetSurfaceMode()
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        data['surface']['draw']['mode'] = { 'value' : value,
                                            'desc' : desc,
                                            'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if apply and self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def OnSurfaceModeAll(self, event):
        """!Set draw mode (including wire color) for all loaded surfaces"""
        value, desc = self.SetSurfaceMode()
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        color = self.FindWindowById(self.win['surface']['draw']['wire-color']).GetColour()
        cvalue = self._getColorString(color)
        
        for name in self.mapWindow.GetLayerNames(type = 'raster'):
            data = self.mapWindow.GetLayerData(type = 'raster', name = name)
            if not data:
                continue # shouldy no happen
            
            data['surface']['draw']['mode'] = { 'value' : value,
                                                'desc' : desc,
                                                'update' : None }
            data['surface']['draw']['resolution'] = { 'coarse' : coarse,
                                                      'fine' : fine,
                                                      'update' : None }
            data['surface']['draw']['wire-color'] = { 'value' : cvalue,
                                                      'update' : None }
            
            # update properties
            event = wxUpdateProperties(data = data)
            wx.PostEvent(self.mapWindow, event)
            
        if apply and self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def _getColorString(self, color):
        """!Set wire color"""
        return str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
    
    def OnSurfaceWireColor(self, event):
        """!Set wire color"""
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        value = self._getColorString(event.GetValue())
        data['surface']['draw']['wire-color'] = { 'value' : value,
                                                  'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnSurfaceAxis(self, event):
        """!Surface position, axis changed"""
        mapLayer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        id = data['surface']['object']['id']
        
        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        slider = self.FindWindowById(self.win['surface']['position']['slider'])
        spin = self.FindWindowById(self.win['surface']['position']['spin'])
        
        x, y, z = self._display.GetSurfacePosition(id)
        
        if axis == 0: # x
            slider.SetValue(x)
            spin.SetValue(x)
        elif axis == 1: # y
            slider.SetValue(y)
            spin.SetValue(y)
        else: # z
            slider.SetValue(z)
            spin.SetValue(z)
        
    def OnSurfacePosition(self, event):
        """!Surface position"""
        winName = self.__GetWindowName(self.win['surface'], event.GetId())
        if not winName:
            return
        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        value = event.GetInt()
        
        for win in self.win['surface']['position'].itervalues():
            if win == self.win['surface']['position']['axis']:
                continue
            else:
                self.FindWindowById(win).SetValue(value)
        
        mapLayer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        id = data['surface']['object']['id']
        x, y, z = self._display.GetSurfacePosition(id)
        
        if axis == 0: # x
            x = value
        elif axis == 1: # y
            y = value
        else: # z
            z = value
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        data['surface']['position']['x'] = x
        data['surface']['position']['y'] = y
        data['surface']['position']['z'] = z
        data['surface']['position']['update'] = None
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        #        self.UpdatePage('surface')

    def UpdateVectorShow(self, vecType, enabled):
        """!Enable/disable lines/points widgets
        
        @param vecType vector type (lines, points)
        """
        if vecType != 'lines' and vecType != 'points':
            return False
        
        for win in self.win['vector'][vecType].keys():
            if win == 'show':
                continue
            if type(self.win['vector'][vecType][win]) == type({}):
                for swin in self.win['vector'][vecType][win].keys():
                    if enabled:
                        self.FindWindowById(self.win['vector'][vecType][win][swin]).Enable(True)
                    else:
                        self.FindWindowById(self.win['vector'][vecType][win][swin]).Enable(False)
            else:
                if enabled:
                    self.FindWindowById(self.win['vector'][vecType][win]).Enable(True)
                else:
                    self.FindWindowById(self.win['vector'][vecType][win]).Enable(False)
        
        return True
    
    def OnVectorShow(self, event):
        """!Show vector lines/points"""
        winId = event.GetId()
        if winId == self.win['vector']['lines']['show']:
            vecType = 'lines'
        else: # points
            vecType = 'points'
        
        checked = event.IsChecked()
        item = self.mapWindow.GetSelectedLayer(type = 'item')
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['vector']
        
        if checked:
            self.mapWindow.LoadVector(item, (vecType,))
        else:
            self.mapWindow.UnloadVector(item, (vecType,))
        
        self.UpdateVectorShow(vecType, checked)
        
        if checked:
            try:
                id = data[vecType]['object']['id']
            except KeyError:
                id = -1
            
            if id > 0:
                self.mapWindow.SetMapObjProperties(item, id, vecType)
                
                # update properties
                event = wxUpdateProperties(data = data)
                wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
    
    def OnVectorDisplay(self, event):
        """!Display vector lines on surface/flat"""
        rasters = self.mapWindow.GetLayerNames('raster')
        if event.GetSelection() == 0: # surface
            if len(rasters) < 1:
                self.FindWindowById(self.win['vector']['lines']['surface']).Enable(False)
                self.FindWindowById(self.win['vector']['lines']['flat']).SetSelection(1)
                return
            
            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(True)
            # set first found surface
            data = self.mapWindow.GetSelectedLayer(type = 'nviz')
            data['vector']['lines']['mode']['surface'] = rasters[0]
            self.FindWindowById(self.win['vector']['lines']['surface']).SetStringSelection( \
                rasters[0])
        else: # flat
            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(False)
        
        self.OnVectorLines(event)
        
        event.Skip()

    def OnVectorLines(self, event):
        """!Set vector lines mode, apply changes if auto-rendering is enabled"""
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        
        width = self.FindWindowById(self.win['vector']['lines']['width']).GetValue()
        
        mode = {}
        if self.FindWindowById(self.win['vector']['lines']['flat']).GetSelection() == 0:
            mode['type'] = 'surface'
            mode['surface'] = self.FindWindowById(self.win['vector']['lines']['surface']).GetValue()
            mode['update'] = None
        else:
            mode['type'] = 'flat'
        
        for attrb in ('width', 'mode'):
            data['vector']['lines'][attrb]['update'] = None
        data['vector']['lines']['width']['value'] = width
        data['vector']['lines']['mode']['value'] = mode
        
        color = self.FindWindowById(self.win['vector']['lines']['color']).GetColour()
        
        if isinstance(color, csel.ColourSelect):
            pass #color picker not yet instantiated
        else:
            color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
            data['vector']['lines']['color']['update'] = None
            data['vector']['lines']['color']['value'] = color
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
                        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVectorHeight(self, event):
        value = event.GetInt()
        id = event.GetId()
        if id == self.win['vector']['lines']['height']['spin'] or \
                id == self.win['vector']['lines']['height']['slider']:
            vtype = 'lines'
        else:
            vtype = 'points'
        
        if type(event) == type(wx.ScrollEvent()):
            # slider
            win = self.FindWindowById(self.win['vector'][vtype]['height']['spin'])
        else:
            # spin
            win = self.FindWindowById(self.win['vector'][vtype]['height']['slider'])
        win.SetValue(value)
        
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['vector'][vtype]
        data['height'] = { 'value' : value,
                           'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        self.mapWindow.render['quick'] = True
        self.mapWindow.render['v' + vtype] = True
        self.mapWindow.Refresh(False)
        
        event.Skip()
    
    def OnVectorHeightFull(self, event):
        """!Vector height changed, render in full resolution"""
        self.OnVectorHeight(event)
        self.OnVectorSurface(event)
        id = event.GetId()
        if id == self.win['vector']['lines']['height']['spin'] or \
                id == self.win['vector']['lines']['height']['slider']:
            vtype = 'lines'
        else:
            vtype = 'points'
        
        self.mapWindow.render['quick'] = False
        self.mapWindow.render['v' + vtype] = False
        self.mapWindow.Refresh(False)

    def OnVectorHeightSpin(self, event):
        """!Vector height changed, render in full resolution"""
        # TODO: use step value instead
        
        #        self.OnVectorHeight(event)
        self.OnVectorHeightFull(event)
        
    def OnVectorSurface(self, event):
        """!Reference surface for vector map (lines/points)"""
        id = event.GetId()
        if id == self.win['vector']['lines']['surface']:
            vtype = 'lines'
        else:
            vtype = 'points'
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        data['vector'][vtype]['mode']['surface'] = { 'value' : event.GetString(),
                                                     'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVectorPoints(self, event):
        """!Set vector points mode, apply changes if auto-rendering is enabled"""
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        
        size  = self.FindWindowById(self.win['vector']['points']['size']).GetValue()
        marker = self.FindWindowById(self.win['vector']['points']['marker']).GetSelection()
        #        width = self.FindWindowById(self.win['vector']['points']['width']).GetValue()
        
        for attrb in ('size', 'marker'):
            data['vector']['points'][attrb]['update'] = None
        data['vector']['points']['size']['value'] = size
        #        data['vector']['points']['width']['value'] = width
        data['vector']['points']['marker']['value'] = marker
        
        color = self.FindWindowById(self.win['vector']['points']['color']).GetColour()
        if isinstance(color, csel.ColourSelect):
            pass #color picker not yet instantiated
        else:
            color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
            data['vector']['points']['color']['update'] = None
            data['vector']['points']['color']['value'] = color
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def UpdateIsosurfButtons(self, list):
        """!Enable/disable buttons 'add', 'delete',
        'move up', 'move down'"""
        nitems = list.GetCount()
        add = self.parent.FindWindowById(self.win['volume']['btnIsosurfAdd'])
        delete = self.parent.FindWindowById(self.win['volume']['btnIsosurfDelete'])
        moveDown = self.parent.FindWindowById(self.win['volume']['btnIsosurfMoveDown'])
        moveUp = self.parent.FindWindowById(self.win['volume']['btnIsosurfMoveUp'])
        if nitems >= wxnviz.MAX_ISOSURFS:
            # disable add button on max
            add.Enable(False)
        else:
            add.Enable(True)
        
        if nitems < 1:
            # disable 'delete' if only one item in the lis
            delete.Enable(False)
        else:
            delete.Enable(True)
        
        if list.GetSelection() >= nitems - 1:
            # disable 'move-down' if last
            moveDown.Enable(False)
        else:
            moveDown.Enable(True)
        
        if list.GetSelection() < 1:
            # disable 'move-up' if first
            moveUp.Enable(False)
        else:
            moveUp.Enable(True)
        
    def OnVolumeIsosurfMode(self, event):
        """!Set isosurface draw mode"""
        self.SetIsosurfaceMode(event.GetSelection())
    
    def SetIsosurfaceMode(self, selection):
        """!Set isosurface draw mode"""
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        mode = 0
        if selection == 0:
            mode |= wxnviz.DM_FLAT
        else:
            mode |= wxnviz.DM_GOURAUD
        
        self._display.SetIsosurfaceMode(id, mode)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVolumeIsosurfResolution(self, event):
        """!Set isosurface draw resolution"""
        self.SetIsosurfaceResolution(event.GetInt())
        
    def SetIsosurfaceResolution(self, res):
        """!Set isosurface draw resolution"""
        layer = self.mapWindow.GetSelectedLayer()
        try:
            data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        except:
            # no volume layer
            return
        
        id = data['object']['id']
        self._display.SetIsosurfaceRes(id, res)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVolumeIsosurfMap(self, event):
        """!Set surface attribute"""
        self.SetMapObjAttrb(nvizType = 'volume', winId = event.GetId())
        
    def OnVolumeIsosurfCheck(self, event):
        """!Isosurface checked (->load) or unchecked (->unload)"""
        index = event.GetSelection()
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        isosurfId = event.GetSelection()
        
        if list.IsChecked(index):
            self._display.SetIsosurfaceTransp(id, isosurfId, False, "0")
        else:
            # disable -> make transparent
            self._display.SetIsosurfaceTransp(id, isosurfId, False, "255")
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVolumeIsosurfSelect(self, event):
        """!Isosurface item selected"""
        winUp = self.FindWindowById(self.win['volume']['btnIsosurfMoveUp'])
        winDown = self.FindWindowById(self.win['volume']['btnIsosurfMoveDown'])
        selection = event.GetSelection()
        if selection == 0:
            winUp.Enable(False)
            if not winDown.IsEnabled():
                winDown.Enable()
        elif selection == self.FindWindowById(event.GetId()).GetCount() - 1:
            winDown.Enable(False)
            if not winUp.IsEnabled():
                winUp.Enable()
        else:
            if not winDown.IsEnabled():
                winDown.Enable()
            if not winUp.IsEnabled():
                winUp.Enable()
        
        # update dialog
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']['isosurface'][selection]
        
        self.UpdateVolumeIsosurfPage(layer, data)
        
    def OnVolumeIsosurfAdd(self, event):
        """!Add new isosurface to the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        level = self.FindWindowById(self.win['volume']['topo']['const']).GetValue()
        
        sel = list.GetSelection()
        if sel < 0 or sel >= list.GetCount() - 1:
            item = list.Append(item = "%s %s" % (_("Level"), str(level)))
        else:
            list.Insert(item = "%s %s" % (_("Level"), str(level)),
                        pos = sel+1) # append
            item = sel + 1
        
        list.Check(item)
        list.SetSelection(item)
        
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        # collect properties
        isosurfData = {}
        for attrb in ('topo', 'color', 'mask',
                      'transp', 'shine', 'emit'):
            if attrb == 'topo':
                isosurfData[attrb] = {}
                win = self.FindWindowById(self.win['volume'][attrb]['const'])
                isosurfData[attrb]['value'] = win.GetValue()
            else:
                uwin = self.FindWindowById(self.win['volume'][attrb]['use'])
                sel = uwin.GetSelection()
                if self.win['volume'][attrb]['required']:
                    sel += 1
                if sel == 0: # unset
                    continue
                
                isosurfData[attrb] = {}
                if sel == 1: # map
                    isosurfData[attrb]['map'] = True
                    vwin = self.FindWindowById(self.win['volume'][attrb]['map'])
                    value = vwin.GetValue()
                else: # const
                    isosurfData[attrb]['map'] = False
                    vwin = self.FindWindowById(self.win['volume'][attrb]['const'])
                    if vwin.GetName() == "color":
                        value = self._getColorString(vwin.GetValue())
                    else:
                        value = vwin.GetValue()
                isosurfData[attrb]['value'] = value
        
        data['isosurface'].insert(item, isosurfData)
        
        # add isosurface        
        self._display.AddIsosurface(id, level)
        # use by default 3d raster map for color
        self._display.SetIsosurfaceColor(id, item, True, str(layer.name))
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeIsosurfDelete(self, event):
        """!Remove isosurface from list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        
        # remove item from list
        isosurfId = list.GetSelection()
        list.Delete(isosurfId)
        # select last item
        if list.GetCount() > 0:
            list.SetSelection(list.GetCount()-1)
        
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        # delete isosurface
        del data['isosurface'][isosurfId]
        
        self._display.DeleteIsosurface(id, isosurfId)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeIsosurfMoveUp(self, event):
        """!Move isosurface up in the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        sel = list.GetSelection()
        
        if sel < 1:
            return # this should not happen
        
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        # move item up
        text = list.GetStringSelection()
        list.Insert(item = text, pos = sel-1)
        list.Check(sel-1)
        list.SetSelection(sel-1)
        list.Delete(sel+1)
        data['isosurface'].insert(sel-1, data['isosurface'][sel])
        del data['isosurface'][sel+1]
        self._display.MoveIsosurface(id, sel, True)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeIsosurfMoveDown(self, event):
        """!Move isosurface dowm in the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        sel = list.GetSelection()
        
        if sel >= list.GetCount() - 1:
            return # this should not happen
        
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')['volume']
        id = data['object']['id']
        
        # move item up
        text = list.GetStringSelection()
        list.Insert(item = text, pos = sel+2)
        list.Check(sel+2)
        list.SetSelection(sel+2)
        list.Delete(sel)
        data['isosurface'].insert(sel+2, data['isosurface'][sel])
        del data['isosurface'][sel]
        self._display.MoveIsosurface(id, sel, False)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def UpdatePage(self, pageId):
        """!Update dialog (selected page)"""
        self.pageChanging = True
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type = 'nviz')
        Debug.msg(1, "NvizToolWindow.UpdatePage(): %s", pageId)
        
        if pageId == 'view':
            self.SetPage('view')
            hmin = self.mapWindow.iview['height']['min']
            hmax = self.mapWindow.iview['height']['max']
            hval = self.mapWindow.iview['height']['value']
            zmin = self.mapWindow.view['z-exag']['min']
            zmax = self.mapWindow.view['z-exag']['max']
            zval = self.mapWindow.view['z-exag']['value']
            
            for control in ('spin', 'slider'):
                self.FindWindowById(self.win['view']['height'][control]).SetRange(hmin,
                                                                                  hmax)
                self.FindWindowById(self.win['view']['height'][control]).SetValue(hval)                                      
                self.FindWindowById(self.win['view']['z-exag'][control]).SetRange(zmin,
                                                                                  zmax)
                self.FindWindowById(self.win['view']['z-exag'][control]).SetValue(zval)                                      
        
        elif pageId in ('surface', 'vector', 'volume'):
            current_page = self.GetSelection()
            if self.GetSelection() != self.page[pageId]['id']:
                self.SetSelection(self.page[pageId]['id'])
            
            if pageId == 'surface':
                self.UpdateSurfacePage(layer, data['surface'])
            elif pageId == 'vector':
                self.UpdateVectorPage(layer, data['vector'])
            elif pageId == 'volume':
                self.UpdateVectorPage(layer, data['vector'])
        elif pageId == 'light':
            zval = self.mapWindow.light['position']['z']
            bval = self.mapWindow.light['bright']
            aval = self.mapWindow.light['ambient']
            for control in ('spin', 'slider'):
                self.FindWindowById(self.win['light']['z'][control]).SetValue(zval)
                self.FindWindowById(self.win['light']['bright'][control]).SetValue(bval)
                self.FindWindowById(self.win['light']['ambient'][control]).SetValue(aval)
        elif pageId == 'fringe':
            mapLayer = self.mapWindow.GetSelectedLayer()
            win = self.FindWindowById(self.win['fringe']['surface'])
            win.SetValue(mapLayer.GetName())
        
        self.Update()
        self.pageChanging = False
        
    def UpdateSurfacePage(self, layer, data, updateName = True):
        """!Update surface page"""
        ret = gcmd.RunCommand('r.info',
                              read = True,
                              flags = 'm',
                              map = layer.name)
        if ret:
            desc = ret.split('=')[1].rstrip('\n')
        else:
            desc = None
        if updateName:
            self.FindWindowById(self.win['surface']['map']).SetValue(layer.name)
        self.FindWindowById(self.win['surface']['desc']).SetLabel(desc)
        
        # attributes
        for attr in ('topo', 'color'): # required
            if layer and layer.type == 'raster':
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue(layer.name)
            else:
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue('')
            self.SetMapObjUseMap(nvizType = 'surface',
                                 attrb = attr, map = True) # -> map
        
        if data['attribute'].has_key('color'):
            value = data['attribute']['color']['value']
            if data['attribute']['color']['map']:
                self.FindWindowById(self.win['surface']['color']['map']).SetValue(value)
            else: # constant
                color = map(int, value.split(':'))
                self.FindWindowById(self.win['surface']['color']['const']).SetColour(color)
            self.SetMapObjUseMap(nvizType = 'surface',
                                 attrb = attr, map = data['attribute']['color']['map'])
        
        self.SetMapObjUseMap(nvizType = 'surface',
                             attrb = 'shine', map = data['attribute']['shine']['map'])
        value = data['attribute']['shine']['value']
        if data['attribute']['shine']['map']:
            self.FindWindowById(self.win['surface']['shine']['map']).SetValue(value)
        else:
            self.FindWindowById(self.win['surface']['shine']['const']).SetValue(value)
        
        #
        # draw
        #
        for control, data in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue
            if control == 'resolution':
                self.FindWindowById(self.win['surface']['draw']['res-coarse']).SetValue(data['coarse'])
                self.FindWindowById(self.win['surface']['draw']['res-fine']).SetValue(data['fine'])
                continue
            
            if control == 'mode':
                if data['desc']['mode'] == 'coarse':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(0)
                elif data['desc']['mode'] == 'fine':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(1)
                else: # both
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(2)
                
                if data['desc']['style'] == 'wire':
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(0)
                else: # surface
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(1)
                
                if data['desc']['shading'] == 'flat':
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(0)
                else: # gouraud
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(1)
                
                continue
            
            value = data['value']
            win = self.FindWindowById(self.win['surface']['draw'][control])
            
            name = win.GetName()
            
            if name == "selection":
                win.SetSelection(value)
            elif name == "colour":
                color = map(int, value.split(':'))
                win.SetColour(color)
            else:
                win.SetValue(value)
        # enable/disable res widget + set draw mode
        self.SetSurfaceMode()
        color = self.FindWindowById(self.win['surface']['draw']['wire-color'])
        
    def UpdateVectorPage(self, layer, data, updateName = True):
        """!Update vector page"""
        vInfo = gcmd.RunCommand('v.info',
                                parent = self,
                                read = True,
                                flags = 't',
                                map = layer.name)
        
        if not vInfo:
            return
        
        npoints = nprimitives = 0
        for line in vInfo.splitlines():
            key, value = line.split('=')
            if key == 'map3d':
                mapIs3D = int(value)
            
            elif key == 'points':
                npoints = int(value)
                nprimitives = npoints
            elif key in ('lines',
                         'boundaries',
                         'centroids',
                         'faces',
                         'kernels'):
                nprimitives += int(value)
        
        if mapIs3D:
            desc = _("Vector map is 3D")
            enable = False
        else:
            desc = _("Vector map is 2D")
            enable = True
        desc += " - " + _("%(primitives)d primitives (%(points)d points)") % \
            { 'primitives' : nprimitives, 'points' : npoints }
        
        if updateName:
            self.FindWindowById(self.win['vector']['map']).SetValue(layer.name)
        self.FindWindowById(self.win['vector']['desc']).SetLabel(desc)
        
        self.FindWindowById(self.win['vector']['lines']['flat']).Enable(enable)
        for v in ('lines', 'points'):
            self.FindWindowById(self.win['vector'][v]['surface']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['slider']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['spin']).Enable(enable)
            
        #
        # lines
        #
        showLines = self.FindWindowById(self.win['vector']['lines']['show'])
        if data['lines'].has_key('object'):
            showLines.SetValue(True)
        else:
            showLines.SetValue(False)
            if nprimitives - npoints > 0:
                showLines.Enable(True)
            else:
                showLines.Enable(False)
        
        self.UpdateVectorShow('lines',
                              showLines.IsChecked())
        
        width = self.FindWindowById(self.win['vector']['lines']['width'])
        width.SetValue(data['lines']['width']['value'])
        
        color = self.FindWindowById(self.win['vector']['lines']['color'])
        color.SetValue(map(int, data['lines']['color']['value'].split(':')))
        
        for vtype in ('lines', 'points'):
            if vtype == 'lines':
                display = self.FindWindowById(self.win['vector']['lines']['flat'])
                if data[vtype]['mode']['type'] == 'flat':
                    display.SetSelection(1)
                else:
                    display.SetSelection(0)
            
            if data[vtype]['mode']['type'] == 'surface':
                rasters = self.mapWindow.GetLayerNames('raster')
                surface = self.FindWindowById(self.win['vector'][vtype]['surface'])
                surface.SetItems(rasters)
                if len(rasters) > 0:
                    try:
                        surface.SetStringSelection(data[vtype]['mode']['surface'])
                    except:
                        pass
        
        for type in ('slider', 'spin'):
            win = self.FindWindowById(self.win['vector']['lines']['height'][type])
            win.SetValue(data['lines']['height']['value'])
        
        #
        # points
        #
        showPoints = self.FindWindowById(self.win['vector']['points']['show'])
        
        if data['points'].has_key('object'):
            showPoints.SetValue(True)
        else:
            showPoints.SetValue(False)
            if npoints > 0:
                showPoints.Enable(True)
            else:
                showPoints.Enable(False)
        
        self.UpdateVectorShow('points',
                              showPoints.IsChecked())
        # size, width, marker, color
        for prop in ('size', 'marker', 'color'):
            win = self.FindWindowById(self.win['vector']['points'][prop])
            name = win.GetName()
            if name == 'selection':
                win.SetSelection(data['points'][prop]['value'])
            elif name == 'color':
                color = map(int, data['points'][prop]['value'].split(':'))
                win.SetValue(color)
            else:
                win.SetValue(data['points'][prop]['value'])
        # height
        for type in ('slider', 'spin'):
            win = self.FindWindowById(self.win['vector']['points']['height'][type])
            win.SetValue(data['points']['height']['value'])
        
    def UpdateVolumePage(self, layer, data, updateName = True):
        """!Update volume page"""
        if updateName:
            self.FindWindowById(self.win['volume']['map']).SetValue(layer.name)
        # self.FindWindowById(self.win['volume']['desc']).SetLabel(desc)
        
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        
        #
        # draw
        #
        for control, data in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue
            
            win = self.FindWindowById(self.win['volume']['draw'][control])
            
            if control == 'shading':
                if data['draw']['shading']['desc'] == 'flat':
                    value = 0
                else:
                    value = 1
            else:
                value = data['value']
            
            if win.GetName() == "selection":
                win.SetSelection(value)
            else:
                win.SetValue(value)
        
        self.SetIsosurfaceMode(data['draw']['shading']['value'])
        self.SetIsosurfaceResolution(data['draw']['resolution']['value'])
            
        self.UpdateVolumeIsosurfPage(layer, data['attribute'])
        
    def UpdateVolumeIsosurfPage(self, layer, data):
        """!Update dialog -- isosurface attributes"""
        #
        # isosurface attributes
        #
        for attrb in ('topo', 'color', 'mask',
                     'transp', 'shine', 'emit'):
            # check required first
            if attrb == 'topo':
                self.FindWindowById(self.win['volume'][attrb]['const']).SetValue(0)
                continue
            if attrb == 'color':
                if layer and layer.type == '3d-raster':
                    self.FindWindowById(self.win['volume'][attrb]['map']).SetValue(layer.name)
                else:
                    self.FindWindowById(self.win['volume'][attrb]['map']).SetValue('')
                self.SetMapObjUseMap(nvizType = 'volume',
                                     attrb = attrb, map = True) # -> map
                continue
            
            # skip empty attributes
            if not data.has_key(attrb):
                continue
            
            value = data[attrb]['value']
            if attrb == 'color':
                if data[attrb]['map']:
                    self.FindWindowById(self.win['volume'][attrb]['map']).SetValue(value)
                else: # constant
                    color = map(int, value.split(':'))
                    self.FindWindowById(self.win['volume'][attrb]['const']).SetColour(color)
            else:
                if data[attrb]['map']:
                    win = self.FindWindowById(self.win['volume'][attrb]['map'])
                else:
                    win = self.FindWindowById(self.win['volume'][attrb]['const'])
                win.SetValue(value)
            
            self.SetMapObjUseMap(nvizType = 'volume',
                                 attrb = attrb, map = data[attrb]['map'])
        
    def SetPage(self, name):
        """!Get named page"""
        self.SetSelection(self.page[name]['id'])

class PositionWindow(wx.Window):
    """!Abstract position control window, see subclasses
    ViewPostionWindow and LightPositionWindow"""
    def __init__(self, parent, mapwindow, id = wx.ID_ANY,
                 **kwargs):
        self.mapWindow = mapwindow
        self.quick = True
        
        wx.Window.__init__(self, parent, id, **kwargs)
        
        self.SetBackgroundColour("WHITE")
        
        self.pdc = wx.PseudoDC()
        
        self.pdc.SetBrush(wx.Brush(colour = 'dark green', style = wx.SOLID))
        self.pdc.SetPen(wx.Pen(colour = 'dark green', width = 2, style = wx.SOLID))

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        # self.Bind(wx.EVT_MOTION,       self.OnMouse)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)
        
    def Draw(self, pos, scale = False):
        w, h = self.GetClientSize()
        x, y = pos
        if scale:
            x = x * w
            y = y * h
        self.pdc.Clear()
        self.pdc.BeginDrawing()
        self.pdc.DrawLine(w / 2, h / 2, x, y)
        self.pdc.DrawCircle(x, y, 5)
        self.pdc.EndDrawing()
        
    def OnPaint(self, event):
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush("White"))
        dc.Clear()
        
        self.PrepareDC(dc)
        self.pdc.DrawToDC(dc)
        
    def UpdatePos(self, xcoord, ycoord):
        """!Update position coordinates (origin: UL)"""
        if xcoord < 0.0:
            xcoord = 0.0
        elif xcoord > 1.0:
            xcoord = 1.0
        if ycoord < 0.0:
            ycoord = 0.0
        elif ycoord > 1.0:
            ycoord = 1.0

        self.data['position']['x'] = xcoord        
        self.data['position']['y'] = ycoord
        
        return xcoord, ycoord
    
    def OnMouse(self, event):
        if event.LeftIsDown():
            x, y = event.GetPosition()
            self.Draw(pos = (x, y))
            w, h = self.GetClientSize()
            x = float(x) / w
            y = float(y) / h
            self.UpdatePos(x, y)
            self.Refresh(False)
        
        event.Skip()
        
    def PostDraw(self):
        x, y = self.UpdatePos(self.data['position']['x'],
                              self.data['position']['y'])
        self.Draw(pos = (x, y), scale = True)

class ViewPositionWindow(PositionWindow):
    """!View position control widget"""
    def __init__(self, parent, mapwindow, id = wx.ID_ANY,
                 **kwargs):
        PositionWindow.__init__(self, parent, mapwindow, id, **kwargs)

        self.data = self.mapWindow.view
        self.PostDraw()
        
    def UpdatePos(self, xcoord, ycoord):
        x, y = PositionWindow.UpdatePos(self, xcoord, ycoord)
        
        event = wxUpdateView(zExag = True)
        wx.PostEvent(self.mapWindow, event)

        return x, y

    def OnMouse(self, event):
        PositionWindow.OnMouse(self, event)
        if event.LeftIsDown():
            self.mapWindow.render['quick'] = self.quick
            self.mapWindow.Refresh(eraseBackground = False)
        elif event.LeftUp():
            self.mapWindow.render['quick'] = False
            self.mapWindow.Refresh(eraseBackground = False)
        
        event.Skip()
    
class LightPositionWindow(PositionWindow):
    """!Light position control widget"""
    def __init__(self, parent, mapwindow, id = wx.ID_ANY,
                 **kwargs):
        PositionWindow.__init__(self, parent, mapwindow, id, **kwargs)

        self.data = self.mapWindow.light
        self.quick = False
        self.PostDraw()

    def UpdatePos(self, xcoord, ycoord):
        x, y = PositionWindow.UpdatePos(self, xcoord, ycoord)
        
        event = wxUpdateLight()
        wx.PostEvent(self.mapWindow, event)
    
        return x, y

    def OnMouse(self, event):
        PositionWindow.OnMouse(self, event)
        if event.LeftUp():
            self.mapWindow.render['quick'] = False
            self.mapWindow.Refresh(eraseBackground = False)
