"""!
@package nviz.tools

@brief Nviz (3D view) tools window

Classes:
 - tools::NvizToolWindow
 - tools::PositionWindow
 - tools::ViewPositionWindow
 - tools::LightPositionWindow

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <kratochanna gmail.com> (Google SoC 2011)
"""

import os
import sys
import copy
import types

import wx
import wx.lib.colourselect  as csel
import wx.lib.scrolledpanel as SP
import wx.lib.filebrowsebutton as filebrowse
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
try:
    from agw import foldpanelbar as fpb
except ImportError: # if it's not there locally, try the wxPython lib.
    try:
        import wx.lib.agw.foldpanelbar as fpb
    except ImportError:
        import wx.lib.foldpanelbar as fpb # versions <=2.5.5.1
try:
    import wx.lib.agw.floatspin as fs
except ImportError:
    fs = None
import grass.script as grass

from core               import globalvar
from gui_core.gselect   import VectorDBInfo
from core.gcmd          import GMessage, RunCommand
from modules.colorrules import ThematicVectorTable
from core.settings      import UserSettings
from gui_core.widgets   import ScrolledPanel, NumTextCtrl, FloatSlider, SymbolButton
from gui_core.gselect   import Select
from core.debug         import Debug
try:
    from nviz.mapwindow import wxUpdateProperties, wxUpdateView,\
                               wxUpdateLight, wxUpdateCPlane
    import wxnviz
except ImportError:
    pass

