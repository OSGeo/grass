# -*- coding: utf-8 -*-
"""!
@package gui_core.simplelmgr

@brief GUI class for simple layer management.

Classes:
 - simplelmgr::SimpleLayerManager
 - simplelmgr::SimpleLmgrToolbar

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova (kratochanna gmail.com)
"""
import os
import sys

# adding a path to wxGUI modules
if __name__ == '__main__':
    WXGUIBASE = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if WXGUIBASE not in sys.path:
        sys.path.append(WXGUIBASE)
import wx
import wx.aui

from grass.pydispatch.signal import Signal

from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon
from gui_core.forms import GUI
from gui_core.dialogs import SetOpacityDialog
from core.utils import GetLayerNameFromCmd
from core.gcmd import GError
from core.layerlist import LayerList

SIMPLE_LMGR_RASTER = 1
SIMPLE_LMGR_VECTOR = 2
SIMPLE_LMGR_RASTER3D = 4
SIMPLE_LMGR_TB_TOP = 8
SIMPLE_LMGR_TB_BOTTOM = 16
SIMPLE_LMGR_TB_LEFT = 32
SIMPLE_LMGR_TB_RIGHT = 64


class SimpleLayerManager(wx.Panel):
    """!Simple layer manager class provides similar functionality to
    Layertree, but it's just list, not tree."""
    def __init__(self, parent, layerList,
                 lmgrStyle=SIMPLE_LMGR_RASTER | SIMPLE_LMGR_VECTOR | SIMPLE_LMGR_TB_LEFT,
                 toolbarCls=None, modal=False):
        wx.Panel.__init__(self, parent=parent, name='SimpleLayerManager')

        self._style = lmgrStyle
        self._layerList = layerList
        self._checkList = wx.CheckListBox(self, style=wx.LB_EXTENDED)
        # dialog windows held separately
        self._dialogs = {}
        if not toolbarCls:
            toolbarCls = SimpleLmgrToolbar
        self._toolbar = toolbarCls(self, lmgrStyle=self._style)

        self._auimgr = wx.aui.AuiManager(self)
        
        self._modal = modal
        

        # needed in order not to change selection when moving layers
        self._blockSelectionChanged = False

        self._checkList.Bind(wx.EVT_LISTBOX, lambda evt: self._selectionChanged())
        self._checkList.Bind(wx.EVT_LISTBOX_DCLICK, self.OnLayerChangeProperties)
        self._checkList.Bind(wx.EVT_CHECKLISTBOX, self.OnLayerChecked)

        # signal emitted when somethin in layer list changes
        self.opacityChanged = Signal('SimpleLayerManager.opacityChanged')
        self.cmdChanged = Signal('SimpleLayerManager.cmdChanged')
        self.layerAdded = Signal('SimpleLayerManager.layerAdded')
        self.layerRemoved = Signal('SimpleLayerManager.layerRemoved')
        self.layerActivated = Signal('SimpleLayerManager.layerActivated')
        self.layerMovedUp = Signal('SimpleLayerManager.layerMovedUp')
        self.layerMovedDown = Signal('SimpleLayerManager.layerMovedDown')
        # emitted by any change (e.g. for rerendering)
        self.anyChange = Signal('SimpleLayerManager.layerChange')

        self._layout()
        self.SetMinSize((200, -1))
        self._update()

    def _layout(self):
        self._auimgr.AddPane(self._checkList,
                             wx.aui.AuiPaneInfo().
                             Name("checklist").
                             CenterPane().
                             CloseButton(False).
                             BestSize((self._checkList.GetBestSize())))
        paneInfo = wx.aui.AuiPaneInfo(). \
                   Name("toolbar").Caption(_("Toolbar")).ToolbarPane(). \
                   CloseButton(False).Layer(1).Gripper(False). \
                   BestSize((self._toolbar.GetBestSize()))
        if self._style & SIMPLE_LMGR_TB_LEFT:
            paneInfo.Left()
        elif self._style & SIMPLE_LMGR_TB_RIGHT:
            paneInfo.Right()
        elif self._style & SIMPLE_LMGR_TB_TOP:
            paneInfo.Top()
        else:
            paneInfo.Bottom()

        self._auimgr.AddPane(self._toolbar, paneInfo)
        self._auimgr.Update()

    def _selectionChanged(self):
        """!Selection was changed externally,
        updates selection info in layers."""
        if self._blockSelectionChanged:
            return
        selected = self._checkList.GetSelections()
        for i, layer in enumerate(self._layerList):
            layer.Select(i in selected)

    def OnLayerChecked(self, event):
        """!Layer was (un)checked, update layer's info."""
        checkedIdxs = self._checkList.GetChecked()
        for i, layer in enumerate(self._layerList):
            if i in checkedIdxs and not layer.IsActive():
                layer.Activate()
                self.layerActivated.emit(index=i, layer=layer)
            elif i not in checkedIdxs and layer.IsActive():
                layer.Activate(False)
                self.layerActivated.emit(index=i, layer=layer)
        self.anyChange.emit()
        event.Skip()

    def OnAddRaster(self, event):
        """!Opens d.rast dialog and adds layer.
        Dummy layer is added first."""
        cmd = ['d.rast']
        layer = self.AddRaster(name='', cmd=cmd, hidden=True, dialog=None)
        GUI(parent=self, giface=None, modal=self._modal).ParseCommand(cmd=cmd,
                                                    completed=(self.GetOptData, layer, ''))
        event.Skip()

    def OnAddVector(self, event):
        """!Opens d.vect dialog and adds layer.
        Dummy layer is added first."""
        cmd = ['d.vect']

        layer = self.AddVector(name='', cmd=cmd, hidden=True, dialog=None)
        GUI(parent=self, giface=None, modal=self._modal).ParseCommand(cmd=cmd,
                                                   completed=(self.GetOptData, layer, ''))
        event.Skip()

    def OnAddRast3d(self, event):
        """!Opens d.rast3d dialog and adds layer.
        Dummy layer is added first."""
        cmd = ['d.rast3d']
        layer = self.AddRast3d(name='', cmd=cmd, hidden=True, dialog=None)
        GUI(parent=self, giface=None, modal=self._modal).ParseCommand(cmd=cmd,
                                                   completed=(self.GetOptData, layer, ''))
        event.Skip()

    def OnRemove(self, event):
        """!Removes selected layers from list."""
        layers = self._layerList.GetSelectedLayers(activeOnly=False)
        for layer in layers:
            self.layerRemoved.emit(index=self._layerList.GetLayerIndex(layer), layer=layer)
            self._layerList.RemoveLayer(layer)
            if layer in self._dialogs and self._dialogs[layer]:
                self._dialogs[layer].Destroy()
        self._update()
        self.anyChange.emit()
        event.Skip()

    def OnLayerUp(self, event):
        """!Moves selected layers one step up.

        Note: not completely correct for multiple layers."""
        layers = self._layerList.GetSelectedLayers()
        self._blockSelectionChanged = True
        for layer in layers:
            idx = self._layerList.GetLayerIndex(layer)
            if idx > 0:
                self.layerMovedUp.emit(index=idx, layer=layer)
                self._layerList.MoveLayerUp(layer)
        self._update()
        self._blockSelectionChanged = False
        self.anyChange.emit()
        event.Skip()

    def OnLayerDown(self, event):
        """!Moves selected layers one step down.

        Note: not completely correct for multiple layers."""
        layers = self._layerList.GetSelectedLayers()
        self._blockSelectionChanged = True
        for layer in layers:
            idx = self._layerList.GetLayerIndex(layer)
            if idx < len(self._layerList) - 1:
                self.layerMovedDown.emit(index=self._layerList.GetLayerIndex(layer), layer=layer)
                self._layerList.MoveLayerDown(layer)
        self._update()
        self._blockSelectionChanged = False
        self.anyChange.emit()
        event.Skip()

    def OnLayerChangeProperties(self, event):
        """Opens module dialog to edit layer command."""
        layers = self._layerList.GetSelectedLayers()
        if not layers or len(layers) > 1:
            return
        self._layerChangeProperties(layers[0])
        event.Skip()

    def _layerChangeProperties(self, layer):
        """!Opens new module dialog or recycles it."""
        if layer in self._dialogs:
            dlg = self._dialogs[layer]
            if dlg.IsShown():
                dlg.Raise()
                dlg.SetFocus()
            else:
                dlg.Show()
        else:
            GUI(parent=self, giface=None,
                modal=self._modal).ParseCommand(cmd=layer.cmd,
                                          completed=(self.GetOptData, layer, ''))

    def OnLayerChangeOpacity(self, event):
        """!Opacity of a layer is changing."""
        layers = self._layerList.GetSelectedLayers()
        if not layers or len(layers) > 1:
            return
        layer = layers[0]
        dlg = SetOpacityDialog(self, opacity=layer.opacity,
                               title=_("Set opacity of <%s>") % layer.name)
        dlg.applyOpacity.connect(lambda value:
                                 self._setLayerOpacity(layer, value))
        dlg.CentreOnParent()

        if dlg.ShowModal() == wx.ID_OK:
            self._setLayerOpacity(layer, dlg.GetOpacity())
        dlg.Destroy()
        event.Skip()

    def _setLayerOpacity(self, layer, value):
        """!Sets layer's opacity.'"""
        layer.opacity = value 
        self._update()
        self.opacityChanged.emit(index=self._layerList.GetLayerIndex(layer), layer=layer)
        self.anyChange.emit()

    def _update(self):
        """!Updates checklistbox according to layerList structure."""
        items = []
        active = []
        selected = []
        for layer in self._layerList:
            if layer.opacity < 1:
                items.append("{name} (opacity {opacity}%)".format(name=layer.name,
                                                                  opacity=int(layer.opacity * 100)))
            else:
                items.append(layer.name)
            active.append(layer.IsActive())
            selected.append(layer.IsSelected())

        self._checkList.SetItems(items)
        for i, check in enumerate(active):
            self._checkList.Check(i, check)

        for i, layer in enumerate(self._layerList):
            if selected[i]:
                self._checkList.Select(i)
            else:
                self._checkList.Deselect(i)

    def GetOptData(self, dcmd, layer, params, propwin):
        """!Handler for module dialogs."""
        if dcmd:
            layer.cmd = dcmd
            self._dialogs[layer] = propwin
            layer.selected = True
            mapName, found = GetLayerNameFromCmd(dcmd)
            if found:
                try:
                    if layer.hidden:
                        layer.hidden = False
                        signal = self.layerAdded
                    else:
                        signal = self.cmdChanged

                    layer.name = mapName
                    signal.emit(index=self._layerList.GetLayerIndex(layer), layer=layer)
                except ValueError, e:
                    self._layerList.RemoveLayer(layer)
                    GError(parent=self,
                           message=str(e),
                           showTraceback=False)

            self._update()
            self.anyChange.emit()

    def AddRaster(self, name, cmd, hidden, dialog):
        """!Ads new raster layer."""
        layer = self._layerList.AddNewLayer(name=name, mapType='rast',
                                            active=True,
                                            cmd=cmd, hidden=hidden)
        self._dialogs[layer] = dialog
        return layer

    def AddRast3d(self, name, cmd, hidden, dialog):
        """!Ads new raster3d layer."""
        layer = self._layerList.AddNewLayer(name=name, mapType='rast3d',
                                            active=True,
                                            cmd=cmd, hidden=hidden)
        self._dialogs[layer] = dialog
        return layer

    def AddVector(self, name, cmd, hidden, dialog):
        """!Ads new vector layer."""
        layer = self._layerList.AddNewLayer(name=name, mapType='vect',
                                            active=True,
                                            cmd=cmd, hidden=hidden)
        self._dialogs[layer] = dialog
        return layer

    def GetLayerInfo(self, layer, key):
        """!Just for compatibility, should be removed in the future"""
        value = getattr(layer, key)
        # hack to return empty list, required in OnCancel in forms
        # not sure why it should be empty
        if key == 'cmd' and len(value) == 1:
            return []
        return value

    def Delete(self, layer):
        """!Just for compatibility, should be removed in the future"""
        self._layerList.RemoveLayer(layer)
        if self._dialogs[layer]:
            self._dialogs[layer].Destroy()


