"""
@package nviz_tools.py

@brief Nviz tools window

Classes:
 - NvizToolWindow
 - ViewPositionWindow

(C) 2008-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
@enhancements by Michael Barton <michael.barton@asu.edu>
"""

import os
import sys
import copy

import wx
import wx.lib.colourselect as csel

import globalvar
import gselect
import gcmd
from preferences import globalSettings as UserSettings
from nviz_mapdisp import wxUpdateView as wxUpdateView
from nviz_mapdisp import wxUpdateProperties as wxUpdateProperties

sys.path.append(os.path.join(globalvar.ETCWXDIR, "nviz"))
import grass7_wxnviz as wxnviz

class NvizToolWindow(wx.Frame):
    """!Experimental window for Nviz tools

    @todo integrate with Map display
    """
    def __init__(self, parent=None, id=wx.ID_ANY, title=_("3d view tools"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 mapWindow=None, 
                 style=wx.CAPTION|wx.MINIMIZE_BOX|wx.RESIZE_BORDER):
        
        self.parent = parent # MapFrame
        self.lmgr = self.parent.GetLayerManager() # GMFrame
        self.mapWindow = mapWindow

        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        #
        # icon
        #
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_nviz.ico'), wx.BITMAP_TYPE_ICO))

        #
        # dialog body
        #
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.win = {} # window ids

        #
        # notebook
        #
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)

        self.page = {}
        self.win['settings'] = {}
        self.win['settings']['general'] = {}

        # view page
        self.viewPos = ""
        self.__createViewPage()
        self.page['view'] = { 'id' : 0 }
        # surface page
        size = self.__createSurfacePage()
        size = (size[0] + 25, size[0] + 20)
        # vector page
        self.__createVectorPage()
        # volume page
        self.__createVolumePage()
        # settings page
