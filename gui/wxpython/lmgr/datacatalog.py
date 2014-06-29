"""
@package lmgr::datacatalog

@brief Data catalog

Classes:
 - datacatalog::DataCatalog
 - datacatalog::DataCatalogTree

(C) 2014 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova
"""

import os
import sys

import wx
import wx.gizmos as gizmos

from core.gcmd import RunCommand, GError, GMessage
from core.utils import GetListOfLocations
from core.gthread import gThread
from core.debug import Debug
from gui_core.dialogs import TextEntryDialog

from grass.pydispatch.signal import Signal

import grass.script as grass

class DataCatalog(wx.Panel):
    """Data catalog panel"""
    def __init__(self, parent, giface=None, id = wx.ID_ANY, title=_("Data catalog"),
                 name='catalog', **kwargs):
        """Panel constructor  """
        self.showNotification = Signal('DataCatalog.showNotification')
        self.parent = parent
        self.baseTitle = title
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        self.SetName("DataCatalog")
        
        Debug.msg(1, "DataCatalog.__init__()")
        
        # tree with layers
        self.tree = DataCatalogTree(self)
        self.thread = gThread()
        self._loaded = False
        self.tree.showNotification.connect(self.showNotification)

        # some layout
        self._layout()
        
    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(item = self.tree.GetControl(), proportion = 1,
                  flag = wx.EXPAND)          
        
        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        
        self.Layout()

    def LoadItems(self):
        if self._loaded:
            return
        
        self.thread.Run(callable=self.tree.InitTreeItems,
                        ondone=lambda event: self.LoadItemsDone())

    def LoadItemsDone(self):
        self._loaded = True

