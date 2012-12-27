"""!
@package iclass.toolbars

@brief wxIClass toolbars and icons.

Classes:
 - toolbars::IClassMapToolbar
 - toolbars::IClassToolbar
 - toolbars::IClassMapManagerToolbar
 - toolbars::IClassMiscToolbar

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon
from iclass.dialogs import IClassMapDialog
from gui_core.forms import GUI

import grass.script as grass

iClassIcons = {
        'opacity' : MetaIcon(img = 'layer-opacity',
                             label = _('Set opacity level')),
        'classManager' : MetaIcon(img = 'table-manager',
                                  label = _('Class manager')),
        'selectGroup' : MetaIcon(img = 'layer-group-add',
                                 label = _('Select imagery group')),
        'run' : MetaIcon(img = 'execute',
                         label = _('Run analysis')),
        'sigFile' : MetaIcon(img = 'script-save',
                             label = _('Save signature file')),
        'delCmd' : MetaIcon(img = 'layer-remove',
                            label = _('Delete selected map layer')),
        'exportAreas' : MetaIcon(img = 'layer-export',
                            label = _('Export training areas')),
        'importAreas' : MetaIcon(img = 'layer-import',
                            label = _('Import training areas')),
        'addRgb' : MetaIcon(img = 'layer-rgb-add',
                            label = _('Add RGB map layer')),
        }
        
class IClassMapToolbar(BaseToolbar):
    """!IClass Map toolbar
    """
    def __init__(self, parent):
        """!IClass Map toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # add tool to toggle active map window
        self.togglemapid = wx.NewId()
        self.togglemap = wx.Choice(parent = self, id = self.togglemapid,
                                   choices = [_('Training'), _('Preview')],
                                   style = wx.CB_READONLY)

        self.InsertControl(9, self.togglemap)

        self.SetToolShortHelp(self.togglemapid, '%s %s %s' % (_('Set map canvas for '),
                                                              BaseIcons["zoomBack"].GetLabel(),
                                                              _('/ Zoom to map')))

        # realize the toolbar
        self.Realize()
        
        self.action = { 'id' : self.pan }
        self.defaultAction = { 'id' : self.pan,
                               'bind' : self.parent.OnPan }
        
        self.OnTool(None)
        
        self.EnableTool(self.zoomBack, False)
        
    def GetActiveMapTool(self):
        """!Return widget for selecting active maps"""
        return self.togglemap
        
    def GetActiveMap(self):
        """!Get currently selected map"""
        return self.togglemap.GetSelection()
        
    def SetActiveMap(self, index):
        """!Set currently selected map"""
        return self.togglemap.SetSelection(index)
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = BaseIcons
        return self._getToolbarData((("displaymap", icons["display"],
                                      self.parent.OnDraw),
                                     ("rendermap", icons["render"],
                                      self.parent.OnRender),
                                     ("erase", icons["erase"],
                                      self.parent.OnErase),
                                     (None, ),
                                     ("pan", icons["pan"],
                                      self.parent.OnPan,
                                      wx.ITEM_CHECK),
                                     ("zoomIn", icons["zoomIn"],
                                      self.parent.OnZoomIn,
                                      wx.ITEM_CHECK),
                                     ("zoomOut", icons["zoomOut"],
                                      self.parent.OnZoomOut,
                                      wx.ITEM_CHECK),
                                     ("zoomMenu", icons["zoomMenu"],
                                      self.parent.OnZoomMenu),
                                     (None, ),
                                     ("zoomBack", icons["zoomBack"],
                                      self.parent.OnZoomBack),
                                     ("zoomToMap", icons["zoomExtent"],
                                      self.parent.OnZoomToMap),
                                    ))