#        self.__createSettingsPage()
#        self.page['settings'] = { 'id' : 1 }

        self.UpdateSettings()
        self.pageChanging = False
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

        mainSizer.Add(item=self.notebook, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
                      

        #
        # bindings
        #
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        
        #
        # layout
        #
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

#        self.SetSize(size) #this produces a size that is consistently too small for the controls
        self.Layout()
        self.notebook.Update()
        self.Update()
        self.SetPage('view')
        self.CentreOnScreen()
        
    def OnPageChanged(self, event):
        new = event.GetSelection()
        self.notebook.ChangeSelection(new)
    
    def PostViewEvent(self, zExag=False):
        """!Change view settings"""
        event = wxUpdateView(zExag=zExag)
        wx.PostEvent(self.mapWindow, event)

    def __createViewPage(self):
        """!Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.notebook.InsertPage(0, page=panel,
                              text=" %s " % _("View"),
                              select=True)
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Control View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        self.win['view'] = {}

        # position
        posSizer = wx.GridBagSizer(vgap=3, hgap=3)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("W")),
                     pos=(1, 0), flag=wx.ALIGN_CENTER)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("N")),
                     pos=(0, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_BOTTOM)
        self.viewPos = ViewPositionWindow(panel, id=wx.ID_ANY, size=(175, 175),
                                     mapwindow=self.mapWindow, win=self.win)
        self.win['view']['pos'] = self.viewPos.GetId()
        posSizer.Add(item=self.viewPos,
                     pos=(1, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("S")),
                     pos=(2, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_TOP)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("E")),
                     pos=(1, 2), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=posSizer, pos=(0, 0))
                  
        # perspective
        # set initial defaults here (or perhaps in a default values file), not in user settings
#        range = UserSettings.Get(group='nviz', key='view', subkey='persp', internal=True)
        self.CreateControl(panel, dict=self.win['view'], name='persp',
                           range=(1,100),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Perspective:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=self.FindWindowById(self.win['view']['persp']['slider']), pos=(2, 0))
        gridSizer.Add(item=self.FindWindowById(self.win['view']['persp']['spin']), pos=(3, 0),
                      flag=wx.ALIGN_CENTER)        

        # twist
#        range = UserSettings.Get(group='nviz', key='view', subkey='twist', internal=True)
        self.CreateControl(panel, dict=self.win['view'], name='twist',
                           range=(-180,180),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Twist:")),
                      pos=(1, 1), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=self.FindWindowById(self.win['view']['twist']['slider']), pos=(2, 1))
        gridSizer.Add(item=self.FindWindowById(self.win['view']['twist']['spin']), pos=(3, 1),
                      flag=wx.ALIGN_CENTER)        

        # height + z-exag
        self.CreateControl(panel, dict=self.win['view'], name='height', sliderHor=False,
                           range=(0, 1),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        
        self.CreateControl(panel, dict=self.win['view'], name='z-exag', sliderHor=False,
                           range=(0, 5),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        self.FindWindowById(self.win['view']['z-exag']['slider']).SetValue(1)
        self.FindWindowById(self.win['view']['z-exag']['spin']).SetValue(1)
        
        heightSizer = wx.GridBagSizer(vgap=3, hgap=3)
        heightSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Height:")),
                      pos=(0, 0), flag=wx.ALIGN_LEFT, span=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['height']['slider']),
                        flag=wx.ALIGN_RIGHT, pos=(1, 0))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['height']['spin']),
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos=(1, 1))
        heightSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Z-exag:")),
                      pos=(0, 2), flag=wx.ALIGN_LEFT, span=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['z-exag']['slider']),
                        flag=wx.ALIGN_RIGHT, pos=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['z-exag']['spin']),
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos=(1, 3))

        gridSizer.Add(item=heightSizer, pos=(0, 1), flag=wx.ALIGN_RIGHT)

        # view setup + reset
        viewSizer = wx.BoxSizer(wx.HORIZONTAL)

        viewSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY,
                                         label=_("Look at:")),
                      flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=5)
        
        viewType = wx.Choice (parent=panel, id=wx.ID_ANY, size=(125, -1),
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
        viewSizer.Add(item=viewType, flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL,
                      border=5)

        reset = wx.Button(panel, id=wx.ID_ANY, label=_("Reset"))
        reset.SetToolTipString(_("Reset to default view"))
        # self.win['reset'] = reset.GetId()
        reset.Bind(wx.EVT_BUTTON, self.OnResetView)

        viewSizer.Add(item=reset, proportion=1,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT,
                      border=5)

        gridSizer.AddGrowableCol(3)
        gridSizer.Add(item=viewSizer, pos=(4, 0), span=(1, 2),
                      flag=wx.EXPAND)

        # body
#        pageSizer.Add(item=gridSizer, proportion=1,
#                      flag=wx.EXPAND | wx.ALL,
#                      border=5)
        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=1)

        # background color
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Image Appearance")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=1, hgap=3)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                      label=_("Background color:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        color = csel.ColourSelect(panel, id=wx.ID_ANY,
                      colour=(255,255,255))
        self.win['settings']['general']['bgcolor'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnBgColor)
        
        gridSizer.Add(item=color, pos=(0, 1))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=1)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=1)

        #
        # buttons
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Saved NVIZ Settings")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        btnLoad = wx.Button(panel, wx.ID_CANCEL, label=_("Load"))
        btnDefault = wx.Button(panel, wx.ID_CANCEL, label=_("Default"))
        btnSave = wx.Button(panel, wx.ID_SAVE)

        btnLoad.Bind(wx.EVT_BUTTON, self.OnLoad)
        btnLoad.SetToolTipString(_("Load and apply settings from user settings file"))
        btnDefault.Bind(wx.EVT_BUTTON, self.OnDefault)
        btnDefault.SetToolTipString(_("Restore default settings"))
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))

        btnSizer = wx.GridSizer(1, 5, vgap=1, hgap=3)
        btnSizer.Add(item=btnDefault, flag=wx.ALIGN_LEFT)
        btnSizer.Add((-1, -1), flag=wx.ALIGN_CENTRE)
        btnSizer.Add(item=btnLoad, flag=wx.ALIGN_RIGHT)
        btnSizer.Add(item=btnSave, flag=wx.ALIGN_RIGHT)

        btnSave.SetDefault()

        boxSizer.Add(item=btnSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=1)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=1)
                    
        panel.SetSizer(pageSizer)
        return panel.GetBestSize()

    def __createSurfacePage(self):
        """!Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.page['surface'] = {}
        self.page['surface']['id'] = -1
        self.page['surface']['panel'] = panel.GetId()

        # panel = scrolled.ScrolledPanel(parent=self.notebook, id=wx.ID_ANY)
        # panel.SetupScrolling(scroll_x=True, scroll_y=True)

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        self.win['surface'] = {}
        #
        # surface attributes
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Surface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

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
            gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                             label=attrb + ':'),
                          pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
            use = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                             choices = [_("map")])
            if code not in ('topo', 'color', 'shine'):
                use.Insert(item=_("unset"), pos=0)
                self.win['surface'][code]['required'] = False
            else:
                self.win['surface'][code]['required'] = True
            if code != 'mask':
                use.Append(item=_('constant'))
            self.win['surface'][code]['use'] = use.GetId()
            use.Bind(wx.EVT_CHOICE, self.OnMapObjUse)
            gridSizer.Add(item=use, flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, 1))

            map = gselect.Select(parent=panel, id=wx.ID_ANY,
                                 # size=globalvar.DIALOG_GSELECT_SIZE,
                                 size=(200, -1),
                                 type="raster")
            self.win['surface'][code]['map'] = map.GetId() - 1 # FIXME
            map.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            # changing map topography not allowed
            if code == 'topo':
                map.Enable(False)
            gridSizer.Add(item=map, flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, 2))

            if code == 'color':
                value = csel.ColourSelect(panel, id=wx.ID_ANY,
                                          colour=(0,0,0))
                value.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceMap)
            elif code == 'mask':
                value = None
            else:
                value = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                                    initial=0)
                if code == 'topo':
                    value.SetRange(minVal=-1e9, maxVal=1e9)
                elif code in ('shine', 'transp', 'emit'):
                    value.SetRange(minVal=0, maxVal=255)
                else:
                    value.SetRange(minVal=0, maxVal=100)
                value.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            
            if value:
                self.win['surface'][code]['const'] = value.GetId()
                value.Enable(False)
                gridSizer.Add(item=value, flag=wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 3))
            else:
                self.win['surface'][code]['const'] = None

            self.SetMapObjUseMap(nvizType='surface',
                                 attrb=code) # -> enable map / disable constant
                
            row += 1

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        #
        # draw
        #
        self.win['surface']['draw'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.AddGrowableCol(6)

        # mode
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Mode:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent=panel, id=wx.ID_ANY, size=(-1, -1),
                          choices = [_("coarse"),
                                     _("fine"),
                                     _("both")])
        mode.SetSelection(0)
        mode.SetName("selection")
        mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['surface']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item=mode, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 1))

        # shading
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Shading:")),
                      pos=(0, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        shade = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['surface']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item=shade, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 3))

        # set to all
        all = wx.Button(panel, id=wx.ID_ANY, label=_("Set to all"))
        all.SetToolTipString(_("Use draw settings for all loaded surfaces"))
        all.Bind(wx.EVT_BUTTON, self.OnSurfaceModeAll)
        gridSizer.Add(item=all, flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(0, 4), span=(1,2), border = 3 )

        # resolution coarse
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Coarse:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        resSizer = wx.BoxSizer(wx.HORIZONTAL)
        resSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                        label=_("res.")),
                     flag=wx.ALL | wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL, 
                     border=3)
        resC = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=6,
                           min=1,
                           max=100)
        resC.SetName("value")
        resC.SetValue(6)
        
        self.win['surface']['draw']['res-coarse'] = resC.GetId()
        resC.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item=resC, flag=wx.ALL | wx.ALIGN_LEFT | 
                      wx.ALIGN_CENTER_VERTICAL, border=3)
        gridSizer.Add(item=resSizer, pos=(1, 1), flag=wx.ALIGN_CENTER_VERTICAL)

        # Coarse style
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("style")),
                      pos=(1, 2), flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
        style = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices = [_("wire"),
                                     _("surface")])
        style.SetName("selection")
        self.win['surface']['draw']['style'] = style.GetId()
        style.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item=style, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 3))

        # color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("wire color")),
                      pos=(1, 4), flag=wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_RIGHT | wx.LEFT, border=3)
        color = csel.ColourSelect(panel, id=wx.ID_ANY)
        color.SetColour((136,136,136))
        color.SetName("colour")
        color.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceWireColor)
        self.win['surface']['draw']['wire-color'] = color.GetId()
        gridSizer.Add(item=color, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos=(1, 5))

        # resolution fine
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Fine:")),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        
        resSizer = wx.BoxSizer(wx.HORIZONTAL)
        resSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                        label=_("res.")),
                     flag=wx.ALL | wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL, 
                     border=3)
        resF = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=3,
                           min=1,
                           max=100)
        resF.SetName("value")
        resF.SetValue(3)
        self.win['surface']['draw']['res-fine'] = resF.GetId()
        resF.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item=resF, flag=wx.ALL | wx.ALIGN_LEFT | 
                      wx.ALIGN_CENTER_VERTICAL, border=3)
        gridSizer.Add(item=resSizer, pos=(2, 1), flag=wx.ALIGN_CENTER_VERTICAL)


        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)


        #
        # mask
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Mask")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Mask zeros:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        elev = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                           label=_("by elevation"))
        elev.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item=elev, pos=(0, 1))

        color = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                           label=_("by color"))
        color.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item=color, pos=(0, 2))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # position
        #
        self.win['surface']['position'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # position
        self.CreateControl(panel, dict=self.win['surface'], name='position',
                           range=(-100, 100),
                           bind=(self.OnSurfacePosition, self.OnSurfacePosition, self.OnSurfacePosition))


        axis = wx.Choice (parent=panel, id=wx.ID_ANY, size=(75, -1),
                          choices = ["X",
                                     "Y",
                                     "Z"])

        self.win['surface']['position']['axis'] = axis.GetId()
        axis.SetSelection(0)
        axis.Bind(wx.EVT_CHOICE, self.OnSurfaceAxis)

        pslide = self.FindWindowById(self.win['surface']['position']['slider'])
        pspin = self.FindWindowById(self.win['surface']['position']['spin'])
        
        gridSizer.Add(item=axis, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        gridSizer.Add(item=pslide, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 1))
        gridSizer.Add(item=pspin, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 2))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        box.SetSizer(boxSizer)
        box.Layout()
        
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)
        
        panel.SetSizer(pageSizer)
        panel.Layout()
        return panel.GetBestSize()

    def __createVectorPage(self):
        """!Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.page['vector'] = {}
        self.page['vector']['id'] = -1
        self.page['vector']['panel'] = panel.GetId()

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        self.win['vector'] = {}

        #
        # desc
        #
        desc = wx.StaticText(parent=panel, id=wx.ID_ANY,
                             label="")
        self.win['vector']['desc'] = desc.GetId()
        pageSizer.Add(item=desc, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=10)

        #
        # vector lines
        #
        self.win['vector']['lines'] = {}

        showLines = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Show vector lines"))
        showLines.SetValue(True)
        
        self.win['vector']['lines']['show'] = showLines.GetId()
        showLines.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)

        pageSizer.Add(item=showLines, proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # width
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Line:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("width")),
                      pos=(0, 1), flag=wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_RIGHT)

        width = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=1,
                            min=1,
                            max=100)
        width.SetValue(1)
        self.win['vector']['lines']['width'] = width.GetId()
        width.Bind(wx.EVT_SPINCTRL, self.OnVectorLines)
        gridSizer.Add(item=width, pos=(0, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)

#        gridSizer.AddGrowableCol(2)

        # color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("color")),
                      pos=(0, 3), flag=wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)

        color = csel.ColourSelect(panel, id=wx.ID_ANY,
                                  colour=(0,0,0))
        self.win['vector']['lines']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnVectorLines)

        gridSizer.Add(item=color, pos=(0, 4), flag=wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_LEFT)

#        gridSizer.AddGrowableCol(5)

        # display
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("display")),
                      pos=(1, 1), flag=wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)

        display = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                             choices = [_("on surface"),
                                        _("flat")])
        self.win['vector']['lines']['flat'] = display.GetId()
        display.Bind(wx.EVT_CHOICE, self.OnVectorDisplay)

        gridSizer.Add(item=display, flag=wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_LEFT, pos=(1, 2), span=(1,2))

        # height
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height above surface:")),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL,
                      span=(1, 3))
        
        surface = wx.ComboBox(parent=panel, id=wx.ID_ANY, size=(250, -1),
                              style=wx.CB_SIMPLE | wx.CB_READONLY,
                              choices=[])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['lines']['surface'] = surface.GetId()
        gridSizer.Add(item=surface, 
                      pos=(2, 3), span=(1, 6),
                      flag=wx.ALIGN_CENTER_VERTICAL)


        self.CreateControl(panel, dict=self.win['vector']['lines'], name='height', size=300,
                           range=(0, 1000),
                           bind=(self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))
        self.FindWindowById(self.win['vector']['lines']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['lines']['height']['spin']).SetValue(0)
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['lines']['height']['slider']),
                      pos=(3, 0), span=(1, 7))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['lines']['height']['spin']),
                      pos=(3, 7),
                      flag=wx.ALIGN_CENTER)

        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)


        #
        # vector points
        #
        self.win['vector']['points'] = {}

        showPoints = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                 label=_("Show vector points"))
        showPoints.SetValue(True)
        self.win['vector']['points']['show'] = showPoints.GetId()
        showPoints.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)

        pageSizer.Add(item=showPoints, proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # icon size
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Icon:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("size")),
                      pos=(0, 1), flag=wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)

        isize = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=1,
                            min=1,
                            max=1e6)
        isize.SetName('value')
        isize.SetValue(100)
        self.win['vector']['points']['size'] = isize.GetId()
        isize.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        isize.Bind(wx.EVT_TEXT, self.OnVectorPoints)
        gridSizer.Add(item=isize, pos=(0, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)

        # icon color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("color")),
                      pos=(0, 3), flag=wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        icolor = csel.ColourSelect(panel, id=wx.ID_ANY)
        icolor.SetName("color")
        icolor.SetColour((0,0,255))
        self.win['vector']['points']['color'] = icolor.GetId()
        icolor.Bind(csel.EVT_COLOURSELECT, self.OnVectorPoints)
        gridSizer.Add(item=icolor, flag=wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_LEFT,
                      pos=(0, 4))

        # icon width (does this do anything???)
#        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
#                                         label=_("width")),
#                      pos=(0, 5), flag=wx.ALIGN_CENTER_VERTICAL |
#                      wx.ALIGN_RIGHT)

#        iwidth = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
#                             initial=1,
#                             min=1,
#                             max=1e6)
#        iwidth.SetName('value')
#        iwidth.SetValue(100)
#        self.win['vector']['points']['width'] = iwidth.GetId()
#        iwidth.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
#        iwidth.Bind(wx.EVT_TEXT, self.OnVectorPoints)
#        gridSizer.Add(item=iwidth, pos=(0, 6),
#                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)


        # icon symbol
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("symbol")),
                      pos=(1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices=UserSettings.Get(group='nviz', key='vector',
                                                   subkey=['points', 'marker'], internal=True))
        isym.SetName("selection")
        self.win['vector']['points']['marker'] = isym.GetId()
        isym.Bind(wx.EVT_CHOICE, self.OnVectorPoints)
        gridSizer.Add(item=isym, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 2), span=(1,2))

        # high
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height above surface:")),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL,
                      span=(1, 3))
        
        surface = wx.ComboBox(parent=panel, id=wx.ID_ANY, size=(250, -1),
                              style=wx.CB_SIMPLE | wx.CB_READONLY,
                              choices=[])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['points']['surface'] = surface.GetId()
        gridSizer.Add(item=surface, 
                      pos=(2, 3), span=(1, 5),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        self.CreateControl(panel, dict=self.win['vector']['points'], name='height', size=300,
                           range=(0, 1000),
                           bind=(self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))

        self.FindWindowById(self.win['vector']['points']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['points']['height']['spin']).SetValue(0)

        gridSizer.Add(item=self.FindWindowById(self.win['vector']['points']['height']['slider']),
                      pos=(3, 0), span=(1, 7))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['points']['height']['spin']),
                      pos=(3, 7),
                      flag=wx.ALIGN_CENTER)

        
        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)


        panel.SetSizer(pageSizer)
        panel.Layout()
        return panel.GetBestSize()

    def __createVolumePage(self):
        """!Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.page['volume'] = {}
        self.page['volume']['id'] = -1
        self.page['volume']['panel'] = panel.GetId()
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['volume'] = {}

        #
        # draw
        #
        self.win['volume']['draw'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.AddGrowableCol(4)

        # mode
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Mode:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent=panel, id=wx.ID_ANY, size=(150, -1),
                          choices = [_("isosurfaces"),
                                     _("slides")])
        mode.SetSelection(0)
        mode.SetName("selection")
        # mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['volume']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item=mode, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 1))

        # shading
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Shading:")),
                      pos=(0, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        shade = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['volume']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnVolumeIsosurfMode)
        gridSizer.Add(item=shade, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 3))

        # resolution (mode)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Resolution:")),
                      pos=(0, 4), flag=wx.ALIGN_CENTER_VERTICAL)
        resol = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=1,
                            min=1,
                            max=100)
        resol.SetName("value")
        self.win['volume']['draw']['resolution'] = resol.GetId()
        resol.Bind(wx.EVT_SPINCTRL, self.OnVolumeIsosurfResolution)
        resol.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfResolution)
        gridSizer.Add(item=resol, pos=(0, 5))
        
        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        #
        # manage isosurfaces
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("List of isosurfaces")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # list
        isolevel = wx.CheckListBox(parent=panel, id=wx.ID_ANY,
                                   size=(300, 150))
        self.Bind(wx.EVT_CHECKLISTBOX, self.OnVolumeIsosurfCheck, isolevel)
        self.Bind(wx.EVT_LISTBOX, self.OnVolumeIsosurfSelect, isolevel)

        self.win['volume']['isosurfs'] = isolevel.GetId()
        gridSizer.Add(item=isolevel, pos=(0, 0), span=(4, 1))
        
        # buttons (add, delete, move up, move down)
        btnAdd = wx.Button(parent=panel, id=wx.ID_ADD)
        self.win['volume']['btnIsosurfAdd'] = btnAdd.GetId()
        btnAdd.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfAdd)
        gridSizer.Add(item=btnAdd,
                      pos=(0, 1))
        btnDelete = wx.Button(parent=panel, id=wx.ID_DELETE)
        self.win['volume']['btnIsosurfDelete'] = btnDelete.GetId()
        btnDelete.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfDelete)
        btnDelete.Enable(False)
        gridSizer.Add(item=btnDelete,
                      pos=(1, 1))
        btnMoveUp = wx.Button(parent=panel, id=wx.ID_UP)
        self.win['volume']['btnIsosurfMoveUp'] = btnMoveUp.GetId()
        btnMoveUp.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfMoveUp)
        btnMoveUp.Enable(False)
        gridSizer.Add(item=btnMoveUp,
                      pos=(2, 1))
        btnMoveDown = wx.Button(parent=panel, id=wx.ID_DOWN)
        self.win['volume']['btnIsosurfMoveDown'] = btnMoveDown.GetId()
        btnMoveDown.Bind(wx.EVT_BUTTON, self.OnVolumeIsosurfMoveDown)
        btnMoveDown.Enable(False)
        gridSizer.Add(item=btnMoveDown,
                      pos=(3, 1))

        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)
        
        #
        # isosurface attributes
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Isosurface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

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
            gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                             label=attrb + ':'),
                          pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
            if code != 'topo':
                use = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                                 choices = [_("map")])
            else:
                use = None
            # check for required properties
            if code not in ('topo', 'color', 'shine'):
                use.Insert(item=_("unset"), pos=0)
                self.win['volume'][code]['required'] = False
            else:
                self.win['volume'][code]['required'] = True
            if use and code != 'mask':
                use.Append(item=_('constant'))
            if use:
                self.win['volume'][code]['use'] = use.GetId()
                use.Bind(wx.EVT_CHOICE, self.OnMapObjUse)
                gridSizer.Add(item=use, flag=wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 1))
            
            if code != 'topo':
                map = gselect.Select(parent=panel, id=wx.ID_ANY,
                                     # size=globalvar.DIALOG_GSELECT_SIZE,
                                     size=(200, -1),
                                     type="grid3")
                self.win['volume'][code]['map'] = map.GetId() - 1 # FIXME
                map.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfMap)
                gridSizer.Add(item=map, flag=wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 2))
            else:
                map = None
            
            if code == 'color':
                value = csel.ColourSelect(panel, id=wx.ID_ANY,
                                          colour=(0,0,0))
                value.SetColour((0,0,0))
                value.Bind(csel.EVT_COLOURSELECT, self.OnVolumeIsosurfMap)
            elif code == 'mask':
                value = None
            else:
                if code == 'topo':
                    size = (200, -1)
                else:
                    size = (65, -1)
                value = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=size,
                                    initial=0)
                if code == 'topo':
                    value.SetRange(minVal=-1e9, maxVal=1e9)
                elif code in ('shine', 'transp', 'emit'):
                    value.SetRange(minVal=0, maxVal=255)
                else:
                    value.SetRange(minVal=0, maxVal=100)
                value.Bind(wx.EVT_SPINCTRL, self.OnVolumeIsosurfMap)
                value.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfMap)
            
            if value:
                self.win['volume'][code]['const'] = value.GetId()
                if code == 'topo':
                    gridSizer.Add(item=value, flag=wx.ALIGN_CENTER_VERTICAL,
                                  pos=(row, 2))
                else:
                    value.Enable(False)
                    gridSizer.Add(item=value, flag=wx.ALIGN_CENTER_VERTICAL,
                                  pos=(row, 3))
            else:
                self.win['volume'][code]['const'] = None
            
            if code != 'topo':
                self.SetMapObjUseMap(nvizType='volume',
                                     attrb=code) # -> enable map / disable constant
                
            row += 1
        
        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)
        
        panel.SetSizer(pageSizer)
        panel.Layout()
        return panel.GetBestSize()

    def CreateControl(self, parent, dict, name, range, bind, sliderHor=True, size=200):
        """!Add control (Slider + SpinCtrl)"""
        dict[name] = {}
        if sliderHor:
            style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                wx.SL_BOTTOM
            sizeW = (size, -1)
        else:
            style = wx.SL_VERTICAL | wx.SL_AUTOTICKS | \
                wx.SL_INVERSE
            sizeW = (-1, size)
        try:
            val = self.mapWindow.view[name]['value']
        except KeyError:
            val=-1
            
        if name == 'z-exag':
            val = 1

        slider = wx.Slider(parent=parent, id=wx.ID_ANY,
                           value=val,
                           minValue=range[0],
                           maxValue=range[1],
                           style=style,
                           size=sizeW)
        slider.SetName('slider')
        slider.Bind(wx.EVT_SCROLL, bind[0])
        slider.Bind(wx.EVT_COMMAND_SCROLL_THUMBRELEASE, bind[1])
