"""
@package gui_core.vselect

@brief wxGUI classes for interactive selection of vector
features. Allows creating a new vector map from selected vector
features or return their categories

Classes:
- vselect::VectorSelectList
- vselect::VectorSelectDialog
- vselect::VectorSelectBase
- vselect::VectorSelectHighlighter

(C) 2014-2015 by Matej Krejci, and the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Matej Krejci <matejkrejci gmail.com> (mentor: Martin Landa)
"""

import string
import random

import wx
import wx.lib.mixins.listctrl as listmix

from core.gcmd import GMessage, GError, GWarning
from core.gcmd import RunCommand
from gui_core.wrap import Button, ListCtrl

import grass.script as grass
from grass.pydispatch.signal import Signal


class VectorSelectList(ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """Widget for managing vector features selected from map display
    """

    def __init__(self, parent):
        ListCtrl.__init__(
            self,
            parent=parent,
            id=wx.ID_ANY,
            style=wx.LC_REPORT | wx.BORDER_SUNKEN)
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.InsertColumn(col=0, heading=_('category'))
        self.InsertColumn(col=1, heading=_('type'))
        self.SetColumnWidth(0, 100)
        self.SetColumnWidth(1, 100)

        self.index = 0
        self.dictIndex = {}

    def AddItem(self, item):
        if 'Category' not in item:
            return

        pos = self.InsertItem(0, str(item['Category']))
        self.SetItem(pos, 1, str(item['Type']))
        self.dictIndex[str(item['Category'])] = pos

    def RemoveItem(self, item):
        index = self.dictIndex.get(str(item['Category']), -1)
        if index > -1:
            self.DeleteItem(index)


class VectorSelectDialog(wx.Dialog):
    """Dialog for managing vector features selected from map display"""

    def __init__(self, parent, title=_("Select features"), size=(200, 300)):
        wx.Dialog.__init__(
            self,
            parent=parent,
            id=wx.ID_ANY,
            title=title,
            size=size,
            style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)

        self._layout()

    def AddWidget(self, widget, proportion=1, flag=wx.EXPAND):
        self.mainSizer.Add(widget, proportion=proportion, flag=flag)
        self.Layout()

    def _layout(self):
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(self.mainSizer)

        self.Show()


class VectorSelectBase():
    """@brief Main class of vector selection function

    It allows selecting vector features from map display and to export
    them as a new vector map. Current version allows selecting
    features one-by-one by single click in map display.

    This class can be initialized with (see CreateDialog()) or without
    (see gselect) dialog (see VectorSelectDialog).
    """

    def __init__(self, parent, giface):
        self.parent = parent
        self._giface = giface
        self.register = False
        self.mapWin = self._giface.GetMapWindow()
        self.mapDisp = giface.GetMapDisplay()
        self.RegisterMapEvtHandler()

        self.selectedFeatures = []
        self.mapName = None  # chosen map for selecting features

        self._dialog = None
        self.onCloseDialog = None

        self.updateLayer = Signal('VectorSelectBase.updateLayer')

        self.painter = VectorSelectHighlighter(self.mapDisp, giface)

    def CreateDialog(self, createButton=True):
        """Create dialog

        :param createButton: True to add 'create new map' button
        """
        if self._dialog:
            return

        self._dialog = VectorSelectDialog(parent=self.parent)
        self._dialog.Bind(wx.EVT_CLOSE, self.OnCloseDialog)
        if createButton:
            createMap = Button(
                self._dialog, wx.ID_ANY, _("Create a new map"))
            createMap.Bind(wx.EVT_BUTTON, self.OnExportMap)
            self._dialog.AddWidget(createMap, proportion=0.1)
        self.slist = VectorSelectList(self._dialog)
        self.slist.Bind(wx.EVT_LIST_KEY_DOWN, self.OnDelete)
        self.slist.Bind(wx.EVT_LIST_ITEM_RIGHT_CLICK, self.OnDeleteRow)
        self._dialog.AddWidget(self.slist)

        self.onCloseDialog = Signal('VectorSelectBase.onCloseDialog')

    def OnDeleteRow(self, event=None):
        """Delete row in widget
        """
        index = self.slist.GetFocusedItem()
        category = self.slist.GetItemText(index)
        for item in self.selectedFeatures:
            if int(item['Category']) == int(category):
                self.selectedFeatures.remove(item)
                break
        self.slist.DeleteItem(index)
        self._draw()

    def OnDelete(self, event):
        """Delete row in widget by press key(delete)
        """
        keycode = event.GetKeyCode()
        if keycode == wx.WXK_DELETE:
            self.OnDeleteRow()

    def RegisterMapEvtHandler(self):
        if not self.register:
            self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN,
                                                  self._onMapClickHandler,
                                                  'cross')
        self.register = True

    def UnregisterMapEvtHandler(self):
        """Unregistrates _onMapClickHandler from mapWin"""
        if self.register:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,
                                                    self._onMapClickHandler)
        self.register = False

    def OnClose(self):
        self.selectedFeatures = []
        self._draw()
        self.UnregisterMapEvtHandler()

    def OnCloseDialog(self, evt=None):
        if not self.onCloseDialog:
            return

        self.onCloseDialog.emit()
        self.selectedFeatures = []
        self.painter.Clear()
        self._dialog.Destroy()
        self.UnregisterMapEvtHandler()

    def Reset(self):
        """Remove items from dialog list"""
        self.selectedFeatures = []
        if self._dialog:
            self.slist.DeleteAllItems()
            self._dialog.Raise()
        self.RegisterMapEvtHandler()

    def _onMapClickHandler(self, event):
        """Registred handler for clicking on grass disp
        """
        if event == "unregistered":
            return
        vWhatDic = self.QuerySelectedMap()
        if 'Category' in vWhatDic:
            self.AddVecInfo(vWhatDic)
            self._draw()
            if self._dialog:
                self._dialog.Raise()

    def AddVecInfo(self, vInfoDictTMP):
        """Update vector in list

        Note: click on features add category
              second click on the same vector remove category from list
        """
        if len(self.selectedFeatures) > 0:
            for sel in self.selectedFeatures:
                if sel['Category'] == vInfoDictTMP[
                        'Category']:  # features is selected=> remove features
                    self.selectedFeatures.remove(sel)
                    if self._dialog:  # if dialog initilized->update dialog
                        self.slist.RemoveItem(vInfoDictTMP)
                    return True

            self.selectedFeatures.append(vInfoDictTMP)
            if self._dialog:
                self.slist.AddItem(vInfoDictTMP)
        else:  # only one is selected
            self.selectedFeatures.append(vInfoDictTMP)
            if self._dialog:
                self.slist.AddItem(vInfoDictTMP)

        if len(self.selectedFeatures) == 0:
            return False

        return True

    def _draw(self):
        """Call class 'VectorSelectHighlighter' to draw selected features"""
        self.updateLayer.emit()
        if len(self.selectedFeatures) > 0:
            self.painter.SetLayer(self.selectedFeatures[0]['Layer'])
            self.painter.SetMap(
                self.selectedFeatures[0]['Map'] + '@' + self.selectedFeatures[0]['Mapset']
            )
            tmp = list()
            for i in self.selectedFeatures:
                tmp.append(i['Category'])

            self.painter.SetCats(tmp)
            self.painter.DrawSelected()
        else:
            self.painter.Clear()

    def GetSelectedMap(self):
        """Return name of selected map in layer tree"""
        layerList = self._giface.GetLayerList()
        layerSelected = layerList.GetSelectedLayer()
        if layerSelected is None:
            return None

        if not layerSelected.maplayer.IsActive():
            GWarning(_("Selected map <%s> has been disabled for rendering. "
                       "Operation canceled.") % str(layerSelected), parent=self.mapWin)
            return None

        if layerSelected:
            mapName = str(layerSelected)
            if self.mapName is not None:
                if self.mapName != mapName:
                    self.Reset()
        else:
            mapName = None
            self.UnregisterMapEvtHandler()
            GError(_("No map layer selected. Operation canceled."))
        return mapName

    def QuerySelectedMap(self):
        """Return w.what info from last clicked coords on display

        """
        self.mapName = self.GetSelectedMap()
        if not self.mapName:
            return {}

        mapInfo = self.mapWin.GetMap()
        threshold = 10.0 * (
            (mapInfo.region['e'] - mapInfo.region['w']) / mapInfo.width)
        try:
            query = grass.vector_what(map=[self.mapName],
                                      coord=self.mapWin.GetLastEN(),
                                      distance=threshold, skip_attributes=True)
        except grass.ScriptError:
            GError(parent=self,
                   message=_("Failed to query vector map(s) <%s>.") % self.map)
            return None

        return query[0]

    def GetLineStringSelectedCats(self):
        """Return line of categories separated by comma"""
        strTMP = ''
        for cat in self.selectedFeatures:
            strTMP += str(cat['Category']) + ','
        return strTMP[:-1]

    def _id_generator(self, size=6,
                      chars=string.ascii_uppercase + string.digits):
        return ''.join(random.choice(chars) for _ in range(size))

    def OnExportMap(self, event):
        """Export selected features to a new map

        Add new map layer to layer tree and checked it

        @todo: set color of map to higlight color
        """

        if len(self.selectedFeatures) == 0:
            GMessage(_('No features selected'))
            return
        lst = ''
        for cat in self.selectedFeatures:  # build text string of categories for v.extract input
            lst += str(cat['Category']) + ','
        lst = lst[:-1]
        outMap = str(self.selectedFeatures[0][
                     'Map']) + '_selection' + str(self._id_generator(3))
        ret, err = RunCommand('v.extract',
                              input=self.selectedFeatures[0]['Map'],
                              layer=self.selectedFeatures[0]['Layer'],
                              output=outMap,
                              cats=lst,
                              getErrorMsg=True)
        if ret == 0:
            tree = self._giface.GetLayerTree()
            if tree:
                tree.AddLayer(ltype='vector', lname=outMap,
                              lcmd=['d.vect', 'map=%s' % outMap],
                              lchecked=True)

                # TODO colorize new map
                self.Reset()
            else:
                GMessage(_('Vector map <%s> was created') % outMap)
                self.Reset()
        else:
            GError(_("Unable to create a new vector map.\n\nReason: %s") % err)

    """
    def SetSelectedCat(self, cats):
        # allows setting selected vector categories by list of cats (per line)
        info = self.QuerySelectedMap()
        if 'Category' not in info:
            return

        for cat in cats.splitlines():
            tmpDict = {}
            tmpDict['Category'] = cat
            tmpDict['Map'] = info['Map']
            tmpDict['Layer'] = info['Layer']
            tmpDict['Type'] = '-'
            self.AddVecInfo(tmpDict)

        self._draw()
    """