class LocationMapTree(wx.TreeCtrl):
    def __init__(self, parent):
        """Location Map Tree constructor."""
        super(LocationMapTree, self).__init__(parent, id=wx.ID_ANY, style = wx.TR_HIDE_ROOT | wx.TR_EDIT_LABELS |
                                              wx.TR_HAS_BUTTONS | wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_COLUMN_LINES | 
                                              wx.TR_SINGLE)
        self.showNotification = Signal('Tree.showNotification')
        self.parent = parent
        self.root = self.AddRoot('Catalog') # will not be displayed when we use TR_HIDE_ROOT flag
        
        self._initVariables()
        self.MakeBackup()

        wx.EVT_TREE_ITEM_RIGHT_CLICK(self, wx.ID_ANY, self.OnRightClick)
        
        self.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)

    def _initTreeItems(self, locations = [], mapsets = []):
        """Add locations, mapsets and layers to the tree."""
        if not locations:
            locations = GetListOfLocations(self.gisdbase)
        if not mapsets:
            mapsets = ['*']
        
        first = True
        for loc in locations:
            location = loc
            if first:
                self.ChangeEnvironment(location, 'PERMANENT')
                first = False
            else:
                self.ChangeEnvironment(location)
            varloc = self.AppendItem(self.root, loc)
            #get list of all maps in location
            maplist = RunCommand('g.mlist', flags='mt', type='rast,rast3d,vect', mapset=','.join(mapsets),
                                 quiet=True, read=True)
            maplist = maplist.splitlines()
            for ml in maplist:
                # parse
                parts1 = ml.split('/')
                parts2 = parts1[1].split('@')
                mapset = parts2[1]
                mlayer = parts2[0]
                ltype = parts1[0]
                if self.itemExists(mapset, varloc) == False:
                    varmapset = self.AppendItem(varloc, mapset)
                if (self.GetItemText(varmapset) == mapset):
                    if (self.itemExists(ltype, varmapset) == False):
                        vartype = self.AppendItem(varmapset, ltype)
                else:
                    varmapset = self.getItemByName(mapset, varloc)
                    if (self.itemExists(ltype, varmapset) == False):
                        vartype = self.AppendItem(varmapset, ltype)
                self.AppendItem(vartype, mlayer)
        self.RestoreBackup()          
        Debug.msg(1, "Tree filled")    

    def InitTreeItems(self):
        """Create popup menu for layers"""
        raise NotImplementedError()

    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        raise NotImplementedError()

    def _popupMenuMapset(self):
        """Create popup menu for mapsets"""
        raise NotImplementedError()

    def _initVariables(self):
        """Init variables."""
        self.selected_layer = None
        self.selected_type = None
        self.selected_mapset = None
        self.selected_location = None
        
        self.gisdbase =  grass.gisenv()['GISDBASE']
        self.ctrldown = False

    def GetControl(self):
        """Returns control itself."""
        return self
    
    def DefineItems(self, item0):
        """Set selected items."""
        self.selected_layer = None
        self.selected_type = None
        self.selected_mapset = None
        self.selected_location = None
        items = []
        item = item0
        while (self.GetItemParent(item)):
            items.insert(0,item)
            item = self.GetItemParent(item)
        
        self.selected_location = items[0]
        length = len(items)
        if (length > 1):
            self.selected_mapset = items[1]
            if (length > 2):
                self.selected_type = items[2]
                if (length > 3):
                    self.selected_layer = items[3]
        
    def getItemByName(self, match, root):
        """Return match item from the root."""
        item, cookie = self.GetFirstChild(root)
        while item.IsOk():
            if self.GetItemText(item) == match:
                return item
            item, cookie = self.GetNextChild(root, cookie)
        return None
    
    def itemExists(self, match, root):
        """Return true if match item exists in the root item."""
        item, cookie = self.GetFirstChild(root)
        while item.IsOk():
            if self.GetItemText(item) == match:
                return True
            item, cookie = self.GetNextChild(root, cookie)
        return False       
    
    def UpdateTree(self):
        """Update whole tree."""
        self.DeleteAllItems()
        self.root = self.AddRoot('Tree')
        self.AddTreeItems()
        label = "Tree updated."
        self.showNotification.emit(message=label)
        
    def OnSelChanged(self, event):
        self.selected_layer = None
        
    def OnRightClick(self, event):
        """Display popup menu."""
        self.DefineItems(event.GetItem())
        if(self.selected_layer):
            self._popupMenuLayer()
        elif(self.selected_mapset and self.selected_type==None):
            self._popupMenuMapset() 
    
    def OnDoubleClick(self, event):
        """Double click"""
        Debug.msg(1, "Double CLICK")
            
    def OnKeyDown(self, event):
        """Set key event and check if control key is down"""
        keycode = event.GetKeyCode()
        if keycode == wx.WXK_CONTROL:
            self.ctrldown = True
            Debug.msg(1,"CONTROL ON")

    def OnKeyUp(self, event):
        """Check if control key is up"""
        keycode = event.GetKeyCode()
        if keycode == wx.WXK_CONTROL:
            self.ctrldown = False
            Debug.msg(1,"CONTROL OFF")

    def MakeBackup(self):
        """Make backup for case of change"""
        self.glocation =  grass.gisenv()['LOCATION_NAME']
        self.gmapset =  grass.gisenv()['MAPSET']
    
    def RestoreBackup(self):
        """Restore backup"""
        stringl = 'LOCATION_NAME='+self.glocation
        RunCommand('g.gisenv', set=stringl)
        stringm = 'MAPSET='+self.gmapset
        RunCommand('g.gisenv', set=stringm)
        
    def ChangeEnvironment(self, location, mapset=None):
        """Change gisenv variables -> location, mapset"""
        stringl = 'LOCATION_NAME='+location
        RunCommand('g.gisenv', set=stringl)
        if mapset:
            stringm = 'MAPSET='+mapset
            RunCommand('g.gisenv', set=stringm)

