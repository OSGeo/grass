"""
@package datacatalog::tree

@brief Data catalog tree classes

Classes:
 - datacatalog::NameEntryDialog
 - datacatalog::DataCatalogNode
 - datacatalog::DataCatalogTree

(C) 2014-2018 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova
@author Anna Petrasova (kratochanna gmail com)
@author Linda Kladivova (l.kladivova@seznam.cz)
"""
import re
import copy
from multiprocessing import Process, Queue, cpu_count

import wx
import os

from core.gcmd import RunCommand, GError, GMessage, GWarning
from core.utils import GetListOfLocations
from core.debug import Debug
from gui_core.dialogs import TextEntryDialog
from core.giface import StandaloneGrassInterface
from core.treemodel import TreeModel, DictNode
from gui_core.treeview import TreeView
from gui_core.wrap import Menu
from datacatalog.dialogs import CatalogReprojectionDialog
from icons.icon import MetaIcon
from startup.guiutils import (
    create_mapset_interactively,
    create_location_interactively,
    rename_mapset_interactively,
    rename_location_interactively,
    delete_mapsets_interactively,
    delete_location_interactively,
    download_location_interactively,
)

from grass.pydispatch.signal import Signal

import grass.script as gscript
from grass.script import gisenv
from grass.grassdb.data import map_exists
from grass.grassdb.checks import (get_mapset_owner, is_mapset_locked,
                                  is_mapset_users)
from grass.exceptions import CalledModuleError


def filterModel(model, element=None, name=None):
    """Filter tree model based on type or name of map using regular expressions.
    Copies tree and remove nodes which don't match."""
    fmodel = copy.deepcopy(model)
    nodesToRemove = []
    if name:
        try:
            regex = re.compile(name)
        except:
            return fmodel
    for gisdbase in fmodel.root.children:
        for location in gisdbase.children:
            for mapset in location.children:
                for layer in mapset.children:
                    if element and layer.data['type'] != element:
                        nodesToRemove.append(layer)
                        continue
                    if name and regex.search(layer.data['name']) is None:
                        nodesToRemove.append(layer)

    for node in reversed(nodesToRemove):
        fmodel.RemoveNode(node)

    cleanUpTree(fmodel)
    return fmodel


def cleanUpTree(model):
    """Removes empty element/mapsets/locations nodes.
    It first removes empty elements, then mapsets, then locations"""
    # removes empty mapsets
    nodesToRemove = []
    for gisdbase in model.root.children:
        for location in gisdbase.children:
            for mapset in location.children:
                if not mapset.children:
                    nodesToRemove.append(mapset)
    for node in reversed(nodesToRemove):
        model.RemoveNode(node)
    # removes empty locations
    nodesToRemove = []
    for gisdbase in model.root.children:
        for location in gisdbase.children:
            if not location.children:
                nodesToRemove.append(location)
    for node in reversed(nodesToRemove):
        model.RemoveNode(node)


def getLocationTree(gisdbase, location, queue, mapsets=None):
    """Creates dictionary with mapsets, elements, layers for given location.
    Returns tuple with the dictionary and error (or None)"""
    tmp_gisrc_file, env = gscript.create_environment(gisdbase, location, 'PERMANENT')
    env['GRASS_SKIP_MAPSET_OWNER_CHECK'] = '1'

    maps_dict = {}
    elements = ['raster', 'raster_3d', 'vector']
    try:
        if not mapsets:
            mapsets = gscript.read_command(
                'g.mapsets',
                flags='l',
                separator='comma',
                quiet=True,
                env=env).strip()
    except CalledModuleError:
        queue.put(
            (maps_dict,
             _("Failed to read mapsets from location <{l}>.").format(
                 l=location)))
        gscript.try_remove(tmp_gisrc_file)
        return
    else:
        mapsets = mapsets.split(',')
        Debug.msg(
            4, "Location <{0}>: {1} mapsets found".format(
                location, len(mapsets)))
        for each in mapsets:
            maps_dict[each] = []
    try:
        maplist = gscript.read_command(
            'g.list', flags='mt', type=elements,
            mapset=','.join(mapsets),
            quiet=True, env=env).strip()
    except CalledModuleError:
        queue.put(
            (maps_dict,
             _("Failed to read maps from location <{l}>.").format(
                 l=location)))
        gscript.try_remove(tmp_gisrc_file)
        return
    else:
        # fill dictionary
        listOfMaps = maplist.splitlines()
        Debug.msg(
            4, "Location <{0}>: {1} maps found".format(
                location, len(listOfMaps)))
        for each in listOfMaps:
            ltype, wholename = each.split('/')
            name, mapset = wholename.split('@')
            maps_dict[mapset].append({'name': name, 'type': ltype})

    queue.put((maps_dict, None))
    gscript.try_remove(tmp_gisrc_file)


class NameEntryDialog(TextEntryDialog):

    def __init__(self, element, mapset, env, **kwargs):
        TextEntryDialog.__init__(self, **kwargs)
        self._element = element
        self._mapset = mapset
        self._env = env
        id_OK = self.GetAffirmativeId()
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.FindWindowById(id_OK))

    def OnOK(self, event):
        new = self.GetValue()
        if not new:
            return
        if map_exists(new, self._element, env=self._env, mapset=self._mapset):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Map of type {elem} <{name}> already exists in mapset <{mapset}>. "
                    "Do you want to overwrite it?").format(
                    elem=self._element,
                    name=new,
                    mapset=self._mapset),
                caption=_("Overwrite?"),
                style=wx.YES_NO)
            if dlg.ShowModal() == wx.ID_YES:
                dlg.Destroy()
                self._env['GRASS_OVERWRITE'] = '1'
                self.EndModal(wx.ID_OK)
            else:
                dlg.Destroy()
                return
        else:
            self.EndModal(wx.ID_OK)