class IClassToolbar(BaseToolbar):
    """!IClass toolbar
    """
    def __init__(self, parent):
        """!IClass toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        self.InitToolbar(self._toolbarData())
        
        self.choice = wx.Choice(parent = self, id = wx.ID_ANY, size = (110, -1))
        choiceid = self.InsertControl(3, self.choice)
        
        self.choice.Bind(wx.EVT_CHOICE, self.OnSelectCategory)
        
        # stupid workaround to insert small space between controls
        self.InsertControl(4, wx.StaticText(self, id = wx.ID_ANY, label = ' '))
        
        self.combo = wx.ComboBox(self, id = wx.ID_ANY, size = (130, -1), 
                                 style = wx.TE_PROCESS_ENTER)
        self.InitStddev()
        comboid = self.InsertControl(5, self.combo)
        
        self.EnableControls(False)
        
        self.combo.Bind(wx.EVT_COMBOBOX, self.OnStdChangeSelection)
        self.combo.Bind(wx.EVT_TEXT_ENTER, self.OnStdChangeText)
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = iClassIcons
        return self._getToolbarData((("selectGroup", icons['selectGroup'],
                                      self.parent.OnAddBands),
                                      (None, ),
                                      ("classManager", icons['classManager'],
                                      self.parent.OnCategoryManager),
                                      (None, ),
                                      ("runAnalysis", icons['run'],
                                      self.parent.OnRunAnalysis),
                                      (None, ),
                                      ("importAreas", icons['importAreas'],
                                      self.parent.OnImportAreas),
                                      ("exportAreas", icons['exportAreas'],
                                      self.parent.OnExportAreas),
                                      ("sigFile", icons['sigFile'],
                                      self.parent.OnSaveSigFile),
                                    ))
                                    
    def OnSelectCategory(self, event):
        idx = self.choice.GetSelection()
        cat = self.choice.GetClientData(idx)
        
        self.parent.CategoryChanged(currentCat = cat)
        
    def SetCategories(self, catNames, catIdx):
        self.choice.Clear()
        for name, idx in zip(catNames, catIdx):
            self.choice.Append(name, idx)
        
    def GetSelectedCategoryName(self):
        return self.choice.GetStringSelection()
        
    def GetSelectedCategoryIdx(self):
        idx = self.choice.GetSelection()
        if idx != wx.NOT_FOUND:
            return self.choice.GetClientData(idx)
        
        return None
        
    def OnStdChangeSelection(self, event):
        idx = self.combo.GetSelection()
        nstd = self.combo.GetClientData(idx)
        
        self.StddevChanged(nstd)
        
    def OnStdChangeText(self, event):
        val = self.combo.GetValue().strip()
        try:
            nstd = float(val)
        except ValueError:
            try:
                nstd = float(val.split()[0])
            except ValueError:
                nstd = None
                
        if nstd is not None:
            self.StddevChanged(nstd)
            
    def StddevChanged(self, nstd):
        idx = self.GetSelectedCategoryIdx()
        if not idx:
            return
            
        self.parent.StddevChanged(cat = idx, nstd = nstd)
        
    def UpdateStddev(self, nstd):
        self.combo.SetValue(' '.join(("%.2f" % nstd, _('std dev'))))
        
    def InitStddev(self):
        for nstd in range(50, 250, 25):
            nstd /= 100.
            self.combo.Append(item = ' '.join(("%.2f" % nstd, _('std dev'))), clientData = nstd)
        self.combo.SetSelection(4) # 1.5
        
    def EnableControls(self, enable = True):
        self.combo.Enable(enable)
        self.choice.Enable(enable)
        
class IClassMapManagerToolbar(BaseToolbar):
    """!IClass toolbar
    """
    def __init__(self, parent, mapManager):
        """!IClass toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        self.choice = wx.Choice(parent = self, id = wx.ID_ANY, size = (300, -1))
        
        self.choiceid = self.AddControl(self.choice)
        
        self.choice.Bind(wx.EVT_CHOICE, self.OnSelectLayer)
        
        self.mapManager = mapManager
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        return self._getToolbarData((("addRast", BaseIcons['addRast'],
                                      self.OnAddRast),
                                     ('addRgb', iClassIcons['addRgb'],
                                      self.OnAddRGB),
                                     ("delRast", iClassIcons['delCmd'],
                                     self.OnDelRast),
                                     ("setOpacity", iClassIcons['opacity'],
                                     self.OnSetOpacity),
                                    ))
                                    
    def OnSelectLayer(self, event):
        layer = self.choice.GetStringSelection()
        self.mapManager.SelectLayer(name = layer)
        
    def OnAddRast(self, event):
        dlg = IClassMapDialog(self, title = _("Add raster map"), element = 'raster')
        if dlg.ShowModal() == wx.ID_OK:
            raster = grass.find_file(name = dlg.GetMap(), element = 'cell')
            if raster['fullname']:
                self.mapManager.AddLayer(name = raster['fullname'])
                
        dlg.Destroy()
        
    def OnAddRGB(self, event):
        cmd = ['d.rgb']
        GUI(parent = self.parent).ParseCommand(cmd, completed = (self.GetOptData, '', ''))

    def GetOptData(self, dcmd, layer, params, propwin):
        if dcmd:
            self.mapManager.AddLayerRGB(cmd = dcmd)

    def OnDelRast(self, event):
        layer = self.choice.GetStringSelection()
        idx = self.choice.GetSelection()
        if layer:
            self.mapManager.RemoveLayer(name = layer, idx = idx)
            
    def OnSetOpacity(self, event):
        layer = self.choice.GetStringSelection()
        idx = self.choice.GetSelection()
        if idx == wx.NOT_FOUND:
            return
            
        self.mapManager.SetOpacity(name = layer)

class IClassMiscToolbar(BaseToolbar):
    """!IClass toolbar
    """
    def __init__(self, parent):
        """!IClass toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = BaseIcons
        return self._getToolbarData((("help", icons['help'],
                                      self.parent.OnHelp),
                                     ("quit", icons['quit'],
                                      self.parent.OnCloseWindow),
                                     ))
    