class NvizToolWindow(FN.FlatNotebook):
    """!Nviz (3D view) tools panel
    """
    def __init__(self, parent, display, id = wx.ID_ANY,
                 style = globalvar.FNPageStyle|FN.FNB_NO_X_BUTTON,
                 **kwargs):
        Debug.msg(5, "NvizToolWindow.__init__()")
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
                    
        # analysis page
        self.AddPage(page = self._createAnalysisPage(),
                     text = " %s " % _("Analysis"))
        # view page
        self.AddPage(page = self._createAnimationPage(),
                     text = " %s " % _("Animation"))
        
        self.UpdateSettings()
        
        self.mapWindow.SetToolWin(self)
        
        self.pageChanging = False
        self.vetoGSelectEvt = False #when setting map, event is invoked
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
        # bindings
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        
        self.mapWindow.GetAnimation().animationFinished.connect(self.OnAnimationFinished)
        self.mapWindow.GetAnimation().animationUpdateIndex.connect(self.OnAnimationUpdateIndex)
        
        Debug.msg(3, "NvizToolWindow.__init__()")
        
        self.Update()
        wx.CallAfter(self.SetPage, 'view')
        wx.CallAfter(self.UpdateScrolling, (self.foldpanelData, self.foldpanelAppear,
                                            self.foldpanelAnalysis))       
        wx.CallAfter(self.SetInitialMaps)
        
    def SetInitialMaps(self):
        """!Set initial raster and vector map"""
        for ltype in ('raster', 'vector', '3d-raster'):
            selectedLayer = self.parent.GetLayerTree().GetSelectedLayer(multi = False, checkedOnly = True)
            if selectedLayer is None:
                continue
            selectedLayer = self.parent.GetLayerTree().GetLayerInfo(selectedLayer, key = 'maplayer')
            layers = self.mapWindow.Map.GetListOfLayers(ltype = ltype, active = True)
            if selectedLayer in layers:
                selection = selectedLayer.GetName()
            else:
                try:
                    selection = layers[0].GetName()
                except:
                    continue
            if ltype == 'raster':
                self.FindWindowById(self.win['surface']['map']).SetValue(selection)
                self.FindWindowById(self.win['fringe']['map']).SetValue(selection)
            elif ltype == 'vector':
                self.FindWindowById(self.win['vector']['map']).SetValue(selection)
            elif ltype == '3d-raster':
                self.FindWindowById(self.win['volume']['map']).SetValue(selection)
               
    def UpdateState(self, **kwargs):
        if 'view' in kwargs:
            self.mapWindow.view = kwargs['view']
            self.FindWindowById(self.win['view']['position']).data = kwargs['view']
            self.FindWindowById(self.win['view']['position']).PostDraw()
        if 'iview' in kwargs:
            self.mapWindow.iview = kwargs['iview']
        if 'light' in kwargs:
            self.mapWindow.light = kwargs['light']  
            self.FindWindowById(self.win['light']['position']).data = kwargs['light']  
            self.FindWindowById(self.win['light']['position']).PostDraw()
        if 'fly' in kwargs:
            self.mapWindow.fly['exag'] = kwargs['fly']['exag']
    
    def LoadSettings(self):
        """!Load Nviz settings and apply to current session"""
        view = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'view')) # copy
        light = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'light')) # copy
        fly = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'fly')) # copy
        self.UpdateState(view = view, light = light, fly = fly)
        self.PostViewEvent(zExag = True)
        self.PostLightEvent()
        self.UpdatePage('view')
        self.UpdatePage('light')
        
        self.mapWindow.ReloadLayersData()
        self.UpdatePage('surface')
        self.UpdatePage('vector')
        self.UpdateSettings()
               
    def OnPageChanged(self, event):
        new = event.GetSelection()
        # self.ChangeSelection(new)
        
    def PostViewEvent(self, zExag = False):
        """!Change view settings"""
        event = wxUpdateView(zExag = zExag)
        wx.PostEvent(self.mapWindow, event)
        
    def PostLightEvent(self, refresh = False): 
        """!Change light settings"""   
        event = wxUpdateLight(refresh = refresh)
        wx.PostEvent(self.mapWindow, event)
        
    def OnSize(self, event):
        """!After window is resized, update scrolling"""
        # workaround to resize captionbars of foldpanelbar
        wx.CallAfter(self.UpdateScrolling, (self.foldpanelData, self.foldpanelAppear,
                                            self.foldpanelAnalysis)) 
        event.Skip()
           
    def OnPressCaption(self, event):
        """!When foldpanel item collapsed/expanded, update scrollbars"""
        foldpanel = event.GetBar().GetGrandParent().GetParent()
        wx.CallAfter(self.UpdateScrolling, (foldpanel,))
        event.Skip()
        
    def UpdateScrolling(self, foldpanels):
        """!Update scrollbars in foldpanel"""
        for foldpanel in foldpanels:
            length = foldpanel.GetPanelsLength(collapsed = 0, expanded = 0)
            # virtual width is set to fixed value to suppress GTK warning
            foldpanel.GetParent().SetVirtualSize((100, length[2]))
            foldpanel.GetParent().Layout()
        
    def _createViewPage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['view'] = { 'id' : 0,
                              'notebook' : self.GetId()}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Control View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 10)
        
        self.win['view'] = {}
        
        # position
        posSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        self._createCompass(panel = panel, sizer = posSizer, type = 'view')
        
        view = ViewPositionWindow(panel, size = (175, 175),
                                  mapwindow = self.mapWindow)
        self.win['view']['position'] = view.GetId()
        posSizer.Add(item = view,
                     pos = (1, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = posSizer, pos = (0, 0))
                  
        # perspective
        # set initial defaults here (or perhaps in a default values file), not in user settings
        #todo: consider setting an absolute max at 360 instead of undefined. (leave the default max value at pi)
        tooltip = _("Adjusts the distance and angular perspective of the image viewpoint")
        self._createControl(panel, data = self.win['view'], name = 'persp',
                            tooltip = tooltip, range = (1, 120),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedText))
        
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Perspective:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = self.FindWindowById(self.win['view']['persp']['slider']), pos = (2, 0),
                      flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = self.FindWindowById(self.win['view']['persp']['text']), pos = (3, 0),
                      flag = wx.ALIGN_CENTER)        
        
        # twist
        tooltip = _("Tilts the plane of the surface from the horizontal")
        self._createControl(panel, data = self.win['view'], name = 'twist',
                            tooltip = tooltip, range = (-180,180),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedText))
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Tilt:")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = self.FindWindowById(self.win['view']['twist']['slider']), pos = (2, 1))
        gridSizer.Add(item = self.FindWindowById(self.win['view']['twist']['text']), pos = (3, 1),
                      flag = wx.ALIGN_CENTER)        
        
        # height + z-exag
        tooltip = _("Adjusts the viewing height above the surface"
                    " (angle of view automatically adjusts to maintain the same center of view)")
        self._createControl(panel, data = self.win['view'], name = 'height', sliderHor = False,
                            tooltip = tooltip, range = (0, 1),
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedText))
        tooltip = _("Adjusts the relative height of features above the plane of the surface")
        self._createControl(panel, data = self.win['view'], name = 'z-exag', sliderHor = False,
                            tooltip = tooltip, range = (0, 10), floatSlider = True,
                            bind = (self.OnViewChange, self.OnViewChanged, self.OnViewChangedText))
        self.FindWindowById(self.win['view']['z-exag']['slider']).SetValue(1)
        self.FindWindowById(self.win['view']['z-exag']['text']).SetValue(1)
        
        heightSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:")),
                      pos = (0, 0), flag = wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['height']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 0))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['height']['text']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT, pos = (1, 1))
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Z-exag:")),
                      pos = (0, 2), flag = wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['z-exag']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['view']['z-exag']['text']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos = (1, 3))
        
        gridSizer.Add(item = heightSizer, pos = (0, 1), flag = wx.ALIGN_CENTER)
        
        # view setup + reset
        viewSizer = wx.BoxSizer(wx.HORIZONTAL)
        viewSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY,
                                           label = _("Look:")),
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
        here = wx.ToggleButton(panel, id = wx.ID_ANY, label = _("here"))
        here.Bind(wx.EVT_TOGGLEBUTTON, self.OnLookAt)
        here.SetName('here')
        here.SetToolTipString(_("Allows you to select a point on the surface "
                                "that becomes the new center of view. "
                                "Click on the button and then on the surface."))
        viewSizer.Add(item = here, flag = wx.TOP|wx.BOTTOM|wx.LEFT|wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
                    
        center = wx.Button(panel, id = wx.ID_ANY, label = _("center"))
        center.Bind(wx.EVT_BUTTON, self.OnLookAt)
        center.SetName('center')
        center.SetToolTipString(_("Resets the view to the original default center of view"))
        viewSizer.Add(item = center, flag = wx.TOP|wx.BOTTOM | wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
                    
        top = wx.Button(panel, id = wx.ID_ANY, label = _("top"))
        top.Bind(wx.EVT_BUTTON, self.OnLookAt)
        top.SetName('top')
        top.SetToolTipString(_("Sets the viewer directly over the scene's center position. This top view orients approximately north south."))
        viewSizer.Add(item = top, flag = wx.TOP|wx.BOTTOM | wx.ALIGN_CENTER_VERTICAL,
                      border = 5)
                    
        reset = wx.Button(panel, id = wx.ID_ANY, label = _("reset"))
        reset.SetToolTipString(_("Reset to default view"))
        reset.Bind(wx.EVT_BUTTON, self.OnResetView)
        viewSizer.Add(item = reset, proportion = 0,
                      flag = wx.TOP|wx.BOTTOM|wx.RIGHT| wx.ALIGN_RIGHT,
                      border = 5)
        
        gridSizer.Add(item = viewSizer, pos = (4, 0), span = (1, 3),
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
        
        # background color
        self.win['view']['background'] = {}
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Background color:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'view',
                                                            subkey = ['background', 'color']),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        self.win['view']['background']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnBgColor)
        gridSizer.Add(item = color, pos = (0, 1))
        gridSizer.AddGrowableCol(0)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
        
    def _createAnimationPage(self):
        """!Create view settings page"""
        panel = SP.ScrolledPanel(parent = self, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False)
        self.page['animation'] = { 'id' : 0,
                                   'notebook' : self.GetId()}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Animation")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        self.win['anim'] = {}
        # animation help text
        help = wx.StaticText(parent = panel, id = wx.ID_ANY,
                             label = _("Press 'Record' button and start changing the view. "
                                       "It is recommended to use fly-through mode "
                                       "(Map Display toolbar) to achieve smooth motion."))
        self.win['anim']['help'] = help.GetId()
        hSizer.Add(item = help, proportion = 0)
        boxSizer.Add(item = hSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 5)
                     
        # animation controls
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        record = SymbolButton(parent = panel, id = wx.ID_ANY,
                              usage = "record", label = _("Record"))
        play = SymbolButton(parent = panel, id = wx.ID_ANY,
                            usage = "play", label = _("Play"))
        pause = SymbolButton(parent = panel, id = wx.ID_ANY,
                             usage = "pause", label = _("Pause"))
        stop = SymbolButton(parent = panel, id = wx.ID_ANY,
                            usage = "stop", label = _("Stop"))
        
        self.win['anim']['record'] = record.GetId()
        self.win['anim']['play'] = play.GetId()
        self.win['anim']['pause'] = pause.GetId()
        self.win['anim']['stop'] = stop.GetId()
                            
        self._createControl(panel, data = self.win['anim'], name = 'frameIndex',
                            range = (0, 1), floatSlider = False,
                            bind = (self.OnFrameIndex, None, self.OnFrameIndexText))
        frameSlider = self.FindWindowById(self.win['anim']['frameIndex']['slider'])
        frameText = self.FindWindowById(self.win['anim']['frameIndex']['text'])
        infoLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Total number of frames :"))
        info = wx.StaticText(parent = panel, id = wx.ID_ANY)
        self.win['anim']['info'] = info.GetId()
        
        fpsLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Frame rate (FPS):"))
        fps = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = UserSettings.Get(group = 'nviz', key = 'animation', subkey = 'fps'),
                           min = 1,
                           max = 50)
        self.win['anim']['fps'] = fps.GetId()
        fps.SetToolTipString(_("Frames are recorded with given frequency (FPS). "))
                            
        record.Bind(wx.EVT_BUTTON, self.OnRecord)
        play.Bind(wx.EVT_BUTTON, self.OnPlay)
        stop.Bind(wx.EVT_BUTTON, self.OnStop)
        pause.Bind(wx.EVT_BUTTON, self.OnPause)
        fps.Bind(wx.EVT_SPINCTRL, self.OnFPS)
        
        hSizer.Add(item = record, proportion = 0)
        hSizer.Add(item = play, proportion = 0)
        hSizer.Add(item = pause, proportion = 0)
        hSizer.Add(item = stop, proportion = 0)
        boxSizer.Add(item = hSizer, proportion = 0,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        
        sliderBox = wx.BoxSizer(wx.HORIZONTAL)
        sliderBox.Add(item = frameSlider, proportion = 1, border = 5, flag = wx.EXPAND | wx.RIGHT)
        sliderBox.Add(item = frameText, proportion = 0, border = 5, flag = wx.EXPAND| wx.RIGHT | wx.LEFT)
        boxSizer.Add(item = sliderBox, proportion = 0, flag = wx.EXPAND)
        
        # total number of frames
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        hSizer.Add(item = infoLabel, proportion = 0, flag = wx.RIGHT, border = 5)
        hSizer.Add(item = info, proportion = 0, flag = wx.LEFT, border = 5)
        
        boxSizer.Add(item = hSizer, proportion = 0,
                     flag = wx.ALL | wx.EXPAND, border = 5)
                     
        # frames per second
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        hSizer.Add(item = fpsLabel, proportion = 0, flag = wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, border = 5)
        hSizer.Add(item = fps, proportion = 0, flag = wx.LEFT | wx.ALIGN_CENTER_VERTICAL, border = 5)
        
        boxSizer.Add(item = hSizer, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
                      
        # save animation
        self.win['anim']['save'] = {}
        self.win['anim']['save']['image'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Save image sequence")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        vSizer = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 10)
        
        pwd = os.getcwd()
        dir = filebrowse.DirBrowseButton(parent = panel, id = wx.ID_ANY,
                                         labelText = _("Choose a directory:"),
                                         dialogTitle = _("Choose a directory for images"),
                                         buttonText = _('Browse'),
                                         startDirectory = pwd)
        dir.SetValue(pwd)
        prefixLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("File prefix:"))
        prefixCtrl = wx.TextCtrl(parent = panel, id = wx.ID_ANY, size = (100, -1),
                                 value = UserSettings.Get(group = 'nviz',
                                                          key = 'animation', subkey = 'prefix'))
        prefixCtrl.SetToolTipString(_("Generated files names will look like this: prefix_1.ppm, prefix_2.ppm, ..."))
        fileTypeLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("File format:"))
        fileTypeCtrl = wx.Choice(parent = panel, id = wx.ID_ANY, choices = ["TIF", "PPM"])
        
        save = wx.Button(parent = panel, id = wx.ID_ANY,
                         label = "Save")
                         
        self.win['anim']['save']['image']['dir'] = dir.GetId()
        self.win['anim']['save']['image']['prefix'] = prefixCtrl.GetId()
        self.win['anim']['save']['image']['format'] = fileTypeCtrl.GetId()
        self.win['anim']['save']['image']['confirm'] = save.GetId()
        
        boxSizer.Add(item = dir, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        
        gridSizer.Add(item = prefixLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.Add(item = prefixCtrl, pos = (0, 1), flag = wx.EXPAND )
        gridSizer.Add(item = fileTypeLabel, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.Add(item = fileTypeCtrl, pos = (1, 1), flag = wx.EXPAND )
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 5)
        boxSizer.Add(item = save, proportion = 0, flag = wx.ALL | wx.ALIGN_RIGHT, border = 5)
        
        save.Bind(wx.EVT_BUTTON, self.OnSaveAnimation)
        
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
        
    def _createDataPage(self):
        """!Create data (surface, vector, volume) settings page"""

        self.mainPanelData = ScrolledPanel(parent = self)
        self.mainPanelData.SetupScrolling(scroll_x = False)
        try:# wxpython <= 2.8.10
            self.foldpanelData = fpb.FoldPanelBar(parent = self.mainPanelData, id = wx.ID_ANY,
                                                  style = fpb.FPB_DEFAULT_STYLE,
                                                  extraStyle = fpb.FPB_SINGLE_FOLD)
        except:
            try:# wxpython >= 2.8.11
                self.foldpanelData = fpb.FoldPanelBar(parent = self.mainPanelData, id = wx.ID_ANY,                               
                                                      agwStyle = fpb.FPB_SINGLE_FOLD)
            except: # to be sure
                self.foldpanelData = fpb.FoldPanelBar(parent = self.mainPanelData, id = wx.ID_ANY,                               
                                                      style = fpb.FPB_SINGLE_FOLD)
            
                     
        self.foldpanelData.Bind(fpb.EVT_CAPTIONBAR, self.OnPressCaption)


        
        # # surface page
        surfacePanel = self.foldpanelData.AddFoldPanel(_("Surface"), collapsed = False)
        self.foldpanelData.AddFoldPanelWindow(surfacePanel, 
            window = self._createSurfacePage(parent = surfacePanel), flags = fpb.FPB_ALIGN_WIDTH)
        self.EnablePage("surface", enabled = False)
        
        # constant page
        constantPanel = self.foldpanelData.AddFoldPanel(_("Constant surface"), collapsed = True)
        self.foldpanelData.AddFoldPanelWindow(constantPanel,
            window = self._createConstantPage(parent = constantPanel), flags = fpb.FPB_ALIGN_WIDTH)
        self.EnablePage("constant", enabled = False)
        # vector page
        vectorPanel = self.foldpanelData.AddFoldPanel(_("Vector"), collapsed = True)
        self.foldpanelData.AddFoldPanelWindow(vectorPanel, 
            window = self._createVectorPage(parent = vectorPanel), flags = fpb.FPB_ALIGN_WIDTH)
        self.EnablePage("vector", enabled = False)
        
        # volume page
        volumePanel = self.foldpanelData.AddFoldPanel(_("Volume"), collapsed = True)
        self.foldpanelData.AddFoldPanelWindow(volumePanel,
            window = self._createVolumePage(parent = volumePanel), flags = fpb.FPB_ALIGN_WIDTH)
        self.EnablePage("volume", enabled = False)
        
##        self.foldpanelData.ApplyCaptionStyleAll(style)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.foldpanelData, proportion = 1, flag = wx.EXPAND)
        self.mainPanelData.SetSizer(sizer)
        self.mainPanelData.Layout()
        self.mainPanelData.Fit()
        
        return self.mainPanelData
        
        
    def _createAppearancePage(self):
        """!Create data (surface, vector, volume) settings page"""
        self.mainPanelAppear = ScrolledPanel(parent = self)
        self.mainPanelAppear.SetupScrolling(scroll_x = False)
        
        try:# wxpython <= 2.8.10
            self.foldpanelAppear = fpb.FoldPanelBar(parent = self.mainPanelAppear, id = wx.ID_ANY,
                                                  style = fpb.FPB_DEFAULT_STYLE,
                                                  extraStyle = fpb.FPB_SINGLE_FOLD)
        except:
            try:# wxpython >= 2.8.11
                self.foldpanelAppear = fpb.FoldPanelBar(parent = self.mainPanelAppear, id = wx.ID_ANY,                               
                                                      agwStyle = fpb.FPB_SINGLE_FOLD)
            except: # to be sure
                self.foldpanelAppear = fpb.FoldPanelBar(parent = self.mainPanelAppear, id = wx.ID_ANY,                               
                                                      style = fpb.FPB_SINGLE_FOLD)
            
        self.foldpanelAppear.Bind(fpb.EVT_CAPTIONBAR, self.OnPressCaption)
        # light page
        lightPanel = self.foldpanelAppear.AddFoldPanel(_("Lighting"), collapsed = False)
        self.foldpanelAppear.AddFoldPanelWindow(lightPanel, 
            window = self._createLightPage(parent = lightPanel), flags = fpb.FPB_ALIGN_WIDTH)
    
        # fringe page
        fringePanel = self.foldpanelAppear.AddFoldPanel(_("Fringe"), collapsed = True)
        self.foldpanelAppear.AddFoldPanelWindow(fringePanel, 
            window = self._createFringePage(parent = fringePanel), flags = fpb.FPB_ALIGN_WIDTH)
        
        self.EnablePage('fringe', False)
        
        # decoration page
        decorationPanel = self.foldpanelAppear.AddFoldPanel(_("Decorations"), collapsed = True)
        self.foldpanelAppear.AddFoldPanelWindow(decorationPanel, 
            window = self._createDecorationPage(parent = decorationPanel), flags = fpb.FPB_ALIGN_WIDTH)
        
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.foldpanelAppear, proportion = 1, flag = wx.EXPAND)
        self.mainPanelAppear.SetSizer(sizer)
        self.mainPanelAppear.Layout()
        self.mainPanelAppear.Fit()
        return self.mainPanelAppear
    
    def _createAnalysisPage(self):
        """!Create data analysis (cutting planes, ...) page"""
        self.mainPanelAnalysis = ScrolledPanel(parent = self)
        self.mainPanelAnalysis.SetupScrolling(scroll_x = False)
        self.foldpanelAnalysis = fpb.FoldPanelBar(parent = self.mainPanelAnalysis, id = wx.ID_ANY,
                                                  style = fpb.FPB_SINGLE_FOLD)
        self.foldpanelAnalysis.Bind(fpb.EVT_CAPTIONBAR, self.OnPressCaption)
        # cutting planes page
        cplanePanel = self.foldpanelAnalysis.AddFoldPanel(_("Cutting planes"), collapsed = False)
        self.foldpanelAnalysis.AddFoldPanelWindow(cplanePanel, 
            window = self._createCPlanePage(parent = cplanePanel), flags = fpb.FPB_ALIGN_WIDTH)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.foldpanelAnalysis, proportion = 1, flag = wx.EXPAND)
        self.mainPanelAnalysis.SetSizer(sizer)
        self.mainPanelAnalysis.Layout()
        self.mainPanelAnalysis.Fit()
        return self.mainPanelAnalysis
        
    def _createSurfacePage(self, parent):
        """!Create view settings page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        self.page['surface'] = { 'id' : 0,
                                 'notebook' : self.foldpanelData.GetId() }
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['surface'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Raster map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        rmaps = Select(parent = panel, type = 'raster',
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
        # draw
        #
        self.win['surface']['draw'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # mode
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Mode:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent = panel, id = wx.ID_ANY, size = (-1, -1),
                          choices = [_("coarse"),
                                     _("fine"),
                                     _("both")])
        mode.SetName("selection")
        mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['surface']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item = mode, flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                      pos = (0, 1),span = (1, 2))
        
        # shading
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Shading:")),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT)
        shade = wx.Choice (parent = panel, id = wx.ID_ANY, size = (-1, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['surface']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item = shade, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 4))
        
        # set to all
        all = wx.Button(panel, id = wx.ID_ANY, label = _("Set to all"))
        all.SetToolTipString(_("Use draw settings for all loaded surfaces"))
        all.Bind(wx.EVT_BUTTON, self.OnSurfaceModeAll)
        gridSizer.Add(item = all, flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (3, 4))
        self.win['surface']['all'] = all.GetId()
        
        # resolution coarse
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Coarse mode:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                        label = _("resolution:")),
                     pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        resC = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = 6,
                           min = 1,
                           max = 100)
        resC.SetName("value")
        self.win['surface']['draw']['res-coarse'] = resC.GetId()
        resC.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        gridSizer.Add(item = resC, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT)
        
        # Coarse style
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("style:")),
                      pos = (3, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        style = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                          choices = [_("wire"),
                                     _("surface")])
        style.SetName("selection")
        self.win['surface']['draw']['style'] = style.GetId()
        style.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item = style, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (3, 2))
        
        # color
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetName("colour")
        color.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceWireColor)
        color.SetToolTipString(_("Change wire color"))
        self.win['surface']['draw']['wire-color'] = color.GetId()
        gridSizer.Add(item = color, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos = (3, 3))
        
        # resolution fine
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Fine mode:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                        label = _("resolution:")),
                     pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        resF = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = 3,
                           min = 1,
                           max = 100)
        resF.SetName("value")
        self.win['surface']['draw']['res-fine'] = resF.GetId()
        resF.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        gridSizer.Add(item = resF, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT)
        gridSizer.AddGrowableCol(3)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
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
        for code, attrb in (('color', _("Color")),
                           ('mask', _("Mask")),
                           ('transp', _("Transparency")),
                           ('shine', _("Shininess"))):
            self.win['surface'][code] = {} 
            gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                             label = attrb + ':'),
                          pos = (row, 0), flag = wx.ALIGN_CENTER_VERTICAL)
            use = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                             choices = [_("map")])
            
            if code not in ('color', 'shine'):
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
            
            map = Select(parent = panel, id = wx.ID_ANY,
                         # size = globalvar.DIALOG_GSELECT_SIZE,
                         size = (-1, -1),
                         type = "raster")
            self.win['surface'][code]['map'] = map.GetId() - 1 # FIXME
            map.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            gridSizer.Add(item = map, flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                          pos = (row, 2))
            
            if code == 'color':
                color = UserSettings.Get(group = 'nviz', key = 'surface', subkey = ['color', 'value'])
                value = csel.ColourSelect(panel, id = wx.ID_ANY,
                                          colour = color,
                                          size = globalvar.DIALOG_COLOR_SIZE)
                value.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceMap)
            elif code == 'mask':
                value = None
            else:
                value = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                                    initial = 0)
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
        gridSizer.AddGrowableCol(2)
        boxSizer.Add(item = gridSizer, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        #
        # position
        #
        self.win['surface']['position'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # position
        tooltip = _("Changes the x, y, and z position of the current surface")
        self._createControl(panel, data = self.win['surface'], name = 'position',
                            tooltip = tooltip, range = (-10000, 10000), floatSlider = True,
                            bind = (self.OnSurfacePosition, self.OnSurfacePositionChanged, self.OnSurfacePositionText))
        
        axis = wx.Choice (parent = panel, id = wx.ID_ANY, size = (75, -1),
                          choices = ["X",
                                     "Y",
                                     "Z"])
                                    
        reset = wx.Button(panel, id = wx.ID_ANY, label = _("Reset"))
        reset.SetToolTipString(_("Reset to default position"))
        reset.Bind(wx.EVT_BUTTON, self.OnResetSurfacePosition)
        self.win['surface']['position']['reset'] = reset.GetId()
        
        self.win['surface']['position']['axis'] = axis.GetId()
        axis.SetSelection(2)
        axis.Bind(wx.EVT_CHOICE, self.OnSurfaceAxis)
        
        pslide = self.FindWindowById(self.win['surface']['position']['slider'])
        ptext = self.FindWindowById(self.win['surface']['position']['text'])
        ptext.SetValue('0')
        
        gridSizer.Add(item = axis, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 0))
        gridSizer.Add(item = pslide, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 1))
        gridSizer.Add(item = ptext, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 2))
        gridSizer.Add(item = reset, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, pos = (0, 3))
        gridSizer.AddGrowableCol(3)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        
        pageSizer.Add(item = boxSizer, proportion = 1,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        #
        # mask
        #
##        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
##                            label = " %s " % (_("Mask")))
##        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
##        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
##        
##        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
##                                         label = _("Mask zeros:")),
##                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
##        
##        elev = wx.CheckBox(parent = panel, id = wx.ID_ANY,
##                           label = _("by elevation"))
##        elev.Enable(False) # TODO: not implemented yet
##        gridSizer.Add(item = elev, pos = (0, 1))
##        
##        color = wx.CheckBox(parent = panel, id = wx.ID_ANY,
##                           label = _("by color"))
##        color.Enable(False) # TODO: not implemented yet
##        gridSizer.Add(item = color, pos = (0, 2))
##        
##        boxSizer.Add(item = gridSizer, proportion = 1,
##                  flag = wx.ALL | wx.EXPAND, border = 3)
##        pageSizer.Add(item = boxSizer, proportion = 0,
##                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
##                      border = 3)
        
        
        panel.SetSizer(pageSizer)

        panel.Layout()
        panel.Fit()
        
        return panel
    def _createCPlanePage(self, parent):
        """!Create cutting planes page"""  
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        self.page['cplane'] = { 'id' : 4, 
                                'notebook' : self.foldpanelData.GetId() }
        self.win['cplane'] = {}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Cutting planes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        horSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        # planes
        horSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Active cutting plane:")),
                     flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 5)
        choice = wx.Choice(parent = panel, id = wx.ID_ANY, choices = [])
        self.win['cplane']['planes'] = choice.GetId()
        choice.Bind(wx.EVT_CHOICE, self.OnCPlaneSelection)
        horSizer.Add(item = choice, flag = wx.ALL, border = 5)
        
        # shading
        horSizer.Add(item = wx.Size(-1, -1), proportion = 1)
        horSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Shading:")),
                     flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 5)
        choices = [_("clear"),
                   _("top color"),
                   _("bottom color"),
                   _("blend"),
                   _("shaded")]
        choice = wx.Choice(parent = panel, id = wx.ID_ANY, choices = choices)
        self.win['cplane']['shading'] = choice.GetId()
        choice.Bind(wx.EVT_CHOICE, self.OnCPlaneShading)
        horSizer.Add(item = choice, flag = wx.ALL, border = 5)
        boxSizer.Add(item = horSizer, flag = wx.EXPAND)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        # cutting plane horizontal x position
        self.win['cplane']['position'] = {}
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Horizontal X:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Sets the X coordinate of the current cutting plane")
        self._createControl(panel, data = self.win['cplane']['position'], name = 'x', size = 250,
                            range = (-1000, 1000), sliderHor = True, floatSlider = True, tooltip = tooltip,
                            bind = (self.OnCPlaneChanging, self.OnCPlaneChangeDone, self.OnCPlaneChangeText))
        self.FindWindowById(self.win['cplane']['position']['x']['slider']).SetValue(0)
        self.FindWindowById(self.win['cplane']['position']['x']['text']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['x']['slider']),
                      pos = (0, 1),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['x']['text']),
                      pos = (0, 2),
                      flag = wx.ALIGN_CENTER)   
        
        # cutting plane horizontal y position
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Horizontal Y:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Sets the Y coordinate of the current cutting plane")
        self._createControl(panel, data = self.win['cplane']['position'], name = 'y', size = 250,
                            range = (-1000, 1000), sliderHor = True, floatSlider = True, tooltip = tooltip,
                            bind = (self.OnCPlaneChanging, self.OnCPlaneChangeDone, self.OnCPlaneChangeText))
        self.FindWindowById(self.win['cplane']['position']['y']['slider']).SetValue(0)
        self.FindWindowById(self.win['cplane']['position']['y']['text']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['y']['slider']),
                      pos = (1, 1),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['y']['text']),
                      pos = (1, 2),
                      flag = wx.ALIGN_CENTER)                         
        
        # cutting plane rotation
        self.win['cplane']['rotation'] = {}
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Rotation:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Rotates the current cutting plane about vertical axis")
        self._createControl(panel, data = self.win['cplane']['rotation'], name = 'rot', size = 250,
                            range = (0, 360), sliderHor = True, tooltip = tooltip,
                            bind = (self.OnCPlaneChanging, self.OnCPlaneChangeDone, self.OnCPlaneChangeText))
        self.FindWindowById(self.win['cplane']['rotation']['rot']['slider']).SetValue(180)
        self.FindWindowById(self.win['cplane']['rotation']['rot']['text']).SetValue(180)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['rotation']['rot']['slider']),
                      pos = (2, 1),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['rotation']['rot']['text']),
                      pos = (2, 2),
                      flag = wx.ALIGN_CENTER)

        # cutting plane tilt        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Tilt:")),
                      pos = (3, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Rotates the current cutting plane about horizontal axis")
        self._createControl(panel, data = self.win['cplane']['rotation'], name = 'tilt', size = 250,
                            range = (0, 360), sliderHor = True, tooltip = tooltip,
                            bind = (self.OnCPlaneChanging, self.OnCPlaneChangeDone, self.OnCPlaneChangeText))
        self.FindWindowById(self.win['cplane']['rotation']['tilt']['slider']).SetValue(0)
        self.FindWindowById(self.win['cplane']['rotation']['tilt']['text']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['rotation']['tilt']['slider']),
                      pos = (3, 1),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['rotation']['tilt']['text']),
                      pos = (3, 2),
                      flag = wx.ALIGN_CENTER)          
        
        # cutting pland height
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height:")),
                      pos = (4, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Sets the Z coordinate of the current cutting plane (only meaningful when tilt is not 0)")
        self._createControl(panel, data = self.win['cplane']['position'], name = 'z', size = 250,
                            range = (-1000, 1000), sliderHor = True, tooltip = tooltip,
                            bind = (self.OnCPlaneChanging, self.OnCPlaneChangeDone, self.OnCPlaneChangeText))
        self.FindWindowById(self.win['cplane']['position']['z']['slider']).SetValue(0)
        self.FindWindowById(self.win['cplane']['position']['z']['text']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['z']['slider']),
                      pos = (4, 1),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['cplane']['position']['z']['text']),
                      pos = (4, 2),
                      flag = wx.ALIGN_CENTER)
        
        boxSizer.Add(gridSizer, proportion = 0, flag = wx.EXPAND|wx.ALL, border = 5)
                    
        horSizer = wx.BoxSizer(wx.HORIZONTAL)
        horSizer.Add(item = wx.Size(-1, -1), proportion = 1, flag = wx.ALL, border = 5)  
        # reset
        reset = wx.Button(parent = panel, id = wx.ID_ANY, label = _("Reset"))
        self.win['cplane']['reset'] = reset.GetId()
        reset.Bind(wx.EVT_BUTTON, self.OnCPlaneReset)
        horSizer.Add(item = reset, flag = wx.ALL, border = 5)
        boxSizer.Add(horSizer, proportion = 0, flag = wx.EXPAND)            
        
        
        pageSizer.Add(boxSizer, proportion = 0, flag = wx.EXPAND)
        
        panel.SetSizer(pageSizer)
        panel.Fit()    
        
        return panel
        
    def _createConstantPage(self, parent):
        """!Create constant page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        self.page['constant'] = { 'id' : 1, 
                                'notebook' : self.foldpanelData.GetId() }
        self.win['constant'] = {}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Constant surface")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        horsizer = wx.BoxSizer(wx.HORIZONTAL)
        
        surface = wx.ComboBox(parent = panel, id = wx.ID_ANY, 
                              style = wx.CB_SIMPLE | wx.CB_READONLY,
                              choices = [])
        self.win['constant']['surface'] = surface.GetId()
        surface.Bind(wx.EVT_COMBOBOX, self.OnConstantSelection)
        horsizer.Add(surface, proportion = 1, flag = wx.EXPAND|wx.RIGHT, border = 20)

        addNew = wx.Button(panel, id = wx.ID_ANY, label = _("New"))
        addNew.Bind(wx.EVT_BUTTON, self.OnNewConstant)
        self.win['constant']['new'] = addNew.GetId()

        delete = wx.Button(panel, id = wx.ID_ANY, label = _("Delete"))
        delete.Bind(wx.EVT_BUTTON, self.OnDeleteConstant)
        self.win['constant']['delete'] = delete.GetId()
        
        horsizer.Add(item = addNew, proportion = 0, flag = wx.RIGHT|wx.LEFT, border = 3)
        horsizer.Add(item = delete, proportion = 0, flag = wx.RIGHT|wx.LEFT, border = 3)
    
        boxSizer.Add(item = horsizer, proportion = 0, flag = wx.ALL|wx.EXPAND,
                      border = 5)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        # fine resolution
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Fine resolution:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        resF = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = 3,
                           min = 1,
                           max = 100)
        resF.SetName("value")
        self.win['constant']['resolution'] = resF.GetId()
        resF.Bind(wx.EVT_SPINCTRL, self.OnSetConstantProp)
        gridSizer.Add(item = resF, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT)
        # value 
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Value:")), pos = (1, 0),
                                         flag = wx.ALIGN_CENTER_VERTICAL)
        
        value = wx.SpinCtrl(panel, id = wx.ID_ANY,
                                  min = -1e9, max = 1e9,
                                  size = (65, -1))
        self.win['constant']['value'] = value.GetId()
        value.Bind(wx.EVT_SPINCTRL, self.OnSetConstantProp)
        gridSizer.Add(item = value, pos = (1, 1))
        
        # transparency 
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Transparency:")), pos = (2, 0),
                                         flag = wx.ALIGN_CENTER_VERTICAL)
        
        transp = wx.SpinCtrl(panel, id = wx.ID_ANY,
                                  min = 0, max = 100,
                                  size = (65, -1))
        self.win['constant']['transp'] = transp.GetId()
        transp.Bind(wx.EVT_SPINCTRL, self.OnSetConstantProp)
        gridSizer.Add(item = transp, pos = (2, 1))
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Color:")), pos = (3, 0),
                                         flag = wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = (0,0,0),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        self.win['constant']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnSetConstantProp)
        gridSizer.Add(item = color, pos = (3, 1))
        boxSizer.Add(item = gridSizer, proportion = 0, flag = wx.ALL,
                      border = 5)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        panel.Fit()    
        
        return panel
        
    def _createVectorPage(self, parent):
        """!Create view settings page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        self.page['vector'] = { 'id' : 2,
                                'notebook' : self.foldpanelData.GetId() }
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['vector'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        vmaps = Select(parent = panel, type = 'vector',
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
                                         label = _("width:")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_RIGHT)
        
        width = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 1,
                            min = 1,
                            max = 100)
        width.SetValue(1)
        self.win['vector']['lines']['width'] = width.GetId()
        width.Bind(wx.EVT_SPINCTRL, self.OnVectorLines)
        gridSizer.Add(item = width, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("color:")),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = (0,0,0),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        self.win['vector']['lines']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnVectorLines)

        gridSizer.Add(item = color, pos = (0, 4), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_LEFT)
        
        # thematic mapping
        self.win['vector']['lines']['thematic'] = {}
        checkThematicColor = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                         label = _("use color for thematic mapping"))
        checkThematicWidth = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                         label = _("use width for thematic mapping"))
        self.win['vector']['lines']['thematic']['checkcolor'] = checkThematicColor.GetId()
        self.win['vector']['lines']['thematic']['checkwidth'] = checkThematicWidth.GetId()
        checkThematicColor.Bind(wx.EVT_CHECKBOX, self.OnCheckThematic)
        checkThematicWidth.Bind(wx.EVT_CHECKBOX, self.OnCheckThematic)
        checkThematicColor.SetValue(False)
        checkThematicWidth.SetValue(False)
        
        vSizer = wx.BoxSizer(wx.VERTICAL)
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        hSizer.Add(item = checkThematicColor, flag = wx.ALIGN_CENTER_VERTICAL,
                    border = 5)
        setThematic = wx.Button(parent = panel, id = wx.ID_ANY,
                                         label = _("Set options..."))
        self.win['vector']['lines']['thematic']['buttoncolor'] = setThematic.GetId()
        setThematic.Bind(wx.EVT_BUTTON, self.OnSetThematic)
        hSizer.Add(item = wx.Size(-1, -1), proportion = 1)
        hSizer.Add(item = setThematic, flag = wx.ALIGN_CENTER_VERTICAL|wx.LEFT,
                    border = 5, proportion = 0)
        vSizer.Add(hSizer, flag = wx.EXPAND)
                    
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        hSizer.Add(item = checkThematicWidth, flag = wx.ALIGN_CENTER_VERTICAL,
                    border = 5)
        setThematic = wx.Button(parent = panel, id = wx.ID_ANY,
                                         label = _("Set options..."))
        self.win['vector']['lines']['thematic']['buttonwidth'] = setThematic.GetId()
        setThematic.Bind(wx.EVT_BUTTON, self.OnSetThematic)
        hSizer.Add(item = wx.Size(-1, -1), proportion = 1)
        hSizer.Add(item = setThematic, flag = wx.ALIGN_CENTER_VERTICAL|wx.LEFT,
                    border = 5, proportion = 0)
        
        vSizer.Add(hSizer, flag = wx.EXPAND)
        gridSizer.Add(item = vSizer, flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (1, 1), span = (1, 5))
        
        # display
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Display")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_LEFT)
        
        display = wx.Choice (parent = panel, id = wx.ID_ANY, size = (-1, -1),
                             choices = [_("on surface(s):"),
                                        _("flat")])
        self.win['vector']['lines']['flat'] = display.GetId()
        display.Bind(wx.EVT_CHOICE, self.OnVectorDisplay)
        
        gridSizer.Add(item = display, flag = wx.ALIGN_CENTER_VERTICAL | 
                      wx.ALIGN_LEFT|wx.EXPAND, pos = (2, 1), span = (1,4))
        
        # height
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height above surface:")),
                      pos = (3, 5), flag = wx.ALIGN_BOTTOM|wx.EXPAND)
        
        surface = wx.CheckListBox(parent = panel, id = wx.ID_ANY, size = (-1, 60),
                                  choices = [], style = wx.LB_NEEDED_SB)
        surface.Bind(wx.EVT_CHECKLISTBOX, self.OnVectorSurface)
        
        self.win['vector']['lines']['surface'] = surface.GetId()
        gridSizer.Add(item = surface, 
                      pos = (3, 0), span = (3, 5),
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        
        self._createControl(panel, data = self.win['vector']['lines'], name = 'height', size = -1,
                            range = (0, 500), sliderHor = True,
                            bind = (self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightText))
        self.FindWindowById(self.win['vector']['lines']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['lines']['height']['text']).SetValue(0)
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['lines']['height']['slider']),
                      pos = (4, 5),  flag = wx.EXPAND|wx.ALIGN_RIGHT)
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['lines']['height']['text']),
                      pos = (5, 5),
                      flag = wx.ALIGN_CENTER)
        gridSizer.AddGrowableCol(5)
        
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
        vertSizer = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        
        # icon size
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Icon:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("size:")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL |
                      wx.ALIGN_RIGHT)
        
        if fs:
            isize = fs.FloatSpin(parent = panel, id = wx.ID_ANY,
                                 min_val = 0, max_val = 1e6,
                                 increment = 1, value = 1, style = fs.FS_RIGHT)
            isize.SetFormat("%f")
            isize.SetDigits(1)
            isize.Bind(fs.EVT_FLOATSPIN, self.OnVectorPoints)
        else:
            isize = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                                initial = 1,
                                min = 1,
                                max = 1e6)
            isize.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        isize.SetName('value')
        isize.SetValue(100)
        self.win['vector']['points']['size'] = isize.GetId()
        gridSizer.Add(item = isize, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        
        # icon color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("color:")),
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

        # icon width - seems to do nothing
##        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
##                                           label = _("width")),
##                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL |
##                      wx.ALIGN_RIGHT)
##        
##        iwidth = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
##                             initial = 1,
##                             min = 1,
##                             max = 1e6)
##        iwidth.SetName('value')
##        iwidth.SetValue(100)
##        self.win['vector']['points']['width'] = iwidth.GetId()
##        iwidth.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
##        iwidth.Bind(wx.EVT_TEXT, self.OnVectorPoints)
##        gridSizer.Add(item = iwidth, pos = (1, 2),
##                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        # icon symbol
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("symbol:")),
                      pos = (0, 5), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        isym = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                          choices = UserSettings.Get(group = 'nviz', key = 'vector',
                                                   subkey = ['points', 'marker'], internal = True))
        isym.SetName("selection")
        self.win['vector']['points']['marker'] = isym.GetId()
        isym.Bind(wx.EVT_CHOICE, self.OnVectorPoints)
        gridSizer.Add(item = isym, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos = (0, 6))
        # thematic mapping
        self.win['vector']['points']['thematic'] = {}
        checkThematicColor = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                         label = _("use color for thematic mapping"))
        checkThematicSize = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                         label = _("use size for thematic mapping"))
        self.win['vector']['points']['thematic']['checkcolor'] = checkThematicColor.GetId()
        self.win['vector']['points']['thematic']['checksize'] = checkThematicSize.GetId()
        checkThematicColor.Bind(wx.EVT_CHECKBOX, self.OnCheckThematic)
        checkThematicSize.Bind(wx.EVT_CHECKBOX, self.OnCheckThematic)
        checkThematicColor.SetValue(False)
        checkThematicSize.SetValue(False)
        
        gridSizer.Add(item = checkThematicColor, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos = (1, 1), span = (1, 5))
        setThematic = wx.Button(parent = panel, id = wx.ID_ANY,
                                         label = _("Set options..."))
        self.win['vector']['points']['thematic']['buttoncolor'] = setThematic.GetId()
        setThematic.Bind(wx.EVT_BUTTON, self.OnSetThematic)
        gridSizer.Add(item = setThematic, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 6))
                    
        gridSizer.Add(item = checkThematicSize, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT,
                      pos = (2, 1), span = (1, 5))
        setThematic = wx.Button(parent = panel, id = wx.ID_ANY,
                                         label = _("Set options..."))
        self.win['vector']['points']['thematic']['buttonsize'] = setThematic.GetId()
        setThematic.Bind(wx.EVT_BUTTON, self.OnSetThematic)
        gridSizer.Add(item = setThematic, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 6))                   
        gridSizer.AddGrowableCol(0)
        gridSizer.AddGrowableCol(2)
        gridSizer.AddGrowableCol(4)
        gridSizer.AddGrowableCol(6)
        vertSizer.Add(gridSizer, proportion = 0, flag = wx.EXPAND, border = 0)
        # high
        gridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Display on surface(s):")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height above surface:")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        surface = wx.CheckListBox(parent = panel, id = wx.ID_ANY, size = (-1, 60),
                                  choices = [], style = wx.LB_NEEDED_SB)
        surface.Bind(wx.EVT_CHECKLISTBOX, self.OnVectorSurface)
        self.win['vector']['points']['surface'] = surface.GetId()
        gridSizer.Add(item = surface, 
                      pos = (1, 0), span = (3, 1),
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        
        self._createControl(panel, data = self.win['vector']['points'], name = 'height', size = -1,
                            range = (0, 500),
                            bind = (self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightText))
        
        self.FindWindowById(self.win['vector']['points']['height']['slider']).SetValue(0)
        self.FindWindowById(self.win['vector']['points']['height']['text']).SetValue(0)
        
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['points']['height']['slider']),
                      pos = (2, 1),flag = wx.EXPAND|wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.FindWindowById(self.win['vector']['points']['height']['text']),
                      pos = (3, 1),
                      flag = wx.ALIGN_CENTER)
        gridSizer.AddGrowableCol(1)
                    
        vertSizer.Add(gridSizer, proportion = 0, flag = wx.EXPAND, border = 0)
        boxSizer.Add(item = vertSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        panel.Fit()

        return panel

    def GselectOnPopup(self, ltype, exclude = False):
        """Update gselect.Select() items"""
        maps = list()
        for layer in self.mapWindow.Map.GetListOfLayers(ltype = ltype, active = True):
            maps.append(layer.GetName())
        return maps, exclude
    
    def _createVolumePage(self, parent):
        """!Create view settings page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        self.page['volume'] = { 'id' : 3,
                                'notebook' : self.foldpanelData.GetId() }
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['volume'] = {}
        
        # selection
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("3D raster map")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        rmaps = Select(parent = panel, type = '3d-raster',
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
##        gridSizer.AddGrowableCol(4)
        
        # mode
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Mode:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent = panel, id = wx.ID_ANY, size = (-1, -1),
                          choices = [_("isosurfaces"),
                                     _("slices")])
        mode.SetSelection(0)
        mode.SetName("selection")
        mode.Bind(wx.EVT_CHOICE, self.OnVolumeMode)
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
        shade.Bind(wx.EVT_CHOICE, self.OnVolumeDrawMode)
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
        resol.Bind(wx.EVT_SPINCTRL, self.OnVolumeResolution)
        resol.Bind(wx.EVT_TEXT, self.OnVolumeResolution)
        gridSizer.Add(item = resol, pos = (0, 5))
        
        # draw wire box
        box = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                            label = _("Draw wire box"))
        box.SetName("value")
        self.win['volume']['draw']['box'] = box.GetId()
        box.Bind(wx.EVT_CHECKBOX, self.OnVolumeDrawBox)
        gridSizer.Add(item = box, pos = (1, 0), span = (1, 6))

        boxSizer.Add(item = gridSizer, proportion = 0,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        #
        # manage isosurfaces
        #
        box = wx.StaticBox(parent = panel, id = wx.ID_ANY, 
                           label = " %s " % (_("List of isosurfaces")))
        box.SetName('listStaticBox')
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # list
        isolevel = wx.CheckListBox(parent = panel, id = wx.ID_ANY,
                                   size = (300, 150))
        self.Bind(wx.EVT_CHECKLISTBOX, self.OnVolumeCheck, isolevel)
        self.Bind(wx.EVT_LISTBOX, self.OnVolumeSelect, isolevel)
        
        self.win['volume']['isosurfs'] = isolevel.GetId()
        self.win['volume']['slices'] = isolevel.GetId()
        gridSizer.Add(item = isolevel, pos = (0, 0), span = (4, 1))
        
        # buttons (add, delete, move up, move down)
        btnAdd = wx.Button(parent = panel, id = wx.ID_ADD)
        self.win['volume']['btnAdd'] = btnAdd.GetId()
        btnAdd.Bind(wx.EVT_BUTTON, self.OnVolumeAdd)
        gridSizer.Add(item = btnAdd,
                      pos = (0, 1))
        btnDelete = wx.Button(parent = panel, id = wx.ID_DELETE)
        self.win['volume']['btnDelete'] = btnDelete.GetId()
        btnDelete.Bind(wx.EVT_BUTTON, self.OnVolumeDelete)
        btnDelete.Enable(False)
        gridSizer.Add(item = btnDelete,
                      pos = (1, 1))
        btnMoveUp = wx.Button(parent = panel, id = wx.ID_UP)
        self.win['volume']['btnMoveUp'] = btnMoveUp.GetId()
        btnMoveUp.Bind(wx.EVT_BUTTON, self.OnVolumeMoveUp)
        btnMoveUp.Enable(False)
        gridSizer.Add(item = btnMoveUp,
                      pos = (2, 1))
        btnMoveDown = wx.Button(parent = panel, id = wx.ID_DOWN)
        self.win['volume']['btnMoveDown'] = btnMoveDown.GetId()
        btnMoveDown.Bind(wx.EVT_BUTTON, self.OnVolumeMoveDown)
        btnMoveDown.Enable(False)
        gridSizer.Add(item = btnMoveDown,
                      pos = (3, 1))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        # isosurface/slice 
        sizer = wx.BoxSizer()
        self.isoPanel = self._createIsosurfacePanel(panel)
        self.slicePanel = self._createSlicePanel(panel)
        sizer.Add(self.isoPanel, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 0)
        sizer.Add(self.slicePanel, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 0)
        sizer.Hide(self.slicePanel)
        pageSizer.Add(item = sizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        #
        # position
        #
        self.win['volume']['position'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # position
        self._createControl(panel, data = self.win['volume'], name = 'position',
                            range = (-10000, 10000), floatSlider = True,
                            bind = (self.OnVolumePosition, self.OnVolumePositionChanged, self.OnVolumePositionText))
        
        axis = wx.Choice (parent = panel, id = wx.ID_ANY, size = (75, -1),
                          choices = ["X",
                                     "Y",
                                     "Z"])
                                    
        reset = wx.Button(panel, id = wx.ID_ANY, label = _("Reset"))
        reset.SetToolTipString(_("Reset to default position"))
        reset.Bind(wx.EVT_BUTTON, self.OnResetVolumePosition)
        self.win['volume']['position']['reset'] = reset.GetId()
        
        self.win['volume']['position']['axis'] = axis.GetId()
        axis.SetSelection(2) # Z
        axis.Bind(wx.EVT_CHOICE, self.OnVolumeAxis)
        
        pslide = self.FindWindowById(self.win['volume']['position']['slider'])
        ptext = self.FindWindowById(self.win['volume']['position']['text'])
        ptext.SetValue('0')
        
        gridSizer.Add(item = axis, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 0))
        gridSizer.Add(item = pslide, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 1))
        gridSizer.Add(item = ptext, flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 2))
        gridSizer.Add(item = reset, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, pos = (0, 3))
        gridSizer.AddGrowableCol(3)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        panel.SetSizer(pageSizer)
        panel.Fit()
        
        return panel
       
        
    def _createLightPage(self, parent):
        """!Create light page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        
        self.page['light'] = { 'id' : 0, 
                               'notebook' : self.foldpanelAppear.GetId() }
        self.win['light'] = {}
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        show = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                           label = _("Show light model"))
        show.Bind(wx.EVT_CHECKBOX, self.OnShowLightModel)
        show.SetValue(True)
        self._display.showLight = True
        pageSizer.Add(item = show, proportion = 0,
                      flag = wx.ALL, border = 3)
##        surface = wx.CheckBox(parent = panel, id = wx.ID_ANY,
##                              label = _("Follow source viewpoint"))
##        pageSizer.Add(item = surface, proportion = 0,
##                      flag = wx.ALL, border = 3)
        
        # position
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Light source position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        posSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        self._createCompass(panel = panel, sizer = posSizer, type = 'light')
        
        pos = LightPositionWindow(panel, id = wx.ID_ANY, size = (175, 175),
                                  mapwindow = self.mapWindow)
        self.win['light']['position'] = pos.GetId()
        posSizer.Add(item = pos,
                     pos = (1, 1), flag = wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = posSizer, pos = (0, 0))
        
        # height
        tooltip = _("Adjusts the light height")
        self._createControl(panel, data = self.win['light'], name = 'z', sliderHor = False,
                            range = (0, 100), tooltip = tooltip,
                            bind = (self.OnLightChange, self.OnLightChanged, self.OnLightChange))
        
        heightSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        heightSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:")),
                      pos = (0, 0), flag = wx.ALIGN_LEFT, span = (1, 2))
        heightSizer.Add(item = self.FindWindowById(self.win['light']['z']['slider']),
                        flag = wx.ALIGN_RIGHT, pos = (1, 0))
        heightSizer.Add(item = self.FindWindowById(self.win['light']['z']['text']),
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos = (1, 1))
        
        gridSizer.Add(item = heightSizer, pos = (0, 2), flag = wx.ALIGN_RIGHT)
        
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
        self.win['light']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnLightColor)
        gridSizer.Add(item = color, pos = (0, 2))

        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Brightness:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Adjusts the brightness of the light")
        self._createControl(panel, data = self.win['light'], name = 'bright', size = 300,
                            range = (0, 100), tooltip = tooltip, 
                            bind = (self.OnLightValue, self.OnLightChanged, self.OnLightValue))
        gridSizer.Add(item = self.FindWindowById(self.win['light']['bright']['slider']),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.FindWindowById(self.win['light']['bright']['text']),
                      pos = (1, 2),
                      flag = wx.ALIGN_CENTER)
        gridSizer.Add(item = wx.StaticText(panel, id = wx.ID_ANY, label = _("Ambient:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        tooltip = _("Adjusts the ambient light")
        self._createControl(panel, data = self.win['light'], name = 'ambient', size = 300,
                            range = (0, 100), tooltip = tooltip,
                            bind = (self.OnLightValue, self.OnLightChanged, self.OnLightValue))
        gridSizer.Add(item = self.FindWindowById(self.win['light']['ambient']['slider']),
                      pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.FindWindowById(self.win['light']['ambient']['text']),
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
        panel.Layout()
        panel.Fit()
        
        return panel

    def _createFringePage(self, parent):
        """!Create fringe page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        
        self.page['fringe'] = { 'id' : 1,
                                'notebook' : self.foldpanelAppear.GetId() }
        self.win['fringe'] = {}

        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        # selection
        rbox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("Surface")))
        rboxSizer = wx.StaticBoxSizer(rbox, wx.VERTICAL)
        rmaps = Select(parent = panel, type = 'raster',
                       onPopup = self.GselectOnPopup)
        rmaps.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnSetSurface)
        self.win['fringe']['map'] = rmaps.GetId()
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
        panel.Layout()
        panel.Fit()

        return panel
    
    def _createDecorationPage(self, parent):
        """!Create decoration (north arrow, scalebar, legend) page"""
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        
        self.page['decoration'] = { 'id' : 2,
                                    'notebook' : self.foldpanelAppear.GetId()}
        self.win['decoration'] = {}

        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        # north arrow
        self.win['decoration']['arrow'] = {}
        nabox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("North Arrow")))
        naboxSizer = wx.StaticBoxSizer(nabox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        # size
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Arrow length (in map units):")),
                      pos = (0,0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        sizeCtrl = NumTextCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1), style = wx.TE_PROCESS_ENTER)
        gridSizer.Add(sizeCtrl, pos = (0, 2))
        self.win['decoration']['arrow']['size'] = sizeCtrl.GetId()
        sizeCtrl.Bind(wx.EVT_TEXT_ENTER, self.OnDecorationProp)
        sizeCtrl.Bind(wx.EVT_KILL_FOCUS, self.OnDecorationProp)
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Arrow color:")),
                      pos = (1,0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                  size = globalvar.DIALOG_COLOR_SIZE)
        gridSizer.Add(color, pos = (1, 2))
        self.win['decoration']['arrow']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnDecorationProp)
        
        # control
        toggle = wx.ToggleButton(parent = panel, id = wx.ID_ANY, label = _("Place arrow"))
        gridSizer.Add(item = toggle, pos = (2, 0))
        toggle.Bind(wx.EVT_TOGGLEBUTTON, self.OnDecorationPlacement)
        self.win['decoration']['arrow']['place'] = toggle.GetId()
        toggle.SetName('placeArrow')

        delete = wx.Button(parent = panel, id = wx.ID_ANY, label = _("Delete"))
        self.win['decoration']['arrow']['delete'] = delete.GetId()
        gridSizer.Add(item = delete, pos = (2, 1))
        delete.Bind(wx.EVT_BUTTON, self.OnArrowDelete)
        shown = self.mapWindow.decoration['arrow']['show']
        delete.Enable(shown)
        naboxSizer.Add(item = gridSizer, proportion = 0, flag = wx.EXPAND, border = 3)
        pageSizer.Add(item = naboxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        
        # scale bars
        self.win['decoration']['scalebar'] = {}
        nabox = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                             label = " %s " % (_("Scale bar")))
        naboxSizer = wx.StaticBoxSizer(nabox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        # size
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Scale bar length (in map units):")),
                      pos = (0,0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        sizeCtrl = NumTextCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1), style = wx.TE_PROCESS_ENTER)
        gridSizer.Add(sizeCtrl, pos = (0, 2))
        self.win['decoration']['scalebar']['size'] = sizeCtrl.GetId()
        sizeCtrl.Bind(wx.EVT_TEXT_ENTER, self.OnDecorationProp)
        sizeCtrl.Bind(wx.EVT_KILL_FOCUS, self.OnDecorationProp)
        
        # color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Scale bar color:")),
                      pos = (1,0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                  size = globalvar.DIALOG_COLOR_SIZE)
        gridSizer.Add(color, pos = (1, 2))
        self.win['decoration']['scalebar']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnDecorationProp)
        
        # control
        toggle = wx.ToggleButton(parent = panel, id = wx.ID_ANY, label = _("Place new scale bar"))
        gridSizer.Add(item = toggle, pos = (2, 0))
        toggle.Bind(wx.EVT_TOGGLEBUTTON, self.OnDecorationPlacement)
        self.win['decoration']['scalebar']['place'] = toggle.GetId()
        toggle.SetName('placeScalebar')

        scalebarChoice = wx.Choice(parent = panel, id = wx.ID_ANY, choices = [])
        self.win['decoration']['scalebar']['choice'] = scalebarChoice.GetId()
        gridSizer.Add(item = scalebarChoice, pos = (3, 0), flag = wx.EXPAND)
        delete = wx.Button(parent = panel, id = wx.ID_ANY, label = _("Delete"))
        self.win['decoration']['scalebar']['delete'] = delete.GetId()
        gridSizer.Add(item = delete, pos = (3, 1))
        delete.Bind(wx.EVT_BUTTON, self.OnScalebarDelete)
        naboxSizer.Add(item = gridSizer, proportion = 0, flag = wx.EXPAND, border = 3)
        pageSizer.Add(item = naboxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)      

        self.DisableScalebarControls()

        panel.SetSizer(pageSizer)
        panel.Layout()
        panel.Fit()

        return panel
    
    def GetLayerData(self, nvizType, nameOnly = False):
        """!Get nviz data"""
        name = self.FindWindowById(self.win[nvizType]['map']).GetValue()
        if nameOnly:
            return name
        
        if nvizType == 'surface' or nvizType == 'fringe':
            return self._getLayerPropertiesByName(name, mapType = 'raster')
        elif nvizType == 'vector':
            return self._getLayerPropertiesByName(name, mapType = 'vector')
        elif nvizType == 'volume':
            return self._getLayerPropertiesByName(name, mapType = '3d-raster')
        
        return None

    def _getMapLayerByName(self, name, mapType):
        """!Get layer (render.Layer) by name and type.

        @param name layer name
        @param mapType map type (raster, vector, 3d-raster)
        """
        layers = self.mapWindow.Map.GetListOfLayers(ltype = mapType, name = name)
        if layers:
            return layers[0]
        return None

    def _getLayerPropertiesByName(self, name, mapType):
        """!Get nviz properties stored in layertree items by name and type.

        @param name layer name
        @param mapType map type (raster, vector, 3d-raster)
        """
        tree = self.parent.GetLayerTree()
        items = tree.FindItemByData(key = 'name', value = name)
        if not items:
            return None
        for item in items:
            if tree.GetLayerInfo(item, key = 'type') == mapType:
                return tree.GetLayerInfo(item, key = 'nviz')
        return None

    def OnRecord(self, event):
        """!Animation: start recording"""
        anim = self.mapWindow.GetAnimation()
        if not anim.IsPaused():
            if anim.Exists() and not anim.IsSaved():
                msg = _("Do you want to record new animation without saving the previous one?")
                dlg = wx.MessageDialog(parent = self,
                                       message = msg,
                                       caption =_("Animation already axists"),
                                       style = wx.YES_NO | wx.CENTRE)
                if dlg.ShowModal() == wx.ID_NO:
                    dlg.Destroy()
                    return
                
        
            anim.Clear()
            self.UpdateFrameIndex(0)
            self.UpdateFrameCount()
            
        anim.SetPause(False)
        anim.SetMode(mode = 'record')
        anim.Start()
        
        self.FindWindowById(self.win['anim']['play']).Disable()
        self.FindWindowById(self.win['anim']['record']).Disable()
        self.FindWindowById(self.win['anim']['pause']).Enable()
        self.FindWindowById(self.win['anim']['stop']).Enable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        
    def OnPlay(self, event):
        """!Animation: replay"""
        anim = self.mapWindow.GetAnimation()
        anim.SetPause(False)
        anim.SetMode(mode = 'play')
        anim.Start()
        
        self.FindWindowById(self.win['anim']['play']).Disable()
        self.FindWindowById(self.win['anim']['record']).Disable()
        self.FindWindowById(self.win['anim']['pause']).Enable()
        self.FindWindowById(self.win['anim']['stop']).Enable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Enable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Enable()
        
    def OnStop(self, event):
        """!Animation: stop recording/replaying"""
        anim = self.mapWindow.GetAnimation()
        anim.SetPause(False)
        if anim.GetMode() == 'save':
            anim.StopSaving()
        if anim.IsRunning():
            anim.Stop()
        
        self.UpdateFrameIndex(0)
        
        self.FindWindowById(self.win['anim']['play']).Enable()
        self.FindWindowById(self.win['anim']['record']).Enable()
        self.FindWindowById(self.win['anim']['pause']).Disable()
        self.FindWindowById(self.win['anim']['stop']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        
    def OnPause(self, event):
        """!Pause animation"""
        anim = self.mapWindow.GetAnimation()
        
        anim.SetPause(True)
        mode = anim.GetMode()
        if anim.IsRunning():
            anim.Pause()
            
        if mode == "record":
            self.FindWindowById(self.win['anim']['play']).Disable()
            self.FindWindowById(self.win['anim']['record']).Enable()
            self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
            self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        elif mode == 'play':
            self.FindWindowById(self.win['anim']['record']).Disable()
            self.FindWindowById(self.win['anim']['play']).Enable()
            self.FindWindowById(self.win['anim']['frameIndex']['slider']).Enable()
            self.FindWindowById(self.win['anim']['frameIndex']['text']).Enable()
        
        self.FindWindowById(self.win['anim']['pause']).Disable()
        self.FindWindowById(self.win['anim']['stop']).Enable()

        
    def OnFrameIndex(self, event):
        """!Frame index changed (by slider)"""
        index = event.GetInt()
        self.UpdateFrameIndex(index = index, sliderWidget = False)
        
    def OnFrameIndexText(self, event):
        """!Frame index changed by (textCtrl)"""
        index = event.GetValue()
        self.UpdateFrameIndex(index = index, textWidget = False)
        
    def OnFPS(self, event):
        """!Frames per second changed"""
        anim = self.mapWindow.GetAnimation()
        anim.SetFPS(event.GetInt())
        
    def UpdateFrameIndex(self, index, sliderWidget = True, textWidget = True, goToFrame = True):
        """!Update frame index"""
        anim = self.mapWindow.GetAnimation()
        
        # check index
        frameCount = anim.GetFrameCount()
        if index >= frameCount:
            index = frameCount - 1
        if index < 0:
            index = 0
            
        if sliderWidget:
            slider = self.FindWindowById(self.win['anim']['frameIndex']['slider'])
            slider.SetValue(index)
        if textWidget:
            text = self.FindWindowById(self.win['anim']['frameIndex']['text'])
            text.SetValue(int(index))
        
        # if called from tool window, update frame
        if goToFrame:
            anim.GoToFrame(int(index))
            
    def UpdateFrameCount(self):
        """!Update frame count label"""
        anim = self.mapWindow.GetAnimation()
        count = anim.GetFrameCount()
        self.FindWindowById(self.win['anim']['info']).SetLabel(str(count))
        
    def OnAnimationFinished(self, mode):
        """!Animation finished"""
        anim = self.mapWindow.GetAnimation()
        self.UpdateFrameIndex(index = 0)
        
        slider = self.FindWindowById(self.win['anim']['frameIndex']['slider'])
        text = self.FindWindowById(self.win['anim']['frameIndex']['text'])
        
        if mode == 'record':
            count = anim.GetFrameCount()
            slider.SetMax(count)
            self.UpdateFrameCount()
            
        self.FindWindowById(self.win['anim']['pause']).Disable()
        self.FindWindowById(self.win['anim']['stop']).Disable()
        self.FindWindowById(self.win['anim']['record']).Enable()
        self.FindWindowById(self.win['anim']['play']).Enable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        self.FindWindowById(self.win['anim']['save']['image']['confirm']).Enable()
        
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
    def OnAnimationUpdateIndex(self, index, mode):
        """!Animation: frame index changed"""
        if mode == 'record':
            self.UpdateFrameCount()
        elif mode == 'play':
            self.UpdateFrameIndex(index = index, goToFrame = False)
        
    def OnSaveAnimation(self, event):
        """!Save animation as a sequence of images"""
        anim = self.mapWindow.GetAnimation()
        
        prefix = self.FindWindowById(self.win['anim']['save']['image']['prefix']).GetValue()
        format = self.FindWindowById(self.win['anim']['save']['image']['format']).GetSelection()
        dir = self.FindWindowById(self.win['anim']['save']['image']['dir']).GetValue()
        
        if not prefix:
            GMessage(parent = self,
                          message = _("No file prefix given."))
            return
        elif not os.path.exists(dir):
            GMessage(parent = self,
                          message = _("Directory %s does not exist.") % dir)
            return
            
        self.FindWindowById(self.win['anim']['pause']).Disable()
        self.FindWindowById(self.win['anim']['stop']).Enable()
        self.FindWindowById(self.win['anim']['record']).Disable()
        self.FindWindowById(self.win['anim']['play']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        
        self.FindWindowById(self.win['anim']['save']['image']['confirm']).Disable()
        
        anim.SaveAnimationFile(path = dir, prefix = prefix, format = format)
        
    def OnNewConstant(self, event):
        """!Create new surface with constant value"""
        #TODO settings
        name = self.mapWindow.NewConstant()
        win = self.FindWindowById(self.win['constant']['surface'])
        name = _("constant#") + str(name)
        win.Append(name)
        win.SetStringSelection(name)
        self.OnConstantSelection(None)
        self.EnablePage(name = 'constant', enabled = True)
        
        self.mapWindow.Refresh(eraseBackground = False)
        
        # need to update list of surfaces in vector page
        for vtype in ('points', 'lines'):
            checklist = self.FindWindowById(self.win['vector'][vtype]['surface'])
            checklist.Append(name)
        win = self.FindWindowById(self.win['vector']['map'])
        win.SetValue(win.GetValue())
                

    def OnDeleteConstant(self, event):
        """!Delete selected constant surface"""
        layerIdx = self.FindWindowById(self.win['constant']['surface']).GetSelection()
        if layerIdx == wx.NOT_FOUND:
            return
        name = self.FindWindowById(self.win['constant']['surface']).GetStringSelection()
        self.mapWindow.DeleteConstant(layerIdx)
        win = self.FindWindowById(self.win['constant']['surface'])
        win.Delete(layerIdx)
        if win.IsEmpty():
            win.SetValue("")
            self.EnablePage(name = 'constant', enabled = False)
        else:
            win.SetSelection(0)
            self.OnConstantSelection(None)
            
        # need to update list of surfaces in vector page
        for vtype in ('points', 'lines'):
            checklist = self.FindWindowById(self.win['vector'][vtype]['surface'])
            checklist.Delete(checklist.FindString(name))
            
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
    
    def OnConstantSelection(self, event):
        """!Constant selected"""
        layerIdx = self.FindWindowById(self.win['constant']['surface']).GetSelection()
        if layerIdx == wx.NOT_FOUND:
            return
        name = _("constant#") + str(layerIdx + 1)
        data = self.mapWindow.constants[layerIdx]
        for attr, value in data['constant'].iteritems():
            if attr == 'color':
                value = self._getColorFromString(value)
            if attr in ('color', 'value', 'resolution', 'transp'):
                if attr == 'transp':
                    self.FindWindowById(self.win['constant'][attr]).SetValue(self._getPercent(value))
                self.FindWindowById(self.win['constant'][attr]).SetValue(value)
        
    def OnSetConstantProp(self, event):
        """!Change properties (color, value, resolution)
            of currently selected constant surface"""
        layerIdx = self.FindWindowById(self.win['constant']['surface']).GetSelection()
        if layerIdx == wx.NOT_FOUND:
            return
        data = self.mapWindow.constants[layerIdx]
        for attr in ('resolution', 'value', 'transp'):
            data['constant'][attr] = self.FindWindowById(self.win['constant'][attr]).GetValue()
        data['constant']['color'] = self._getColorString(
                self.FindWindowById(self.win['constant']['color']).GetValue())
        data['constant']['transp'] = self._getPercent(data['constant']['transp'], toPercent = False)
        
       # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event) 
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnFringe(self, event):
        """!Show/hide fringe"""
        data = self.GetLayerData('fringe')['surface']
        
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
        data[winName] = self.FindWindowById(event.GetId()).GetValue()
        for w in win[winName].itervalues():
            self.FindWindowById(w).SetValue(data[winName])
        
        event.Skip()
        
    def AdjustSliderRange(self, slider, value):
        minim, maxim = slider.GetRange()
        if not (minim <= value <= maxim):
            slider.SetRange(min(minim, value), max(maxim, value))
    
    def _createIsosurfacePanel(self, parent):
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        
        vSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Isosurface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        self.win['volume']['attr'] = {}
        inout = wx.CheckBox(parent = panel, id = wx.ID_ANY, 
                            label = _("toggle normal direction"))
        gridSizer.Add(item = inout, pos = (0,0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL)
        inout.Bind(wx.EVT_CHECKBOX, self.OnInOutMode)
        self.win['volume']['inout'] = inout.GetId()
        
        row = 1
        for code, attrb in (('topo', _("Isosurface value")),
                            ('color', _("Color")),
                            ('mask', _("Mask")),
                            ('transp', _("Transparency")),
                            ('shine', _("Shininess"))):
            self.win['volume'][code] = {} 
            # label
            colspan = 1
            if code == 'topo':
                colspan = 2
            gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                             label = attrb + ':'),
                          pos = (row, 0), span = (1, colspan),flag = wx.ALIGN_CENTER_VERTICAL)
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
                map = Select(parent = panel, id = wx.ID_ANY,
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
                color = UserSettings.Get(group = 'nviz', key = 'volume', subkey = ['color', 'value'])
                value = csel.ColourSelect(panel, id = wx.ID_ANY,
                                          colour = color,
                                          size = globalvar.DIALOG_COLOR_SIZE)
                value.Bind(csel.EVT_COLOURSELECT, self.OnVolumeIsosurfMap)
                value.SetName('color')
            elif code == 'mask':
                value = None
            elif code == 'topo':
                value = NumTextCtrl(parent = panel, id = wx.ID_ANY, size = (200, -1),
                            style = wx.TE_PROCESS_ENTER)
                value.Bind(wx.EVT_TEXT_ENTER, self.OnVolumeIsosurfMap)
                value.Bind(wx.EVT_KILL_FOCUS, self.OnVolumeIsosurfMap)
##                value.Bind(wx.EVT_TEXT, self.OnVolumeIsosurfMap)
            else:
                size = (65, -1)
                value = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = size,
                                    initial = 0)
                if code == 'topo':
                    value.SetRange(minVal = -1e9, maxVal = 1e9)
                elif code in ('shine', 'transp'):
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
        vSizer.Add(item = boxSizer, proportion = 1,
                     flag = wx.EXPAND, border = 0)
        panel.SetSizer(vSizer)
        
        return panel
    
    def _createSlicePanel(self, parent):
        panel = wx.Panel(parent = parent, id = wx.ID_ANY)
        
        vSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Slice attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        hSizer = wx.BoxSizer()
        
        self.win['volume']['slice'] = {}
        hSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                      label = _("Slice parallel to axis:")), proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, border = 3)
        axes = wx.Choice(parent = panel, id = wx.ID_ANY, size = (65, -1), choices = ("X", "Y", "Z"))
        hSizer.Add(axes, proportion = 0, flag = wx.ALIGN_LEFT|wx.LEFT, border = 3)
        self.win['volume']['slice']['axes'] = axes.GetId()
        axes.Bind(wx.EVT_CHOICE, self.OnVolumeSliceAxes)
        boxSizer.Add(hSizer, proportion = 0, flag = wx.ALL|wx.EXPAND, border = 3)
        
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # text labels
        for i in range(2):
            label = wx.StaticText(parent = panel, id = wx.ID_ANY)
            label.SetName('label_edge_' + str(i))
            gridSizer.Add(item = label, pos = (0, i + 1),
                          flag = wx.ALIGN_CENTER)
        for i in range(2,4):
            label = wx.StaticText(parent = panel, id = wx.ID_ANY)
            label.SetName('label_edge_' + str(i))
            gridSizer.Add(item = label, pos = (3, i -1),
                          flag = wx.ALIGN_CENTER)
        for i in range(2):
            label = wx.StaticText(parent = panel, id = wx.ID_ANY)
            label.SetName('label_coord_' + str(i))
            gridSizer.Add(item = label, pos = (i + 1, 0),
                          flag = wx.ALIGN_CENTER_VERTICAL)
        label = wx.StaticText(parent = panel, id = wx.ID_ANY)
        label.SetName('label_coord_2')
        gridSizer.Add(item = label, pos = (4, 0), 
                          flag = wx.ALIGN_CENTER_VERTICAL)
        # sliders
        for i, coord in enumerate(('x1', 'x2')):
            slider = wx.Slider(parent = panel, id = wx.ID_ANY, minValue = 0, maxValue = 100, value = 0)
            self.win['volume']['slice']['slider_' + coord] = slider.GetId()
            slider.Bind(wx.EVT_SPIN, self.OnSlicePositionChange)
            slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSlicePositionChanged)
            gridSizer.Add(item = slider, pos = (1, i + 1), 
                          flag = wx.ALIGN_CENTER|wx.EXPAND)
                        
        for i, coord in enumerate(('y1', 'y2')):
            slider = wx.Slider(parent = panel, id = wx.ID_ANY, minValue = 0, maxValue = 100, value = 0)
            self.win['volume']['slice']['slider_' + coord] = slider.GetId()
            slider.Bind(wx.EVT_SPIN, self.OnSlicePositionChange)
            slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSlicePositionChanged)
            gridSizer.Add(item = slider, pos = (2, i + 1), 
                          flag = wx.ALIGN_CENTER|wx.EXPAND)
        
        for i, coord in enumerate(('z1', 'z2')):
            slider = wx.Slider(parent = panel, id = wx.ID_ANY, minValue = 0, maxValue = 100, value = 0)
            self.win['volume']['slice']['slider_' + coord] = slider.GetId()
            slider.Bind(wx.EVT_SPIN, self.OnSlicePositionChange)
            slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSlicePositionChanged)
            gridSizer.Add(item = slider, pos = (4,i+1), 
                          flag = wx.ALIGN_CENTER|wx.EXPAND)
                        
        gridSizer.AddGrowableCol(0,1)
        gridSizer.AddGrowableCol(1,2)
        gridSizer.AddGrowableCol(2,2)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                     flag = wx.ALL | wx.EXPAND, border = 3)
        
        # transparency, reset
        hSizer = wx.BoxSizer()
        hSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                      label = _("Transparency:")), proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.RIGHT|wx.TOP, border = 7)
        spin = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           min = 0, max = 100, initial = 0)
        spin.Bind(wx.EVT_SPINCTRL, self.OnSliceTransparency)
        self.win['volume']['slice']['transp'] = spin.GetId()
        hSizer.Add(item = spin, proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP, border = 7)
                    
        hSizer.Add(item = wx.Size(-1, -1), proportion = 1,
                      flag = wx.EXPAND)
        reset = wx.Button(parent = panel, id = wx.ID_ANY, label = _("Reset"))
        reset.Bind(wx.EVT_BUTTON, self.OnSliceReset)
        self.win['volume']['slice']['reset'] = reset.GetId()
        hSizer.Add(item = reset, proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL|wx.TOP, border = 7)
        
        boxSizer.Add(hSizer, proportion = 0, flag = wx.ALL|wx.EXPAND, border = 3)
        panel.SetSizer(boxSizer)
        
        return panel
    
    def _createControl(self, parent, data, name, range, tooltip = None, bind = (None, None, None),
                       sliderHor = True, size = 200, floatSlider = False):
        """!Add control (Slider + TextCtrl)"""
        data[name] = dict()
        if sliderHor:
            style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                wx.SL_BOTTOM
            sizeW = (size, -1)
        else:
            style = wx.SL_VERTICAL | wx.SL_AUTOTICKS | \
                wx.SL_INVERSE
            sizeW = (-1, size)
        
        kwargs = dict(parent = parent, id = wx.ID_ANY,
                           minValue = range[0],
                           maxValue = range[1],
                           style = style,
                           size = sizeW)
        if floatSlider:
            slider = FloatSlider(**kwargs)
        else:
            slider = wx.Slider(**kwargs)
            
        slider.SetName('slider')
        if bind[0]:
            #EVT_SCROLL emits event after slider is released, EVT_SPIN not
            slider.Bind(wx.EVT_SPIN, bind[0])
        
        if bind[1]:
            slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, bind[1]) 
        data[name]['slider'] = slider.GetId()
        
        text = NumTextCtrl(parent = parent, id = wx.ID_ANY, size = (65, -1),
                            style = wx.TE_PROCESS_ENTER)
        
        text.SetName('text')
        if tooltip:
            text.SetToolTipString(tooltip)
        if bind[2]:
            text.Bind(wx.EVT_TEXT_ENTER, bind[2])
            text.Bind(wx.EVT_KILL_FOCUS, bind[2])
        
        data[name]['text'] = text.GetId()
        
    def _createCompass(self, panel, sizer, type):
        """!Create 'compass' widget for light and view page"""
        w = wx.Button(panel, id = wx.ID_ANY, label = _("W"))
        n = wx.Button(panel, id = wx.ID_ANY, label = _("N"))
        s = wx.Button(panel, id = wx.ID_ANY, label = _("S"))
        e = wx.Button(panel, id = wx.ID_ANY, label = _("E"))
        nw = wx.Button(panel, id = wx.ID_ANY, label = _("NW"))
        ne = wx.Button(panel, id = wx.ID_ANY, label = _("NE"))
        se = wx.Button(panel, id = wx.ID_ANY, label = _("SE"))
        sw = wx.Button(panel, id = wx.ID_ANY, label = _("SW"))
        padding = 15
        if sys.platform == 'darwin':
            padding = 20
        minWidth = sw.GetTextExtent(sw.GetLabel())[0] + padding
        for win, name in zip((w, n, s, e, nw, ne, se, sw),
                        ('w', 'n', 's', 'e', 'nw', 'ne', 'se', 'sw')):
            win.SetMinSize((minWidth, -1))
            win.Bind(wx.EVT_BUTTON, self.OnLookFrom)
            win.SetName(type + '_' + name)
        sizer.Add(item = nw, pos = (0, 0), flag = wx.ALIGN_CENTER)
        sizer.Add(item = n, pos = (0, 1), flag = wx.ALIGN_CENTER)
        sizer.Add(item = ne, pos = (0, 2), flag = wx.ALIGN_CENTER)
        sizer.Add(item = e, pos = (1, 2), flag = wx.ALIGN_CENTER)
        sizer.Add(item = se, pos = (2, 2), flag = wx.ALIGN_CENTER)
        sizer.Add(item = s, pos = (2, 1), flag = wx.ALIGN_CENTER)
        sizer.Add(item = sw, pos = (2, 0), flag = wx.ALIGN_CENTER)
        sizer.Add(item = w, pos = (1, 0), flag = wx.ALIGN_CENTER)
        
        
        
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
                try:
                    if control == 'height':
                        value = int(self.mapWindow.iview[control]['value'])
                    else:
                        value = self.mapWindow.view[control]['value']
                except KeyError:
                    value = -1
                        
                self.FindWindowById(win).SetValue(value)
        
        viewWin = self.FindWindowById(self.win['view']['position'])
        x, y = viewWin.UpdatePos(self.mapWindow.view['position']['x'],
                                 self.mapWindow.view['position']['y'])
        viewWin.Draw(pos = (x, y), scale = True)
        viewWin.Refresh(False)
        
        color = self._getColorString(self.mapWindow.view['background']['color'])
        self._display.SetBgColor(str(color))
        
        self.Update()
        
        self.mapWindow.Refresh(eraseBackground = False)
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(True)
        
    def OnShowLightModel(self, event):
        """!Show light model"""
        self._display.showLight = event.IsChecked()
        self._display.DrawLightingModel()
        
    def OnLightChange(self, event):
        """!Position of the light changing"""
        winName = self.__GetWindowName(self.win['light'], event.GetId())
        if not winName:
            return
        
        value = self.FindWindowById(event.GetId()).GetValue()
        
        self.mapWindow.light['position']['z'] = value
        for win in self.win['light'][winName].itervalues():
            self.FindWindowById(win).SetValue(value)
            
        self.PostLightEvent()
        
        event.Skip()
        
    def OnLightChanged(self, event):
        """!Light changed"""
        self.PostLightEvent(refresh = True)
        
    def OnLightColor(self, event):
        """!Color of the light changed"""
        self.mapWindow.light['color'] = tuple(event.GetValue())
        
        self.PostLightEvent(refresh = True)
        
        event.Skip()
        
    def OnLightValue(self, event):
        """!Light brightness/ambient changing"""
        data = self.mapWindow.light
        self.OnScroll(event, self.win['light'], data)
        
        self.PostLightEvent()
        event.Skip()
        
    def OnBgColor(self, event):
        """!Background color changed"""
        color = event.GetValue()
        self.mapWindow.view['background']['color'] = tuple(color)
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
        self._display.SetBgColor(str(color))
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnSetSurface(self, event):
        """!Surface selected, currently used for fringes"""
        name = event.GetString()
        try:
            data = self._getLayerPropertiesByName(name, mapType = 'raster')['surface']
        except:
            self.EnablePage('fringe', False)
            return
        
        layer = self._getMapLayerByName(name, mapType = 'raster')
        self.EnablePage('fringe', True)
        
    def OnSetRaster(self, event):
        """!Raster map selected, update surface page"""
        name = event.GetString()
        try:
            data = self._getLayerPropertiesByName(name, mapType = 'raster')['surface']
        except TypeError, e:
            self.EnablePage('surface', False)
            return

        layer = self._getMapLayerByName(name, mapType = 'raster')
        self.EnablePage('surface', True)
        self.UpdateSurfacePage(layer, data, updateName = False)
        
    def OnSetVector(self, event):
        """!Vector map selected, update properties page"""
        name = event.GetString()
        try:
            data = self._getLayerPropertiesByName(name, mapType = 'vector')['vector']
        except:
            self.EnablePage('vector', False)
            return
        layer = self._getMapLayerByName(name, mapType = 'vector')
        self.EnablePage('vector', True)
        self.UpdateVectorPage(layer, data, updateName = False)

    def OnSetRaster3D(self, event):
        """!3D Raster map selected, update surface page"""
        name = event.GetString()
        try:
            data = self._getLayerPropertiesByName(name, mapType = '3d-raster')['volume']
        except:
            self.EnablePage('volume', False)
            return
        
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        self.EnablePage('volume', True)
        self.UpdateVolumePage(layer, data, updateName = False)
        
    def OnViewChange(self, event):
        """!Change view, render in quick mode"""
        # find control
        winName = self.__GetWindowName(self.win['view'], event.GetId())
        if not winName:
            return
        
        value = self.FindWindowById(event.GetId()).GetValue()
        slider = self.FindWindowById(self.win['view'][winName]['slider'])
        self.AdjustSliderRange(slider = slider, value = value)
        
        if winName == 'height':
            view = self.mapWindow.iview # internal
        else:
            view = self.mapWindow.view
        
        if winName == 'z-exag' and value >= 0:
            self.PostViewEvent(zExag = True)
        else:
            self.PostViewEvent(zExag = False)
        
        if winName in ('persp', 'twist'):
            convert = int
        else:
            convert = float
        
        view[winName]['value'] = convert(value)
            
        for win in self.win['view'][winName].itervalues():
            self.FindWindowById(win).SetValue(value)

        self.mapWindow.iview['dir']['use'] = False
        self.mapWindow.render['quick'] = True
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnViewChanged(self, event):
        """!View changed, render in full resolution"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        self.UpdateSettings()
        try:# when calling event = None
            event.Skip()
        except AttributeError:
            pass
            
    def OnViewChangedText(self, event):
        """!View changed, render in full resolution""" 
        self.mapWindow.render['quick'] = False
        self.OnViewChange(event)
        self.OnViewChanged(None)
        self.Update()
        
        event.Skip()
    
    def OnLookAt(self, event):
        """!Look here/center"""
        name = self.FindWindowById(event.GetId()).GetName()
        if name == 'center':
            self._display.LookAtCenter()
            focus = self.mapWindow.iview['focus']
            focus['x'], focus['y'], focus['z'] = self._display.GetFocus()
            self.mapWindow.saveHistory = True
            self.mapWindow.Refresh(False)
        elif name == 'top':
            self.mapWindow.view['position']['x'] = 0.5
            self.mapWindow.view['position']['y'] = 0.5
            self.PostViewEvent(zExag = True)
            self.UpdateSettings()
            self.mapWindow.Refresh(False)
        else: # here
            if self.FindWindowById(event.GetId()).GetValue():
                self.mapDisplay.Raise()
                self.mapWindow.mouse['use'] = 'lookHere'
                self.mapWindow.SetCursor(self.mapWindow.cursors["cross"])
            else:
                self.mapWindow.mouse['use'] = 'default'
                self.mapWindow.SetCursor(self.mapWindow.cursors['default'])
            
    def OnResetView(self, event):
        """!Reset to default view (view page)"""
        self.mapWindow.ResetView()
        self.UpdateSettings()
        self.mapWindow.Refresh(False)
        
    def OnResetSurfacePosition(self, event):
        """!Reset position of surface"""
        
        for win in self.win['surface']['position'].itervalues():
            if win == self.win['surface']['position']['axis']:
                self.FindWindowById(win).SetSelection(2) # Z
            elif win == self.win['surface']['position']['reset']:
                continue
            else:
                self.FindWindowById(win).SetValue(0)
                
        data = self.GetLayerData('surface')
        data['surface']['position']['x'] = 0
        data['surface']['position']['y'] = 0
        data['surface']['position']['z'] = 0
        data['surface']['position']['update'] = None
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
            
    def OnLookFrom(self, event):
        """!Position of view/light changed by buttons"""
        name = self.FindWindowById(event.GetId()).GetName()
        buttonName = name.split('_')[1]
        if name.split('_')[0] == 'view':
            type = 'view'
            data = self.mapWindow.view
        else:
            type = 'light'
            data = self.mapWindow.light
        if buttonName == 'n': # north
            data['position']['x'] = 0.5
            data['position']['y'] = 0.0
        elif buttonName == 's': # south
            data['position']['x'] = 0.5
            data['position']['y'] = 1.0
        elif buttonName == 'e': # east
            data['position']['x'] = 1.0
            data['position']['y'] = 0.5
        elif buttonName =='w': # west
            data['position']['x'] = 0.0
            data['position']['y'] = 0.5
        elif buttonName == 'nw': # north-west
            data['position']['x'] = 0.0
            data['position']['y'] = 0.0
        elif buttonName == 'ne': # north-east
            data['position']['x'] = 1.0
            data['position']['y'] = 0.0
        elif buttonName == 'se': # south-east
            data['position']['x'] = 1.0
            data['position']['y'] = 1.0
        elif buttonName == 'sw': # south-west
            data['position']['x'] = 0.0
            data['position']['y'] = 1.0
        if type == 'view':    
            self.PostViewEvent(zExag = True)
            
            self.UpdateSettings()
        else:
            self.PostLightEvent()
            lightWin = self.FindWindowById(self.win['light']['position'])
            x, y = lightWin.UpdatePos(self.mapWindow.light['position']['x'],
                                     self.mapWindow.light['position']['y'])
            lightWin.Draw(pos = (x, y), scale = True)
            lightWin.Refresh(False)
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

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
                value = self._getPercent(self.FindWindowById(self.win[nvizType][attrb]['const']).GetValue(), toPercent = False)
        
        self.SetMapObjUseMap(nvizType = nvizType,
                             attrb = attrb, map = useMap)
        
        name = self.FindWindowById(self.win[nvizType]['map']).GetValue()
        if nvizType == 'surface':
            data = self._getLayerPropertiesByName(name, mapType = 'raster')
            data[nvizType]['attribute'][attrb] = { 'map' : useMap,
                                                   'value' : str(value),
                                                   'update' : None }
        else: # volume / isosurface
            data = self._getLayerPropertiesByName(name, mapType = '3d-raster')
            list = self.FindWindowById(self.win['volume']['isosurfs'])
            id = list.GetSelection()
            if id != -1:
                data[nvizType]['isosurface'][id][attrb] = { 'map' : useMap,
                                                            'value' : str(value),
                                                            'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
 
    def EnablePage(self, name, enabled = True):
        """!Enable/disable all widgets on page"""
        for key, item in self.win[name].iteritems():
            if key in ('map', 'surface', 'new','planes'):
                continue
            if type(item) == types.DictType:
                for skey, sitem in self.win[name][key].iteritems():
                    if type(sitem) == types.DictType:
                        for ssitem in self.win[name][key][skey].itervalues():
                            if type(ssitem) == types.IntType:
                                self.FindWindowById(ssitem).Enable(enabled)
                    else:
                        if type(sitem) == types.IntType:
                            self.FindWindowById(sitem).Enable(enabled)
            else:
                if type(item) == types.IntType:
                    self.FindWindowById(item).Enable(enabled)
        
    def SetMapObjUseMap(self, nvizType, attrb, map = None):
        """!Update dialog widgets when attribute type changed"""
        if attrb in ('topo', 'color', 'shine'):
            incSel = -1 # decrement selection (no 'unset')
        else:
            incSel = 0
        if nvizType == 'volume' and attrb == 'topo':
            return
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
            self.FindWindowById(self.win[nvizType][attrb]['use']).SetSelection(0)
            self.FindWindowById(self.win[nvizType][attrb]['map'] + 1).Enable(False)
            if self.win[nvizType][attrb]['const']:
                self.FindWindowById(self.win[nvizType][attrb]['const']).Enable(False)
            
        
    def OnSurfaceMap(self, event):
        """!Set surface attribute"""
        if self.vetoGSelectEvt:
            self.vetoGSelectEvt = False
            return
        self.SetMapObjAttrb(nvizType = 'surface', winId = event.GetId())
        
    def SetMapObjAttrb(self, nvizType, winId):
        """!Set map object (surface/isosurface) attribute (map/constant)"""
        if not self.mapWindow.init:
            return
        
        attrb = self.__GetWindowName(self.win[nvizType], winId)
        if not attrb:
            return
        
        if not (nvizType == 'volume' and attrb == 'topo'):
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
                    value = self._getPercent(
                        self.FindWindowById(self.win[nvizType][attrb]['const']).GetValue(), toPercent = False)
                    
                useMap = False
        else:
            useMap = None
            value = self.FindWindowById(self.win[nvizType][attrb]['const']).GetValue()
        if not self.pageChanging:
            name = self.FindWindowById(self.win[nvizType]['map']).GetValue()
            if nvizType == 'surface':
                data = self._getLayerPropertiesByName(name, mapType = 'raster')
                data[nvizType]['attribute'][attrb] = { 'map' : useMap,
                                                       'value' : str(value),
                                                       'update' : None }
            else:
                data = self._getLayerPropertiesByName(name, mapType = '3d-raster')
                list = self.FindWindowById(self.win['volume']['isosurfs'])
                id = list.GetSelection()
                if id > -1:
                    data[nvizType]['isosurface'][id][attrb] = { 'map' : useMap,
                                                                'value' : str(value),
                                                                'update' : None }
                    if attrb == 'topo':
                        list = self.FindWindowById(self.win['volume']['isosurfs'])
                        sel = list.GetSelection()
                        list.SetString(sel, "%s %s" % (_("Level"), str(value)))
                        list.Check(sel)
            
            # update properties
            event = wxUpdateProperties(data = data)
            wx.PostEvent(self.mapWindow, event)
            
            if self.mapDisplay.IsAutoRendered():
                self.mapWindow.Refresh(False)
        
    def OnSurfaceResolution(self, event):
        """!Draw resolution changed"""
        self.SetSurfaceResolution()
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
    

    def SetSurfaceResolution(self):
        """!Set draw resolution"""
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        
        data = self.GetLayerData('surface')
        data['surface']['draw']['resolution'] = { 'coarse' : coarse,
                                                  'fine' : fine,
                                                  'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
    def SetSurfaceMode(self):
        """!Set draw mode"""
        mode = self.FindWindowById(self.win['surface']['draw']['mode']).GetSelection()
        style = self.FindWindowById(self.win['surface']['draw']['style']).GetSelection()
        if style == 0: # wire
            self.FindWindowById(self.win['surface']['draw']['wire-color']).Enable(True)
        elif style == 1: # surface
            self.FindWindowById(self.win['surface']['draw']['wire-color']).Enable(False)
        
        shade = self.FindWindowById(self.win['surface']['draw']['shading']).GetSelection()
        
        value, desc = self.mapWindow.nvizDefault.GetDrawMode(mode, style, shade)
        
        return value, desc

    def OnSurfaceMode(self, event):
        """!Set draw mode"""
        value, desc = self.SetSurfaceMode()
        
        data = self.GetLayerData('surface')
        data['surface']['draw']['mode'] = { 'value' : value,
                                            'desc' : desc,
                                            'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)

    def OnSurfaceModeAll(self, event):
        """!Set draw mode (including wire color) for all loaded surfaces"""
        value, desc = self.SetSurfaceMode()
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
        color = self.FindWindowById(self.win['surface']['draw']['wire-color']).GetColour()
        cvalue = self._getColorString(color)
        
        for name in self.mapWindow.GetLayerNames(type = 'raster'):
            
            data = self._getLayerPropertiesByName(name, mapType = 'raster')
            if not data:
                continue # shouldy no happen
            
            data['surface']['draw']['all'] = True
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
            
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def _getColorString(self, color):
        """!Convert color tuple to R:G:B format

        @param color tuple
        
        @return string R:G:B
        """
        return str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])
    
    def _getColorFromString(self, color, delim = ':'):
        """!Convert color string (R:G:B) to wx.Colour

        @param color string
        @param delim delimiter

        @return wx.Colour instance
        """
        return wx.Colour(*map(int, color.split(delim)))
    
    def _get3dRange(self, name):
        """!Gelper func for getting range of 3d map"""
        ret = RunCommand('r3.info', read = True, flags = 'r', map = name)
        if ret:
            range = []
            for value in ret.strip('\n').split('\n'):
                range.append(float(value.split('=')[1]))
            return range
        
        return -1e6, 1e6
    
    def _getPercent(self, value, toPercent = True):
        """!Convert values 0 - 255 to percents and vice versa"""
        value = int(value)
        if toPercent:
            value = int(value/255. * 100)
        else:
            value = int(value/100. * 255)
        return value
    
    def OnSurfaceWireColor(self, event):
        """!Set wire color"""
        data = self.GetLayerData('surface')
        value = self._getColorString(event.GetValue())
        data['surface']['draw']['wire-color'] = { 'value' : value,
                                                  'update' : None }
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnSurfaceAxis(self, event):
        """!Surface position, axis changed"""
        data = self.GetLayerData('surface')
        id = data['surface']['object']['id']
        
        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        slider = self.FindWindowById(self.win['surface']['position']['slider'])
        text = self.FindWindowById(self.win['surface']['position']['text'])
        xydim = self._display.GetLongDim()
        zdim = self._display.GetZRange()
        zdim = zdim[1] - zdim[0]
        
        x, y, z = self._display.GetSurfacePosition(id)
        
        if axis == 0: # x
            slider.SetRange(-3 * xydim, 3 * xydim)
            slider.SetValue(x)
            text.SetValue(x)
        elif axis == 1: # y
            slider.SetRange(-3 * xydim, 3 * xydim)
            slider.SetValue(y)
            text.SetValue(y)
        else: # z
            slider.SetRange(-3 * zdim, 3 * zdim)
            slider.SetValue(z)
            text.SetValue(z)
        
    def OnSurfacePosition(self, event):
        """!Surface position"""
        winName = self.__GetWindowName(self.win['surface'], event.GetId())
        if not winName:
            return
        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        
        value = self.FindWindowById(event.GetId()).GetValue()
        slider = self.FindWindowById(self.win['surface'][winName]['slider'])
        self.AdjustSliderRange(slider = slider, value = value)
        
        for win in self.win['surface']['position'].itervalues():
            if win in (self.win['surface']['position']['axis'],
                       self.win['surface']['position']['reset']):
                continue
            else:
                self.FindWindowById(win).SetValue(value)
        
        data = self.GetLayerData('surface')
        id = data['surface']['object']['id']
        x, y, z = self._display.GetSurfacePosition(id)
        
        if axis == 0: # x
            x = value
        elif axis == 1: # y
            y = value
        else: # z
            z = value
        
        data['surface']['position']['x'] = x
        data['surface']['position']['y'] = y
        data['surface']['position']['z'] = z
        data['surface']['position']['update'] = None
        # update properties
        
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        self.mapWindow.render['quick'] = True
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        #        self.UpdatePage('surface')
        
    def OnSurfacePositionChanged(self, event):
        """!Surface position changed"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

    def OnSurfacePositionText(self, event):
        """!Surface position changed by textctrl"""
        self.OnSurfacePosition(event)
        self.OnSurfacePositionChanged(None)
        
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
            points = False
        else: # points
            vecType = 'points' 
            points = True
       
        checked = event.IsChecked()
        name = self.FindWindowById(self.win['vector']['map']).GetValue()
        items = self.parent.GetLayerTree().FindItemByData(key = 'name', value = name)
        for item in items:
            if self.parent.GetLayerTree().GetLayerInfo(item, key = 'type') == 'vector':
                break
        data = self.GetLayerData('vector')['vector']
        
        if checked:
            self.mapWindow.LoadVector(item, points = points, append = False)
        else:
            self.mapWindow.UnloadVector(item, points = points, remove = False)
        
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
        
        if self.mapDisplay.IsAutoRendered():
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
            data = self.GetLayerData('vector')
            data['vector']['lines']['mode']['surface'] = rasters[0]
            self.FindWindowById(self.win['vector']['lines']['surface']).SetStringSelection( \
                rasters[0])
        else: # flat
            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(False)
        
        self.OnVectorLines(event)
        
        event.Skip()

    def OnVectorLines(self, event):
        """!Set vector lines mode, apply changes if auto-rendering is enabled"""
        data = self.GetLayerData('vector')
        width = self.FindWindowById(self.win['vector']['lines']['width']).GetValue()
        
        mode = {}
        if self.FindWindowById(self.win['vector']['lines']['flat']).GetSelection() == 0:
            mode['type'] = 'surface'
            mode['surface'] = {}
            checklist = self.FindWindowById(self.win['vector']['lines']['surface'])
            value = list()
            checked = list()
            for surface in range(checklist.GetCount()):
                value.append(checklist.GetString(surface))
                checked.append(checklist.IsChecked(surface))
                    
            mode['surface']['value'] = value
            mode['surface']['show'] = checked
        else:
            mode['type'] = 'flat'
        
        for attrb in ('width', 'mode'):
            data['vector']['lines'][attrb]['update'] = None
        data['vector']['lines']['width']['value'] = width
        data['vector']['lines']['mode'] = mode
        
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
                        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnVectorHeight(self, event):
        id = event.GetId()
        if id in self.win['vector']['lines']['height'].values():
            vtype = 'lines'
        else:
            vtype = 'points'
        
        value = self.FindWindowById(id).GetValue()
        slider = self.FindWindowById(self.win['vector'][vtype]['height']['slider'])
        self.AdjustSliderRange(slider = slider, value = value)
        
        for win in self.win['vector'][vtype]['height'].itervalues():
            self.FindWindowById(win).SetValue(value)
        
        data = self.GetLayerData('vector')
        data['vector'][vtype]['height'] = { 'value' : value,
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
##        self.OnVectorSurface(event)
        id = event.GetId()
        if id in self.win['vector']['lines']['height'].values():
            vtype = 'lines'
        else:
            vtype = 'points'
        
        self.mapWindow.render['quick'] = False
        self.mapWindow.render['v' + vtype] = False
        self.mapWindow.Refresh(False)

    def OnVectorHeightText(self, event):
        """!Vector height changed, render in full resolution"""
        
        #        self.OnVectorHeight(event)
        self.OnVectorHeightFull(event)
    
    def OnVectorSurface(self, event):
        """!Reference surface for vector map (lines/points)"""   
        id = event.GetId()
        if id == self.win['vector']['lines']['surface']:
            vtype = 'lines'
        else:
            vtype = 'points'
        checkList = self.FindWindowById(self.win['vector'][vtype]['surface'])
        checked = []
        surfaces = []
        for items in range(checkList.GetCount()):
            checked.append(checkList.IsChecked(items))
            surfaces.append(checkList.GetString(items))
        
        data = self.GetLayerData('vector')
        data['vector'][vtype]['mode']['surface'] = { 'value' : surfaces,
                                                     'show'  : checked}
        data['vector'][vtype]['mode']['update'] = None 
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
            
        
    def OnVectorPoints(self, event):
        """!Set vector points mode, apply changes if auto-rendering is enabled"""
        data = self.GetLayerData('vector')
        
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
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)

    def OnCheckThematic(self, event):
        """!Switch on/off thematic mapping"""
        # can be called with no event to enable/disable button
        if not event:
            ids = (self.win['vector']['points']['thematic']['checkcolor'],
                  self.win['vector']['lines']['thematic']['checkcolor'],
                  self.win['vector']['points']['thematic']['checksize'],
                  self.win['vector']['lines']['thematic']['checkwidth'])
        else:
            ids = (event.GetId(),)
        for id in ids:
            if id in self.win['vector']['points']['thematic'].values():
                vtype = 'points'
                if id == self.win['vector'][vtype]['thematic']['checkcolor']:
                    attrType = 'color'
                else:
                    attrType = 'size'
            else:
                vtype = 'lines'
                if id == self.win['vector'][vtype]['thematic']['checkcolor']:
                    attrType = 'color'
                else:
                    attrType = 'width'
                
            check = self.win['vector'][vtype]['thematic']['check' + attrType]
            button = self.win['vector'][vtype]['thematic']['button' + attrType]
            if self.FindWindowById(check).GetValue():
                checked = True
            else:
                checked = False
            self.FindWindowById(button).Enable(checked)
            
            data = self.GetLayerData('vector')
            
            # decide if use GRASSRGB column
            if attrType == 'color':
                name = self.FindWindowById(self.win['vector']['map']).GetValue()
                if not data['vector'][vtype]['thematic']['rgbcolumn']:
                    try:
                        id =  data['vector'][vtype]['object']['id']
                    
                        # if GRASSRGB exists and color table doesn't, use GRGB
                        if self.HasGRASSRGB(name)  and \
                            not self._display.CheckColorTable(id = id, type = vtype):
                            data['vector'][vtype]['thematic']['rgbcolumn'] = 'GRASSRGB'
                    except KeyError:
                        pass
                        
            data['vector'][vtype]['thematic']['use' + attrType] = checked
            data['vector'][vtype]['thematic']['update'] = None
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
            
    def HasGRASSRGB(self, name):
        """!Check if GRASSRGB column exist."""
        column = False
        
        dbInfo = VectorDBInfo(name)
        if len(dbInfo.layers):
            table = dbInfo.layers[1]['table']
            if 'GRASSRGB' in dbInfo.GetTableDesc(table):
                column = True
                
        return column
        
    def OnSetThematic(self, event):
        """!Set options for thematic points"""
        if event.GetId() in self.win['vector']['points']['thematic'].values():
            vtype = 'points'
        else:
            vtype = 'lines'
        if event.GetId() == self.win['vector'][vtype]['thematic']['buttoncolor']:
            attrType = 'color'
        elif vtype == 'points':
            attrType = 'size'
        else:
            attrType = 'width'
        ctable = ThematicVectorTable(self, vtype, attributeType = attrType)
        ctable.CentreOnScreen()
        ctable.Show()
        
    def UpdateIsosurfButtons(self, list):
        """!Enable/disable buttons 'add', 'delete',
        'move up', 'move down'"""
        nitems = list.GetCount()
        add = self.parent.FindWindowById(self.win['volume']['btnAdd'])
        delete = self.parent.FindWindowById(self.win['volume']['btnDelete'])
        moveDown = self.parent.FindWindowById(self.win['volume']['btnMoveDown'])
        moveUp = self.parent.FindWindowById(self.win['volume']['btnMoveUp'])
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
            
    def OnVolumeMode(self, event):
        """!Change mode isosurfaces/slices"""
        mode = self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection()
        data = self.GetLayerData('volume')['volume']
        
        sizer = self.isoPanel.GetContainingSizer()
        sizer = self.slicePanel.GetContainingSizer()
        listBox = self.FindWindowByName('listStaticBox')
        if mode == 0:
            sizer.Show(self.isoPanel)
            sizer.Hide(self.slicePanel)
            listBox.SetLabel(" %s " % _("List of isosurfaces"))
            data['draw']['mode']['value'] = 0
            data['draw']['mode']['desc'] = 'isosurface'
        else:
            sizer.Hide(self.isoPanel)
            sizer.Show(self.slicePanel)
            listBox.SetLabel(" %s " % _("List of slices"))
            data['draw']['mode']['value'] = 1
            data['draw']['mode']['desc'] = 'slice'
        
        if event:
            name = self.FindWindowById(self.win['volume']['map']).GetValue()
            layer = self._getMapLayerByName(name, mapType = '3d-raster')
            self.UpdateVolumePage(layer, data, updateName = False)
            
        sizer.Layout()
        listBox.GetParent().Fit()
            
    def OnVolumeDrawMode(self, event):
        """!Set isosurface/slice draw mode"""
        self.SetVolumeDrawMode(event.GetSelection())
        
    def OnVolumeDrawBox(self, event):
        """!Set wire box drawing"""
        data = self.GetLayerData('volume')['volume']
        vid = data['object']['id']
        checked = self.FindWindowById(self.win['volume']['draw']['box']).GetValue()
        self._display.SetVolumeDrawBox(vid, checked)
        data['draw']['box']['enabled'] = checked

        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)

    def SetVolumeDrawMode(self, selection):
        """!Set isosurface draw mode"""
        data = self.GetLayerData('volume')['volume']
        id = data['object']['id']
        
        mode = 0
        if selection == 0:
            mode |= wxnviz.DM_FLAT
        else:
            mode |= wxnviz.DM_GOURAUD
            
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            self._display.SetIsosurfaceMode(id, mode)
            data['draw']['shading']['isosurface']['desc'] = 'gouraud'
            data['draw']['shading']['isosurface']['value'] = mode
        else:
            self._display.SetSliceMode(id, mode)
            data['draw']['shading']['slice']['desc'] = 'flat'
            data['draw']['shading']['slice']['value'] = mode
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnVolumeResolution(self, event):
        """!Set isosurface/slice draw resolution"""
        self.SetVolumeResolution(event.GetInt())
        
    def SetVolumeResolution(self, res):
        """!Set isosurface draw resolution"""
        data = self.GetLayerData('volume')['volume']
        id = data['object']['id']
        
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            self._display.SetIsosurfaceRes(id, res)
            data['draw']['resolution']['isosurface']['value'] = res
        else:
            self._display.SetSliceRes(id, res)
            data['draw']['resolution']['slice']['value'] = res
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
    
    def OnInOutMode(self, event):
        """!Change isosurfaces mode inout"""
        data = self.GetLayerData('volume')['volume']
        id = data['object']['id']
        isosurfId = self.FindWindowById(self.win['volume']['isosurfs']).GetSelection()
        
        ret = self._display.SetIsosurfaceInOut(id, isosurfId, event.GetInt())
        if ret == 1:
            data['isosurface'][isosurfId]['inout'] = event.GetInt()
            
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
    
        
    def OnVolumeIsosurfMap(self, event):
        """!Set surface attribute"""
        if self.vetoGSelectEvt:
            self.vetoGSelectEvt = False
            return
        self.SetMapObjAttrb(nvizType = 'volume', winId = event.GetId())
        
    def OnVolumeCheck(self, event):
        """!Isosurface/slice checked (->load) or unchecked (->unload)"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
        index = event.GetSelection()
        list = self.FindWindowById(self.win['volume'][mode + 's'])
        
        data = self.GetLayerData('volume')['volume']
        vid = data['object']['id']
        
        id = event.GetSelection()
        
        if mode == 'isosurf':
            if list.IsChecked(index):
                if 'transp' in data['isosurface'][id] and\
                    data['isosurface'][id]['transp']['map'] is not None:
                    if data['isosurface'][id]['transp']['map']:
                        map = True
                        value = data['isosurface'][id]['transp']['value']
                    elif data['isosurface'][id]['transp']['map'] is not None:
                        map = False
                        value = data['isosurface'][id]['transp']['value']
                    self._display.SetIsosurfaceTransp(vid, id, map, value)
                else:
                    self._display.SetIsosurfaceTransp(vid, id, False, "0")
            else:
                # disable -> make transparent
                self._display.SetIsosurfaceTransp(vid, id, False, "255")
        else:
            if list.IsChecked(index):
                value = data['slice'][id]['transp']['value']
                self._display.SetSliceTransp(vid, id, value)
            else:
                # disable -> make transparent
                self._display.SetSliceTransp(vid, id, 255)
                
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnVolumeSelect(self, event):
        """!Isosurface/Slice item selected"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
            
        winUp = self.FindWindowById(self.win['volume']['btnMoveUp'])
        winDown = self.FindWindowById(self.win['volume']['btnMoveDown'])
        selection = event.GetSelection()
        if selection == -1:
            return
        elif selection == 0:
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
        name = self.FindWindowById(self.win['volume']['map']).GetValue()
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        
        if mode == 'isosurf':
            data = self.GetLayerData('volume')['volume']['isosurface'][selection]
            self.UpdateVolumeIsosurfPage(data)
        else:
            data = self.GetLayerData('volume')['volume']['slice'][selection]
            self.UpdateVolumeSlicePage(data)
        
        
        
    def OnVolumeAdd(self, event):
        """!Add new isosurface/slice to the list"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
        list = self.FindWindowById(self.win['volume'][mode + 's'])
        
        name = self.FindWindowById(self.win['volume']['map']).GetValue()
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        data = self.GetLayerData('volume')['volume']
        id = data['object']['id']
        
        sel = list.GetSelection()
        if mode == 'isosurf':
            isosurfData = self.mapWindow.nvizDefault.SetIsosurfaceDefaultProp()
            if isosurfData['color']['map']:
                isosurfData['color']['value'] = layer.name

            level = isosurfData['topo']['value'] = round(self._get3dRange(name = layer.name)[0], 2)
        
            if sel < 0 or sel >= list.GetCount() - 1:
                item = list.Append(item = "%s %s" % (_("Level"), str(level)))
            else:
                list.Insert(item = "%s %s" % (_("Level"), str(level)),
                            pos = sel+1) # append
                item = sel + 1
        else:
            sliceData = self.mapWindow.nvizDefault.SetSliceDefaultProp()
            axis = ("X", "Y", "Z")[sliceData['position']['axis']]
            if sel < 0 or sel >= list.GetCount() - 1:
                item = list.Append(item = "%s %s" % (_("Slice parallel to"), axis))
            else:
                list.Insert(item = "%s" % (_("Slice parallel to"), axis),
                            pos = sel+1) # append
                item = sel + 1
        
        list.Check(item)
        list.SetSelection(item)
        
        if mode == 'isosurf':
            data['isosurface'].insert(item, isosurfData)
            # add isosurface        
            self._display.AddIsosurface(id, float(level))
        else:
            data['slice'].insert(item, sliceData)
            # add isosurface        
            nslice = self._display.AddSlice(id)
            self._display.SetSlicePosition(id, nslice -1, sliceData['position']['x1'], sliceData['position']['x2'],
                                               sliceData['position']['y1'], sliceData['position']['y2'],
                                               sliceData['position']['z1'], sliceData['position']['z2'],
                                               sliceData['position']['axis'])
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        if mode == 'isosurf':
            self.UpdateVolumeIsosurfPage(isosurfData)
        else:
            self.UpdateVolumeSlicePage(sliceData)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeDelete(self, event):
        """!Remove isosurface/slice from list"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
        list = self.FindWindowById(self.win['volume'][mode + 's'])
        
        # remove item from list
        id = list.GetSelection()
        list.Delete(id)
        # select last item
        if list.GetCount() > 0:
            list.SetSelection(list.GetCount()-1)
        
        name = self.FindWindowById(self.win['volume']['map']).GetValue()
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        data = self.GetLayerData('volume')['volume']

        vid = data['object']['id']
        
        # delete isosurface
        if mode == 'isosurf':
            del data['isosurface'][id]
            self._display.DeleteIsosurface(vid, id)
        else:
            del data['slice'][id]
            self._display.DeleteSlice(vid, id)
        
        # update buttons
        if list.GetCount() > 0:
            if mode == 'isosurf':
                self.UpdateVolumeIsosurfPage(data['isosurface'][list.GetSelection()])
            else:
                self.UpdateVolumeSlicePage(data['slice'][list.GetSelection()])
        else:
            if mode == 'isosurf':
                self.UpdateVolumeIsosurfPage(data['attribute'])
            else:
                self.UpdateVolumeSlicePage(None)
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeMoveUp(self, event):
        """!Move isosurface/slice up in the list"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
        list = self.FindWindowById(self.win['volume'][mode + 's'])
        sel = list.GetSelection()
        
        if sel < 1:
            return # this should not happen
        
        name = self.FindWindowById(self.win['volume']['map']).GetValue()
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        data = self.GetLayerData('volume')['volume']
        
        id = data['object']['id']
        
        # move item up
        text = list.GetStringSelection()
        list.Insert(item = text, pos = sel-1)
        list.Check(sel-1)
        list.SetSelection(sel-1)
        list.Delete(sel+1)
        if mode == 'isosurf':
            data['isosurface'].insert(sel-1, data['isosurface'][sel])
            del data['isosurface'][sel+1]
            self._display.MoveIsosurface(id, sel, True)
        else:
            data['slice'].insert(sel-1, data['slice'][sel])
            del data['slice'][sel+1]
            self._display.MoveSlice(id, sel, True)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
        event.Skip()
        
    def OnVolumeMoveDown(self, event):
        """!Move isosurface/slice down in the list"""
        if self.FindWindowById(self.win['volume']['draw']['mode']).GetSelection() == 0:
            mode = 'isosurf'
        else:
            mode = 'slice'
        list = self.FindWindowById(self.win['volume'][mode + 's'])
        sel = list.GetSelection()
        
        if sel >= list.GetCount() - 1:
            return # this should not happen
        
        name = self.FindWindowById(self.win['volume']['map']).GetValue()
        layer = self._getMapLayerByName(name, mapType = '3d-raster')
        data = self.GetLayerData('volume')['volume']
        
        id = data['object']['id']
        
        # move item up
        text = list.GetStringSelection()
        list.Insert(item = text, pos = sel+2)
        list.Check(sel+2)
        list.SetSelection(sel+2)
        list.Delete(sel)
        if mode == 'isosurf':
            data['isosurface'].insert(sel+2, data['isosurface'][sel])
            del data['isosurface'][sel]
            self._display.MoveIsosurface(id, sel, False)
        else:
            data['slice'].insert(sel+2, data['slice'][sel])
            del data['slice'][sel]
            self._display.MoveSlice(id, sel, False)
        
        # update buttons
        self.UpdateIsosurfButtons(list)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
        event.Skip()
    
    def OnVolumePositionChanged(self, event):
        """!Volume position changed"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
    def OnVolumePosition(self, event):
        """!Volume position"""
        winName = self.__GetWindowName(self.win['volume'], event.GetId())
        if not winName:
            return
        axis = self.FindWindowById(self.win['volume']['position']['axis']).GetSelection()
        
        value = self.FindWindowById(event.GetId()).GetValue()
        slider = self.FindWindowById(self.win['volume'][winName]['slider'])
        self.AdjustSliderRange(slider = slider, value = value)
        
        for win in self.win['volume']['position'].itervalues():
            if win in (self.win['volume']['position']['axis'],
                       self.win['volume']['position']['reset']):
                continue
            else:
                self.FindWindowById(win).SetValue(value)
        
        data = self.GetLayerData('volume')
        id = data['volume']['object']['id']
        x, y, z = self._display.GetVolumePosition(id)
        
        if axis == 0: # x
            x = value
        elif axis == 1: # y
            y = value
        else: # z
            z = value
        
        data['volume']['position']['x'] = x
        data['volume']['position']['y'] = y
        data['volume']['position']['z'] = z
        data['volume']['position']['update'] = None
        # update properties
        
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        self.mapWindow.render['quick'] = True
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnVolumeAxis(self, event):
        """!Volume position, axis changed"""
        data = self.GetLayerData('volume')
        id = data['volume']['object']['id']
        
        axis = self.FindWindowById(self.win['volume']['position']['axis']).GetSelection()
        slider = self.FindWindowById(self.win['volume']['position']['slider'])
        text = self.FindWindowById(self.win['volume']['position']['text'])
        xydim = self._display.GetLongDim()
        zdim = self._display.GetZRange()
        zdim = zdim[1] - zdim[0]
        x, y, z = self._display.GetVolumePosition(id)
        
        if axis == 0: # x
            slider.SetRange(-3 * xydim, 3 * xydim)
            slider.SetValue(x)
            text.SetValue(x)
        elif axis == 1: # y
            slider.SetRange(-3 * xydim, 3 * xydim)
            slider.SetValue(y)
            text.SetValue(y)
        else: # z
            slider.SetRange(-3 * zdim, 3 * zdim)
            slider.SetValue(z)
            text.SetValue(z)
            
    def OnVolumePositionText(self, event):
        """!Volume position changed by textctrl"""
        self.OnVolumePosition(event)
        self.OnVolumePositionChanged(None)
        
    def OnResetVolumePosition(self, event):
        """!Reset position of volume"""
        for win in self.win['volume']['position'].itervalues():
            if win == self.win['volume']['position']['axis']:
                self.FindWindowById(win).SetSelection(2) # Z
            elif win == self.win['volume']['position']['reset']:
                continue
            else:
                self.FindWindowById(win).SetValue(0)
                
        data = self.GetLayerData('volume')
        data['volume']['position']['x'] = 0
        data['volume']['position']['y'] = 0
        data['volume']['position']['z'] = 0
        data['volume']['position']['update'] = None
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnVolumeSliceAxes(self, event):
        """!Slice axis changed"""
        self.UpdateSliceLabels()
        data = self.GetLayerData('volume')
        list = self.FindWindowById(self.win['volume']['slices'])
        sel = list.GetSelection()
        if sel < 0:
            return
        axis = self.FindWindowById(self.win['volume']['slice']['axes']).GetSelection()
        data['volume']['slice'][sel]['position']['axis'] = axis
        data['volume']['slice'][sel]['position']['update'] = None
        
        axis = ("X", "Y", "Z")[axis]
        list.SetString(sel, "%s %s" % (_("Slice parallel to"), axis))
        list.Check(sel)
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event) 
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
    
    def OnSliceTransparency(self, event):
        """!Slice transparency changed"""
        data = self.GetLayerData('volume')
        
        list = self.FindWindowById(self.win['volume']['slices'])
        sel = list.GetSelection()
        if sel < 0:
            return
        
        val = self.FindWindowById(self.win['volume']['slice']['transp']).GetValue()
        data['volume']['slice'][sel]['transp']['value'] = self._getPercent(val, toPercent = False)
        data['volume']['slice'][sel]['transp']['update'] = None
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnSliceReset(self, event):
        """!Slice position reset"""
        data = self.GetLayerData('volume')
        
        list = self.FindWindowById(self.win['volume']['slices'])
        sel = list.GetSelection()
        if sel < 0:
            return
        
        for coord, val in zip(('x1', 'x2', 'y1', 'y2', 'z1', 'z2'),(0, 1, 0, 1, 0, 1, 0)):
            data['volume']['slice'][sel]['position'][coord] = val
        data['volume']['slice'][sel]['position']['update'] = None
        
        self.UpdateVolumeSlicePage(data['volume']['slice'][sel])
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        
    def OnSlicePositionChange(self, event):
        """!Slice position is changing"""
        data = self.GetLayerData('volume')
        list = self.FindWindowById(self.win['volume']['slices'])
        sel = list.GetSelection()
        if sel < 0:
            return
        win = self.win['volume']['slice']
        winId = event.GetId()
        value = event.GetInt()/100.
        
        for coord in ('x1', 'x2', 'y1', 'y2', 'z1', 'z2'):
            if win['slider_' + coord] == winId:
                data['volume']['slice'][sel]['position'][coord] = value
                data['volume']['slice'][sel]['position']['update'] = None
                break
        self.mapWindow.render['quick'] = True
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.mapWindow, event) 
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
               
    def OnSlicePositionChanged(self, event):
        """!Slice position is changed"""
        self.mapWindow.render['quick'] = False
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
                
    def OnCPlaneSelection(self, event):
        """!Cutting plane selected"""
        plane = self.FindWindowById(self.win['cplane']['planes']).GetStringSelection()
        try:
            planeIndex = int(plane.split()[-1]) - 1
            self.EnablePage("cplane", enabled = True)
        except:
            planeIndex = -1
            self.EnablePage("cplane", enabled = False)
        self.mapWindow.SelectCPlane(planeIndex)
        if planeIndex >= 0:
            self.mapWindow.UpdateCPlane(planeIndex, changes = ['rotation', 'position', 'shading'])

        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
        self.UpdateCPlanePage(planeIndex)
        
    def OnCPlaneChanging(self, event):
        """!Cutting plane is changing"""
        plane = self.FindWindowById(self.win['cplane']['planes']).GetStringSelection()
        try:
            planeIndex = int(plane.split()[-1]) - 1
        except:#TODO disabled page
            planeIndex = -1
    
        if event.GetId() in (self.win['cplane']['rotation']['rot'].values() +
                            self.win['cplane']['rotation']['tilt'].values()):
            action = 'rotation'
        else:
            action = 'position'
        data = self.mapWindow.cplanes[planeIndex][action]
        self.OnScroll(event, self.win['cplane'][action], data)
        
        self.mapWindow.render['quick'] = True
        event = wxUpdateCPlane(update = (action,), current = planeIndex)
        wx.PostEvent(self.mapWindow, event)
        
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)

    def OnCPlaneChangeDone(self, event):
        """!Cutting plane change done"""
        self.mapWindow.render['quick'] = False
        if self.mapDisplay.IsAutoRendered():
            self.mapWindow.Refresh(False)
            
    def OnCPlaneChangeText(self, event):
        """!Cutting plane changed by textctrl"""
        for axis in ('x', 'y', 'z'):
            if event.GetId() == self.win['cplane']['position'][axis]['text']:
                value = self.FindWindowById(event.GetId()).GetValue()
                slider = self.FindWindowById(self.win['cplane']['position'][axis]['slider'])
                self.AdjustSliderRange(slider = slider, value = value)
        self.OnCPlaneChanging(event = event)
        self.OnCPlaneChangeDone(None)   
        
    def OnCPlaneShading(self, event):
        """!Cutting plane shading changed"""
        shading = self.FindWindowById(self.win['cplane']['shading']).GetSelection()
        plane = self.FindWindowById(self.win['cplane']['planes']).GetStringSelection()
        try:
            planeIndex = int(plane.split()[-1]) - 1
        except:#TODO disabled page
            planeIndex = -1
            
        self.mapWindow.cplanes[planeIndex]['shading'] = shading
        
        event = wxUpdateCPlane(update = ('shading',), current = planeIndex)
        wx.PostEvent(self.mapWindow, event)
        
        self.OnCPlaneChangeDone(None)
        
    def OnCPlaneReset(self, event):
        """!Reset current cutting plane"""
        plane = self.FindWindowById(self.win['cplane']['planes']).GetStringSelection()
        try:
            planeIndex = int(plane.split()[-1]) - 1
        except:#TODO disabled page
            planeIndex = -1

        self.mapWindow.cplanes[planeIndex] = copy.deepcopy(UserSettings.Get(group = 'nviz',
                                                                            key = 'cplane'))
        self.mapWindow.cplanes[planeIndex]['on'] = True
        event = wxUpdateCPlane(update = ('position','rotation','shading'), current = planeIndex)
        wx.PostEvent(self.mapWindow, event)
        self.OnCPlaneChangeDone(None)
        self.UpdateCPlanePage(planeIndex)
    
    def OnDecorationPlacement(self, event):
        """!Place an arrow/scalebar by clicking on display"""
        if event.GetId() == self.win['decoration']['arrow']['place']:
            type = 'arrow'
        elif event.GetId() == self.win['decoration']['scalebar']['place']:
            type = 'scalebar'
        else: return
        
        if event.GetInt():
            self.mapDisplay.Raise()
            self.mapWindow.mouse['use'] = type
            self.mapWindow.SetCursor(self.mapWindow.cursors["cross"])
        else:
            self.mapWindow.mouse['use'] = 'default'
            self.mapWindow.SetCursor(self.mapWindow.cursors["default"])
    
    def OnArrowDelete(self, event):
        """!Delete arrow"""
        self._display.DeleteArrow()
        self.mapWindow.decoration['arrow']['show'] = False
        self.FindWindowById( self.win['decoration']['arrow']['delete']).Disable()
        self.mapWindow.Refresh(False)
    
    def OnScalebarDelete(self, event):
        """!Delete scalebar"""
        choice = self.FindWindowById(self.win['decoration']['scalebar']['choice'])
        choiceIndex = choice.GetSelection()
        index = choice.GetClientData(choiceIndex)
        if index == wx.NOT_FOUND:
            return
        self._display.DeleteScalebar(id = index)
        
        self.FindWindowById(self.win['decoration']['scalebar']['choice']).Delete(choiceIndex)
        if not choice.IsEmpty():
            choice.SetSelection(choice.GetCount() - 1)
        self.DisableScalebarControls()

        self.mapWindow.Refresh(False)
         
    def AddScalebar(self, scalebarNum):
        choice = self.FindWindowById(self.win['decoration']['scalebar']['choice'])
        choice.Append(_("Scalebar %d") % (scalebarNum + 1), scalebarNum)
        choice.SetSelection(choice.GetCount() - 1)
        self.DisableScalebarControls()

    def AddArrow(self):
        self.FindWindowById( self.win['decoration']['arrow']['delete']).Enable()

    def DisableScalebarControls(self):
        choice = self.FindWindowById(self.win['decoration']['scalebar']['choice'])
        self.FindWindowById(self.win['decoration']['scalebar']['delete']).Enable(not choice.IsEmpty())
        self.FindWindowById(self.win['decoration']['scalebar']['choice']).Enable(not choice.IsEmpty())

    def OnDecorationProp(self, event):
        """!Set arrow/scalebar properties"""
        if event.GetId() in self.win['decoration']['arrow'].values():
            type = 'arrow'
        elif event.GetId() in self.win['decoration']['scalebar'].values():
            type = 'scalebar'
        else: return
        
        color = self.FindWindowById(self.win['decoration'][type]['color']).GetValue()
        size = self.FindWindowById(self.win['decoration'][type]['size']).GetValue()
        if type == 'arrow':
            self.mapWindow.decoration[type]['color'] = self._getColorString(color)
            self.mapWindow.decoration[type]['size'] = size
        elif type == 'scalebar'and self.mapWindow.decoration['scalebar']:
            for scalebar in self.mapWindow.decoration[type]:
                scalebar['color'] = self._getColorString(color)
                scalebar['size'] = size
        
        if type == 'arrow' and self.mapWindow.decoration['arrow']['show']:
            self._display.SetArrow(self.mapWindow.decoration['arrow']['position']['x'],
                                   self.mapWindow.decoration['arrow']['position']['y'],
                                   self.mapWindow.decoration['arrow']['size'],
                                   self.mapWindow.decoration['arrow']['color'])
            self._display.DrawArrow()
        elif type == 'scalebar' and self.mapWindow.decoration['scalebar']:
            ids = []
            choice = self.FindWindowById(self.win['decoration']['scalebar']['choice'])
            for index in range(choice.GetCount()):
                ids.append(choice.GetClientData(index))
            for scalebar in self.mapWindow.decoration[type]:
                if scalebar['id'] in ids:
                    self._display.SetScalebar(scalebar['id'],
                                              scalebar['position']['x'],
                                              scalebar['position']['y'],
                                              scalebar['size'],
                                              scalebar['color'])
            self._display.DrawScalebar()
            self.mapWindow.Refresh(False)
        
    def UpdatePage(self, pageId):
        """!Update dialog (selected page)"""
        self.pageChanging = True
        Debug.msg(1, "NvizToolWindow.UpdatePage(): %s", pageId)
        
        if pageId == 'view':
            self.SetPage('view')
            hmin = self.mapWindow.iview['height']['min']
            hmax = self.mapWindow.iview['height']['max']
            hval = self.mapWindow.iview['height']['value']
            zmin = self.mapWindow.view['z-exag']['min']
            zmax = self.mapWindow.view['z-exag']['max']
            zval = self.mapWindow.view['z-exag']['value']
            
            for control in ('slider','text'):
                self.FindWindowById(self.win['view']['height'][control]).SetRange(
                                                                        hmin,hmax)
                self.FindWindowById(self.win['view']['z-exag'][control]).SetRange(
                                                                        zmin, zmax) 
                self.FindWindowById(self.win['view']['height'][control]).SetValue(hval)                                      
                
                self.FindWindowById(self.win['view']['z-exag'][control]).SetValue(zval)                                      
        
            self.FindWindowById(self.win['view']['background']['color']).SetColour(\
                            self.mapWindow.view['background']['color'])
                            
            tval = self.mapWindow.view['twist']['value']
            pval = self.mapWindow.view['persp']['value']
            for control in ('slider','text'):
                self.FindWindowById(self.win['view']['twist'][control]).SetValue(tval)                                      
                
                self.FindWindowById(self.win['view']['persp'][control]).SetValue(pval)
            
            
        elif pageId in ('surface', 'vector', 'volume'):
            name = self.FindWindowById(self.win[pageId]['map']).GetValue()
            data = self.GetLayerData(pageId)
            if data:
                if pageId == 'surface':
                    layer = self._getMapLayerByName(name, mapType = 'raster')
                    self.UpdateSurfacePage(layer, data['surface'])
                elif pageId == 'vector':
                    layer = self._getMapLayerByName(name, mapType = 'vector')
                    self.UpdateVectorPage(layer, data['vector'])
                elif pageId == 'volume':
                    layer = self._getMapLayerByName(name, mapType = '3d-raster')
                    self.UpdateVolumePage(layer, data['volume'])
        elif pageId == 'light':
            zval = self.mapWindow.light['position']['z']
            bval = self.mapWindow.light['bright']
            aval = self.mapWindow.light['ambient']
            for control in ('slider','text'):
                self.FindWindowById(self.win['light']['z'][control]).SetValue(zval)
                self.FindWindowById(self.win['light']['bright'][control]).SetValue(bval)
                self.FindWindowById(self.win['light']['ambient'][control]).SetValue(aval)
            self.FindWindowById(self.win['light']['color']).SetColour(self.mapWindow.light['color'])
            self.FindWindowById(self.win['light']['position']).PostDraw()
        elif pageId == 'fringe':
            win = self.FindWindowById(self.win['fringe']['map'])
            win.SetValue(self.FindWindowById(self.win['surface']['map']).GetValue())
        elif pageId == 'decoration':
            win = self.FindWindowById(self.win['decoration']['arrow']['size'])
            win.SetValue(self.mapWindow.decoration['arrow']['size'])
            win = self.FindWindowById(self.win['decoration']['scalebar']['size'])
            win.SetValue(self.mapWindow._getDecorationSize())
        elif pageId == 'constant':
            if self.mapWindow.constants:
                surface = self.FindWindowById(self.win['constant']['surface'])
                for item in self.mapWindow.constants:
                    surface.Append(_("constant#") + str(item['constant']['object']['name']))
                surface.SetSelection(0)
                self.OnConstantSelection(None)
                self.EnablePage('constant', True)
        elif pageId == 'cplane':
            count = self._display.GetCPlanesCount()
            choices = [_("None"),]
            for plane in range(count):
                choices.append("%s %i" % (_("Plane"), plane+1))
            self.FindWindowById(self.win['cplane']['planes']).SetItems(choices)
            current = 0
            for i, cplane in enumerate(self.mapWindow.cplanes):
                if cplane['on']:
                    current = i + 1
            self.FindWindowById(self.win['cplane']['planes']).SetSelection(current)
            
            xyRange, zRange = self._display.GetXYRange(), self._display.GetZRange()
            if xyRange > 0: # GTK warning
                self.FindWindowById(self.win['cplane']['position']['x']['slider']).SetRange(
                                                                    -xyRange/2., xyRange/2.)
                self.FindWindowById(self.win['cplane']['position']['y']['slider']).SetRange(
                                                                    -xyRange/2., xyRange/2.)
            if zRange[0] - zRange[1] > 0:
                self.FindWindowById(self.win['cplane']['position']['z']['slider']).SetRange(zRange[0], zRange[1])
            self.FindWindowById(self.win['cplane']['position']['z']['slider']).SetValue(zRange[0])
            self.FindWindowById(self.win['cplane']['position']['z']['text']).SetValue(zRange[0])
            self.OnCPlaneSelection(None)
            
        elif pageId == 'animation':
            self.UpdateAnimationPage()
            
        self.Update()
        self.pageChanging = False
        
    def UpdateAnimationPage(self):
        """!Update animation page"""
        # wrap help text according to tool window
        help = self.FindWindowById(self.win['anim']['help'])
        width = help.GetGrandParent().GetSizeTuple()[0]
        help.Wrap(width - 15)
        anim = self.mapWindow.GetAnimation()
        if anim.Exists():
            self.FindWindowById(self.win['anim']['play']).Enable()
        else:
            self.UpdateFrameIndex(index = 0)
            
        self.UpdateFrameCount()
        
        self.FindWindowById(self.win['anim']['play']).Disable()
        self.FindWindowById(self.win['anim']['record']).Enable()
        self.FindWindowById(self.win['anim']['pause']).Disable()
        self.FindWindowById(self.win['anim']['stop']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['slider']).Disable()
        self.FindWindowById(self.win['anim']['frameIndex']['text']).Disable()
        
    def UpdateCPlanePage(self, index):
        """!Update widgets according to selected clip plane"""
        if index == -1:   
            return
        data = self.mapWindow.cplanes[index]
        for widget in ('text', 'slider'):
            for axes in ('x', 'y', 'z'):
                self.FindWindowById(self.win['cplane']['position'][axes][widget]).SetValue(data['position'][axes])
            for each in ('tilt', 'rot'):
                self.FindWindowById(self.win['cplane']['rotation'][each][widget]).SetValue(data['rotation'][each])
        self.FindWindowById(self.win['cplane']['shading']).SetSelection(data['shading'])
                
    def UpdateSurfacePage(self, layer, data, updateName = True):
        """!Update surface page"""
        desc = grass.raster_info(layer.name)['title']
        if updateName:
            self.FindWindowById(self.win['surface']['map']).SetValue(layer.name)
        self.FindWindowById(self.win['surface']['desc']).SetLabel(desc)
        
        # attributes
        if layer and layer.type == 'raster':
            self.vetoGSelectEvt = True
            self.FindWindowById(self.win['surface']['color']['map']).SetValue(layer.name)
        else:
            self.FindWindowById(self.win['surface']['color']['map']).SetValue('')

        self.SetMapObjUseMap(nvizType = 'surface',
                             attrb = 'color', map = True) # -> map
                                
        if 'color' in data['attribute']:
            value = data['attribute']['color']['value']

            if data['attribute']['color']['map']:
                self.FindWindowById(self.win['surface']['color']['map']).SetValue(value)
            else: # constant
                color = map(int, value.split(':'))
                self.FindWindowById(self.win['surface']['color']['const']).SetColour(color)
            self.SetMapObjUseMap(nvizType = 'surface',
                                 attrb = 'color', map = data['attribute']['color']['map'])

        self.SetMapObjUseMap(nvizType = 'surface',
                             attrb = 'shine', map = data['attribute']['shine']['map'])
        value = data['attribute']['shine']['value']
        if data['attribute']['shine']['map']:
            self.FindWindowById(self.win['surface']['shine']['map']).SetValue(value)
        else:
            self.FindWindowById(self.win['surface']['shine']['const']).SetValue(self._getPercent(value))
        if 'transp' in data['attribute']:  
            value = data['attribute']['transp']['value']  
            if data['attribute']['transp']['map']:
                self.FindWindowById(self.win['surface']['color']['map']).SetValue(value)
            else:
                self.FindWindowById(self.win['surface']['transp']['const']).SetValue(self._getPercent(value))
            self.SetMapObjUseMap(nvizType = 'surface', attrb = 'transp', map = data['attribute']['transp']['map'])
        else:
            self.SetMapObjUseMap(nvizType = 'surface', attrb = 'transp', map = None)
        #
        # draw
        #
        for control, drawData in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue
            if control == 'resolution':
                self.FindWindowById(self.win['surface']['draw']['res-coarse']).SetValue(drawData['coarse'])
                self.FindWindowById(self.win['surface']['draw']['res-fine']).SetValue(drawData['fine'])
                continue
            
            if control == 'mode':
                if drawData['desc']['mode'] == 'coarse':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(0)
                elif drawData['desc']['mode'] == 'fine':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(1)
                else: # both
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(2)
                
                if drawData['desc']['style'] == 'wire':
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(0)
                else: # surface
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(1)
                
                if drawData['desc']['shading'] == 'flat':
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(0)
                else: # gouraud
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(1)
                
                continue
            
            value = drawData['value']
            win = self.FindWindowById(self.win['surface']['draw'][control])
            
            name = win.GetName()
            
            if name == "selection":
                win.SetSelection(value)
            elif name == "colour":
                color = map(int, value.split(':'))
                win.SetColour(color)
            else:
                win.SetValue(value)
        #
        # position
        #
        self.OnSurfaceAxis(None)

        # enable/disable res widget + set draw mode
        self.OnSurfaceMode(event = None)
        
    def UpdateVectorPage(self, layer, data, updateName = True):
        """!Update vector page"""
        vInfo = grass.vector_info_topo(layer.GetName())
        if not vInfo:
            return
        if vInfo['map3d']:
            desc = _("Vector map is 3D")
            enable = False
        else:
            desc = _("Vector map is 2D")
            enable = True
        desc += " - " + _("%(features)d features (%(points)d points)") % \
            { 'features' : vInfo['primitives'], 'points' : vInfo['points']}
        
        if updateName:
            self.FindWindowById(self.win['vector']['map']).SetValue(layer.name)
        self.FindWindowById(self.win['vector']['desc']).SetLabel(desc)
        
        self.FindWindowById(self.win['vector']['lines']['flat']).Enable(enable)
        for v in ('lines', 'points'):
            self.FindWindowById(self.win['vector'][v]['surface']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['slider']).Enable(enable)
            self.FindWindowById(self.win['vector'][v]['height']['text']).Enable(enable)
            
            if data[v]['thematic']['usecolor']:
                check = self.FindWindowById(self.win['vector'][v]['thematic']['checkcolor'])
                check.SetValue(data[v]['thematic']['usecolor'])
            if 'usesize' in data[v]['thematic'] and data[v]['thematic']['usesize']:
                check = self.FindWindowById(self.win['vector'][v]['thematic']['checksize'])
                check.SetValue(data[v]['thematic']['usesize'])
            elif 'usewidth' in data[v]['thematic'] and data[v]['thematic']['usewidth']:
                check = self.FindWindowById(self.win['vector'][v]['thematic']['checkwidth'])
                check.SetValue(data[v]['thematic']['usewidth'])
            self.OnCheckThematic(None)
        #
        # lines
        #
        showLines = self.FindWindowById(self.win['vector']['lines']['show'])
        if 'object' in data['lines']:
            showLines.SetValue(True)
        else:
            showLines.SetValue(False)
        if (vInfo['lines'] + vInfo['boundaries']) > 0:
            showLines.Enable(True)
        else:
            showLines.Enable(False)
        
        self.UpdateVectorShow('lines', showLines.IsChecked())
        
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
                constants = self.mapWindow.GetLayerNames('constant')
                surfaces = rasters + constants
                surfaceWin = self.FindWindowById(self.win['vector'][vtype]['surface'])
                surfaceWin.SetItems(surfaces)
                for idx, surface in enumerate(surfaces):
                    try:# TODO fix this mess
                        selected = data[vtype]['mode']['surface']['show'][idx]
                    except (TypeError, IndexError, KeyError):
                        selected = False
                    surfaceWin.Check(idx, selected)

        for type in ('slider', 'text'):
            win = self.FindWindowById(self.win['vector']['lines']['height'][type])
            win.SetValue(data['lines']['height']['value'])
        
        #
        # points
        #
        showPoints = self.FindWindowById(self.win['vector']['points']['show'])
        
        if 'object' in data['points']:
            showPoints.SetValue(True)
        else:
            showPoints.SetValue(False)
        if (vInfo['points'] + vInfo['centroids']) > 0:
            showPoints.Enable(True)
        else:
            showPoints.Enable(False)
        
        self.UpdateVectorShow('points', showPoints.IsChecked())
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
##        self.OnCheckThematic(None)
        # height
        for type in ('slider', 'text'):
            win = self.FindWindowById(self.win['vector']['points']['height'][type])
            win.SetValue(data['points']['height']['value'])
        
    def UpdateVolumePage(self, layer, data, updateName = True):
        """!Update volume page"""
        if updateName:
            self.FindWindowById(self.win['volume']['map']).SetValue(layer.name)
        
        # draw
        for control, idata in data['draw'].iteritems():
            if control == 'all': # skip 'all' property
                continue
                
            win = self.FindWindowById(self.win['volume']['draw'][control])
            if control == 'mode':
                value = data['draw']['mode']['value']
            if control == 'shading':
                if data['draw']['shading'][data['draw']['mode']['desc']]['desc'] == 'flat':
                    value = 0
                else:
                    value = 1
            if control == 'resolution':
                value = idata[data['draw']['mode']['desc']]['value']
            if control == 'box':
                value = idata['enabled']
            
            if win.GetName() == "selection":
                win.SetSelection(value)
            else:
                win.SetValue(value)
                
        self.OnVolumeMode(None)
        id = data['object']['id']
        if data['draw']['mode']['desc'] == 'isosurface':
            self._display.SetIsosurfaceMode(id, data['draw']['shading']['isosurface']['value'])
            self._display.SetIsosurfaceRes(id, data['draw']['resolution']['isosurface']['value'])
        else:
            self._display.SetSliceMode(id, data['draw']['shading']['slice']['value'])
            self._display.SetSliceRes(id, data['draw']['resolution']['slice']['value'])
        box = self.FindWindowById(self.win['volume']['isosurfs'])
        
        if data['draw']['mode']['desc'] == 'isosurface':
            isosurfaces = []
            for iso in data['isosurface']:
                level = iso['topo']['value']
                isosurfaces.append("%s %s" % (_("Level"), level))
            box.Set(isosurfaces)
            for i in range(len(isosurfaces)):
                box.Check(i)
            if data['isosurface']:
                box.SetSelection(0)
                self.UpdateVolumeIsosurfPage(data['isosurface'][0])
            else:
                self.UpdateVolumeIsosurfPage(data['attribute'])
        else:
            slices = []
            for slice in data['slice']:
                axis = ("X", "Y", "Z")[slice['position']['axis']]
                slices.append("%s %s" % (_("Slice parallel to"), axis))
            box.Set(slices)
            for i in range(len(slices)):
                box.Check(i)
            if data['slice']:
                box.SetSelection(0)
                self.UpdateVolumeSlicePage(data['slice'][0])
            else:
                self.UpdateVolumeSlicePage(None)
        #
        # position
        #
        if 'z' in data['position']:
            zval = data['position']['z']
            self.FindWindowById(self.win['volume']['position']['axis']).SetSelection(2)
            for control in ('slider','text'):
                    self.FindWindowById(self.win['volume']['position'][control]).SetValue(zval)
        # set topo range
        mapRange = self._get3dRange(name = layer.name)
        desc = self.FindWindowById(self.win['volume']['desc'])
        desc.SetLabel("%s %.2f - %.2f" % (_("range:"), mapRange[0], mapRange[1]))
        
    def UpdateVolumeIsosurfPage(self, data):
        """!Update dialog -- isosurface attributes"""
        #
        # isosurface attributes
        #
        for attrb in ('topo', 'color', 'mask',
                     'transp', 'shine'):
            # skip empty attributes
            if attrb not in data:
                self.SetMapObjUseMap(nvizType = 'volume', attrb = attrb, map = None)
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
                    self.vetoGSelectEvt = True
                    win = self.FindWindowById(self.win['volume'][attrb]['map'])
                    win.SetValue(value)
                else:
                    if value:
                        win = self.FindWindowById(self.win['volume'][attrb]['const'])
                        if attrb == 'topo':
                            win.SetValue(float(value))
                        else:
                            win.SetValue(self._getPercent(value))
                    
            self.SetMapObjUseMap(nvizType = 'volume',
                                 attrb = attrb, map = data[attrb]['map'])
        # set inout
        if 'inout' in data:
            self.FindWindowById(self.win['volume']['inout']).SetValue(data['inout'])
            
    def UpdateVolumeSlicePage(self, data):
        """!Update dialog -- slice attributes"""
        if data:
            for coord in ('x1', 'x2', 'y1', 'y2', 'z1', 'z2'):
                win = self.FindWindowById(self.win['volume']['slice']['slider_' + coord])
                win.Enable()
                win.SetValue(data['position'][coord] * 100)
            win = self.FindWindowById(self.win['volume']['slice']['axes'])
            win.SetSelection(data['position']['axis'])
            win.Enable()
            
            win = self.FindWindowById(self.win['volume']['slice']['transp'])
            win.SetValue(self._getPercent(data['transp']['value']))
            win.Enable()
            self.FindWindowById(self.win['volume']['slice']['reset']).Enable()
        else:
            for coord in ('x1', 'x2', 'y1', 'y2', 'z1', 'z2'):
                self.FindWindowById(self.win['volume']['slice']['slider_' + coord]).Disable()
            self.FindWindowById(self.win['volume']['slice']['axes']).Disable()
            self.FindWindowById(self.win['volume']['slice']['transp']).Disable()
            self.FindWindowById(self.win['volume']['slice']['reset']).Disable()
        
        self.UpdateSliceLabels()
        
    def UpdateSliceLabels(self):
        """!Update text labels of slice controls according to axis"""
        sel = self.FindWindowById(self.win['volume']['slice']['axes']).GetSelection()
        if sel == 0:
            self.FindWindowByName('label_edge_0').SetLabel(_("North edge:"))
            self.FindWindowByName('label_edge_1').SetLabel(_("South edge:"))
            self.FindWindowByName('label_edge_2').SetLabel(_("West edge:"))
            self.FindWindowByName('label_edge_3').SetLabel(_("East edge:"))
            
            self.FindWindowByName('label_coord_0').SetLabel(_("Northing (Y):"))
            self.FindWindowByName('label_coord_1').SetLabel(_("Height (Z):"))
            self.FindWindowByName('label_coord_2').SetLabel(_("Easting (X):"))
        elif sel == 1:
            self.FindWindowByName('label_edge_0').SetLabel(_("West edge:"))
            self.FindWindowByName('label_edge_1').SetLabel(_("East edge:"))
            self.FindWindowByName('label_edge_2').SetLabel(_("North edge:"))
            self.FindWindowByName('label_edge_3').SetLabel(_("South edge:"))
            
            self.FindWindowByName('label_coord_0').SetLabel(_("Easting (X):"))
            self.FindWindowByName('label_coord_1').SetLabel(_("Height (Z):"))
            self.FindWindowByName('label_coord_2').SetLabel(_("Northing (Y):"))
        else:
            self.FindWindowByName('label_edge_0').SetLabel(_("West edge:"))
            self.FindWindowByName('label_edge_1').SetLabel(_("East edge:"))
            self.FindWindowByName('label_edge_2').SetLabel(_("Bottom edge:"))
            self.FindWindowByName('label_edge_3').SetLabel(_("Top edge:"))  
            
            self.FindWindowByName('label_coord_0').SetLabel(_("Easting (X):"))
            self.FindWindowByName('label_coord_1').SetLabel(_("Northing (Y):"))
            self.FindWindowByName('label_coord_2').SetLabel(_("Height (Z):")) 
        
    def SetPage(self, name):
        """!Get named page"""
        if name == 'view':
            self.SetSelection(0)
        elif name in ('surface', 'vector', 'volume'):
            self.SetSelection(1)
        else:
            self.SetSelection(2)

        win = self.FindWindowById(self.page[name]['notebook'])
        try:
            win.Expand(win.GetFoldPanel(self.page[name]['id']))
            self.UpdateScrolling((win.GetFoldPanel(self.page[name]['id']).GetGrandParent(),))
        except AttributeError:
            win.SetSelection(self.page[name]['id'])

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
        
        # probably does nothing, removed from wxPython 2.9
        # self.PrepareDC(dc)
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
        
        x, y = self.TransformCoordinates(xcoord, ycoord)
        self.data['position']['x'] = x        
        self.data['position']['y'] = y
        
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
        
        self.Draw(pos = (x,y), scale = True)