class DataCatalogNode(DictNode):
    """Node representing item in datacatalog."""

    def __init__(self, data=None):
        super(DataCatalogNode, self).__init__(data=data)

    @property
    def label(self):
        if self.data["lock"] == True and self.data["user"]:
            return self.data["name"] + " <user: {}>".format(self.data["user"]) +" (locked)"
        elif self.data["lock"] == True:
            return self.data["name"] + " (locked)"
        elif self.data["user"]:
            return self.data["name"] + " <user: {}>".format(self.data["user"])
        else:
            return self.data["name"]

    def match(self, **kwargs):
        """Method used for searching according to given parameters.

        :param value: dictionary value to be matched
        :param key: data dictionary key
        """
        if not kwargs:
            return False

        for key in kwargs:
            if not (key in self.data and self.data[key] == kwargs[key]):
                return False
        return True


class DataCatalogTree(TreeView):

    def __init__(
            self, parent, model=None, giface=None,
            style=wx.TR_HIDE_ROOT | wx.TR_EDIT_LABELS |
            wx.TR_LINES_AT_ROOT | wx.TR_HAS_BUTTONS |
            wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_MULTIPLE):
        """Location Map Tree constructor."""
        self._model = TreeModel(DataCatalogNode)
        self._orig_model = self._model
        super(
            DataCatalogTree,
            self).__init__(
            parent=parent,
            model=self._model,
            id=wx.ID_ANY,
            style=style)

        self._giface = giface
        self._restricted = True

        self.showNotification = Signal('Tree.showNotification')
        self.changeMapset = Signal('Tree.changeMapset')
        self.changeLocation = Signal('Tree.changeLocation')
        self.parent = parent
        self.contextMenu.connect(self.OnRightClick)
        self.itemActivated.connect(self.OnDoubleClick)
        self._iconTypes = ['grassdb', 'location', 'mapset', 'raster',
                           'vector', 'raster_3d']
        self._initImages()

        self._initVariables()
        self._initVariablesCatalog()
        self.UpdateCurrentDbLocationMapsetNode()

        self.grassdatabases = []
        self.grassdatabases.append(gisenv()['GISDBASE'])

        self.beginDrag = Signal('DataCatalogTree.beginDrag')
        self.endDrag = Signal('DataCatalogTree.endDrag')
        self.startEdit = Signal('DataCatalogTree.startEdit')
        self.endEdit = Signal('DataCatalogTree.endEdit')

        self.Bind(wx.EVT_TREE_BEGIN_DRAG, lambda evt:
                  self._emitSignal(evt.GetItem(), self.beginDrag, event=evt))
        self.Bind(wx.EVT_TREE_END_DRAG, lambda evt:
                  self._emitSignal(evt.GetItem(), self.endDrag, event=evt))
        self.beginDrag.connect(self.OnBeginDrag)
        self.endDrag.connect(self.OnEndDrag)

        self.Bind(wx.EVT_TREE_BEGIN_LABEL_EDIT, lambda evt:
                  self._emitSignal(evt.GetItem(), self.startEdit, event=evt))
        self.Bind(wx.EVT_TREE_END_LABEL_EDIT, lambda evt:
                  self._emitSignal(evt.GetItem(), self.endEdit, event=evt))
        self.startEdit.connect(self.OnStartEditLabel)
        self.endEdit.connect(self.OnEditLabel)


    def _reloadMapsetNode(self, mapset_node):
        """Recursively reload the model of a specific mapset node"""
        if mapset_node.children:
            del mapset_node.children[:]

        q = Queue()
        p = Process(
            target=getLocationTree,
            args=(
                mapset_node.parent.parent.data['name'],
                mapset_node.parent.data['name'],
                q,
                mapset_node.data['name']))
        p.start()
        maps, error = q.get()
        self._populateMapsetItem(mapset_node,
                                 maps[mapset_node.data['name']])
        self._orig_model = copy.deepcopy(self._model)
        return error

    def _reloadLocationNode(self, location_node):
        """Recursively reload the model of a specific location node"""
        if location_node.children:
            del location_node.children[:]

        q = Queue()
        p = Process(
            target=getLocationTree,
            args=(
                location_node.parent.data['name'],
                location_node.data['name'],
                q,
                None))
        p.start()
        maps, error = q.get()

        for mapset in maps:
            mapset_path = os.path.join(location_node.parent.data['name'],
                                       location_node.data['name'],
                                       mapset)
            # Check mapset ownership
            different_user = None
            if not is_mapset_users(mapset_path):
                different_user = get_mapset_owner(mapset_path)

            # Check mapset gislock
            locked = False
            if is_mapset_locked(mapset_path):
                locked = True

            mapset_node = self._model.AppendNode(
                                parent=location_node,
                                data=dict(type='mapset',
                                          name=mapset,
                                          lock=locked,
                                          user=different_user))
            self._populateMapsetItem(mapset_node,
                                     maps[mapset])
        self._model.SortChildren(location_node)
        self._orig_model = copy.deepcopy(self._model)
        return error

    def _reloadGrassDBNode(self, grassdb_node):
        """Recursively reload the model of a specific grassdb node.
        Runs reloading locations in parallel."""
        if grassdb_node.children:
            del grassdb_node.children[:]
        locations = GetListOfLocations(grassdb_node.data["name"])

        loc_count = proc_count = 0
        queue_list = []
        proc_list = []
        loc_list = []
        try:
            nprocs = cpu_count()
        except NotImplementedError:
            nprocs = 4

        results = dict()
        errors = []
        location_nodes = []
        all_location_nodes = []
        nlocations = len(locations)
        for location in locations:
            results[location] = dict()
            varloc = self._model.AppendNode(parent=grassdb_node,
                                            data=dict(type='location',
                                                      name=location))
            location_nodes.append(varloc)
            all_location_nodes.append(varloc)
            loc_count += 1

            Debug.msg(
                3, "Scanning location <{0}> ({1}/{2})".format(location, loc_count, nlocations))

            q = Queue()
            p = Process(target=getLocationTree,
                        args=(grassdb_node.data["name"], location, q))
            p.start()

            queue_list.append(q)
            proc_list.append(p)
            loc_list.append(location)

            proc_count += 1
            # Wait for all running processes
            if proc_count == nprocs or loc_count == nlocations:
                Debug.msg(4, "Process subresults")
                for i in range(len(loc_list)):
                    maps, error = queue_list[i].get()
                    proc_list[i].join()
                    if error:
                        errors.append(error)

                    for key in sorted(maps.keys()):
                        mapset_node = self._model.AppendNode(
                                parent=location_nodes[i],
                                data=dict(type='mapset', name=key))
                        self._populateMapsetItem(mapset_node, maps[key])

                proc_count = 0
                proc_list = []
                queue_list = []
                loc_list = []
                location_nodes = []    

        for node in all_location_nodes:
            self._model.SortChildren(node)
        self._model.SortChildren(grassdb_node)
        self._orig_model = copy.deepcopy(self._model)
        return errors

    def _reloadTreeItems(self):
        """Updates grass databases, locations, mapsets and layers in the tree.
        Saves resulting data and error."""
        errors = []
        for grassdatabase in self.grassdatabases:
            grassdb_nodes = self._model.SearchNodes(name=grassdatabase,
                                                    type='grassdb')
            if not grassdb_nodes:
                grassdb_node = self._model.AppendNode(parent=self._model.root,
                                                      data=dict(type='grassdb',
                                                                name=grassdatabase))
            else:
                grassdb_node = grassdb_nodes[0]
            error = self._reloadGrassDBNode(grassdb_node)
            if error:
                errors.append(error)
            
        if errors:
            wx.CallAfter(GWarning, '\n'.join(errors))
        Debug.msg(1, "Tree filled")

        self.UpdateCurrentDbLocationMapsetNode()
        self.RefreshItems()

    def _renameNode(self, node, name):
        """Rename node (map, mapset, location), sort and refresh.
        Should be called after actual renaming of a map, mapset, location."""
        node.data['name'] = name
        self._model.SortChildren(node.parent)
        self.RefreshNode(node.parent, recursive=True)

    def UpdateCurrentDbLocationMapsetNode(self):
        self.current_grassdb_node, self.current_location_node, self.current_mapset_node = \
            self.GetCurrentDbLocationMapsetNode()

    def ReloadTreeItems(self):
        """Reload dbs, locations, mapsets and layers in the tree."""
        self._reloadTreeItems()

    def ReloadCurrentMapset(self):
        """Reload current mapset tree only."""
        self.UpdateCurrentDbLocationMapsetNode()
        if not self.current_grassdb_node or not self.current_location_node or not self.current_mapset_node:
            return

        self._reloadMapsetNode(self.current_mapset_node)
        self.RefreshNode(self.current_mapset_node, recursive=True)

    def _populateMapsetItem(self, mapset_node, data):
        for item in data:
            self._model.AppendNode(parent=mapset_node,
                                   data=dict(**item))
        self._model.SortChildren(mapset_node)

    def _initVariables(self):
        """Init variables."""
        self.selected_grassdb = []
        self.selected_layer = []
        self.selected_mapset = []
        self.selected_location = []
        self.mixed = False

    def _initImages(self):
        bmpsize = (16, 16)
        icons = {
            'grassdb': MetaIcon(img='grassdb').GetBitmap(bmpsize),
            'location': MetaIcon(img='location').GetBitmap(bmpsize),
            'mapset': MetaIcon(img='mapset').GetBitmap(bmpsize),
            'raster': MetaIcon(img='raster').GetBitmap(bmpsize),
            'vector': MetaIcon(img='vector').GetBitmap(bmpsize),
            'raster_3d': MetaIcon(img='raster3d').GetBitmap(bmpsize)
        }
        il = wx.ImageList(bmpsize[0], bmpsize[1], mask=False)
        for each in self._iconTypes:
            il.Add(icons[each])
        self.AssignImageList(il)

    def GetControl(self):
        """Returns control itself."""
        return self

    def DefineItems(self, selected):
        """Set selected items."""
        self._initVariables()
        mixed = []
        for item in selected:
            type = item.data['type']
            if type in ('raster', 'raster_3d', 'vector'):
                self.selected_layer.append(item)
                self.selected_mapset.append(item.parent)
                self.selected_location.append(item.parent.parent)
                self.selected_grassdb.append(item.parent.parent.parent)
                mixed.append('layer')
            elif type == 'mapset':
                self.selected_layer.append(None)
                self.selected_mapset.append(item)
                self.selected_location.append(item.parent)
                self.selected_grassdb.append(item.parent.parent)
                mixed.append('mapset')
            elif type == 'location':
                self.selected_layer.append(None)
                self.selected_mapset.append(None)
                self.selected_location.append(item)
                self.selected_grassdb.append(item.parent)
                mixed.append('location')
            elif type == 'grassdb':
                self.selected_layer.append(None)
                self.selected_mapset.append(None)
                self.selected_location.append(None)
                self.selected_grassdb.append(item)
                mixed.append('grassdb')
        self.mixed = False
        if len(set(mixed)) > 1:
            self.mixed = True

    def OnSelChanged(self, event):
        self.selected_layer = None

    def OnRightClick(self, node):
        """Display popup menu."""
        self.DefineItems(self.GetSelected())
        if self.mixed:
            self._popupMenuEmpty()
            return

        if not self.selected_layer:
            self._popupMenuEmpty()
        elif self.selected_layer[0]:
            self._popupMenuLayer()
        elif self.selected_mapset[0] and len(self.selected_mapset) == 1:
            self._popupMenuMapset()
        elif self.selected_location[0] and not self.selected_mapset[0] and len(self.selected_location) == 1:
            self._popupMenuLocation()
        elif self.selected_grassdb[0] and not self.selected_location[0] and len(self.selected_grassdb) == 1:
            self._popupMenuGrassDb()
        elif len(self.selected_mapset) > 1:
            self._popupMenuMultipleMapsets()
        else:
            self._popupMenuEmpty()

    def OnDoubleClick(self, node):
        """Double click on item/node.

        Display selected layer if node is a map layer otherwise
        expand/collapse node.
        """
        if not isinstance(self._giface, StandaloneGrassInterface):
            self.DefineItems([node])
            if self.selected_layer[0] is not None:
                # display selected layer and return
                self.DisplayLayer()
                return

        # expand/collapse location/mapset...
        if self.IsNodeExpanded(node):
            self.CollapseNode(node, recursive=False)
        else:
            self.ExpandNode(node, recursive=False)

    def ExpandCurrentLocation(self):
        """Expand current location"""
        location = gscript.gisenv()['LOCATION_NAME']
        item = self._model.SearchNodes(name=location, type='location')
        if item:
            self.Select(item[0], select=True)
            self.ExpandNode(item[0], recursive=False)
        else:
            Debug.msg(1, "Location <%s> not found" % location)

    def GetCurrentDbLocationMapsetNode(self):
        """Get current mapset node"""
        genv = gisenv()
        gisdbase = genv['GISDBASE']
        location = genv['LOCATION_NAME']
        mapset = genv['MAPSET']

        grassdbItem = self._model.SearchNodes(
            name=gisdbase, type='grassdb')
        if not grassdbItem:
            return None, None, None

        locationItem = self._model.SearchNodes(
            parent=grassdbItem[0],
            name=location, type='location')
        if not locationItem:
            return grassdbItem[0], None, None

        mapsetItem = self._model.SearchNodes(
            parent=locationItem[0],
            name=mapset, type='mapset')
        if not mapsetItem:
            return grassdbItem[0], locationItem[0], None

        return grassdbItem[0], locationItem[0], mapsetItem[0]

    def OnGetItemImage(self, index, which=wx.TreeItemIcon_Normal, column=0):
        """Overriden method to return image for each item."""
        node = self._model.GetNodeByIndex(index)
        try:
            return self._iconTypes.index(node.data['type'])
        except ValueError:
            return 0

    def OnGetItemTextColour(self, index):
        """Overriden method to return colour for each item.
           Used to distinquish lock and ownership on mapsets."""
        node = self._model.GetNodeByIndex(index)
        text_colour = self.GetTextColour()
        if node.data['lock'] == True or node.data['user']:
            text_colour.SetTextColour(wx.SYS_COLOUR_GRAYTEXT)
        else:
            text_colour.SetTextColour(wx.SYS_COLOUR_WINDOWTEXT)
        return text_colour

    def OnGetItemFont(self, index):
        """Overriden method to return font for each item.
           Used to highlight current db/loc/mapset."""
        node = self._model.GetNodeByIndex(index)
        font = self.GetFont()
        if node.data['type'] in ('grassdb', 'location', 'mapset'):
            if node in (self.current_grassdb_node, self.current_location_node, self.current_mapset_node):
                font.SetWeight(wx.FONTWEIGHT_BOLD)
            else:
                font.SetWeight(wx.FONTWEIGHT_NORMAL)
        return font

    def ExpandCurrentMapset(self):
        """Expand current mapset"""
        if self.current_mapset_node:
            self.Select(self.current_mapset_node, select=True)
            self.ExpandNode(self.current_mapset_node, recursive=True)

    def _initVariablesCatalog(self):
        """Init variables."""
        self.copy_mode = False
        self.copy_layer = None
        self.copy_mapset = None
        self.copy_location = None
        self.copy_grassdb = None

    def SetRestriction(self, restrict):
        self._restricted = restrict

    def _runCommand(self, prog, **kwargs):
        cmdString = ' '.join(gscript.make_command(prog, **kwargs))
        ret = RunCommand(prog, parent=self, **kwargs)

        return ret, cmdString

    def OnMoveMap(self, event):
        """Move layer or mapset (just save it temporarily, copying is done by paste)"""
        self.copy_mode = False
        self.copy_layer = self.selected_layer[:]
        self.copy_mapset = self.selected_mapset[:]
        self.copy_location = self.selected_location[:]
        self.copy_grassdb = self.selected_grassdb[:]
        if len(self.copy_layer) > 1:
            label = _("{c} maps marked for moving.").format(c=len(self.selected_layer))
        else:
            label = _("Map <{layer}> marked for moving.").format(layer=self.copy_layer[0].data['name'])
        self.showNotification.emit(message=label)

    def OnCopyMap(self, event):
        """Copy layer or mapset (just save it temporarily, copying is done by paste)"""
        self.copy_mode = True
        self.copy_layer = self.selected_layer[:]
        self.copy_mapset = self.selected_mapset[:]
        self.copy_location = self.selected_location[:]
        self.copy_grassdb = self.selected_grassdb[:]
        if len(self.copy_layer) > 1:
            label = _("{c} maps marked for copying.").format(c=len(self.selected_layer))
        else:
            label = _("Map <{layer}> marked for copying.").format(layer=self.copy_layer[0].data['name'])
        self.showNotification.emit(message=label)

    def OnRenameMap(self, event):
        """Rename layer with dialog"""
        old_name = self.selected_layer[0].data['name']
        gisrc, env = gscript.create_environment(
            self.selected_grassdb[0].data['name'],
            self.selected_location[0].data['name'],
            self.selected_mapset[0].data['name'])

        new_name = self._getNewMapName(
            _('New name'),
            _('Rename map'),
            old_name,
            env=env,
            mapset=self.selected_mapset[0].data['name'],
            element=self.selected_layer[0].data['type'])
        if new_name:
            self.Rename(old_name, new_name)

    def CreateMapset(self, grassdb_node, location_node):
        """Creates new mapset interactively and adds it to the tree."""
        mapset = create_mapset_interactively(self, grassdb_node.data["name"],
                                             location_node.data["name"])
        if mapset:
            self.InsertMapset(name=mapset,
                              location_node=location_node)
            self.ReloadTreeItems()

    def OnCreateMapset(self, event):
        """Create new mapset"""
        self.CreateMapset(self.selected_grassdb[0], self.selected_location[0])

    def CreateLocation(self, grassdb_node):
        """
        Creates new location interactively and adds it to the tree.
        """
        grassdatabase, location, mapset = (
            create_location_interactively(self, grassdb_node.data['name'])
        )
        if location:
            grassdb_nodes = self._model.SearchNodes(name=grassdatabase, type='grassdb')
            if not grassdb_nodes:
                grassdb_node = self.InsertGrassDb(name=grassdatabase)
            self.InsertLocation(location, grassdb_node)

    def OnCreateLocation(self, event):
        """Create new location"""
        self.CreateLocation(self.selected_grassdb[0])

    def OnRenameMapset(self, event):
        """
        Rename selected mapset
        """
        newmapset = rename_mapset_interactively(
                self,
                self.selected_grassdb[0].data['name'],
                self.selected_location[0].data['name'],
                self.selected_mapset[0].data['name'])
        if newmapset:
            self._renameNode(self.selected_mapset[0], newmapset)

    def OnRenameLocation(self, event):
        """
        Rename selected location
        """
        newlocation = rename_location_interactively(
                self,
                self.selected_grassdb[0].data['name'],
                self.selected_location[0].data['name'])
        if newlocation:
            self._renameNode(self.selected_location[0], newlocation)

    def OnStartEditLabel(self, node, event):
        """Start label editing"""
        self.DefineItems([node])
        # TODO: add renaming mapset/location
        if not self.selected_layer[0]:
            event.Veto()
            return
        Debug.msg(1, "Start label edit {name}".format(name=node.data['name']))
        label = _("Editing {name}").format(name=node.data['name'])
        self.showNotification.emit(message=label)

    def OnEditLabel(self, node, event):
        """End label editing"""
        if self.selected_layer and not event.IsEditCancelled():
            old_name = node.data['name']
            Debug.msg(1, "End label edit {name}".format(name=old_name))
            new_name = event.GetLabel()
            self.Rename(old_name, new_name)

    def Rename(self, old, new):
        """Rename layer"""
        string = old + ',' + new
        gisrc, env = gscript.create_environment(
            self.selected_grassdb[0].data['name'],
            self.selected_location[0].data['name'],
            self.selected_mapset[0].data['name'])
        label = _("Renaming map <{name}>...").format(name=string)
        self.showNotification.emit(message=label)
        if self.selected_layer[0].data['type'] == 'vector':
            renamed, cmd = self._runCommand('g.rename', vector=string, env=env)
        elif self.selected_layer[0].data['type'] == 'raster':
            renamed, cmd = self._runCommand('g.rename', raster=string, env=env)
        else:
            renamed, cmd = self._runCommand(
                'g.rename', raster3d=string, env=env)
        if renamed == 0:
            self._renameNode(self.selected_layer[0], new)
            self.showNotification.emit(
                message=_("{cmd} -- completed").format(cmd=cmd))
            Debug.msg(1, "LAYER RENAMED TO: " + new)
        gscript.try_remove(gisrc)

    def OnPasteMap(self, event):
        # copying between mapsets of one location
        if not self.copy_layer:
            if self.copy_mode:
                GMessage(_("No map selected for copying."), parent=self)
            else:
                GMessage(_("No map selected for moving."), parent=self)
            return

        for i in range(len(self.copy_layer)):
            gisrc, env = gscript.create_environment(self.selected_grassdb[0].data['name'],
                                                    self.selected_location[0].data['name'],
                                                    self.selected_mapset[0].data['name'])
            gisrc2, env2 = gscript.create_environment(self.copy_grassdb[i].data['name'],
                                                      self.copy_location[i].data['name'],
                                                      self.copy_mapset[i].data['name'])
            new_name = self.copy_layer[i].data['name']
            if self.selected_location[0] == self.copy_location[i]:
                # within one mapset
                if self.selected_mapset[0] == self.copy_mapset[i]:
                    # ignore when just moves map
                    if self.copy_mode is False:
                        return
                    new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].data['name']),
                                                   _('Select new name'),
                                                   self.copy_layer[i].data['name'], env=env,
                                                   mapset=self.selected_mapset[0].data['name'],
                                                   element=self.copy_layer[i].data['type'])
                    if not new_name:
                        return
                # within one location, different mapsets
                else:
                    if map_exists(new_name, element=self.copy_layer[i].data['type'], env=env,
                                  mapset=self.selected_mapset[0].data['name']):
                        new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].data['name']),
                                                       _('Select new name'),
                                                       self.copy_layer[i].data['name'], env=env,
                                                       mapset=self.selected_mapset[0].data['name'],
                                                       element=self.copy_layer[i].data['type'])
                        if not new_name:
                            return

                string = self.copy_layer[i].data['name'] + '@' + self.copy_mapset[i].data['name'] + ',' + new_name
                pasted = 0
                if self.copy_mode:
                    label = _("Copying <{name}>...").format(name=string)
                else:
                    label = _("Moving <{name}>...").format(name=string)
                self.showNotification.emit(message=label)
                if self.copy_layer[i].data['type'] == 'vector':
                    pasted, cmd = self._runCommand('g.copy', vector=string, env=env)
                    node = 'vector'
                elif self.copy_layer[i].data['type'] == 'raster':
                    pasted, cmd = self._runCommand('g.copy', raster=string, env=env)
                    node = 'raster'
                else:
                    pasted, cmd = self._runCommand('g.copy', raster_3d=string, env=env)
                    node = 'raster_3d'
                if pasted == 0:
                    self.InsertLayer(name=new_name, mapset_node=self.selected_mapset[0],
                                     element_name=node)
                    Debug.msg(1, "COPIED TO: " + new_name)
                    if self.copy_mode:
                        self.showNotification.emit(message=_("g.copy completed"))
                    else:
                        self.showNotification.emit(message=_("g.copy completed"))

                    # remove old
                    if not self.copy_mode:
                        self._removeMapAfterCopy(self.copy_layer[i], self.copy_mapset[i], env2)

                gscript.try_remove(gisrc)
                gscript.try_remove(gisrc2)
                # expand selected mapset
            else:
                if self.copy_layer[i].data['type'] == 'raster_3d':
                    GError(_("Reprojection is not implemented for 3D rasters"), parent=self)
                    return
                if map_exists(new_name, element=self.copy_layer[i].data['type'], env=env,
                              mapset=self.selected_mapset[0].data['name']):
                    new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                                   self.copy_layer[i].data['name'], env=env,
                                                   mapset=self.selected_mapset[0].data['name'],
                                                   element=self.copy_layer[i].data['type'])
                    if not new_name:
                        continue
                callback = lambda gisrc2=gisrc2, gisrc=gisrc, cLayer=self.copy_layer[i], \
                                  cMapset=self.copy_mapset[i], cMode=self.copy_mode, name=new_name: \
                                  self._onDoneReprojection(env2, gisrc2, gisrc, cLayer, cMapset, cMode, name)
                dlg = CatalogReprojectionDialog(self, self._giface,
                                                self.copy_grassdb[i].data['name'],
                                                self.copy_location[i].data['name'],
                                                self.copy_mapset[i].data['name'],
                                                self.copy_layer[i].data['name'],
                                                env2,
                                                self.selected_grassdb[0].data['name'],
                                                self.selected_location[0].data['name'],
                                                self.selected_mapset[0].data['name'],
                                                new_name,
                                                self.copy_layer[i].data['type'],
                                                env, callback)
                if dlg.ShowModal() == wx.ID_CANCEL:
                    return
        self.ExpandNode(self.selected_mapset[0], recursive=True)
        self._initVariablesCatalog()

    def _onDoneReprojection(self, iEnv, iGisrc, oGisrc, cLayer, cMapset, cMode, name):
        self.InsertLayer(name=name, mapset_node=self.selected_mapset[0],
                         element_name=cLayer.data['type'])
        if not cMode:
            self._removeMapAfterCopy(cLayer, cMapset, iEnv)
        gscript.try_remove(iGisrc)
        gscript.try_remove(oGisrc)
        self.ExpandNode(self.selected_mapset[0], recursive=True)

    def _removeMapAfterCopy(self, cLayer, cMapset, env):
        removed, cmd = self._runCommand('g.remove', type=cLayer.data['type'],
                                        name=cLayer.data['name'], flags='f', env=env)
        if removed == 0:
            self._model.RemoveNode(cLayer)
            self.RefreshNode(cMapset, recursive=True)
            Debug.msg(1, "LAYER " + cLayer.data['name'] + " DELETED")
            self.showNotification.emit(message=_("g.remove completed"))

    def InsertLayer(self, name, mapset_node, element_name):
        """Insert layer into model and refresh tree"""
        self._model.AppendNode(parent=mapset_node,
                               data=dict(type=element_name, name=name))
        self._model.SortChildren(mapset_node)
        self.RefreshNode(mapset_node, recursive=True)

    def InsertMapset(self, name, location_node):
        """Insert new mapset into model and refresh tree.
        Assumes mapset is empty."""
        mapset_node = self._model.AppendNode(parent=location_node,
                                             data=dict(type="mapset", name=name))
        self._model.SortChildren(location_node)
        self.RefreshNode(location_node, recursive=True)
        return mapset_node

    def InsertLocation(self, name, grassdb_node):
        """Insert new location into model and refresh tree"""
        location_node = self._model.AppendNode(parent=grassdb_node,
                                               data=dict(type="location", name=name))
        # reload new location since it has a mapset
        self._reloadLocationNode(location_node)
        self._model.SortChildren(grassdb_node)
        self.RefreshNode(grassdb_node, recursive=True)
        return location_node

    def InsertGrassDb(self, name):
        """Insert new grass db into model and refresh tree"""
        self.grassdatabases.append(name)
        grassdb_node = self._model.AppendNode(parent=self._model.root,
                                              data=dict(type="grassdb", name=name))
        self._reloadGrassDBNode(grassdb_node)
        self.RefreshItems()
        return grassdb_node

    def OnDeleteMap(self, event):
        """Delete layer or mapset"""
        names = [self.selected_layer[i].data['name'] + '@' + self.selected_mapset[i].data['name']
                 for i in range(len(self.selected_layer))]
        if len(names) < 10:
            question = _("Do you really want to delete map(s) <{m}>?").format(m=', '.join(names))
        else:
            question = _("Do you really want to delete {n} maps?").format(n=len(names))
        if self._confirmDialog(question, title=_('Delete map')) == wx.ID_YES:
            label = _("Deleting {name}...").format(name=names)
            self.showNotification.emit(message=label)
            for i in range(len(self.selected_layer)):
                gisrc, env = gscript.create_environment(
                    self.selected_grassdb[i].data['name'],
                    self.selected_location[i].data['name'],
                    self.selected_mapset[i].data['name'])
                removed, cmd = self._runCommand(
                        'g.remove', flags='f', type=self.selected_layer[i].data['type'],
                        name=self.selected_layer[i].data['name'], env=env)
                if removed == 0:
                    self._model.RemoveNode(self.selected_layer[i])
                    self.RefreshNode(self.selected_mapset[i], recursive=True)
                    Debug.msg(1, "LAYER " + self.selected_layer[i].data['name'] + " DELETED")

                    # remove map layer from layer tree if exists
                    if not isinstance(self._giface, StandaloneGrassInterface):
                        name = self.selected_layer[i].data['name'] + '@' + self.selected_mapset[i].data['name']
                        layers = self._giface.GetLayerList().GetLayersByName(name)
                        for layer in layers:
                            self._giface.GetLayerList().DeleteLayer(layer)

                gscript.try_remove(gisrc)
            self.UnselectAll()
            self.showNotification.emit(message=_("g.remove completed"))

    def OnDeleteMapset(self, event):
        """
        Delete selected mapset or mapsets
        """
        mapsets = []
        for i in range(len(self.selected_mapset)):
            # Append to the list of tuples
            mapsets.append((
                self.selected_grassdb[i].data['name'],
                self.selected_location[i].data['name'],
                self.selected_mapset[i].data['name']
            ))
        if delete_mapsets_interactively(self, mapsets):
            locations = set([each for each in self.selected_location])
            for loc_node in locations:
                self._reloadLocationNode(loc_node)
                self.UpdateCurrentDbLocationMapsetNode()
                self.RefreshNode(loc_node, recursive=True)

    def OnDeleteLocation(self, event):
        """
        Delete selected location
        """
        if (delete_location_interactively(
                self,
                self.selected_grassdb[0].data['name'],
                self.selected_location[0].data['name'])):
            self._reloadGrassDBNode(self.selected_grassdb[0])
            self.UpdateCurrentDbLocationMapsetNode()
            self.RefreshNode(self.selected_grassdb[0], recursive=True)

    def DownloadLocation(self, grassdb_node):
        """
        Download new location interactively.
        """
        grassdatabase, location, mapset = (
            download_location_interactively(self, grassdb_node.data['name'])
        )
        if location:
            self._reloadGrassDBNode(grassdb_node)
            self.UpdateCurrentDbLocationMapsetNode()
            self.RefreshItems()

    def OnDownloadLocation(self, event):
        """
        Download location online
        """
        self.DownloadLocation(self.selected_grassdb[0])

    def OnDisplayLayer(self, event):
        """
        Display layer in current graphics view
        """
        self.DisplayLayer()

    def DisplayLayer(self):
        """Display selected layer in current graphics view"""
        all_names = []
        names = {'raster': [], 'vector': [], 'raster3d': []}
        for i in range(len(self.selected_layer)):
            name = self.selected_layer[i].data['name'] + '@' + self.selected_mapset[i].data['name']
            names[self.selected_layer[i].data['type']].append(name)
            all_names.append(name)
        #if self.selected_location[0].data['name'] == gisenv()['LOCATION_NAME'] and self.selected_mapset[0]:
        for ltype in names:
            if names[ltype]:
                self._giface.lmgr.AddMaps(list(reversed(names[ltype])), ltype, True)

        if len(self._giface.GetLayerList()) == 1:
            # zoom to map if there is only one map layer
            self._giface.GetMapWindow().ZoomToMap()

        Debug.msg(1, "Displayed layer(s): " + str(all_names))

    def OnBeginDrag(self, node, event):
        """Just copy necessary data"""
        self.DefineItems(self.GetSelected())

        if self.selected_location and None in self.selected_mapset and \
           None in self.selected_layer:
            GMessage(_("Move or copy location isn't allowed"))
            event.Veto()
            return
        elif self.selected_location and self.selected_mapset and \
             None in self.selected_layer:
            GMessage(_("Move or copy mapset isn't allowed"))
            event.Veto()
            return

        if self.selected_layer and not (self._restricted and gisenv()[
                                        'LOCATION_NAME'] != self.selected_location[0].data['name']):
            event.Allow()
            self.OnCopyMap(event)
            Debug.msg(1, "DRAG")
        else:
            event.Veto()

    def OnEndDrag(self, node, event):
        """Copy layer into target"""
        self.copy_mode = wx.GetMouseState().ControlDown()
        if node:
            self.DefineItems([node])
            if None not in self.selected_mapset:
                if self._restricted and gisenv()['MAPSET'] != self.selected_mapset[0].data['name']:
                    GMessage(_("To move or copy maps to other mapsets, unlock editing of other mapsets"),
                             parent=self)
                    event.Veto()
                    return

                event.Allow()
                Debug.msg(1, "DROP DONE")
                self.OnPasteMap(event)
            else:
                GMessage(_("To move or copy maps to other location, "
                           "please drag them to a mapset in the "
                           "destination location"),
                         parent=self)
                event.Veto()
                return

    def OnSwitchDbLocationMapset(self, event):
        """Switch to location and mapset"""
        self._SwitchDbLocationMapset()

    def _SwitchDbLocationMapset(self):
        """Switch to location and mapset"""
        genv = gisenv()
        # Distinguish when only part of db/location/mapset is changed.
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
        ):
            self.changeMapset.emit(mapset=self.selected_mapset[0].data['name'])
        elif self.selected_grassdb[0].data['name'] == genv['GISDBASE']:
            self.changeLocation.emit(mapset=self.selected_mapset[0].data['name'],
                                     location=self.selected_location[0].data['name'],
                                     dbase=None)
        else:
            self.changeLocation.emit(mapset=self.selected_mapset[0].data['name'],
                                     location=self.selected_location[0].data['name'],
                                     dbase=self.selected_grassdb[0].data['name'])
        self.UpdateCurrentDbLocationMapsetNode()
        self.ExpandCurrentMapset()
        self.RefreshItems()

    def OnMetadata(self, event):
        """Show metadata of any raster/vector/3draster"""
        def done(event):
            gscript.try_remove(event.userData)

        for i in range(len(self.selected_layer)):
            if self.selected_layer[i].data['type'] == 'raster':
                cmd = ['r.info']
            elif self.selected_layer[i].data['type'] == 'vector':
                cmd = ['v.info']
            elif self.selected_layer[i].data['type'] == 'raster_3d':
                cmd = ['r3.info']
            cmd.append('map=%s@%s' % (self.selected_layer[i].data['name'], self.selected_mapset[i].data['name']))

            gisrc, env = gscript.create_environment(
                self.selected_grassdb[i].data['name'],
                self.selected_location[i].data['name'],
                self.selected_mapset[i].data['name'])
            # print output to command log area
            # temp gisrc file must be deleted onDone
            self._giface.RunCmd(cmd, env=env, onDone=done, userData=gisrc)

    def OnCopyName(self, event):
        """Copy layer name to clipboard"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            text = []
            for i in range(len(self.selected_layer)):
                text.append('%s@%s' % (self.selected_layer[i].data['name'], self.selected_mapset[i].data['name']))
            do.SetText(','.join(text))
            wx.TheClipboard.SetData(do)
            wx.TheClipboard.Close()

    def Filter(self, text):
        """Filter tree based on name and type."""
        text = text.strip()
        if len(text.split(':')) > 1:
            name = text.split(':')[1].strip()
            elem = text.split(':')[0].strip()
            if 'r' == elem:
                element = 'raster'
            elif 'r3' == elem:
                element = 'raster_3d'
            elif 'v' == elem:
                element = 'vector'
            else:
                element = None
        else:
            element = None
            name = text.strip()

        self._model = filterModel(self._orig_model, name=name, element=element)
        self.UpdateCurrentDbLocationMapsetNode()
        self.RefreshItems()
        self.ExpandCurrentMapset()

    def _getNewMapName(self, message, title, value, element, mapset, env):
        """Dialog for simple text entry"""
        dlg = NameEntryDialog(parent=self, message=message, caption=title,
                              element=element, env=env, mapset=mapset)
        dlg.SetValue(value)
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
        else:
            name = None
        dlg.Destroy()

        return name

    def _confirmDialog(self, question, title):
        """Confirm dialog"""
        dlg = wx.MessageDialog(self, question, title, wx.YES_NO)
        res = dlg.ShowModal()
        dlg.Destroy()
        return res

    def _isCurrent(self, genv):
        if self._restricted:
            currentMapset = currentLocation = currentGrassDb = True
            for i in range(len(self.selected_grassdb)):
                if self.selected_grassdb[i].data['name'] != genv['GISDBASE']:
                    currentGrassDb = False
                    currentLocation = False
                    currentMapset = False
                    break
            if currentLocation:
                for i in range(len(self.selected_location)):
                    if self.selected_location[i].data['name'] != genv['LOCATION_NAME']:
                        currentLocation = False
                        currentMapset = False
                        break
            if currentMapset:
                for i in range(len(self.selected_mapset)):
                    if self.selected_mapset[i].data['name'] != genv['MAPSET']:
                        currentMapset = False
                        break

            return currentGrassDb, currentLocation, currentMapset
        else:
            return True, True, True

    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        menu = Menu()
        genv = gisenv()
        currentGrassDb, currentLocation, currentMapset = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Cut"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMoveMap, item)
        if not currentMapset:
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Copy"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCopyMap, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("Copy &name"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCopyName, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteMap, item)
        item.Enable(currentMapset)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameMap, item)
        item.Enable(currentMapset and len(self.selected_layer) == 1)

        menu.AppendSeparator()

        if not isinstance(self._giface, StandaloneGrassInterface):
            if all([each.data['name'] == genv['LOCATION_NAME'] for each in self.selected_location]):
                if len(self.selected_layer) > 1:
                    item = wx.MenuItem(menu, wx.ID_ANY, _("&Display layers"))
                else:
                    item = wx.MenuItem(menu, wx.ID_ANY, _("&Display layer"))
                menu.AppendItem(item)
                self.Bind(wx.EVT_MENU, self.OnDisplayLayer, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("Show &metadata"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMetadata, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuMapset(self):
        """Create popup menu for mapsets"""
        menu = Menu()
        genv = gisenv()
        currentGrassDb, currentLocation, currentMapset = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Switch mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnSwitchDbLocationMapset, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
            and self.selected_mapset[0].data['name'] == genv['MAPSET']
        ):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteMapset, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
            and self.selected_mapset[0].data['name'] == genv['MAPSET']
        ):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameMapset, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
            and self.selected_mapset[0].data['name'] == genv['MAPSET']
        ):
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuLocation(self):
        """Create popup menu for locations"""
        menu = Menu()
        genv = gisenv()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Create mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCreateMapset, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete location"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteLocation, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
        ):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename location"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameLocation, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_location[0].data['name'] == genv['LOCATION_NAME']
        ):
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuGrassDb(self):
        """Create popup menu for grass db"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Create new location"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCreateLocation, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Download sample location"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDownloadLocation, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuElement(self):
        """Create popup menu for elements"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        genv = gisenv()
        currentGrassDb, currentLocation, currentMapset = self._isCurrent(genv)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuMultipleMapsets(self):
        """Create popup menu for multiple selected mapsets"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete mapsets"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteMapset, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuEmpty(self):
        """Create empty popup when multiple different types of items are selected"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("No available options"))
        menu.AppendItem(item)
        item.Enable(False)
        self.PopupMenu(menu)
        menu.Destroy()
