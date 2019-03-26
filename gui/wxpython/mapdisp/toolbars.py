"""
@package mapdisp.toolbars

@brief Map display frame - toolbars

Classes:
 - toolbars::MapToolbar

(C) 2007-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar, BaseIcons
from nviz.main import haveNviz
from vdigit.main import haveVDigit
from icons.icon import MetaIcon

MapIcons = {
    'query': MetaIcon(img='info',
                      label=_('Query raster/vector map(s)'),
                      desc=_('Query selected raster/vector map(s)')),
    'select': MetaIcon(img='select',
                       label=_('Select vector feature(s)'),
                       desc=_('Select features interactively from vector map')),
    'addBarscale': MetaIcon(img='scalebar-add',
                            label=_('Add scale bar')),
    'addRasterLegend': MetaIcon(img='legend-add',
                          label=_('Add raster legend')),
    'addVectorLegend': MetaIcon(img='legend-add',
                                label=_('Add vector legend')),
    'addNorthArrow': MetaIcon(img='north-arrow-add',
                              label=_('Add north arrow')),
    'analyze': MetaIcon(img='layer-raster-analyze',
                        label=_('Analyze map'),
                        desc=_('Measuring, profiling, histogramming, ...')),
    'measureDistance': MetaIcon(img='measure-length',
                                label=_('Measure distance')),
    'measureArea': MetaIcon(img='area-measure',
                            label=_('Measure area')),
    'profile': MetaIcon(img='layer-raster-profile',
                        label=_('Profile surface map')),
    'scatter': MetaIcon(img='layer-raster-profile',
                        label=_("Create bivariate scatterplot of raster maps")),
    'addText': MetaIcon(img='text-add',
                        label=_('Add text')),
    'histogram': MetaIcon(img='layer-raster-histogram',
                          label=_('Create histogram of raster map')),
    'vnet': MetaIcon(img='vector-tools',
                     label=_('Vector network analysis tool')),
}

NvizIcons = {
    'rotate': MetaIcon(
        img='3d-rotate',
        label=_('Rotate 3D scene'),
        desc=_('Drag with mouse to rotate 3D scene')),
    'flyThrough': MetaIcon(
        img='flythrough',
        label=_('Fly-through mode'),
        desc=_(
            'Drag with mouse, hold Ctrl down for different mode'
            ' or Shift to accelerate')),
    'zoomIn': BaseIcons['zoomIn'].SetLabel(
        desc=_('Click mouse to zoom')),
    'zoomOut': BaseIcons['zoomOut'].SetLabel(
        desc=_('Click mouse to unzoom'))}


class MapToolbar(BaseToolbar):
    """Map Display toolbar
    """

    def __init__(self, parent, toolSwitcher):
        """Map Display constructor

        :param parent: reference to MapFrame
        """
        BaseToolbar.__init__(
            self,
            parent=parent,
            toolSwitcher=toolSwitcher)  # MapFrame

        self.InitToolbar(self._toolbarData())
        self._default = self.pointer

        # optional tools
        toolNum = 0
        choices = [_('2D view'), ]
        self.toolId = {'2d': toolNum}
        toolNum += 1
        if self.parent.GetLayerManager():
            log = self.parent.GetLayerManager().GetLogWindow()

        if haveNviz:
            choices.append(_('3D view'))
            self.toolId['3d'] = toolNum
            toolNum += 1
        else:
            from nviz.main import errorMsg
            if self.parent.GetLayerManager():
                log.WriteCmdLog(_('3D view mode not available'))
                log.WriteWarning(_('Reason: %s') % str(errorMsg))

            self.toolId['3d'] = -1

        if haveVDigit:
            choices.append(_("Vector digitizer"))
            self.toolId['vdigit'] = toolNum
            toolNum += 1
        else:
            from vdigit.main import errorMsg
            if self.parent.GetLayerManager():
                log.WriteCmdLog(_('Vector digitizer not available'))
                log.WriteWarning(_('Reason: %s') % errorMsg)
                log.WriteLog(
                    _(
                        'Note that the wxGUI\'s vector digitizer is disabled in this installation. '
                        'Please keep an eye out for updated versions of GRASS. '
                        'In the meantime you can use "v.edit" for non-interactive editing '
                        'from the Develop vector map menu.'),
                    wrap=60)

            self.toolId['vdigit'] = -1
        choices.append(_("Raster digitizer"))
        self.toolId['rdigit'] = toolNum

        self.combo = wx.ComboBox(parent=self, id=wx.ID_ANY,
                                 choices=choices,
                                 style=wx.CB_READONLY, size=(110, -1))
        self.combo.SetSelection(0)

        self.comboid = self.AddControl(self.combo)
        self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectTool, self.comboid)

        # realize the toolbar
        self.Realize()

        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.combo.Hide()
        self.combo.Show()

        for tool in (self.pointer, self.select, self.query,
                     self.pan, self.zoomIn, self.zoomOut):
            self.toolSwitcher.AddToolToGroup(
                group='mouseUse', toolbar=self, tool=tool)

        self.EnableTool(self.zoomBack, False)

        self.FixSize(width=90)

    def _toolbarData(self):
        """Toolbar data"""
        return self._getToolbarData((
            ('renderMap', BaseIcons['render'],
             self.parent.OnRender),
            ('pointer', BaseIcons['pointer'],
             self.parent.OnPointer,
             wx.ITEM_CHECK),
            ('select', MapIcons['select'],
             self.parent.OnSelect,
             wx.ITEM_CHECK),
            ('query', MapIcons['query'],
             self.parent.OnQuery,
             wx.ITEM_CHECK),
            ('pan', BaseIcons['pan'],
             self.parent.OnPan,
             wx.ITEM_CHECK),
            ('zoomIn', BaseIcons['zoomIn'],
             self.parent.OnZoomIn,
             wx.ITEM_CHECK),
            ('zoomOut', BaseIcons['zoomOut'],
             self.parent.OnZoomOut,
             wx.ITEM_CHECK),
            ('zoomExtent', BaseIcons['zoomExtent'],
             self.parent.OnZoomToMap),
            ('zoomRegion', BaseIcons['zoomRegion'],
             self.parent.OnZoomToWind),
            ('zoomBack', BaseIcons['zoomBack'],
             self.parent.OnZoomBack),
            ('zoomMenu', BaseIcons['zoomMenu'],
             self.parent.OnZoomMenu),
            ('analyze', MapIcons['analyze'],
             self.OnAnalyze),
            ('overlay', BaseIcons['overlay'],
             self.OnDecoration),
            ('saveFile', BaseIcons['saveFile'],
             self.parent.SaveToFile),
        ))

    def InsertTool(self, data):
        """Insert tool to toolbar

        :param data: toolbar data"""
        data = self._getToolbarData(data)
        for tool in data:
            self.CreateTool(*tool)
        self.Realize()

        self.parent._mgr.GetPane('mapToolbar').BestSize(self.GetBestSize())
        self.parent._mgr.Update()

    def RemoveTool(self, tool):
        """Remove tool from toolbar

        :param tool: tool id"""
        self.DeleteTool(tool)

        self.parent._mgr.GetPane('mapToolbar').BestSize(self.GetBestSize())
        self.parent._mgr.Update()

    def ChangeToolsDesc(self, mode2d):
        """Change description of zoom tools for 2D/3D view"""
        if mode2d:
            icons = BaseIcons
        else:
            icons = NvizIcons
        for i, data in enumerate(self._data):
            for tool in (('zoomIn', 'zoomOut')):
                if data[0] == tool:
                    tmp = list(data)
                    tmp[4] = icons[tool].GetDesc()
                    self._data[i] = tuple(tmp)

    def OnSelectTool(self, event):
        """Select / enable tool available in tools list
        """
        tool = event.GetSelection()

        if tool == self.toolId['2d']:
            self.ExitToolbars()
            self.Enable2D(True)

        elif tool == self.toolId['3d'] and \
                not (self.parent.MapWindow3D and self.parent.IsPaneShown('3d')):
            self.ExitToolbars()
            self.parent.AddNviz()

        elif tool == self.toolId['vdigit'] and \
                not self.parent.GetToolbar('vdigit'):
            self.ExitToolbars()
            self.parent.AddToolbar("vdigit")
            self.parent.MapWindow.SetFocus()

        elif tool == self.toolId['rdigit']:
            self.ExitToolbars()
            self.parent.AddRDigit()

    def OnAnalyze(self, event):
        """Analysis tools menu
        """
        self._onMenu(((MapIcons["measureDistance"], self.parent.OnMeasureDistance),
                      (MapIcons["measureArea"], self.parent.OnMeasureArea),
                      (MapIcons["profile"], self.parent.OnProfile),
                      (MapIcons["scatter"], self.parent.OnScatterplot),
                      (MapIcons["histogram"], self.parent.OnHistogramPyPlot),
                      (BaseIcons["histogramD"], self.parent.OnHistogram),
                      (MapIcons["vnet"], self.parent.OnVNet)))

    def OnDecoration(self, event):
        """Decorations overlay menu
        """
        self._onMenu(
            ((MapIcons["addRasterLegend"],
              lambda evt: self.parent.AddLegendRast()),
             (MapIcons["addVectorLegend"],
              lambda evt: self.parent.AddLegendVect()),
             (MapIcons["addBarscale"],
              lambda evt: self.parent.AddBarscale()),
             (MapIcons["addNorthArrow"],
              lambda evt: self.parent.AddArrow()),
             (MapIcons["addText"],
              lambda evt: self.parent.AddDtext())))

    def ExitToolbars(self):
        if self.parent.GetToolbar('vdigit'):
            self.parent.toolbars['vdigit'].OnExit()
        if self.parent.GetLayerManager() and \
                self.parent.GetLayerManager().IsPaneShown('toolbarNviz'):
            self.parent.RemoveNviz()
        if self.parent.GetToolbar('rdigit'):
            self.parent.QuitRDigit()

    def Enable2D(self, enabled):
        """Enable/Disable 2D display mode specific tools"""
        for tool in (self.zoomRegion,
                     self.zoomMenu,
                     self.analyze,
                     self.select):
            self.EnableTool(tool, enabled)
        self.ChangeToolsDesc(enabled)
        if enabled:
            self.combo.SetValue(_("2D view"))
