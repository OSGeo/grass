"""!
@package vdigit.dialogs

@brief wxGUI vector digitizer dialogs

Classes:
 - dialogs::VDigitCategoryDialog
 - dialogs::CategoryListCtrl
 - dialogs::VDigitZBulkDialog
 - dialogs::VDigitDuplicatesDialog
 - dialogs::CheckListFeature

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import sys
import copy

import wx
import wx.lib.mixins.listctrl as listmix

from core.gcmd        import RunCommand, GError
from core.debug       import Debug
from core.settings    import UserSettings
from core.utils import _


class VDigitCategoryDialog(wx.Dialog, listmix.ColumnSorterMixin):
    def __init__(self, parent, title,
                 vectorName, query = None, cats = None,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Dialog used to display/modify categories of vector objects
        
        @param parent
        @param title dialog title
        @param query {coordinates, qdist} - used by v.edit/v.what
        @param cats  directory of lines (layer/categories) - used by vdigit
        @param style dialog style
        """
        self.parent = parent       # mapdisplay.BufferedWindow class instance
        self.digit = parent.digit
        
        # map name
        self.vectorName = vectorName
        
        # line : {layer: [categories]}
        self.cats = {}
        
        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            if self._getCategories(query[0], query[1]) == 0 or not self.line:
                Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
        else:
            self.cats = cats
            for line in cats.keys():
                for layer in cats[line].keys():
                    self.cats[line][layer] = list(cats[line][layer])
            
            layers = []
            for layer in self.digit.GetLayers():
                layers.append(str(layer))
        
        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)
        
        wx.Dialog.__init__(self, parent = self.parent, id = wx.ID_ANY, title = title,
                           style = style, **kwargs)
        
        # list of categories
        box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                           label = " %s " % _("List of categories - right-click to delete"))
        listSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        self.list = CategoryListCtrl(parent = self, id = wx.ID_ANY,
                                     style = wx.LC_REPORT |
                                     wx.BORDER_NONE |
                                     wx.LC_SORT_ASCENDING |
                                     wx.LC_HRULES |
                                     wx.LC_VRULES)
        # sorter
        self.fid = self.cats.keys()[0]
        self.itemDataMap = self.list.Populate(self.cats[self.fid])
        listmix.ColumnSorterMixin.__init__(self, 2)
        self.fidMulti = wx.Choice(parent = self, id = wx.ID_ANY,
                                  size = (150, -1))
        self.fidMulti.Bind(wx.EVT_CHOICE, self.OnFeature)
        self.fidText = wx.StaticText(parent = self, id = wx.ID_ANY)
        if len(self.cats.keys()) == 1:
            self.fidMulti.Show(False)
            self.fidText.SetLabel(str(self.fid))
        else:
            self.fidText.Show(False)
            choices = []
            for fid in self.cats.keys():
                choices.append(str(fid))
            self.fidMulti.SetItems(choices)
            self.fidMulti.SetSelection(0)
        
        listSizer.Add(item = self.list, proportion = 1, flag = wx.EXPAND)

        # add new category
        box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                           label = " %s " % _("Add new category"))
        addSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 5, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(3)

        layerNewTxt = wx.StaticText(parent = self, id = wx.ID_ANY,
                                 label = "%s:" % _("Layer"))
        self.layerNew = wx.Choice(parent = self, id = wx.ID_ANY, size = (75, -1),
                                  choices = layers)
        if len(layers) > 0:
            self.layerNew.SetSelection(0)
        
        catNewTxt = wx.StaticText(parent = self, id = wx.ID_ANY,
                               label = "%s:" % _("Category"))

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
        self.catNew = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (75, -1),
                                  initial = newCat, min = 0, max = 1e9)
        btnAddCat = wx.Button(self, wx.ID_ADD)
        flexSizer.Add(item = layerNewTxt, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = self.layerNew, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = catNewTxt, proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                      border = 10)
        flexSizer.Add(item = self.catNew, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = btnAddCat, proportion = 0,
                      flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
        addSizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)

        # buttons
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnApply.SetToolTipString(_("Apply changes"))
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnCancel.SetToolTipString(_("Ignore changes and close dialog"))
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetToolTipString(_("Apply changes and close dialog"))
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        #btnSizer.AddButton(btnReload)
        #btnSizer.SetNegativeButton(btnReload)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = listSizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        mainSizer.Add(item = addSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        fidSizer = wx.BoxSizer(wx.HORIZONTAL)
        fidSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _("Feature id:")),
                     proportion = 0, border = 5,
                     flag = wx.ALIGN_CENTER_VERTICAL)
        fidSizer.Add(item = self.fidMulti, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        fidSizer.Add(item = self.fidText, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        mainSizer.Add(item = fidSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize(self.GetBestSize())

        # bindings
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnOk.Bind(wx.EVT_BUTTON, self.OnOK)
        btnAddCat.Bind(wx.EVT_BUTTON, self.OnAddCat)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
                                     
        # list
        self.list.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnRightUp) #wxMSW
        self.list.Bind(wx.EVT_RIGHT_UP, self.OnRightUp) #wxGTK
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnBeginEdit, self.list)
        self.Bind(wx.EVT_LIST_END_LABEL_EDIT, self.OnEndEdit, self.list)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick, self.list)

    def GetListCtrl(self):
        """!Used by ColumnSorterMixin
        """
        return self.list

    def OnColClick(self, event):
        """!Click on column header (order by)
        """
        event.Skip()
        
    def OnBeginEdit(self, event):
        """!Editing of item started
        """
        event.Allow()

    def OnEndEdit(self, event):
        """!Finish editing of item
        """
        itemIndex = event.GetIndex()
        layerOld = int (self.list.GetItem(itemIndex, 0).GetText())
        catOld = int (self.list.GetItem(itemIndex, 1).GetText())

        if event.GetColumn() == 0:
            layerNew = int(event.GetLabel())
            catNew = catOld
        else:
            layerNew = layerOld
            catNew = int(event.GetLabel())
        
        try:
            if layerNew not in self.cats[self.fid].keys():
                self.cats[self.fid][layerNew] = []
            self.cats[self.fid][layerNew].append(catNew)
            self.cats[self.fid][layerOld].remove(catOld)
        except:
            event.Veto()
            self.list.SetStringItem(itemIndex, 0, str(layerNew))
            self.list.SetStringItem(itemIndex, 1, str(catNew))
            dlg = wx.MessageDialog(self, _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                                           "Layer and category number must be integer.\n"
                                           "Layer number must be greater than zero.") %
                                   { 'layer': self.layerNew.GetStringSelection(),
                                     'category' : str(self.catNew.GetValue()) },
                                   _("Error"), wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

    def OnRightDown(self, event):
        """!Mouse right button down
        """
        x = event.GetX()
        y = event.GetY()
        item, flags = self.list.HitTest((x, y))

        if item !=  wx.NOT_FOUND and \
                flags & wx.LIST_HITTEST_ONITEM:
            self.list.Select(item)

        event.Skip()

    def OnRightUp(self, event):
        """!Mouse right button up
        """
        if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnItemDelete,    id = self.popupID1)
            self.Bind(wx.EVT_MENU, self.OnItemDeleteAll, id = self.popupID2)
            self.Bind(wx.EVT_MENU, self.OnReload, id = self.popupID3)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupID1, _("Delete selected"))
        if self.list.GetFirstSelected() == -1:
            menu.Enable(self.popupID1, False)

        menu.Append(self.popupID2, _("Delete all"))
        menu.AppendSeparator()
        menu.Append(self.popupID3, _("Reload"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnItemSelected(self, event):
        """!Item selected
        """
        event.Skip()

    def OnItemDelete(self, event):
        """!Delete selected item(s) from the list (layer/category pair)
        """
        item = self.list.GetFirstSelected()
        while item != -1:
            layer = int (self.list.GetItem(item, 0).GetText())
            cat = int (self.list.GetItem(item, 1).GetText())
            self.list.DeleteItem(item)
            self.cats[self.fid][layer].remove(cat)

            item = self.list.GetFirstSelected()
            
        event.Skip()
        
    def OnItemDeleteAll(self, event):
        """!Delete all items from the list
        """
        self.list.DeleteAllItems()
        self.cats[self.fid] = {}

        event.Skip()

    def OnFeature(self, event):
        """!Feature id changed (on duplicates)
        """
        self.fid = int(event.GetString())
        
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
            
        self.catNew.SetValue(newCat)
        
        event.Skip()
        
    def _getCategories(self, coords, qdist):
        """!Get layer/category pairs for all available
        layers

        Return True line found or False if not found
        """
        ret = RunCommand('v.what',
                         parent = self,
                         quiet = True,
                         map = self.vectorName,
                         east_north = '%f,%f' % \
                             (float(coords[0]), float(coords[1])),
                         distance = qdist)

        if not ret:
            return False

        for item in ret.splitlines():
            litem = item.lower()
            if "id:" in litem: # get line id
                self.line = int(item.split(':')[1].strip())
            elif "layer:" in litem: # add layer
                layer = int(item.split(':')[1].strip())
                if layer not in self.cats.keys():
                    self.cats[layer] = []
            elif "category:" in litem: # add category
                self.cats[layer].append(int(item.split(':')[1].strip()))

        return True

    def OnReload(self, event):
        """!Reload button pressed
        """
        # restore original list
        self.cats = copy.deepcopy(self.cats_orig)

        # polulate list
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        event.Skip()

    def OnCancel(self, event):
        """!Cancel button pressed
        """
        self.parent.parent.dialogs['category'] = None
        if self.digit:
            self.digit.GetDisplay().SetSelected([])
            self.parent.UpdateMap(render = False)
        else:
            self.parent.parent.OnRender(None)
            
        self.Close()

    def OnApply(self, event):
        """!Apply button pressed
        """
        for fid in self.cats.keys():
            newfid = self.ApplyChanges(fid)
            if fid == self.fid and newfid > 0:
                self.fid = newfid
            
    def ApplyChanges(self, fid):
        """!Apply changes 

        @param fid feature id
        """
        cats = self.cats[fid]
        cats_orig = self.cats_orig[fid]

        # action : (catsFrom, catsTo)
        check = {'catadd': (cats,      cats_orig),
                 'catdel': (cats_orig, cats)}

        newfid = -1
        
        # add/delete new category
        for action, catsCurr in check.iteritems():
            for layer in catsCurr[0].keys():
                catList = []
                for cat in catsCurr[0][layer]:
                    if layer not in catsCurr[1].keys() or \
                            cat not in catsCurr[1][layer]:
                        catList.append(cat)
                if catList != []:
                    if action == 'catadd':
                        add = True
                    else:
                        add = False
                        
                    newfid = self.digit.SetLineCats(fid, layer,
                                                    catList, add)
                    if len(self.cats.keys()) == 1:
                        self.fidText.SetLabel("%d" % newfid)
                    else:
                        choices = self.fidMulti.GetItems()
                        choices[choices.index(str(fid))] = str(newfid)
                        self.fidMulti.SetItems(choices)
                        self.fidMulti.SetStringSelection(str(newfid))
                    
                    self.cats[newfid] = self.cats[fid]
                    del self.cats[fid]
                    
                    fid = newfid
                    if self.fid < 0:
                        wx.MessageBox(parent = self, message = _("Unable to update vector map."),
                                      caption = _("Error"), style = wx.OK | wx.ICON_ERROR)
        
        self.cats_orig[fid] = copy.deepcopy(cats)
        
        return newfid

    def OnOK(self, event):
        """!OK button pressed
        """
        self.OnApply(event)
        self.OnCancel(event)

    def OnAddCat(self, event):
        """!Button 'Add' new category pressed
        """
        try:
            layer = int(self.layerNew.GetStringSelection())
            cat   = int(self.catNew.GetValue())
            if layer <= 0:
                raise ValueError
        except ValueError:
            GError(parent = self,
                   message = _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                               "Layer and category number must be integer.\n"
                               "Layer number must be greater than zero.") %
                   {'layer' : str(self.layerNew.GetValue()),
                    'category' : str(self.catNew.GetValue())})
            return False
        
        if layer not in self.cats[self.fid].keys():
            self.cats[self.fid][layer] = []
        
        self.cats[self.fid][layer].append(cat)
        
        # reload list
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)
        
        # update category number for add
        self.catNew.SetValue(cat + 1)
        
        event.Skip()

        return True

    def GetLine(self):
        """!Get id of selected line of 'None' if no line is selected
        """
        return self.cats.keys()

    def UpdateDialog(self, query = None, cats = None):
        """!Update dialog
        
        @param query {coordinates, distance} - v.what
        @param cats  directory layer/cats    - vdigit
        Return True if updated otherwise False
        """
        # line: {layer: [categories]}
        self.cats = {}
        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            ret = self._getCategories(query[0], query[1])
        else:
            self.cats = cats
            for line in cats.keys():
                for layer in cats[line].keys():
                    self.cats[line][layer] = list(cats[line][layer])
            ret = 1
        if ret == 0 or len(self.cats.keys()) < 1:
            Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
            return False
        
        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)

        # polulate list
        self.fid = self.cats.keys()[0]
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
        self.catNew.SetValue(newCat)
        
        if len(self.cats.keys()) == 1:
            self.fidText.Show(True)
            self.fidMulti.Show(False)
            self.fidText.SetLabel("%d" % self.fid)
        else:
            self.fidText.Show(False)
            self.fidMulti.Show(True)
            choices = []
            for fid in self.cats.keys():
                choices.append(str(fid))
            self.fidMulti.SetItems(choices)
            self.fidMulti.SetSelection(0)

        self.Layout()
        
        return True