#        slider.Bind(wx.EVT_SCROLL_CHANGED, bind[1]) # this only works in MSW
        dict[name]['slider'] = slider.GetId()
        
        slider.SetValue(val)

        spin = wx.SpinCtrl(parent=parent, id=wx.ID_ANY, size=(65, -1),
                           initial=val,
                           min=range[0],
                           max=range[1])
                           
        spin.SetValue(val)
        
        #         spin = wx.SpinButton(parent=parent, id=wx.ID_ANY)
        #         spin.SetValue (self.mapWindow.view[name]['value'])
        #         spin.SetRange(self.mapWindow.view[name]['min'],
        #                      self.mapWindow.view[name]['max'])

        # no 'changed' event ... (FIXME)
        spin.SetName('spin')
        spin.Bind(wx.EVT_SPINCTRL, bind[2])
        
        dict[name]['spin'] = spin.GetId()

    def UpdateSettings(self):
        """!Update view from settings values 
        stored in self.mapWindow.view dictionary"""
        for control in ('height',
                        'persp',
                        'twist',
                        'z-exag'):
            for win in self.win['view'][control].itervalues():                
                if control == 'height':
                    value = UserSettings.Get(group='nviz', key='view',
                                             subkey=['height', 'value'], internal=True)
                else:
                    value = self.mapWindow.view[control]['value']
                self.FindWindowById(win).SetValue(value)

        x = self.mapWindow.view['pos']['x']
        y = self.mapWindow.view['pos']['y']
        self.viewPos.UpdatePos(x, y)        
        self.FindWindowById(self.win['view']['pos']).Draw()
        self.FindWindowById(self.win['view']['pos']).Refresh(False)
        
        bgcolor = self.FindWindowById(self.win['settings']['general']['bgcolor']).GetColour()
        self.OnBgColor(event=bgcolor)
        self.Update()

        self.mapWindow.Refresh(eraseBackground=False)
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

    def __GetWindowName(self, dict, id):
        for name in dict.iterkeys():
            if type(dict[name]) is type({}):
                for win in dict[name].itervalues():
                    if win == id:
                        return name
            else:
                if dict[name] == id:
                    return name

        return None

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
        
    def OnViewChanged(self, event):
        """!View changed, render in full resolution"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
        self.UpdateSettings()

    def OnViewChangedSpin(self, event):
        """!View changed, render in full resolution"""
        # TODO: use step value instead
        self.OnViewChange(event)
        self.OnViewChanged(None)
        self.Update()
        
    def OnResetView(self, event):
        """!Reset to default view (view page)"""
        self.mapWindow.ResetView()
        self.UpdateSettings()
        self.mapWindow.Refresh(False)

    def OnLookAt(self, event):
        """!Look at (view page)"""
        sel = event.GetSelection()
        if sel == 0: # top
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 1: # north
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 2: # south
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 1.0
        elif sel == 3: # east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 4: # west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 5: # north-west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 6: # north-east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 7: # south-east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 1.0
        elif sel == 8: # south-west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 1.0

        self.PostViewEvent(zExag=True)
        
        self.UpdateSettings()

        self.mapWindow.Refresh(False)

    def OnDefault(self, event):
        """!Restore default settings"""
        settings = copy.deepcopy(UserSettings.GetDefaultSettings()['nviz'])
        UserSettings.Set(group='nviz',
                         value=settings)
        
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    if subvalue == 'step':
                        continue
                    else:
                        defsetting = value[subvalue]
                        
                    if subgroup == 'view':
                        for viewkey, viewitem in self.mapWindow.view[subkey].iteritems(): 
                            if viewkey == subvalue:
                                self.mapWindow.view[subkey][viewkey] = defsetting 
                            else:
                                continue                

                    elif subgroup == 'settings' and type(defsetting) in [tuple, list]:
                        defsetting = tuple(defsetting)
                        self.FindWindowById(self.win[subgroup][subkey][subvalue]).SetColour(defsetting) 
                        
                    else:
                        continue
                        # should default reset all values or just view (as it does now)?
                            
#                        for otherkey, otheritem in self.win[subgroup][subkey].iteritems():
#                            if type(otheritem) == dict:
#                                for endkey, enditem in otheritem.iteritems():
#                                    if endkey == subvalue:
#                                        paramwin = self.FindWindowById(enditem)
#                                    else:
#                                        continue
#                            else:
#                                if otherkey == subvalue:
#                                    paramwin = self.FindWindowById(otheritem) 
#                                else:
#                                    continue
#                            if type(insetting) in [tuple, list]:
#                                 inseting = tuple(insetting)
#                                paramwin.SetColour(insetting)
#                            else:
#                                try:
#                                    paramwin.SetValue(insetting)
#                                except:
#                                    try:
#                                        paramwin.SetStringSelection(insetting)
#                                    except:
#                                        continue

                                
        self.UpdateSettings()
        self.FindWindowById(self.win['view']['pos']).Draw()
        self.FindWindowById(self.win['view']['pos']).Refresh(False)

        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
         
        event.Skip()

    def OnLoad(self, event):
        """!Apply button pressed"""
        
        self.LoadSettings()
        
        if event:
            event.Skip()

    def LoadSettings(self):
        """!Load saved Nviz settings and apply to current session"""

        UserSettings.ReadSettingsFile()
        settings = copy.deepcopy(UserSettings.Get(group='nviz'))

        for subgroup, key in settings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    if subvalue == 'step':
                        continue
                    else:
                        insetting = value[subvalue]                                                    
                    if subgroup == 'view':
                        for viewkey, viewitem in self.mapWindow.view[subkey].iteritems(): 
                            if viewkey == subvalue:
                                self.mapWindow.view[subkey][viewkey] = insetting 
                            else:
                                continue
                    else:
                        for otherkey, otheritem in self.win[subgroup][subkey].iteritems():
                            if type(otheritem) == dict:
                                for endkey, enditem in otheritem.iteritems():
                                    if endkey == subvalue:
                                        paramwin = self.FindWindowById(enditem)
                                    else:
                                        continue
                            else:
                                if otherkey == subvalue:
                                    paramwin = self.FindWindowById(otheritem)
                                else:
                                    continue
                            if type(insetting) in [tuple, list] and len(insetting) > 2:
                                insetting = tuple(insetting)
                                paramwin.SetColour(insetting)
                            else:
                                try:
                                    paramwin.SetValue(insetting)
                                except:
                                    try:
                                        paramwin.SetStringSelection(insetting)
                                    except:
                                        continue
                                
        self.UpdateSettings()
        self.FindWindowById(self.win['view']['pos']).Draw()
        self.FindWindowById(self.win['view']['pos']).Refresh(False)

        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
                                                
    def OnSave(self, event):
        """!Save button pressed
        
        Save settings to configuration file
        """
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)

        self.UpdateSettings()
        
        nvsettings = UserSettings.Get(group='nviz')
        for subgroup, key in nvsettings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                if subkey == 'height': continue
                for subvalue in value.keys():
                    if subvalue == 'step':
                        #no way to change steps for sliders or spinctrls on non-MSW systems
                        nvsettings[subgroup][subkey][subvalue] = 1 
                    else:
                        if subgroup == 'view':
                            nvsettings[subgroup][subkey][subvalue] = self.mapWindow.view[subkey][subvalue]                            
                        elif subvalue == 'map':
                            if subkey == 'shine': 
                                nvsettings[subgroup][subkey][subvalue] = False
                            if subkey == 'color': 
                                nvsettings[subgroup][subkey][subvalue] = True
                        else:
                            for otherkey, otheritem in self.win[subgroup][subkey].iteritems():
                                if type(otheritem) == dict:
                                    for endkey, enditem in otheritem.iteritems():
                                        if endkey == subvalue:
                                            if self.FindWindowById(enditem).GetClassName() == 'wxChoice':
                                                outsetting = self.FindWindowById(enditem).GetSelection()
                                            else:
                                                try:
                                                    outsetting = self.FindWindowById(enditem).GetColour()
                                                    outsetting = str(outsetting.Red())+':'+str(outsetting.Green())+':'+str(outsetting.Blue())
                                                except:
                                                    try:
                                                        outsetting = self.FindWindowById(enditem).GetValue()
                                                    except:
                                                        try:
                                                            outsetting = self.FindWindowById(enditem).GetString()
                                                        except:
                                                            outsetting = ''
                                            if (type(outsetting) == list or type(outsetting) == tuple) and len(outsetting) > 2:
                                                outsetting = str(outsetting[0])+':'+str(outsetting[1])+':'+str(outsetting[2])
                                                
                                            nvsettings[subgroup][subkey][subvalue][endkey] = outsetting
                                else:
                                    if otherkey == subvalue:
                                        if self.FindWindowById(otheritem).GetClassName() == 'wxChoice':
                                            outsetting = self.FindWindowById(otheritem).GetSelection()
                                        else:
                                            try:
                                                outsetting = self.FindWindowById(otheritem).GetColour()
                                                outsetting = str(outsetting.Red())+':'+str(outsetting.Green())+':'+str(outsetting.Blue())
                                            except:
                                                try:
                                                    outsetting = self.FindWindowById(otheritem).GetValue()
                                                except:
                                                    try:
                                                        outsetting = self.FindWindowById(enditem).GetString()
                                                    except:
                                                        outsetting = ''
                                        if (type(outsetting) == list or type(outsetting) == tuple) and len(outsetting) > 2:
                                            outsetting = str(outsetting[0])+':'+str(outsetting[1])+':'+str(outsetting[2])

                                        nvsettings[subgroup][subkey][subvalue] = outsetting
                               
        UserSettings.Set(group='nviz', value=nvsettings)
        file = UserSettings.SaveToFile()
        self.lmgr.goutput.WriteLog(_('Nviz settings saved to file <%s>.') % file)

    def OnBgColor(self, event):
        """!Background color changed"""
   
        try:
            color = event.GetValue()
        except:
            color = event

        if type(color) != wx._gdi.Colour: return
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        self.mapWindow.nvizClass.SetBgColor(str(color))

        if self.parent.statusbarWin['render'].IsChecked():
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

        self.SetMapObjUseMap(nvizType=nvizType,
                             attrb=attrb, map=useMap)
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
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
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def SetMapObjUseMap(self, nvizType, attrb, map=None):
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
        self.SetMapObjAttrb(nvizType='surface', winId=event.GetId())
        
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
            data = self.mapWindow.GetSelectedLayer(type='nviz')
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
            event = wxUpdateProperties(data=data)
            wx.PostEvent(self.mapWindow, event)
            
            if self.parent.statusbarWin['render'].IsChecked():
                self.mapWindow.Refresh(False)

    def OnSurfaceResolution(self, event):
        """!Draw resolution changed"""
        self.SetSurfaceResolution()

        if apply and self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def SetSurfaceResolution(self):
        """!Set draw resolution"""
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['draw']['resolution'] = { 'coarse' : coarse,
                                                  'fine' : fine,
                                                  'update' : None }
        
        # update properties
        event = wxUpdateProperties(data=data)
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
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['draw']['mode'] = { 'value' : value,
                                            'desc' : desc,
                                            'update' : None }
        
        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if apply and self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def OnSurfaceModeAll(self, event):
        """!Set draw mode (including wire color) for all loaded surfaces"""
        value, desc = self.SetSurfaceMode()
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        color = self.FindWindowById(self.win['surface']['draw']['wire-color']).GetColour()
        cvalue = self._getColorString(color)
        
        for name in self.mapWindow.GetLayerNames(type='raster'):
            data = self.mapWindow.GetLayerData(type='raster', name=name)
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
            event = wxUpdateProperties(data=data)
            wx.PostEvent(self.mapWindow, event)
            
        if apply and self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def _getColorString(self, color):
        """!Set wire color"""
        return str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
    
    def OnSurfaceWireColor(self, event):
        """!Set wire color"""
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        value = self._getColorString(event.GetValue())
        data['surface']['draw']['wire-color'] = { 'value' : value,
                                                  'update' : None }
                
        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def OnSurfaceAxis(self, event):
        """!Surface position, axis changed"""
        mapLayer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        id = data['surface']['object']['id']

        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        slider = self.FindWindowById(self.win['surface']['position']['slider'])
        spin = self.FindWindowById(self.win['surface']['position']['spin'])

        x, y, z = self.mapWindow.nvizClass.GetSurfacePosition(id)

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
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        id = data['surface']['object']['id']
        x, y, z = self.mapWindow.nvizClass.GetSurfacePosition(id)

        if axis == 0: # x
            x = value
        elif axis == 1: # y
            y = value
        else: # z
            z = value
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['position']['x'] = x
        data['surface']['position']['y'] = y
        data['surface']['position']['z'] = z
        data['surface']['position']['update'] = None
        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.parent.statusbarWin['render'].IsChecked():
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
        item = self.mapWindow.GetSelectedLayer(type='item')
        data = self.mapWindow.GetSelectedLayer(type='nviz')['vector']
        
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
                event = wxUpdateProperties(data=data)
                wx.PostEvent(self.mapWindow, event)
                
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
    
    def OnVectorDisplay(self, event):
        """!Display vector lines on surface/flat"""
        rasters = self.mapWindow.GetLayerNames('raster')
        if event.GetSelection() == 0: # surface
            if len(rasters) < 1:
                event.Veto()
                return

            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(True)
            # set first found surface
            data = self.mapWindow.GetSelectedLayer(type='nviz')
            data['vector']['lines']['mode']['surface'] = rasters[0]
            self.FindWindowById(self.win['vector']['lines']['surface']).SetStringSelection( \
                rasters[0])
        else: # flat
            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(False)

        self.OnVectorLines(event)

        event.Skip()

    def OnVectorLines(self, event):
        """!Set vector lines mode, apply changes if auto-rendering is enabled"""
        width = self.FindWindowById(self.win['vector']['lines']['width']).GetValue()

        color = self.FindWindowById(self.win['vector']['lines']['color']).GetColour()
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        mode = {}
        if self.FindWindowById(self.win['vector']['lines']['flat']).GetSelection() == 0:
            mode['type'] = 'surface'
            mode['surface'] = self.FindWindowById(self.win['vector']['lines']['surface']).GetValue()
            mode['update'] = None
        else:
            mode['type'] = 'flat'
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        for attrb in ('width', 'color', 'mode'):
            data['vector']['lines'][attrb]['update'] = None
        data['vector']['lines']['width']['value'] = width
        data['vector']['lines']['color']['value'] = color
        data['vector']['lines']['mode']['value'] = mode
        
        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
                        
        if self.parent.statusbarWin['render'].IsChecked():
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
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')['vector'][vtype]
        data['height'] = { 'value' : value,
                           'update' : None }
        
        # update properties
        event = wxUpdateProperties(data=data)
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
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['vector'][vtype]['mode']['surface'] = { 'value' : event.GetString(),
                                                     'update' : None }
        
        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVectorPoints(self, event):
        """!Set vector points mode, apply changes if auto-rendering is enabled"""
        size  = self.FindWindowById(self.win['vector']['points']['size']).GetValue()
#        width = self.FindWindowById(self.win['vector']['points']['width']).GetValue()

        color = self.FindWindowById(self.win['vector']['points']['color']).GetColour()
        if type(color) != wx._gdi.Colour: return #color picker not yet instantiated
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        marker = self.FindWindowById(self.win['vector']['points']['marker']).GetSelection()
        
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        for attrb in ('size', 'color', 'marker'):
            data['vector']['points'][attrb]['update'] = None
        data['vector']['points']['size']['value'] = size
#        data['vector']['points']['width']['value'] = width
        data['vector']['points']['color']['value'] = color
        data['vector']['points']['marker']['value'] = marker

        # update properties
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.parent.statusbarWin['render'].IsChecked():
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
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        id = data['object']['id']

        mode = 0
        if selection == 0:
            mode |= wxnviz.DM_FLAT
        else:
            mode |= wxnviz.DM_GOURAUD
        
        self.mapWindow.nvizClass.SetIsosurfaceMode(id, mode)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVolumeIsosurfResolution(self, event):
        """!Set isosurface draw resolution"""
        self.SetIsosurfaceResolution(event.GetInt())
    
    def SetIsosurfaceResolution(self, res):
        """!Set isosurface draw resolution"""
        layer = self.mapWindow.GetSelectedLayer()
        try:
            data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        except:
            # no volume layer
            return
        
        id = data['object']['id']
        self.mapWindow.nvizClass.SetIsosurfaceRes(id, res)

        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)

    def OnVolumeIsosurfMap(self, event):
        """!Set surface attribute"""
        self.SetMapObjAttrb(nvizType='volume', winId=event.GetId())

    def OnVolumeIsosurfCheck(self, event):
        """!Isosurface checked (->load) or unchecked (->unload)"""
        index = event.GetSelection()
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        id = data['object']['id']
        
        isosurfId = event.GetSelection()
        
        if list.IsChecked(index):
            self.mapWindow.nvizClass.SetIsosurfaceTransp(id, isosurfId, False, "0")
        else:
            # disable -> make transparent
            self.mapWindow.nvizClass.SetIsosurfaceTransp(id, isosurfId, False, "255")
        
        if self.parent.statusbarWin['render'].IsChecked():
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
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']['isosurface'][selection]
        
        self.UpdateVolumeIsosurfPage(layer, data)

    def OnVolumeIsosurfAdd(self, event):
        """!Add new isosurface to the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        level = self.FindWindowById(self.win['volume']['topo']['const']).GetValue()
        
        sel = list.GetSelection()
        if sel < 0 or sel >= list.GetCount() - 1:
            item = list.Append(item="%s %s" % (_("Level"), str(level)))
        else:
            list.Insert(item="%s %s" % (_("Level"), str(level)),
                        pos=sel+1) # append
            item = sel + 1
        
        list.Check(item)
        list.SetSelection(item)
        
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
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
        self.mapWindow.nvizClass.AddIsosurface(id, level)
        # use by default 3d raster map for color
        self.mapWindow.nvizClass.SetIsosurfaceColor(id, item, True, str(layer.name))

        # update buttons
        self.UpdateIsosurfButtons(list)

        if self.parent.statusbarWin['render'].IsChecked():
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
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        id = data['object']['id']

        # delete isosurface
        del data['isosurface'][isosurfId]
        
        self.mapWindow.nvizClass.DeleteIsosurface(id, isosurfId)

        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeIsosurfMoveUp(self, event):
        """!Move isosurface up in the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        sel = list.GetSelection()

        if sel < 1:
            return # this should not happen

        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        id = data['object']['id']

        # move item up
        text = list.GetStringSelection()
        list.Insert(item=text, pos=sel-1)
        list.Check(sel-1)
        list.SetSelection(sel-1)
        list.Delete(sel+1)
        data['isosurface'].insert(sel-1, data['isosurface'][sel])
        del data['isosurface'][sel+1]
        self.mapWindow.nvizClass.MoveIsosurface(id, sel, True)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeIsosurfMoveDown(self, event):
        """!Move isosurface dowm in the list"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])
        sel = list.GetSelection()

        if sel >= list.GetCount() - 1:
            return # this should not happen

        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')['volume']
        id = data['object']['id']

        # move item up
        text = list.GetStringSelection()
        list.Insert(item=text, pos=sel+2)
        list.Check(sel+2)
        list.SetSelection(sel+2)
        list.Delete(sel)
        data['isosurface'].insert(sel+2, data['isosurface'][sel])
        del data['isosurface'][sel]
        self.mapWindow.nvizClass.MoveIsosurface(id, sel, False)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.parent.statusbarWin['render'].IsChecked():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def UpdatePage(self, pageId):
        """!Update dialog (selected page)"""
        self.pageChanging = True
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        
        if pageId == 'view':
            self.SetPage('view')

