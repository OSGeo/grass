"""
@package datacatalog::tree

@brief Data catalog tree classes

Classes:
 - datacatalog::LocationMapTree
 - datacatalog::DataCatalogTree

(C) 2014-2015 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova
@author Anna Petrasova (kratochanna gmail com)
"""
import os
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
                for elem in mapset.children:
                    if element and elem.label != element:
                        nodesToRemove.append(elem)
                        continue
                    for node in elem.children:
                        if name and regex.search(node.label) is None:
                            nodesToRemove.append(node)

    for node in reversed(nodesToRemove):
        fmodel.RemoveNode(node)

    cleanUpTree(fmodel)
    return fmodel


def cleanUpTree(model):
    """Removes empty element/mapsets/locations nodes.
    It first removes empty elements, then mapsets, then locations"""
    # removes empty elements
    nodesToRemove = []
    for gisdbase in model.root.children:
        for location in gisdbase.children:
            for mapset in location.children:
                for element in mapset.children:
                    if not element.children:
                        nodesToRemove.append(element)
    for node in reversed(nodesToRemove):
        model.RemoveNode(node)
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


class LocationMapTree(TreeView):

    def __init__(
            self, parent, model=None, style=wx.TR_HIDE_ROOT | wx.TR_EDIT_LABELS
            | wx.TR_LINES_AT_ROOT | wx.TR_HAS_BUTTONS | wx.TR_FULL_ROW_HIGHLIGHT |
            wx.TR_SINGLE):
        """Location Map Tree constructor."""
        self._model = TreeModel(DataCatalogNode)
        self._orig_model = self._model
        super(
            LocationMapTree,
            self).__init__(
            parent=parent,
            model=self._model,
            id=wx.ID_ANY,
            style=style)
        self.showNotification = Signal('Tree.showNotification')
        self.changeMapset = Signal('Tree.changeMapset')
        self.changeLocation = Signal('Tree.changeLocation')
        self.parent = parent
        self.contextMenu.connect(self.OnRightClick)
        self.itemActivated.connect(self.OnDoubleClick)

        self._initVariables()

    def _initTreeItems(self, locations=None, mapsets=None):
        """Add locations, mapsets and layers to the tree.
        Runs in multiple processes. Saves resulting data and error."""
        # mapsets param currently unused
        genv = gisenv()
        if not locations:
            locations = GetListOfLocations(genv['GISDBASE'])

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
            parent=self._model.root, label=_('GRASS locations in {0}').format(
                genv['GISDBASE']), data=dict(
                type='grassdata'))
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
                        args=(genv['GISDBASE'], location, q))
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

    def InitTreeItems(self):
        """Load locations, mapsets and layers in the tree."""
        raise NotImplementedError()

    def ReloadTreeItems(self):
        """Reload locations, mapsets and layers in the tree."""
        self._orig_model = self._model
        self._model.RemoveNode(self._model.root)
        self.InitTreeItems()

    def ReloadCurrentMapset(self):
        """Reload current mapset tree only."""
        def get_first_child(node):
            try:
                child = mapsetItem.children[0]
            except IndexError:
                child = None
            return child

        genv = gisenv()
        locationItem, mapsetItem = self.GetCurrentLocationMapsetNode()
        if not locationItem or not mapsetItem:
            return

        if mapsetItem.children:
            node = get_first_child(mapsetItem)
            while node:
                self._model.RemoveNode(node)
                node = get_first_child(mapsetItem)

        q = Queue()
        p = Process(
            target=getLocationTree,
            args=(
                genv['GISDBASE'],
                locationItem.data['name'],
                q,
                mapsetItem.data['name']))
        p.start()
        maps, error = q.get()
        if error:
            raise CalledModuleError(error)

        self._populateMapsetItem(mapsetItem, maps[mapsetItem.data['name']])
        self._orig_model = copy.deepcopy(self._model)
        self.RefreshNode(mapsetItem)
        self.RefreshItems()

    def _populateMapsetItem(self, mapset_node, data):
        for elem in data:
            if data[elem]:
                element_node = self._model.AppendNode(
                    parent=mapset_node, label=elem,
                    data=dict(type='element', name=elem))
                for layer in data[elem]:
                    self._model.AppendNode(parent=element_node, label=layer,
                                           data=dict(type=elem, name=layer))

    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        raise NotImplementedError()

    def _popupMenuMapset(self):
        """Create popup menu for mapsets"""
        raise NotImplementedError()

    def _popupMenuElement(self):
        """Create popup menu for elements"""
        raise NotImplementedError()

    def _initVariables(self):
        """Init variables."""
        self.selected_layer = None
        self.selected_type = None
        self.selected_mapset = None
        self.selected_location = None

    def GetControl(self):
        """Returns control itself."""
        return self

    def DefineItems(self, item):
        """Set selected items."""
        self.selected_layer = None
        self.selected_type = None
        self.selected_mapset = None
        self.selected_location = None

        type = item.data['type']
        if type in ('raster', 'raster_3d', 'vector'):
            self.selected_layer = item
            type = 'element'
            item = item.parent

        if type == 'element':
            self.selected_type = item
            type = 'mapset'
            item = item.parent

        if type == 'mapset':
            self.selected_mapset = item
            type = 'location'
            item = item.parent

        if type == 'location':
            self.selected_location = item

    def OnSelChanged(self, event):
        self.selected_layer = None

    def OnRightClick(self, node):
        """Display popup menu."""
        self.DefineItems(node)
        if self.selected_layer:
            self._popupMenuLayer()
        elif self.selected_mapset and not self.selected_type:
            self._popupMenuMapset()
        elif self.selected_type:
            self._popupMenuElement()

    def OnDoubleClick(self, node):
        """Expand/Collapse node."""
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

    def GetCurrentLocationMapsetNode(self):
        """Get current mapset node"""
        genv = gisenv()
        location = genv['LOCATION_NAME']
        mapset = genv['MAPSET']
        locationItem = self._model.SearchNodes(name=location, type='location')
        if not locationItem:
            return None, None

        mapsetItem = self._model.SearchNodes(
            parent=locationItem[0],
            name=mapset, type='mapset')
        if not mapsetItem:
            return locationItem[0], None

        return locationItem[0], mapsetItem[0]

    def ExpandCurrentMapset(self):
        """Expand current mapset"""
        locationItem, mapsetItem = self.GetCurrentLocationMapsetNode()
        if mapsetItem:
            self.Select(mapsetItem, select=True)
            self.ExpandNode(mapsetItem, recursive=True)