class ViewPositionWindow(PositionWindow):
    """!View position control widget"""
    def __init__(self, parent, mapwindow, id = wx.ID_ANY,
                 **kwargs):
        PositionWindow.__init__(self, parent, mapwindow, id, **kwargs)
        self.SetToolTipString(_("Adjusts the distance and direction of the image viewpoint"))
        self.data = self.mapWindow.view
        self.PostDraw()
        
    def UpdatePos(self, xcoord, ycoord):
        x, y = PositionWindow.UpdatePos(self, xcoord, ycoord)
        
        event = wxUpdateView(zExag = True)
        wx.PostEvent(self.mapWindow, event)

        return x, y
    
    def TransformCoordinates(self, x, y, toLight = True):
        return x, y
        
    def OnMouse(self, event):
        self.mapWindow.iview['dir']['use'] = False # use focus instead of viewdir
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
        self.SetToolTipString(_("Adjusts the light direction. "
                                "Click and drag the puck to change the light direction."))
        
        self.data = self.mapWindow.light
        self.quick = False
        self.PostDraw()

    def UpdatePos(self, xcoord, ycoord):
        x, y = PositionWindow.UpdatePos(self, xcoord, ycoord)
        
        event = wxUpdateLight(refresh = False)
        wx.PostEvent(self.mapWindow, event)
        
        return x, y
    
    def TransformCoordinates(self, x, y, toLight = True):
        if toLight:
            x = 2 * x - 1
            y = -2 * y + 1
        else:
            x = (x + 1)/2
            y = (1 - y)/2
        return x, y
    
    def PostDraw(self):
        event = wxUpdateLight(refresh = True)
        wx.PostEvent(self.mapWindow, event)
        x, y = self.data['position']['x'], self.data['position']['y']
        x, y = self.TransformCoordinates(x, y, toLight = False)
        
        self.Draw(pos = (x,y), scale = True)
        
    def OnMouse(self, event):
        PositionWindow.OnMouse(self, event)
        if event.LeftUp():
            self.mapWindow.render['quick'] = False
            self.mapWindow.Refresh(eraseBackground = False)