#            max = self.mapWindow.view['z-exag']['value'] * 10 #this value is essentially null
            hmin = self.mapWindow.iview['height']['min']
            hmax = self.mapWindow.iview['height']['max']
            hval = int(0.4 * (hmax - hmin)) + hmin
            for control in ('spin', 'slider'):
#                self.FindWindowById(self.win['view']['z-exag'][control]).SetRange(0,
#                                                                                  max)
                self.FindWindowById(self.win['view']['height'][control]).SetRange(hmin,
                                                                                  hmax)
                self.FindWindowById(self.win['view']['height'][control]).SetValue(hval)                                      
                                                              
        elif pageId in ('surface', 'vector', 'volume'):
            current_page = self.notebook.GetSelection()
            if self.notebook.GetSelection() != self.page[pageId]['id']:
                for page in ('surface', 'vector', 'volume'):
                    if self.page[page]['id'] > -1:
                        self.notebook.RemovePage(self.page[page]['id'])
                        self.page[page]['id'] = -1
                        oldpanel = wx.FindWindowById(self.page[page]['panel'])
                        oldpanel.Hide()

                self.page[pageId]['id'] = 1
#                self.page['settings']['id'] = 2

                panel = wx.FindWindowById(self.page[pageId]['panel'])
                self.notebook.InsertPage(n=self.page[pageId]['id'],
                                         page=panel,
                                         text=" %s " % _("Layer properties"),
                                         select=True)
                                         
            self.notebook.ChangeSelection(current_page) 
            if pageId == 'surface':
                self.UpdateSurfacePage(layer, data['surface'])
            elif pageId == 'vector':
                self.UpdateVectorPage(layer, data['vector'])
            elif pageId == 'volume':
                self.UpdateVectorPage(layer, data['vector'])
            