class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin,
                       listmix.TextEditMixin):
    def __init__(self, parent, id, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        """!List of layers/categories"""
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)

    def Populate(self, cats, update = False):
        """!Populate the list
        """
        itemData = {} # requested by sorter

        if not update:
            self.InsertColumn(0, _("Layer"))
            self.InsertColumn(1, _("Category"))
        else:
            self.DeleteAllItems()

        i = 1
        for layer in cats.keys():
            catsList = cats[layer]
            for cat in catsList:
                index = self.InsertStringItem(sys.maxint, str(catsList[0]))
                self.SetStringItem(index, 0, str(layer))
                self.SetStringItem(index, 1, str(cat))
                self.SetItemData(index, i)
                itemData[i] = (str(layer), str(cat))
                i = i + 1

        if not update:
            self.SetColumnWidth(0, 100)
            self.SetColumnWidth(1, wx.LIST_AUTOSIZE)

        self.currentItem = 0

        return itemData

class VDigitZBulkDialog(wx.Dialog):
    def __init__(self, parent, title, nselected, style = wx.DEFAULT_DIALOG_STYLE):
        """!Dialog used for Z bulk-labeling tool
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style)

        self.parent = parent # mapdisplay.BufferedWindow class instance

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        border = wx.BoxSizer(wx.VERTICAL)
        
        txt = wx.StaticText(parent = self,
                            label = _("%d lines selected for z bulk-labeling") % nselected);
        border.Add(item = txt, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Set value"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        # starting value
        txt = wx.StaticText(parent = self,
                            label = _("Starting value"));
        self.value = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (150, -1),
                                 initial = 0,
                                 min = -1e6, max = 1e6)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.value, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        # step
        txt = wx.StaticText(parent = self,
                            label = _("Step"))
        self.step  = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (150, -1),
                                 initial = 0,
                                 min = 0, max = 1e6)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.step, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 0)

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = border, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

class VDigitDuplicatesDialog(wx.Dialog):
    def __init__(self, parent, data, title = _("List of duplicates"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 pos = wx.DefaultPosition):
        """!Show duplicated feature ids
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style,
                           pos = pos)
        
        self.parent = parent # BufferedWindow
        self.data = data
        self.winList = []

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        # notebook
        self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)

        id = 1
        for key in self.data.keys():
            panel = wx.Panel(parent = self.notebook, id = wx.ID_ANY)
            self.notebook.AddPage(page = panel, text = " %d " % (id))
            
            # notebook body
            border = wx.BoxSizer(wx.VERTICAL)

            win = CheckListFeature(parent = panel, data = list(self.data[key]))
            self.winList.append(win.GetId())

            border.Add(item = win, proportion = 1,
                       flag = wx.ALL | wx.EXPAND, border = 5)

            panel.SetSizer(border)

            id += 1

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.notebook, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize((250, 180))

    def GetUnSelected(self):
        """!Get unselected items (feature id)

        @return list of ids
        """
        ids = []
        for id in self.winList:
            wlist = self.FindWindowById(id)

            for item in range(wlist.GetItemCount()):
                if not wlist.IsChecked(item):
                    ids.append(int(wlist.GetItem(item, 0).GetText()))
                    
        return ids

class CheckListFeature(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    def __init__(self, parent, data,
                 pos = wx.DefaultPosition, log = None):
        """!List of mapset/owner/group
        """
        self.parent = parent
        self.data = data

        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style = wx.LC_REPORT)

        listmix.CheckListCtrlMixin.__init__(self)

        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.LoadData(self.data)

    def LoadData(self, data):
        """!Load data into list
        """
        self.InsertColumn(0, _('Feature id'))
        self.InsertColumn(1, _('Layer (Categories)'))

        for item in data:
            index = self.InsertStringItem(sys.maxint, str(item[0]))
            self.SetStringItem(index, 1, str(item[1]))

        # enable all items by default
        for item in range(self.GetItemCount()):
            self.CheckItem(item, True)

        self.SetColumnWidth(col = 0, width = wx.LIST_AUTOSIZE_USEHEADER)
        self.SetColumnWidth(col = 1, width = wx.LIST_AUTOSIZE_USEHEADER)
                
    def OnCheckItem(self, index, flag):
        """!Mapset checked/unchecked
        """
        pass