class DataCatalogTree(LocationMapTree):

    def __init__(self, parent, giface=None):
        """Data Catalog Tree constructor."""
        super(DataCatalogTree, self).__init__(parent)
        self._giface = giface
        self._restricted = True

        self._initVariablesCatalog()
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

    def _initVariablesCatalog(self):
        """Init variables."""
        self.copy_mode = False
        self.copy_layer = None
        self.copy_type = None
        self.copy_mapset = None
        self.copy_location = None

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
        self.copy_layer = self.selected_layer
        self.copy_type = self.selected_type
        self.copy_mapset = self.selected_mapset
        self.copy_location = self.selected_location
        label = _("Map <{layer}> marked for moving.").format(layer=self.copy_layer)
        self.showNotification.emit(message=label)

    def OnCopyMap(self, event):
        """Copy layer or mapset (just save it temporarily, copying is done by paste)"""
        self.copy_mode = True
        self.copy_layer = self.selected_layer
        self.copy_type = self.selected_type
        self.copy_mapset = self.selected_mapset
        self.copy_location = self.selected_location
        label = _("Map <{layer}> marked for copying.").format(layer=self.copy_layer)
        self.showNotification.emit(message=label)

    def OnRenameMap(self, event):
        """Rename layer with dialog"""
        old_name = self.selected_layer.label
        gisrc, env = gscript.create_environment(
            gisenv()['GISDBASE'],
            self.selected_location.label, mapset=self.selected_mapset.label)
        new_name = self._getNewMapName(
            _('New name'),
            _('Rename map'),
            old_name,
            env=env,
            mapset=self.selected_mapset.label,
            element=self.selected_type.label)
        if new_name:
            self.Rename(old_name, new_name)

    def OnStartEditLabel(self, node, event):
        """Start label editing"""
        self.DefineItems(node)
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
            gisenv()['GISDBASE'],
            self.selected_location.label, self.selected_mapset.label)
        label = _("Renaming map <{name}>...").format(name=string)
        self.showNotification.emit(message=label)
        if self.selected_type.label == 'vector':
            renamed, cmd = self._runCommand('g.rename', vector=string, env=env)
        elif self.selected_type.label == 'raster':
            renamed, cmd = self._runCommand('g.rename', raster=string, env=env)
        else:
            renamed, cmd = self._runCommand(
                'g.rename', raster3d=string, env=env)
        if renamed == 0:
            self.selected_layer.label = new
            self.selected_layer.data['name'] = new
            self.RefreshNode(self.selected_layer)
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

        gisrc, env = gscript.create_environment(
                        gisenv()['GISDBASE'], self.selected_location.label, mapset=self.selected_mapset.label)
        gisrc2, env2 = gscript.create_environment(
                        gisenv()['GISDBASE'], self.copy_location.label, mapset=self.copy_mapset.label)
        new_name = self.copy_layer.label
        if self.selected_location == self.copy_location:
            # within one mapset
            if self.selected_mapset == self.copy_mapset:
                # ignore when just moves map
                if self.copy_mode is False:
                    return
                new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                               self.copy_layer.label, env=env,
                                               mapset=self.selected_mapset.label,
                                               element=self.copy_type.label)
                if not new_name:
                    return
            # within one location, different mapsets
            else:
                if map_exists(new_name, element=self.copy_type.label, env=env,
                              mapset=self.selected_mapset.label):
                    new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                                   self.copy_layer.label, env=env,
                                                   mapset=self.selected_mapset.label,
                                                   element=self.copy_type.label)
                    if not new_name:
                        return

            string = self.copy_layer.label + '@' + self.copy_mapset.label + ',' + new_name
            pasted = 0
            if self.copy_mode:
                label = _("Copying <{name}>...").format(name=string)
            else:
                label = _("Moving <{name}>...").format(name=string)
            self.showNotification.emit(message=label)
            if self.copy_type.label == 'vector':
                pasted, cmd = self._runCommand('g.copy', vector=string, env=env)
                node = 'vector'
            elif self.copy_type.label == 'raster':
                pasted, cmd = self._runCommand('g.copy', raster=string, env=env)
                node = 'raster'
            else:
                pasted, cmd = self._runCommand('g.copy', raster_3d=string, env=env)
                node = 'raster_3d'
            if pasted == 0:
                self.InsertLayer(name=new_name, mapset_node=self.selected_mapset,
                                 element_name=node)
                Debug.msg(1, "COPIED TO: " + new_name)
                if self.copy_mode:
                    self.showNotification.emit(message=_("g.copy completed").format(cmd=cmd))
                else:
                    self.showNotification.emit(message=_("g.copy completed").format(cmd=cmd))

                # remove old
                if not self.copy_mode:
                    self._removeMapAfterCopy(env2)

            gscript.try_remove(gisrc)
            gscript.try_remove(gisrc2)
            # expand selected mapset
            self.ExpandNode(self.selected_mapset, recursive=True)
            self._initVariablesCatalog()
        else:
            if self.copy_type.label == 'raster_3d':
                GError(_("Reprojection is not implemented for 3D rasters"), parent=self)
                return
            if map_exists(new_name, element=self.copy_type.label, env=env,
                          mapset=self.selected_mapset.label):
                new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                               self.copy_layer.label, env=env,
                                               mapset=self.selected_mapset.label,
                                               element=self.copy_type.label)
                if not new_name:
                    return
            gisdbase = gisenv()['GISDBASE']
            callback = lambda: self._onDoneReprojection(iEnv=env2, iGisrc=gisrc2, oGisrc=gisrc)
            dlg = CatalogReprojectionDialog(self, self._giface, gisdbase, self.copy_location.label,
                                            self.copy_mapset.label, self.copy_layer.label, env2,
                                            gisdbase, self.selected_location.label, self.selected_mapset.label,
                                            new_name, self.copy_type.label, env, callback)
            dlg.ShowModal()

    def _onDoneReprojection(self, iEnv, iGisrc, oGisrc):
        self.InsertLayer(name=self.copy_layer.label, mapset_node=self.selected_mapset,
                         element_name=self.copy_type.label)
        if not self.copy_mode:
            self._removeMapAfterCopy(iEnv)
        gscript.try_remove(iGisrc)
        gscript.try_remove(oGisrc)
        self.ExpandNode(self.selected_mapset, recursive=True)
        self._initVariablesCatalog()

    def _removeMapAfterCopy(self, env):
        removed, cmd = self._runCommand('g.remove', type=self.copy_type.label,
                                        name=self.copy_layer.label, flags='f', env=env)
        if removed == 0:
            self._model.RemoveNode(self.copy_layer)
            self.RefreshNode(self.copy_type, recursive=True)
            Debug.msg(1, "LAYER " + self.copy_layer.label + " DELETED")
            self.showNotification.emit(message=_("g.remove completed").format(cmd=cmd))

    def InsertLayer(self, name, mapset_node, element_name):
        """Insert layer into model and refresh tree"""
        found_element = self._model.SearchNodes(
            parent=mapset_node, type='element', name=element_name)
        found_element = found_element[0] if found_element else None
        if not found_element:
            # add type node if not exists
            found_element = self._model.AppendNode(
                parent=mapset_node, label=element_name,
                data=dict(type='element', name=element_name))
        found = self._model.SearchNodes(parent=found_element, name=name)
        if len(found) == 0:
            self._model.AppendNode(parent=found_element, label=name,
                                   data=dict(type=element_name, name=name))
            self._model.SortChildren(found_element)
            self.RefreshNode(mapset_node, recursive=True)

    def OnDeleteMap(self, event):
        """Delete layer or mapset"""
        name = self.selected_layer.label
        gisrc, env = gscript.create_environment(
            gisenv()['GISDBASE'],
            self.selected_location.label, self.selected_mapset.label)
        if self._confirmDialog(
                question=_(
                    "Do you really want to delete map <{m}> of type <{etype}> from mapset "
                    "<{mapset}> in location <{loc}>?").format(
                    m=name, mapset=self.selected_mapset.label,
                    etype=self.selected_type.label,
                    loc=self.selected_location.label),
                title=_('Delete map')) == wx.ID_YES:
            label = _("Deleting {name}...").format(name=name)
            self.showNotification.emit(message=label)
            if self.selected_type.label == 'vector':
                removed, cmd = self._runCommand(
                    'g.remove', flags='f', type='vector', name=name, env=env)
            elif self.selected_type.label == 'raster':
                removed, cmd = self._runCommand(
                    'g.remove', flags='f', type='raster', name=name, env=env)
            else:
                removed, cmd = self._runCommand(
                    'g.remove', flags='f', type='raster_3d', name=name, env=env)
            if removed == 0:
                self._model.RemoveNode(self.selected_layer)
                self.RefreshNode(self.selected_type, recursive=True)
                Debug.msg(1, "LAYER " + name + " DELETED")
                self.showNotification.emit(
                    message=_("g.remove completed").format(cmd=cmd))
        gscript.try_remove(gisrc)

    def OnDisplayLayer(self, event):
        """Display layer in current graphics view"""
        layerName = []
        if self.selected_location.label == gisenv(
        )['LOCATION_NAME'] and self.selected_mapset:
            string = self.selected_layer.label + '@' + self.selected_mapset.label
            layerName.append(string)
            label = _("Displaying {name}...").format(name=string)
            self.showNotification.emit(message=label)
            label = "d." + self.selected_type.label[:4] + " --q map=" + string + \
                    _(" -- completed. Go to Layers tab for further operations.")
            if self.selected_type.label == 'vector':
                self._giface.lmgr.AddMaps(layerName, 'vector', True)
            elif self.selected_type.label == 'raster':
                self._giface.lmgr.AddMaps(layerName, 'raster', True)
            else:
                self._giface.lmgr.AddMaps(layerName, 'raster_3d', True)
                # generate this message (command) automatically?
                label = "d.rast --q map=" + string + _(
                    " -- completed. Go to Layers tab for further operations.")
            self.showNotification.emit(message=label)
            Debug.msg(1, "LAYER " + self.selected_layer.label + " DISPLAYED")
        else:
            GError(
                _("Failed to display layer: not in current mapset or invalid layer"),
                parent=self)

    def OnBeginDrag(self, node, event):
        """Just copy necessary data"""
        self.DefineItems(node)
        if self.selected_layer and not (self._restricted and gisenv()[
                                        'LOCATION_NAME'] != self.selected_location.label):
            event.Allow()
            self.OnCopyMap(event)
            Debug.msg(1, "DRAG")
        else:
            event.Veto()

    def OnEndDrag(self, node, event):
        """Copy layer into target"""
        self.copy_mode = wx.GetMouseState().ControlDown()
        if node:
            self.DefineItems(node)
            if self._restricted and gisenv()['MAPSET'] != self.selected_mapset.label:
                GMessage(_("To move or copy maps to other mapsets, unlock editing of other mapsets"),
                         parent=self)
                event.Veto()
                return

            event.Allow()
            Debug.msg(1, "DROP DONE")
            self.OnPasteMap(event)

    def OnSwitchLocationMapset(self, event):
        genv = gisenv()
        if self.selected_location.label == genv['LOCATION_NAME']:
            self.changeMapset.emit(mapset=self.selected_mapset.label)
        else:
            self.changeLocation.emit(mapset=self.selected_mapset.label, location=self.selected_location.label)
        self.ExpandCurrentMapset()

    def OnMetadata(self, event):
        """Show metadata of any raster/vector/3draster"""
        def done(event):
            gscript.try_remove(gisrc)

        if self.selected_type.label == 'raster':
            cmd = ['r.info']
        elif self.selected_type.label == 'vector':
            cmd = ['v.info']
        elif self.selected_type.label == 'raster_3d':
            cmd = ['r3.info']
        cmd.append('map=%s@%s' % (self.selected_layer.label, self.selected_mapset.label))

        gisrc, env = gscript.create_environment(
            gisenv()['GISDBASE'],
            self.selected_location.label, self.selected_mapset.label)
        # print output to command log area
        # temp gisrc file must be deleted onDone
        self._giface.RunCmd(cmd, env=env, onDone=done)

    def OnCopyName(self, event):
        """Copy layer name to clipboard"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            do.SetText('%s@%s' % (self.selected_layer.label, self.selected_mapset.label))
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
            currentMapset = currentLocation = False
            if self.selected_location.label == genv['LOCATION_NAME']:
                currentLocation = True
                if self.selected_mapset.label == genv['MAPSET']:
                    currentMapset = True
            return currentLocation, currentMapset
        else:
            return True, True

    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        menu = Menu()
        genv = gisenv()
        currentLocation, currentMapset = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.NewId(), _("&Cut"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMoveMap, item)
        if not currentMapset:
            item.Enable(False)

        item = wx.MenuItem(menu, wx.NewId(), _("&Copy"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCopyMap, item)

        item = wx.MenuItem(menu, wx.NewId(), _("Copy &name"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCopyName, item)

        item = wx.MenuItem(menu, wx.NewId(), _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.NewId(), _("&Delete"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteMap, item)
        item.Enable(currentMapset)

        item = wx.MenuItem(menu, wx.NewId(), _("&Rename"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameMap, item)
        item.Enable(currentMapset)

        menu.AppendSeparator()

        if not isinstance(self._giface, StandaloneGrassInterface) and \
           self.selected_location.label == genv['LOCATION_NAME']:
            item = wx.MenuItem(menu, wx.NewId(), _("&Display layer"))
            menu.AppendItem(item)
            self.Bind(wx.EVT_MENU, self.OnDisplayLayer, item)

        item = wx.MenuItem(menu, wx.NewId(), _("Show &metadata"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMetadata, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuMapset(self):
        """Create popup menu for mapsets"""
        menu = wx.Menu()
        genv = gisenv()
        currentLocation, currentMapset = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.NewId(), _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.NewId(), _("&Switch mapset"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnSwitchLocationMapset, item)
        if (self.selected_location.label == genv['LOCATION_NAME']
                and self.selected_mapset.label == genv['MAPSET']):
            item.Enable(False)
        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuElement(self):
        """Create popup menu for elements"""
        menu = wx.Menu()
        item = wx.MenuItem(menu, wx.NewId(), _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        genv = gisenv()
        currentLocation, currentMapset = self._isCurrent(genv)
        if not(currentMapset and self.copy_layer):
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()