class DataCatalogTree(LocationMapTree):
    def __init__(self, parent):
        """Data Catalog Tree constructor."""
        super(DataCatalogTree, self).__init__(parent)
        
        self._initVariablesCatalog()

        wx.EVT_TREE_BEGIN_DRAG(self, wx.ID_ANY, self.OnBeginDrag)
        wx.EVT_TREE_END_DRAG(self, wx.ID_ANY, self.OnEndDrag)
        
        wx.EVT_TREE_END_LABEL_EDIT(self, wx.ID_ANY, self.OnEditLabel)
        wx.EVT_TREE_BEGIN_LABEL_EDIT(self, wx.ID_ANY, self.OnStartEditLabel)
    
    def _initVariablesCatalog(self):
        """Init variables."""
        self.copy_layer = None
        self.copy_type = None
        self.copy_mapset = None
        self.copy_location = None

    def InitTreeItems(self):
        """Add locations, mapsets and layers to the tree."""
        self._initTreeItems()
        
    def OnCopy(self, event): 
        """Copy layer or mapset (just save it temporarily, copying is done by paste)"""
        self.copy_layer = self.selected_layer
        self.copy_type = self.selected_type
        self.copy_mapset = self.selected_mapset
        self.copy_location = self.selected_location 
        label = "Layer "+self.GetItemText(self.copy_layer)+" copied to clipboard. You can paste it to selected mapset."
        self.showNotification.emit(message=label)
        
    def OnRename(self, event): 
        """Rename levent with dialog"""
        if (self.selected_layer):
            self.old_name = self.GetItemText(self.selected_layer)
            self._textDialog(_('New name'), _('Rename map'), self.old_name)
            self.rename() 
    
    def OnStartEditLabel(self, event):
        """Start label editing"""
        item = event.GetItem()
        self.DefineItems(item)
        Debug.msg(1, "Start label edit "+self.GetItemText(item))
        label = _("Editing") + " " + self.GetItemText(item)
        self.showNotification.emit(message=label)
        if (self.selected_layer == None):
            event.Veto()
    
    def OnEditLabel(self, event):
        """End label editing"""
        if (self.selected_layer):
            item = event.GetItem()
            self.old_name = self.GetItemText(item)
            Debug.msg(1, "End label edit "+self.old_name)
            wx.CallAfter(self.afterEdit, self, item)
            
    def afterEdit(pro, self, item):
        self.new_name = self.GetItemText(item)
        self.rename()
    
    def rename(self):
        """Rename layer"""
        if (self.selected_layer):
            string = self.old_name+','+self.new_name
            self.ChangeEnvironment(self.GetItemText(self.selected_location), self.GetItemText(self.selected_mapset))
            renamed = 0
            label = _("Renaming") + " " + string + " ..."
            self.showNotification.emit(message=label)
            if (self.GetItemText(self.selected_type)=='vect'):
                renamed = RunCommand('g.rename', vect=string)
            elif (self.GetItemText(self.selected_type)=='rast'):
                renamed = RunCommand('g.rename', rast=string)
            else:
                renamed = RunCommand('g.rename', rast3d=string)
            if (renamed==0):
                self.SetItemText(self.selected_layer,self.new_name)
                label = "g.rename "+self.GetItemText(self.selected_type)+"="+string+"   -- completed"
                self.showNotification.emit(message=label)
                Debug.msg(1,"LAYER RENAMED TO: "+self.new_name)
            self.RestoreBackup()    
        
    def OnPaste(self, event):
        """Paste layer or mapset""" 
        # copying between mapsets of one location
        if (self.copy_layer == None):
                return
        if (self.selected_location == self.copy_location and self.selected_mapset):
            if (self.selected_type != None):
                if (self.GetItemText(self.copy_type) != self.GetItemText(self.selected_type)): # copy raster to vector or vice versa
                    GError(_("Failed to copy layer: invalid type."), parent = self)
                    return
            self._textDialog(_('New name'), _('Copy map'), self.GetItemText(self.copy_layer)+'_copy')
            if (self.GetItemText(self.copy_layer) == self.new_name):
                GMessage(_("Layer was not copied: new layer has the same name"), parent=self) 
                return
            string = self.GetItemText(self.copy_layer)+'@'+self.GetItemText(self.copy_mapset)+','+self.new_name
            self.ChangeEnvironment(self.GetItemText(self.selected_location), self.GetItemText(self.selected_mapset))
            pasted = 0
            type = None
            label = _("Copying") + " " + string + " ..."
            self.showNotification.emit(message=label)
            if (self.GetItemText(self.copy_type)=='vect'):
                pasted = RunCommand('g.copy', vect=string)
                node = 'vect'     
            elif (self.GetItemText(self.copy_type)=='rast'):
                pasted = RunCommand('g.copy', rast=string)
                node = 'rast'
            else:
                pasted = RunCommand('g.copy', rast3d=string)
                node = 'rast3d'
            if (pasted==0):
                if (self.selected_type == None):
                    self.selected_type = self.getItemByName(node, self.selected_mapset)
                self.AppendItem(self.selected_type,self.new_name) 
                self.SortChildren(self.selected_type)
                Debug.msg(1,"COPIED TO: "+self.new_name)
                label = "g.copy "+self.GetItemText(self.copy_type)+"="+string+"    -- completed" # generate this message (command) automatically?
                self.showNotification.emit(message=label)
        else:
            GError(_("Failed to copy layer: action is allowed only within the same location."),
                   parent=self)
        
        self.RestoreBackup()
        
        
    def OnDelete(self, event):
        """Delete layer or mapset"""
        if (self.selected_layer):
            string = self.GetItemText(self.selected_layer)
            self.ChangeEnvironment(self.GetItemText(self.selected_location), self.GetItemText(self.selected_mapset))
            removed = 0
            if (self._confirmDialog(_('Do you really want to delete layer') +string+'?', _('Delete map')) == wx.ID_YES):
                label = _("Deleting") + " " + string + " ..."
                self.showNotification.emit(message=label)
                if (self.GetItemText(self.selected_type)=='vect'):
                    removed = RunCommand('g.remove', vect=string)
                elif (self.GetItemText(self.selected_type)=='rast'):
                    removed = RunCommand('g.remove', rast=string)
                else:
                    removed = RunCommand('g.remove', rast3d=string)
                if (removed==0):
                    self.Delete(self.selected_layer)
                    Debug.msg(1,"LAYER "+string+" DELETED")
                    label = "g.remove "+self.GetItemText(self.selected_type)+"="+string+"    -- completed" # generate this message (command) automatically?
                    self.showNotification.emit(message=label)
            self.RestoreBackup()
            
    def OnDisplayLayer(self, event):
        """Display layer in current graphics view"""
        layerName = []
        if (self.GetItemText(self.selected_location) == self.glocation and self.selected_mapset):
            string = self.GetItemText(self.selected_layer)+'@'+self.GetItemText(self.selected_mapset)
            layerName.append(string)
            label = _("Displaying") + " " + string + " ..."
            self.showNotification.emit(message=label)
            label = "d."+self.GetItemText(self.selected_type)+" --q map="+string+"    -- completed. Go to Map layers for further operations."
            if (self.GetItemText(self.selected_type)=='vect'):
                self.parent.parent.AddMaps(layerName, 'vect', True)
            elif (self.GetItemText(self.selected_type)=='rast'):
                self.parent.parent.AddMaps(layerName, 'rast', True)     
            else:
                self.parent.parent.AddMaps(layerName, 'rast3d', True)
                label = "d.rast --q map="+string+"    -- completed. Go to 'Map layers' for further operations." # generate this message (command) automatically?
            self.showNotification.emit(message=label)
            Debug.msg(1,"LAYER "+self.GetItemText(self.selected_layer)+" DISPLAYED")
        else:
            GError(_("Failed to display layer: not in current mapset or invalid layer"),
                   parent = self)

    def OnBeginDrag(self, event):
        """Just copy necessary data"""
        if (self.ctrldown):
            #cursor = wx.StockCursor(wx.CURSOR_HAND)
            #self.SetCursor(cursor)
            event.Allow()
            self.DefineItems(event.GetItem())
            self.OnCopy(event)
            Debug.msg(1,"DRAG")
        else:
            event.Veto()
            Debug.msg(1,"DRAGGING without ctrl key does nothing") 
        
    def OnEndDrag(self, event):
        """Copy layer into target"""
        #cursor = wx.StockCursor(wx.CURSOR_ARROW)
        #self.SetCursor(cursor)
        if (event.GetItem()): 
            self.DefineItems(event.GetItem())
            if (self.selected_location == self.copy_location and self.selected_mapset):
                event.Allow()
                self.OnPaste(event)
                self.ctrldown = False
                #cursor = wx.StockCursor(wx.CURSOR_DEFAULT)
                #self.SetCursor(cursor) # TODO: change cursor while dragging and then back, this is not working
                Debug.msg(1,"DROP DONE") 
            else:
                event.Veto()

    def _textDialog(self, message, title, value):
        """Dialog for simple text entry"""
        dlg = TextEntryDialog(self, message, title)
        dlg.SetValue(value)
        res = dlg.ShowModal()
        self.new_name = dlg.GetValue()
        dlg.Destroy()
    
    def _confirmDialog(self, question, title):
        """Confirm dialog"""
        dlg = wx.MessageDialog(self, question, title, wx.YES_NO)
        res = dlg.ShowModal()
        dlg.Destroy()
        return res
    
    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        menu = wx.Menu()
        
        item = wx.MenuItem(menu, wx.NewId(), _("&Copy"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCopy, item)

        item = wx.MenuItem(menu, wx.NewId(), _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPaste, item)
 
        item = wx.MenuItem(menu, wx.NewId(), _("&Delete"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDelete, item)
        
        item = wx.MenuItem(menu, wx.NewId(), _("&Rename"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRename, item)
        
        item = wx.MenuItem(menu, wx.NewId(), _("&Display layer"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDisplayLayer, item)
        
        self.PopupMenu(menu)
        menu.Destroy()
        
    def _popupMenuMapset(self):
        """Create popup menu for mapsets"""
        menu = wx.Menu()

        item = wx.MenuItem(menu, wx.NewId(), _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPaste, item)
        
        self.PopupMenu(menu)
        menu.Destroy()

# testing...
if __name__ == "__main__":
    class TestTree(LocationMapTree):
        def __init__(self, parent):
            """Test Tree constructor."""
            super(TestTree, self).__init__(parent)
            
        def InitTreeItems(self):
            """Add locations, mapsets and layers to the tree."""
            gisenv = grass.gisenv()
            location = gisenv['LOCATION_NAME']
            mapset = gisenv['MAPSET']
            self._initTreeItems(locations=[location],
                                mapsets=[mapset])
            
            self.ExpandAll()
        
        def _popupMenuLayer(self):
            """Create popup menu for layers"""
            pass

        def _popupMenuMapset(self):
            """Create popup menu for mapsets"""
            pass

    class TestFrame(wx.Frame):
        """Frame for testing purposes only."""
        def __init__(self, model=None):
            wx.Frame.__init__(self, None, title='Test tree')

            panel = wx.Panel(self)
            self.tree = TestTree(parent=self)
            self.tree.SetMinSize((300, 500))
            self.tree.InitTreeItems()

            szr = wx.BoxSizer(wx.VERTICAL)
            szr.Add(self.tree, 1, wx.ALIGN_CENTER)
            panel.SetSizerAndFit(szr)
            szr.SetSizeHints(self)

    def main():
        app = wx.App()
        frame = TestFrame()
        frame.Show()
        app.MainLoop()
    
    main()