#        self.notebook.Update()
        self.pageChanging = False
        
    def UpdateSurfacePage(self, layer, data):
        #
        # attributes
        #
        for attr in ('topo', 'color'): # required
            if layer and layer.type == 'raster':
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue(layer.name)
            else:
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue('')
            self.SetMapObjUseMap(nvizType='surface',
                                 attrb=attr, map=True) # -> map

        if data['attribute'].has_key('color'):
            value = data['attribute']['color']['value']
            if data['attribute']['color']['map']:
                self.FindWindowById(self.win['surface']['color']['map']).SetValue(value)
            else: # constant
                color = map(int, value.split(':'))
                self.FindWindowById(self.win['surface']['color']['const']).SetColour(color)
            self.SetMapObjUseMap(nvizType='surface',
                                 attrb=attr, map=data['attribute']['color']['map'])

        self.SetMapObjUseMap(nvizType='surface',
                             attrb='shine', map=data['attribute']['shine']['map'])
        value = data['attribute']['shine']['value']
        if data['attribute']['shine']['map']:
            self.FindWindowById(self.win['surface']['shine']['map']).SetValue(value)
        else:
            self.FindWindowById(self.win['surface']['shine']['const']).SetValue(value)

        #
        # draw
        #
        for control, dict in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue
            if control == 'resolution':
                self.FindWindowById(self.win['surface']['draw']['res-coarse']).SetValue(dict['coarse'])
                self.FindWindowById(self.win['surface']['draw']['res-fine']).SetValue(dict['fine'])
                continue

            if control == 'mode':
                if dict['desc']['mode'] == 'coarse':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(0)
                elif dict['desc']['mode'] == 'fine':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(1)
                else: # both
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(2)
                    
                if dict['desc']['style'] == 'wire':
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(0)
                else: # surface
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(1)

                if dict['desc']['shading'] == 'flat':
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(0)
                else: # gouraud
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(1)
                
                continue

            value = dict['value']
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
        
    def UpdateVectorPage(self, layer, data):
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
            desc = _("Vector map <%s> is 3D") % layer.name
            enable = False
        else:
            desc = _("Vector map <%s> is 2D") % layer.name
            enable = True
        desc += " - " + _("%(primitives)d primitives (%(points)d points)") % \
            { 'primitives' : nprimitives, 'points' : npoints }

        self.FindWindowById(self.win['vector']['lines']['flat']).Enable(enable)
        for v in ('lines', 'points'):
            self.FindWindowById(self.win['vector'][v]['surface']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['slider']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['spin']).Enable(enable)
            
        self.FindWindowById(self.win['vector']['desc']).SetLabel(desc)
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
                    surface.SetStringSelection(data[vtype]['mode']['surface'])
                
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

    def UpdateVolumePage(self, layer, data):
        """!Update volume layer properties page"""
        list = self.FindWindowById(self.win['volume']['isosurfs'])

        #
        # draw
        #
        for control, dict in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue

            win = self.FindWindowById(self.win['volume']['draw'][control])

            if control == 'shading':
                if data['draw']['shading']['desc'] == 'flat':
                    value = 0
                else:
                    value = 1
            else:
                value = dict['value']

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
                self.SetMapObjUseMap(nvizType='volume',
                                     attrb=attrb, map=True) # -> map
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
            
            self.SetMapObjUseMap(nvizType='volume',
                                 attrb=attrb, map=data[attrb]['map'])
            
    def SetPage(self, name):
        """!Get named page"""
        self.notebook.SetSelection(self.page[name]['id'])