class SimpleLmgrToolbar(BaseToolbar):
    """!Toolbar of simple layer manager.

    Style of the toolbar can be changed (horizontal,
    vertical, which map types to include).
    """
    def __init__(self, parent, lmgrStyle):
        """!Toolbar constructor
        """
        self._style = lmgrStyle
        if lmgrStyle & (SIMPLE_LMGR_TB_LEFT | SIMPLE_LMGR_TB_RIGHT):
            direction = wx.TB_VERTICAL
        else:
            direction = wx.TB_HORIZONTAL
        BaseToolbar.__init__(self, parent, style=wx.NO_BORDER | direction)

        self.InitToolbar(self._getToolbarData(self._toolbarData()))

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data"""
        data = [('edit', icons['edit'],
                 self.parent.OnLayerChangeProperties),
                 ('remove', icons['remove'],
                 self.parent.OnRemove),
                 (None, ),
                 ('up', icons['up'],
                 self.parent.OnLayerUp),
                 ('down', icons['down'],
                 self.parent.OnLayerDown),
                 (None, ),
                 ('opacity', icons['opacity'],
                 self.parent.OnLayerChangeOpacity),
                 ]
        if self._style & SIMPLE_LMGR_RASTER3D:
            data.insert(0, ('addRaster3d', icons['addRast3d'],
                            self.parent.OnAddRast3d))
        if self._style & SIMPLE_LMGR_VECTOR:
            data.insert(0, ('addVector', BaseIcons['addVect'],
                            self.parent.OnAddVector))
        if self._style & SIMPLE_LMGR_RASTER:
            data.insert(0, ('addRaster', BaseIcons['addRast'],
                            self.parent.OnAddRaster))

        return data


icons = {
    'remove': MetaIcon(img='layer-remove',
                       label=_("Remove"),
                       desc=_("Remove selected map(s) from list")),
    'up': MetaIcon(img='layer-up',
                   label=_("Layer up"),
                   desc=_("Move selected layer(s) up")),
    'down': MetaIcon(img='layer-down',
                     label=_("Layer down"),
                     desc=_("Move selected layer(s) down")),
    'edit': MetaIcon(img='layer-edit',
                     label=_("Edit layer properties"),
                     desc=_("Edit layer properties")),
    'opacity': MetaIcon(img='layer-opacity',
                        label=_("Change opacity"),
                        desc=_("Change layer opacity")),
    'addRast3d': MetaIcon(img='layer-raster3d-add',
                          label=_("Add 3D raster map layer"),
                          desc=_("Add 3D raster map layer")),
    }


class TestFrame(wx.Frame):
    def __init__(self, parent):
        wx.Frame.__init__(self, parent=parent)
        SimpleLayerManager(parent=self, layerList=LayerList())


def test():
    app = wx.App()
    frame = TestFrame(None)
    frame.Show()
    app.MainLoop()

if __name__ == '__main__':
    test()