class VectorSelectHighlighter():
    """Class for highlighting selected features on display

    :param mapdisp: Map display frame
    """

    def __init__(self, mapdisp, giface):
        self.qlayer = None
        self.mapdisp = mapdisp
        self.giface = giface
        self.layerCat = {}
        self.data = {}
        self.data['Category'] = list()
        self.data['Map'] = None
        self.data['Layer'] = None

    def SetMap(self, map):
        self.data['Map'] = map

    def SetLayer(self, layer):
        self.data['Layer'] = layer

    def SetCats(self, cats):
        self.data['Category'] = cats

    def Clear(self):
        self.data['Category'] = list()
        self.data['Map'] = None
        self.data['Layer'] = None
        self.mapdisp.RemoveQueryLayer()
        self.giface.GetMapWindow().UpdateMap(render=False)

    def DrawSelected(self):
        """Highlight selected features"""
        self.layerCat[int(self.data['Layer'])] = self.data['Category']

        # add map layer with higlighted vector features
        self.AddQueryMapLayer()  # -> self.qlayer
        self.qlayer.SetOpacity(0.7)
        self.giface.updateMap.emit(render=True, renderVector=True)

    def AddQueryMapLayer(self):
        """Redraw a map

        :return: True if map has been redrawn, False if no map is given
        """
        if self.mapdisp.GetMap().GetLayerIndex(self.qlayer) < 0:
            self.qlayer = None

        if self.qlayer:
            self.qlayer.SetCmd(
                self.mapdisp.AddTmpVectorMapLayer(
                    self.data['Map'],
                    self.layerCat,
                    addLayer=False))
        else:
            self.qlayer = self.mapdisp.AddTmpVectorMapLayer(
                self.data['Map'], self.layerCat)

        return self.qlayer