class ViewPositionWindow(wx.Window):
    """!Position control window (for NvizToolWindow)"""
    def __init__(self, parent, id, mapwindow, win,
                 pos=wx.DefaultPosition,
                 size=wx.DefaultSize):
        self.mapWindow = mapwindow
        self.toolwin = parent.GetGrandParent()

        wx.Window.__init__(self, parent, id, pos, size)

        self.SetBackgroundColour("WHITE")

        self.pdc = wx.PseudoDC()

        self.pdc.SetBrush(wx.Brush(colour='dark green', style=wx.SOLID))
        self.pdc.SetPen(wx.Pen(colour='dark green', width=2, style=wx.SOLID))

        self.Draw()

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        # self.Bind(wx.EVT_MOTION,       self.OnMouse)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)

    def Draw(self, pos=None):
        w, h = self.GetClientSize()
        
        if pos is None:
            x = self.mapWindow.view['pos']['x']
            y = self.mapWindow.view['pos']['y']
            x = x * w
            y = y * h
        else:
            x, y = pos

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

    def OnMouse(self, event):
        if event.LeftIsDown():
            x, y = event.GetPosition()
            self.mapWindow.view['x'] = x
            self.mapWindow.view['y'] = y
            self.Draw(pos=(x, y))
            self.Refresh(False)
            w, h = self.GetClientSize()
            x = float(x) / w
            y = float(y) / h
            self.UpdatePos(x, y)
            self.mapWindow.render['quick'] = True
            
        elif event.LeftUp():
            self.mapWindow.render['quick'] = False
            
        self.mapWindow.Refresh(eraseBackground=False)
        
        event.Skip()
        
    def UpdatePos(self, xcoord, ycoord):
        
        if xcoord >= 0 and xcoord <= 1.0:
            self.mapWindow.view['pos']['x'] = xcoord
        if ycoord >= 0 and ycoord <= 1.0:
            self.mapWindow.view['pos']['y'] = ycoord
               
        event = wxUpdateView(zExag=True)
        wx.PostEvent(self.mapWindow, event)
         
  
