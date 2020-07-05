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
from startup.utils import create_mapset, get_default_mapset_name
from startup.guiutils import NewMapsetDialog

from grass.pydispatch.signal import Signal

import grass.script as gscript
from grass.script import gisenv
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
                    if name and regex.search(layer.label) is None:
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
        listOfMapsets = mapsets.split(',')
        Debug.msg(
            4, "Location <{0}>: {1} mapsets found".format(
                location, len(listOfMapsets)))
        for each in listOfMapsets:
            maps_dict[each] = {}
            for elem in elements:
                maps_dict[each][elem] = []
    try:
        maplist = gscript.read_command(
            'g.list', flags='mt', type=elements,
            mapset=','.join(listOfMapsets),
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
            maps_dict[mapset][ltype].append(name)

    queue.put((maps_dict, None))
    gscript.try_remove(tmp_gisrc_file)


def map_exists(name, element, env, mapset=None):
    """Check is map is present in the mapset given in the environment

    :param name: name of the map
    :param element: data type ('raster', 'raster_3d', and 'vector')
    :param env environment created by function gscript.create_environment
    """
    if not mapset:
        mapset = gscript.run_command('g.mapset', flags='p', env=env).strip()
    # change type to element used by find file
    if element == 'raster':
        element = 'cell'
    elif element == 'raster_3d':
        element = 'grid3'
    # g.findfile returns non-zero when file was not found
    # se we ignore return code and just focus on stdout
    process = gscript.start_command(
        'g.findfile',
        flags='n',
        element=element,
        file=name,
        mapset=mapset,
        stdout=gscript.PIPE,
        stderr=gscript.PIPE,
        env=env)
    output, errors = process.communicate()
    info = gscript.parse_key_val(output, sep='=')
    # file is the key questioned in grass.script.core find_file()
    # return code should be equivalent to checking the output
    if info['file']:
        return True
    else:
        return False


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
        if map_exists(new, self._element, self._env, self._mapset):
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

    def __init__(self, label, data=None):
        super(DataCatalogNode, self).__init__(label=label, data=data)

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
        self.changeGrassDb = Signal('Tree.changeGrassDb')
        self.parent = parent
        self.contextMenu.connect(self.OnRightClick)
        self.itemActivated.connect(self.OnDoubleClick)
        self._iconTypes = ['grassdb', 'location', 'mapset', 'raster',
                           'vector', 'raster_3d']
        self._initImages()

        self._initVariables()
        self._initVariablesCatalog()
        self.UpdateCurrentDbLocationMapsetNode()

        self._grassdatabases = []
        self._grassdatabases.append(gisenv()['GISDBASE'])

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

    def _initTreeItems(self, locations=None, mapsets=None):
        """Add grass databases, locations, mapsets and layers to the tree.
        Runs in multiple processes. Saves resulting data and error."""
        # mapsets param currently unused

        print(self._grassdatabases)
        for grassdatabase in self._grassdatabases:

            print(grassdatabase)
            locations = GetListOfLocations(grassdatabase)
            print(locations)

            loc_count = proc_count = 0
            queue_list = []
            proc_list = []
            loc_list = []
            nprocs = 4
            try:
                nprocs = cpu_count()
            except NotImplementedError:
                nprocs = 4

            results = dict()
            errors = []
            location_nodes = []
            nlocations = len(locations)
            grassdata_node = self._model.AppendNode(
                parent=self._model.root,
                label=grassdatabase,
                data=dict(type='grassdb', name=grassdatabase))
            for location in locations:
                results[location] = dict()
                varloc = self._model.AppendNode(
                    parent=grassdata_node, label=location, data=dict(
                        type='location', name=location))
                location_nodes.append(varloc)
                loc_count += 1

                Debug.msg(
                    3, "Scanning location <{0}> ({1}/{2})".format(location, loc_count, nlocations))

                q = Queue()
                p = Process(target=getLocationTree,
                            args=(grassdatabase, location, q))
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
                                label=key, data=dict(
                                    type='mapset', name=key))
                            self._populateMapsetItem(mapset_node, maps[key])

                    proc_count = 0
                    proc_list = []
                    queue_list = []
                    loc_list = []
                    location_nodes = []

        if errors:
            wx.CallAfter(GWarning, '\n'.join(errors))
        Debug.msg(1, "Tree filled")

        self.RefreshItems()

    def UpdateCurrentDbLocationMapsetNode(self):
        self.current_grassdb_node, self.current_location_node, self.current_mapset_node = \
            self.GetCurrentDbLocationMapsetNode()

    def ReloadTreeItems(self):
        """Reload dbs, locations, mapsets and layers in the tree."""
        self._orig_model = self._model
        self._model.SortChildren(self._model.root)
        self._model.RemoveNode(self._model.root)
        self.InitTreeItems()
        self.UpdateCurrentDbLocationMapsetNode()

    def ReloadCurrentMapset(self):
        """Reload current mapset tree only."""
        def get_first_child(node):
            try:
                child = self.current_mapset_node.children[0]
            except IndexError:
                child = None
            return child

        self.UpdateCurrentDbLocationMapsetNode()
        if not self.current_grassdb_node or not self.current_location_node or not self.current_mapset_node:
            return

        if self.current_mapset_node.children:
            node = get_first_child(self.current_mapset_node)
            while node:
                self._model.RemoveNode(node)
                node = get_first_child(self.current_mapset_node)

        q = Queue()
        p = Process(
            target=getLocationTree,
            args=(
                self.selected_grassdb[0],
                self.current_location_node.data['name'],
                q,
                self.current_mapset_node.data['name']))
        p.start()
        maps, error = q.get()
        if error:
            raise CalledModuleError(error)

        self._populateMapsetItem(self.current_mapset_node,
                                 maps[self.current_mapset_node.data['name']])
        self._orig_model = copy.deepcopy(self._model)
        self.RefreshNode(self.current_mapset_node)
        self.RefreshItems()

    def _populateMapsetItem(self, mapset_node, data):
        for elem in data:
            if data[elem]:
                for layer in data[elem]:
                    self._model.AppendNode(parent=mapset_node, label=layer,
                                           data=dict(type=elem, name=layer))
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
        elif self.selected_grassdb[0] and not self.selected_location[0] and not self.selected_mapset[0] and len(self.selected_grassdb) == 1:
            self._popupMenuGrassDb()
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

    def OnGetItemFont(self, index):
        """Overriden method to return font for each item.
           Used to highlight current db/loc/mapset."""
        node = self._model.GetNodeByIndex(index)
        font = self.GetFont()
        if node.data['type'] in ('grassdb','location', 'mapset'):
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

    def InitTreeItems(self):
        """Add locations, mapsets and layers to the tree."""
        self._initTreeItems()

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
            label = _("Map <{layer}> marked for moving.").format(layer=self.copy_layer[0].label)
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
            label = _("Map <{layer}> marked for copying.").format(layer=self.copy_layer[0].label)
        self.showNotification.emit(message=label)

    def OnRenameMap(self, event):
        """Rename layer with dialog"""
        old_name = self.selected_layer[0].label
        gisrc, env = gscript.create_environment(
            self.selected_grassdb[0].label,
            self.selected_location[0].label,
            self.selected_mapset[0].label)
        new_name = self._getNewMapName(
            _('New name'),
            _('Rename map'),
            old_name,
            env=env,
            mapset=self.selected_mapset[0].label,
            element=self.selected_layer[0].data['type'])
        if new_name:
            self.Rename(old_name, new_name)

    def OnStartEditLabel(self, node, event):
        """Start label editing"""
        self.DefineItems([node])
        Debug.msg(1, "Start label edit {name}".format(name=node.label))
        label = _("Editing {name}").format(name=node.label)
        self.showNotification.emit(message=label)
        if not self.selected_layer:
            event.Veto()

    def OnEditLabel(self, node, event):
        """End label editing"""
        if self.selected_layer and not event.IsEditCancelled():
            old_name = node.label
            Debug.msg(1, "End label edit {name}".format(name=old_name))
            new_name = event.GetLabel()
            self.Rename(old_name, new_name)

    def Rename(self, old, new):
        """Rename layer"""
        string = old + ',' + new
        gisrc, env = gscript.create_environment(
            self.selected_grassdb[0].label,
            self.selected_location[0].label,
            self.selected_mapset[0].label)
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
            self.selected_layer[0].label = new
            self.selected_layer[0].data['name'] = new
            self.RefreshNode(self.selected_layer[0])
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
            gisrc, env = gscript.create_environment(
                            self.selected_grassdb[0].label,
                            self.selected_location[0].label,
                            self.selected_mapset[0].label)
            gisrc2, env2 = gscript.create_environment(
                            self.selected_grassdb[i].label,
                            self.copy_location[i].label,
                            self.copy_mapset[i].label)
            new_name = self.copy_layer[i].label

            if self.selected_grassdb[0] == self.copy_grassdb[i]:
                if self.selected_location[0] == self.copy_location[i]:
                    # within one mapset
                    if self.selected_mapset[0] == self.copy_mapset[i]:
                        # ignore when just moves map
                        if self.copy_mode is False:
                            return
                        new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].label),
                                                       _('Select new name'),
                                                       self.copy_layer[i].label, env=env,
                                                       mapset=self.selected_mapset[0].label,
                                                       element=self.copy_layer[i].data['type'])
                        if not new_name:
                            return
                    # within one location, different mapsets
                    else:
                        if map_exists(new_name, element=self.copy_layer[i].data['type'], env=env,
                                      mapset=self.selected_mapset[0].label):
                            new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].label),
                                                           _('Select new name'),
                                                           self.copy_layer[i].label, env=env,
                                                           mapset=self.selected_mapset[0].label,
                                                           element=self.copy_layer[i].data['type'])
                            if not new_name:
                                return

                    string = self.copy_layer[i].label + '@' + self.copy_mapset[i].label + ',' + new_name
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
                              mapset=self.selected_mapset[0].label):
                    new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                                   self.copy_layer[i].label, env=env,
                                                   mapset=self.selected_mapset[0].label,
                                                   element=self.copy_layer[i].data['type'])
                    if not new_name:
                        continue
                gisdbase = self.selected_grassdb[0]
                callback = lambda gisrc2=gisrc2, gisrc=gisrc, cLayer=self.copy_layer[i], \
                                  cMapset=self.copy_mapset[i], cMode=self.copy_mode, name=new_name: \
                                  self._onDoneReprojection(env2, gisrc2, gisrc, cLayer, cMapset, cMode, name)
                dlg = CatalogReprojectionDialog(self, self._giface, gisdbase, self.copy_location[i].label,
                                                self.copy_mapset[i].label, self.copy_layer[i].label, env2,
                                                gisdbase, self.selected_location[0].label, self.selected_mapset[0].label,
                                                new_name, self.copy_layer[i].data['type'], env, callback)
                dlg.ShowModal()
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
                                        name=cLayer.label, flags='f', env=env)
        if removed == 0:
            self._model.RemoveNode(cLayer)
            self.RefreshNode(cMapset, recursive=True)
            Debug.msg(1, "LAYER " + cLayer.label + " DELETED")
            self.showNotification.emit(message=_("g.remove completed"))

    def InsertLayer(self, name, mapset_node, element_name):
        """Insert layer into model and refresh tree"""
        self._model.AppendNode(parent=mapset_node, label=name,
                               data=dict(type=element_name, name=name))
        self._model.SortChildren(mapset_node)
        self.RefreshNode(mapset_node, recursive=True)

    def InsertMapset(self, name, location_node):
        """Insert mapset into model and refresh tree"""
        self._model.AppendNode(parent=location_node, label=name,
                               data=dict(type="mapset", name=name))
        self._model.SortChildren(location_node)
        self.RefreshNode(location_node, recursive=True)

    def InsertGrassDb(self, name):
        """Insert grass db into model and refresh tree"""
        self._model.SortChildren(self._model.root)
        self.ReloadTreeItems()

    def OnDeleteMap(self, event):
        """Delete layer or mapset"""
        names = [self.selected_layer[i].label + '@' + self.selected_mapset[i].label
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
                    self.selected_grassdb[i].label,
                    self.selected_location[i].label,
                    self.selected_mapset[i].label)
                removed, cmd = self._runCommand(
                        'g.remove', flags='f', type=self.selected_layer[i].data['type'],
                        name=self.selected_layer[i].label, env=env)
                if removed == 0:
                    self._model.RemoveNode(self.selected_layer[i])
                    self.RefreshNode(self.selected_mapset[i], recursive=True)
                    Debug.msg(1, "LAYER " + self.selected_layer[i].label + " DELETED")

                    # remove map layer from layer tree if exists
                    if not isinstance(self._giface, StandaloneGrassInterface):
                        name = self.selected_layer[i].label + '@' + self.selected_mapset[i].label
                        layers = self._giface.GetLayerList().GetLayersByName(name)
                        for layer in layers:
                            self._giface.GetLayerList().DeleteLayer(layer)

                gscript.try_remove(gisrc)
            self.UnselectAll()
            self.showNotification.emit(message=_("g.remove completed"))

    def OnDisplayLayer(self, event):
        """Display layer in current graphics view"""
        self.DisplayLayer()

    def DisplayLayer(self):
        """Display selected layer in current graphics view"""
        all_names = []
        names = {'raster': [], 'vector': [], 'raster3d': []}
        for i in range(len(self.selected_layer)):
            name = self.selected_layer[i].label + '@' + self.selected_mapset[i].label
            names[self.selected_layer[i].data['type']].append(name)
            all_names.append(name)
        #if self.selected_location[0].label == gisenv()['LOCATION_NAME'] and self.selected_mapset[0]:
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
        if self.selected_layer and not (self._restricted and gisenv()[
                                        'LOCATION_NAME'] != self.selected_location[0].label):
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
            if self._restricted and gisenv()['MAPSET'] != self.selected_mapset[0].label:
                GMessage(_("To move or copy maps to other mapsets, unlock editing of other mapsets"),
                         parent=self)
                event.Veto()
                return

            event.Allow()
            Debug.msg(1, "DROP DONE")
            self.OnPasteMap(event)

    def OnSwitchDbLocationMapset(self, event):
        genv = gisenv()
        if self.selected_location[0].label == genv['LOCATION_NAME']:
            self.changeMapset.emit(mapset=self.selected_mapset[0].label)
        elif self.selected_grassdb[0].label == genv['GISDBASE']:
            self.changeLocation.emit(mapset=self.selected_mapset[0].label,
                                     location=self.selected_location[0].label)
        else:
            self.changeGrassDb.emit(mapset=self.selected_mapset[0].label,
                                     location=self.selected_location[0].label,
                                     grassdb=self.selected_grassdb[0].label)
        self.UpdateCurrentDbLocationMapsetNode()
        self.ExpandCurrentMapset()
        self.RefreshItems()

    def OnCreateMapset(self, event):
        """Create new mapset"""
        gisdbase = self.selected_grassdb[0]
        location = self.selected_location[0]

        dlg = NewMapsetDialog(
            parent=self,
            default=get_default_mapset_name(),
            database=gisdbase.label,
            location=location.label
        )
        if dlg.ShowModal() == wx.ID_OK:
            mapset = dlg.GetValue()
            try:
                create_mapset(gisdbase.label,
                              location.label,
                              mapset)
            except OSError as err:
                GError(parent=self,
                       message=_("Unable to create new mapset: %s") % err,
                       showTraceback=False)
            self.InsertMapset(name=mapset,
                              location_node=location)

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
            cmd.append('map=%s@%s' % (self.selected_layer[i].label, self.selected_mapset[i].label))

            gisrc, env = gscript.create_environment(
                self.selected_grassdb[i].label,
                self.selected_location[i].label,
                self.selected_mapset[i].label)
            # print output to command log area
            # temp gisrc file must be deleted onDone
            self._giface.RunCmd(cmd, env=env, onDone=done, userData=gisrc)

    def OnCopyName(self, event):
        """Copy layer name to clipboard"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            text = []
            for i in range(len(self.selected_layer)):
                text.append('%s@%s' % (self.selected_layer[i].label, self.selected_mapset[i].label))
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
        self.UpdateCurrentLocationMapsetNode()
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
                if self.selected_grassdb[i].label != genv['GISDBASE']:
                    currentGrassDb = False
                    currentLocation = False
                    currentMapset = False
                    break
            if currentLocation:
                for i in range(len(self.selected_location)):
                    if self.selected_location[i].label != genv['LOCATION_NAME']:
                        currentLocation = False
                        currentMapset = False
                        break
            if currentMapset:
                for i in range(len(self.selected_mapset)):
                    if self.selected_mapset[i].label != genv['MAPSET']:
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
            if all([each.label == genv['LOCATION_NAME'] for each in self.selected_location]):
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
        if (self.selected_grassdb[0].label == genv['GISDBASE']
            and self.selected_location[0].label == genv['LOCATION_NAME']
            and self.selected_mapset[0].label == genv['MAPSET']):
            item.Enable(False)
        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuLocation(self):
        """Create popup menu for locations"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Create mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCreateMapset, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuGrassDb(self):
        """Create popup menu for grass db"""
        menu = Menu()
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

    def _popupMenuEmpty(self):
        """Create empty popup when multiple different types of items are selected"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("No available options"))
        menu.AppendItem(item)
        item.Enable(False)
        self.PopupMenu(menu)
        menu.Destroy()
