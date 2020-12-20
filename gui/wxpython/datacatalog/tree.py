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
import os
import re
import copy
from multiprocessing import Process, Queue, cpu_count

watchdog_used = True
try:
    from watchdog.observers import Observer
    from watchdog.events import PatternMatchingEventHandler
except ImportError:
    watchdog_used = False
    PatternMatchingEventHandler = object


import wx
from wx.lib.newevent import NewEvent

from core.gcmd import RunCommand, GError, GMessage, GWarning
from core.utils import GetListOfProjects
from core.debug import Debug
from core.gthread import gThread
from gui_core.dialogs import TextEntryDialog
from core.giface import StandaloneGrassInterface
from core.treemodel import TreeModel, DictNode
from gui_core.treeview import TreeView
from gui_core.wrap import Menu
from datacatalog.dialogs import CatalogReprojectionDialog
from icons.icon import MetaIcon
from core.settings import UserSettings
from startup.guiutils import (
    create_subproject_interactively,
    create_project_interactively,
    rename_subproject_interactively,
    rename_project_interactively,
    delete_subprojects_interactively,
    delete_projects_interactively,
    download_project_interactively,
    delete_grassdb_interactively,
    can_switch_subproject_interactive,
    switch_subproject_interactively,
    get_reason_subproject_not_removable,
    get_reasons_project_not_removable,
    get_subproject_name_invalid_reason,
    get_project_name_invalid_reason
)
from grass.grassdb.manage import (
    rename_subproject,
    rename_project
)

from grass.pydispatch.signal import Signal

import grass.script as gscript
from grass.script import gisenv
from grass.grassdb.data import map_exists
from grass.grassdb.checks import (get_subproject_owner, is_subproject_locked,
                                  is_different_subproject_owner)
from grass.exceptions import CalledModuleError


updateSubproject, EVT_UPDATE_MAPSET = NewEvent()


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
        for project in gisdbase.children:
            for subproject in project.children:
                for layer in subproject.children:
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
    """Removes empty element/subprojects/projects nodes.
    It first removes empty elements, then subprojects, then projects"""
    # removes empty subprojects
    nodesToRemove = []
    for gisdbase in model.root.children:
        for project in gisdbase.children:
            for subproject in project.children:
                if not subproject.children:
                    nodesToRemove.append(subproject)
    for node in reversed(nodesToRemove):
        model.RemoveNode(node)
    # removes empty projects
    nodesToRemove = []
    for gisdbase in model.root.children:
        for project in gisdbase.children:
            if not project.children:
                nodesToRemove.append(project)
    for node in reversed(nodesToRemove):
        model.RemoveNode(node)


def getProjectTree(gisdbase, project, queue, subprojects=None):
    """Creates dictionary with subprojects, elements, layers for given project.
    Returns tuple with the dictionary and error (or None)"""
    tmp_gisrc_file, env = gscript.create_environment(gisdbase, project, 'PERMANENT')
    env['GRASS_SKIP_MAPSET_OWNER_CHECK'] = '1'

    maps_dict = {}
    elements = ['raster', 'raster_3d', 'vector']
    try:
        if not subprojects:
            subprojects = gscript.read_command(
                'g.subprojects',
                flags='l',
                separator='comma',
                quiet=True,
                env=env).strip()
    except CalledModuleError:
        queue.put(
            (maps_dict,
             _("Failed to read subprojects from project <{l}>.").format(
                 l=project)))
        gscript.try_remove(tmp_gisrc_file)
        return
    else:
        subprojects = subprojects.split(',')
        Debug.msg(
            4, "Project <{0}>: {1} subprojects found".format(
                project, len(subprojects)))
        for each in subprojects:
            maps_dict[each] = []
    try:
        maplist = gscript.read_command(
            'g.list', flags='mt', type=elements,
            subproject=','.join(subprojects),
            quiet=True, env=env).strip()
    except CalledModuleError:
        queue.put(
            (maps_dict,
             _("Failed to read maps from project <{l}>.").format(
                 l=project)))
        gscript.try_remove(tmp_gisrc_file)
        return
    else:
        # fill dictionary
        listOfMaps = maplist.splitlines()
        Debug.msg(
            4, "Project <{0}>: {1} maps found".format(
                project, len(listOfMaps)))
        for each in listOfMaps:
            ltype, wholename = each.split('/')
            name, subproject = wholename.split('@', maxsplit=1)
            maps_dict[subproject].append({'name': name, 'type': ltype})

    queue.put((maps_dict, None))
    gscript.try_remove(tmp_gisrc_file)


class MapWatch(PatternMatchingEventHandler):
    """Monitors file events (create, delete, move files) using watchdog
    to inform about changes in current subproject. One instance monitors
    only one element (raster, vector, raster_3d).
    Patterns are not used/needed in this case, use just '*' for matching
    everything. When file/directory change is detected, wx event is dispatched
    to event handler (can't use Signals because this is different thread),
    containing info about the change."""
    def __init__(self, patterns, element, event_handler):
        PatternMatchingEventHandler.__init__(self, patterns=patterns)
        self.element = element
        self.event_handler = event_handler
        
    def on_created(self, event):
        if (self.element == 'vector' or self.element == 'raster_3d') and not event.is_directory:
            return
        evt = updateSubproject(src_path=event.src_path, event_type=event.event_type,
                           is_directory=event.is_directory, dest_path=None)
        wx.PostEvent(self.event_handler, evt)

    def on_deleted(self, event):
        if (self.element == 'vector' or self.element == 'raster_3d') and not event.is_directory:
            return
        evt = updateSubproject(src_path=event.src_path, event_type=event.event_type,
                           is_directory=event.is_directory, dest_path=None)
        wx.PostEvent(self.event_handler, evt)

    def on_moved(self, event):
        if (self.element == 'vector' or self.element == 'raster_3d') and not event.is_directory:
            return
        evt = updateSubproject(src_path=event.src_path, event_type=event.event_type,
                           is_directory=event.is_directory, dest_path=event.dest_path)
        wx.PostEvent(self.event_handler, evt)  


class NameEntryDialog(TextEntryDialog):

    def __init__(self, element, subproject, env, **kwargs):
        TextEntryDialog.__init__(self, **kwargs)
        self._element = element
        self._subproject = subproject
        self._env = env
        id_OK = self.GetAffirmativeId()
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.FindWindowById(id_OK))

    def OnOK(self, event):
        new = self.GetValue()
        if not new:
            return
        if map_exists(new, self._element, env=self._env, subproject=self._subproject):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Map of type {elem} <{name}> already exists in subproject <{subproject}>. "
                    "Do you want to overwrite it?").format(
                    elem=self._element,
                    name=new,
                    subproject=self._subproject),
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
        data = self.data
        if data['type'] == 'subproject':
            owner = data['owner'] if data['owner'] else _("name unknown")
            if data['current']:
                return _("{name}  (current)").format(**data)
            elif data['is_different_owner'] and data['lock']:
                return _("{name}  (in use, owner: {owner})").format(
                    name=data["name"], owner=owner
                )
            elif data['lock']:
                return _("{name}  (in use)").format(**data)
            elif data['is_different_owner']:
                return _("{name}  (owner: {owner})").format(name=data["name"],
                                                            owner=owner)

        return _("{name}").format(**data)

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
    """Tree structure visualizing and managing grass database.
    Uses virtual tree and model defined in core/treemodel.py.

    When changes to data are initiated from inside, the model
    and the tree are not changed directly, rather a grassdbChanged
    signal needs to be emitted and the handler of the signal
    takes care of the refresh. At the same time, watchdog (if installed)
    monitors changes in current subproject and refreshes the tree.
    """
    def __init__(
            self, parent, model=None, giface=None,
            style=wx.TR_HIDE_ROOT | wx.TR_EDIT_LABELS |
            wx.TR_LINES_AT_ROOT | wx.TR_HAS_BUTTONS |
            wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_MULTIPLE):
        """Project Map Tree constructor."""
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
        self.parent = parent
        self.contextMenu.connect(self.OnRightClick)
        self.itemActivated.connect(self.OnDoubleClick)
        self._giface.currentSubprojectChanged.connect(self._updateAfterSubprojectChanged)
        self._giface.grassdbChanged.connect(self._updateAfterGrassdbChanged)
        self._iconTypes = ['grassdb', 'project', 'subproject', 'raster',
                           'vector', 'raster_3d']
        self._initImages()
        self.thread = gThread()

        self._resetSelectVariables()
        self._resetCopyVariables()
        self.current_grassdb_node = None
        self.current_project_node = None
        self.current_subproject_node = None
        self.UpdateCurrentDbProjectSubprojectNode()

        # Get databases from settings
        # add current to settings if it's not included
        self.grassdatabases = self._getValidSavedGrassDBs()
        currentDB = gisenv()['GISDBASE']
        if currentDB not in self.grassdatabases:
            self.grassdatabases.append(currentDB)
            self._saveGrassDBs()

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
        self.Bind(EVT_UPDATE_MAPSET, self.OnWatchdogSubprojectReload)
        
        self.observer = None

    def _resetSelectVariables(self):
        """Reset variables related to item selection."""
        self.selected_grassdb = []
        self.selected_layer = []
        self.selected_subproject = []
        self.selected_project = []
        self.mixed = False

    def _resetCopyVariables(self):
        """Reset copy related variables."""
        self.copy_mode = False
        self.copy_layer = None
        self.copy_subproject = None
        self.copy_project = None
        self.copy_grassdb = None

    def _getValidSavedGrassDBs(self):
        """Returns list of GRASS databases from settings.
        Returns only existing directories."""
        dbs = UserSettings.Get(group='datacatalog',
                               key='grassdbs',
                               subkey='listAsString')
        dbs = [db for db in dbs.split(',') if os.path.isdir(db)]
        return dbs

    def _saveGrassDBs(self):
        """Save current grass dbs in tree to settings"""
        UserSettings.Set(group='datacatalog',
                         key='grassdbs',
                         subkey='listAsString',
                         value=",".join(self.grassdatabases))
        grassdbSettings = {}
        UserSettings.ReadSettingsFile(settings=grassdbSettings)
        if 'datacatalog' not in grassdbSettings:
            grassdbSettings['datacatalog'] = UserSettings.Get(group='datacatalog')
        # update only dbs
        grassdbSettings['datacatalog']['grassdbs'] = UserSettings.Get(group='datacatalog', key='grassdbs')
        UserSettings.SaveToFile(grassdbSettings)

    def _reloadSubprojectNode(self, subproject_node):
        """Recursively reload the model of a specific subproject node"""
        if subproject_node.children:
            del subproject_node.children[:]

        q = Queue()
        p = Process(
            target=getProjectTree,
            args=(
                subproject_node.parent.parent.data['name'],
                subproject_node.parent.data['name'],
                q,
                subproject_node.data['name']))
        p.start()
        maps, error = q.get()
        self._populateSubprojectItem(subproject_node,
                                 maps[subproject_node.data['name']])
        self._orig_model = copy.deepcopy(self._model)
        return error

    def _reloadProjectNode(self, project_node):
        """Recursively reload the model of a specific project node"""
        if project_node.children:
            del project_node.children[:]

        q = Queue()
        p = Process(
            target=getProjectTree,
            args=(
                project_node.parent.data['name'],
                project_node.data['name'],
                q,
                None))
        p.start()
        maps, error = q.get()

        for subproject in maps:
            subproject_path = os.path.join(project_node.parent.data['name'],
                                       project_node.data['name'],
                                       subproject)

            subproject_node = self._model.AppendNode(
                                parent=project_node,
                                data=dict(type='subproject',
                                          name=subproject,
                                          current=False,
                                          lock=is_subproject_locked(subproject_path),
                                          is_different_owner=is_different_subproject_owner(subproject_path),
                                          owner=get_subproject_owner(subproject_path)))
            self._populateSubprojectItem(subproject_node,
                                     maps[subproject])
        self._model.SortChildren(project_node)
        self._orig_model = copy.deepcopy(self._model)
        return error

    def _reloadGrassDBNode(self, grassdb_node):
        """Recursively reload the model of a specific grassdb node.
        Runs reloading projects in parallel."""
        if grassdb_node.children:
            del grassdb_node.children[:]
        projects = GetListOfProjects(grassdb_node.data['name'])

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
        project_nodes = []
        all_project_nodes = []
        nprojects = len(projects)
        for project in projects:
            results[project] = dict()
            varloc = self._model.AppendNode(parent=grassdb_node,
                                            data=dict(type='project',
                                                      name=project))
            project_nodes.append(varloc)
            all_project_nodes.append(varloc)
            loc_count += 1

            Debug.msg(
                3, "Scanning project <{0}> ({1}/{2})".format(project, loc_count, nprojects))

            q = Queue()
            p = Process(target=getProjectTree,
                        args=(grassdb_node.data['name'], project, q))
            p.start()

            queue_list.append(q)
            proc_list.append(p)
            loc_list.append(project)

            proc_count += 1
            # Wait for all running processes
            if proc_count == nprocs or loc_count == nprojects:
                Debug.msg(4, "Process subresults")
                for i in range(len(loc_list)):
                    maps, error = queue_list[i].get()
                    proc_list[i].join()
                    if error:
                        errors.append(error)

                    for key in sorted(maps.keys()):
                        subproject_path = os.path.join(project_nodes[i].parent.data['name'],
                                                   project_nodes[i].data['name'],
                                                   key)
                        subproject_node = self._model.AppendNode(
                                parent=project_nodes[i],
                                data=dict(type='subproject',
                                          name=key,
                                          lock=is_subproject_locked(subproject_path),
                                          current=False,
                                          is_different_owner=is_different_subproject_owner(subproject_path),
                                          owner=get_subproject_owner(subproject_path)))
                        self._populateSubprojectItem(subproject_node, maps[key])

                proc_count = 0
                proc_list = []
                queue_list = []
                loc_list = []
                project_nodes = []

        for node in all_project_nodes:
            self._model.SortChildren(node)
        self._model.SortChildren(grassdb_node)
        self._orig_model = copy.deepcopy(self._model)
        return errors

    def _reloadTreeItems(self):
        """Updates grass databases, projects, subprojects and layers in the tree.

        It runs in thread, so it should not directly interact with GUI.
        In case of any errors it returns the errors as a list of strings, otherwise None.
        """
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
                errors += error

        if errors:
            return errors
        return None

    def ScheduleWatchCurrentSubproject(self):
        """Using watchdog library, sets up watching of current subproject folder
        to detect changes not captured by other means (e.g. from command line).
        Schedules 3 watches (raster, vector, 3D raster).
        If watchdog observers are active, it restarts the observers in current subproject.
        """
        global watchdog_used
        if not watchdog_used:
            return

        if self.observer and self.observer.is_alive():
            self.observer.stop()
            self.observer.join()
            self.observer.unschedule_all()
        self.observer = Observer()

        gisenv = gscript.gisenv()
        for element, directory in (('raster', 'cell'), ('vector', 'vector'), ('raster_3d', 'grid3')):
            path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'],
                                gisenv['MAPSET'], directory)
            if not os.path.exists(path):
                try:
                    os.mkdir(path)
                except OSError:
                    pass
            if os.path.exists(path):
                self.observer.schedule(MapWatch("*", element, self), path=path, recursive=False)
        try:
            self.observer.start()
        except OSError:
            # in case inotify on linux exceeds limits
            watchdog_used = False
            return

    def OnWatchdogSubprojectReload(self, event):
        """Reload subproject node associated with watchdog event"""
        subproject_path = os.path.dirname(os.path.dirname(os.path.abspath(event.src_path)))
        project_path = os.path.dirname(os.path.abspath(subproject_path))
        db = os.path.dirname(os.path.abspath(project_path))
        node = self.GetDbNode(grassdb=db, project=os.path.basename(project_path),
                              subproject=os.path.basename(subproject_path))
        if node:
            self._reloadSubprojectNode(node)
            self.RefreshNode(node, recursive=True)

    def GetDbNode(self, grassdb, project=None, subproject=None, map=None, map_type=None):
        """Returns node representing db/project/subproject/map or None if not found."""
        grassdb_nodes = self._model.SearchNodes(name=grassdb, type='grassdb')
        if grassdb_nodes:
            if not project:
                return grassdb_nodes[0]
            project_nodes = self._model.SearchNodes(parent=grassdb_nodes[0],
                                                     name=project, type='project')
            if project_nodes:
                if not subproject:
                    return project_nodes[0]
                subproject_nodes = self._model.SearchNodes(parent=project_nodes[0],
                                                       name=subproject, type='subproject')
                if subproject_nodes:
                    if not map:
                        return subproject_nodes[0]
                    map_nodes = self._model.SearchNodes(parent=subproject_nodes[0],
                                                        name=map, type=map_type)
                    if map_nodes:
                        return map_nodes[0]
        return None

    def _renameNode(self, node, name):
        """Rename node (map, subproject, project), sort and refresh.
        Should be called after actual renaming of a map, subproject, project."""
        node.data['name'] = name
        self._model.SortChildren(node.parent)
        self.RefreshNode(node.parent, recursive=True)

    def UpdateCurrentDbProjectSubprojectNode(self):
        """Update variables storing current subproject/project/grassdb node.
        Updates associated subproject node data ('lock' and 'current').
        """

        def is_current_subproject_node_locked():
            subproject_path = os.path.join(self.current_grassdb_node.data['name'],
                                       self.current_project_node.data['name'],
                                       self.current_subproject_node.data["name"])
            return is_subproject_locked(subproject_path)

        if self.current_subproject_node:
            self.current_subproject_node.data["current"] = False
            self.current_subproject_node.data["lock"] = is_current_subproject_node_locked()

        self.current_grassdb_node, self.current_project_node, self.current_subproject_node = \
            self.GetCurrentDbProjectSubprojectNode()

        if self.current_subproject_node:
            self.current_subproject_node.data["current"] = True
            self.current_subproject_node.data["lock"] = is_current_subproject_node_locked()

    def ReloadTreeItems(self):
        """Reload dbs, projects, subprojects and layers in the tree."""
        self.busy = wx.BusyCursor()
        self._quickLoading()
        self.thread.Run(callable=self._reloadTreeItems,
                        ondone=self._loadItemsDone)

    def _quickLoading(self):
        """Quick loading of projects to show
        something when loading for the first time"""
        if self._model.root.children:
            return
        gisenv = gscript.gisenv()
        for grassdatabase in self.grassdatabases:
            grassdb_node = self._model.AppendNode(parent=self._model.root,
                                                  data=dict(type='grassdb',
                                                            name=grassdatabase))
            for project in GetListOfProjects(grassdatabase):
                self._model.AppendNode(parent=grassdb_node,
                                       data=dict(type='project',
                                                 name=project))
            self.RefreshItems()
            if grassdatabase == gisenv['GISDBASE']:
                self.ExpandNode(grassdb_node, recursive=False)

    def _loadItemsDone(self, event):
        Debug.msg(1, "Tree filled")
        del self.busy
        if event.ret is not None:
            self._giface.WriteWarning('\n'.join(event.ret))
        self.UpdateCurrentDbProjectSubprojectNode()
        self.ScheduleWatchCurrentSubproject()
        self.RefreshItems()
        self.ExpandCurrentSubproject()

    def ReloadCurrentSubproject(self):
        """Reload current subproject tree only."""
        self.UpdateCurrentDbProjectSubprojectNode()
        if not self.current_grassdb_node or not self.current_project_node or not self.current_subproject_node:
            return

        self._reloadSubprojectNode(self.current_subproject_node)
        self.RefreshNode(self.current_subproject_node, recursive=True)

    def _populateSubprojectItem(self, subproject_node, data):
        for item in data:
            self._model.AppendNode(parent=subproject_node,
                                   data=dict(**item))
        self._model.SortChildren(subproject_node)

    def _initImages(self):
        bmpsize = (16, 16)
        icons = {
            'grassdb': MetaIcon(img='grassdb').GetBitmap(bmpsize),
            'project': MetaIcon(img='project').GetBitmap(bmpsize),
            'subproject': MetaIcon(img='subproject').GetBitmap(bmpsize),
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
        self._resetSelectVariables()
        mixed = []
        for item in selected:
            type = item.data['type']
            if type in ('raster', 'raster_3d', 'vector'):
                self.selected_layer.append(item)
                self.selected_subproject.append(item.parent)
                self.selected_project.append(item.parent.parent)
                self.selected_grassdb.append(item.parent.parent.parent)
                mixed.append('layer')
            elif type == 'subproject':
                self.selected_layer.append(None)
                self.selected_subproject.append(item)
                self.selected_project.append(item.parent)
                self.selected_grassdb.append(item.parent.parent)
                mixed.append('subproject')
            elif type == 'project':
                self.selected_layer.append(None)
                self.selected_subproject.append(None)
                self.selected_project.append(item)
                self.selected_grassdb.append(item.parent)
                mixed.append('project')
            elif type == 'grassdb':
                self.selected_layer.append(None)
                self.selected_subproject.append(None)
                self.selected_project.append(None)
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
        elif self.selected_subproject[0] and len(self.selected_subproject) == 1:
            self._popupMenuSubproject()
        elif self.selected_project[0] and not self.selected_subproject[0] and len(self.selected_project) == 1:
            self._popupMenuProject()
        elif self.selected_grassdb[0] and not self.selected_project[0] and len(self.selected_grassdb) == 1:
            self._popupMenuGrassDb()
        elif len(self.selected_grassdb) > 1 and not self.selected_project[0]:
            self._popupMenuEmpty()
        elif len(self.selected_project) > 1 and not self.selected_subproject[0]:
            self._popupMenuMultipleProjects()
        elif len(self.selected_subproject) > 1:
            self._popupMenuMultipleSubprojects()
        else:
            self._popupMenuEmpty()

    def OnDoubleClick(self, node):
        """Double click on item/node.

        Display selected layer if node is a map layer otherwise
        expand/collapse node.
        """
        if not isinstance(self._giface, StandaloneGrassInterface):
            self.DefineItems([node])
            selected_layer = self.selected_layer[0]
            selected_subproject = self.selected_subproject[0]
            selected_loc = self.selected_project[0]

            if selected_layer is not None:
                genv = gisenv()

                # Check if the layer is in different project
                if selected_loc.data['name'] != genv['LOCATION_NAME']:
                    dlg = wx.MessageDialog(
                        parent=self,
                        message=_(
                            "Map <{0}@{1}> is not in the current project"
                            " and therefore cannot be displayed."
                            "\n\n"
                            "To display this map switch to subproject <{1}> first."
                        ).format(selected_layer.data['name'],
                                 selected_subproject.data['name']),
                        caption=_("Unable to display the map"),
                        style=wx.OK | wx.ICON_WARNING
                    )
                    dlg.ShowModal()
                    dlg.Destroy()
                else:
                    self.DisplayLayer()
                    return

        # expand/collapse project/subproject...
        if self.IsNodeExpanded(node):
            self.CollapseNode(node, recursive=False)
        else:
            self.ExpandNode(node, recursive=False)

    def ExpandCurrentProject(self):
        """Expand current project"""
        project = gscript.gisenv()['LOCATION_NAME']
        item = self._model.SearchNodes(name=project, type='project')
        if item:
            self.Select(item[0], select=True)
            self.ExpandNode(item[0], recursive=False)
        else:
            Debug.msg(1, "Project <%s> not found" % project)

    def GetCurrentDbProjectSubprojectNode(self):
        """Get current subproject node"""
        genv = gisenv()
        gisdbase = genv['GISDBASE']
        project = genv['LOCATION_NAME']
        subproject = genv['MAPSET']

        grassdbItem = self._model.SearchNodes(
            name=gisdbase, type='grassdb')
        if not grassdbItem:
            return None, None, None

        projectItem = self._model.SearchNodes(
            parent=grassdbItem[0],
            name=project, type='project')
        if not projectItem:
            return grassdbItem[0], None, None

        subprojectItem = self._model.SearchNodes(
            parent=projectItem[0],
            name=subproject,
            type='subproject')
        if not subprojectItem:
            return grassdbItem[0], projectItem[0], None

        return grassdbItem[0], projectItem[0], subprojectItem[0]

    def OnGetItemImage(self, index, which=wx.TreeItemIcon_Normal, column=0):
        """Overriden method to return image for each item."""
        node = self._model.GetNodeByIndex(index)
        try:
            return self._iconTypes.index(node.data['type'])
        except ValueError:
            return 0

    def OnGetItemTextColour(self, index):
        """Overriden method to return colour for each item.
           Used to distinquish lock and ownership on subprojects."""
        node = self._model.GetNodeByIndex(index)
        if node.data['type'] == 'subproject':
            if node.data['current']:
                return wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT)
            elif node.data['lock'] or node.data['is_different_owner']:
                return wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT)
        return wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT)

    def OnGetItemFont(self, index):
        """Overriden method to return font for each item.
           Used to highlight current db/loc/subproject."""
        node = self._model.GetNodeByIndex(index)
        font = self.GetFont()
        if node.data['type'] in ('grassdb', 'project', 'subproject'):
            if node in (self.current_grassdb_node, self.current_project_node, self.current_subproject_node):
                font.SetWeight(wx.FONTWEIGHT_BOLD)
            else:
                font.SetWeight(wx.FONTWEIGHT_NORMAL)
        return font

    def ExpandCurrentSubproject(self):
        """Expand current subproject"""
        if self.current_subproject_node:
            self.Select(self.current_subproject_node, select=True)
            self.ExpandNode(self.current_subproject_node, recursive=True)

    def SetRestriction(self, restrict):
        self._restricted = restrict

    def _runCommand(self, prog, **kwargs):
        cmdString = ' '.join(gscript.make_command(prog, **kwargs))
        ret = RunCommand(prog, parent=self, **kwargs)

        return ret, cmdString

    def OnMoveMap(self, event):
        """Move layer or subproject (just save it temporarily, copying is done by paste)"""
        self.copy_mode = False
        self.copy_layer = self.selected_layer[:]
        self.copy_subproject = self.selected_subproject[:]
        self.copy_project = self.selected_project[:]
        self.copy_grassdb = self.selected_grassdb[:]
        if len(self.copy_layer) > 1:
            label = _("{c} maps marked for moving.").format(c=len(self.selected_layer))
        else:
            label = _("Map <{layer}> marked for moving.").format(layer=self.copy_layer[0].data['name'])
        self.showNotification.emit(message=label)

    def OnCopyMap(self, event):
        """Copy layer or subproject (just save it temporarily, copying is done by paste)"""
        self.copy_mode = True
        self.copy_layer = self.selected_layer[:]
        self.copy_subproject = self.selected_subproject[:]
        self.copy_project = self.selected_project[:]
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
            self.selected_project[0].data['name'],
            self.selected_subproject[0].data['name'])

        new_name = self._getNewMapName(
            _('New name'),
            _('Rename map'),
            old_name,
            env=env,
            subproject=self.selected_subproject[0].data['name'],
            element=self.selected_layer[0].data['type'])
        if new_name:
            self.Rename(old_name, new_name)

    def CreateSubproject(self, grassdb_node, project_node):
        """Creates new subproject interactively and adds it to the tree."""
        subproject = create_subproject_interactively(self, grassdb_node.data['name'],
                                             project_node.data['name'])
        if subproject:
            self._giface.grassdbChanged.emit(grassdb=grassdb_node.data['name'],
                                             project=project_node.data['name'],
                                             subproject=subproject,
                                             element='subproject',
                                             action='new')

    def OnCreateSubproject(self, event):
        """Create new subproject"""
        self.CreateSubproject(self.selected_grassdb[0], self.selected_project[0])

    def CreateProject(self, grassdb_node):
        """
        Creates new project interactively and adds it to the tree.
        """
        grassdatabase, project, subproject = (
            create_project_interactively(self, grassdb_node.data['name'])
        )
        if project:
            self._giface.grassdbChanged.emit(grassdb=grassdatabase,
                                             project=project,
                                             element='project',
                                             action='new')

    def OnCreateProject(self, event):
        """Create new project"""
        self.CreateProject(self.selected_grassdb[0])

    def OnRenameSubproject(self, event):
        """
        Rename selected subproject
        """
        newsubproject = rename_subproject_interactively(
                self,
                self.selected_grassdb[0].data['name'],
                self.selected_project[0].data['name'],
                self.selected_subproject[0].data['name'])
        if newsubproject:
            self._giface.grassdbChanged.emit(grassdb=self.selected_grassdb[0].data['name'],
                                             project=self.selected_project[0].data['name'],
                                             subproject=self.selected_subproject[0].data['name'],
                                             element='subproject',
                                             action='rename',
                                             newname=newsubproject)

    def OnRenameProject(self, event):
        """
        Rename selected project
        """
        newproject = rename_project_interactively(
                self,
                self.selected_grassdb[0].data['name'],
                self.selected_project[0].data['name'])
        if newproject:
            self._giface.grassdbChanged.emit(grassdb=self.selected_grassdb[0].data['name'],
                                             project=self.selected_project[0].data['name'],
                                             element='project',
                                             action='rename',
                                             newname=newproject)

    def OnStartEditLabel(self, node, event):
        """Start label editing"""
        self.DefineItems([node])

        # Not allowed for grassdb node
        if node.data['type'] == 'grassdb':
            event.Veto()
        # Check selected subproject
        elif node.data['type'] == 'subproject':
            if (
                self._restricted
                or get_reason_subproject_not_removable(self.selected_grassdb[0].data['name'],
                                                   self.selected_project[0].data['name'],
                                                   self.selected_subproject[0].data['name'],
                                                   check_permanent=True)
            ):
                event.Veto()
        # Check selected project
        elif node.data['type'] == 'project':
            if (
                self._restricted
                or get_reasons_project_not_removable(self.selected_grassdb[0].data['name'],
                                                      self.selected_project[0].data['name'])
            ):
                event.Veto()
        elif node.data['type'] in ('raster', 'raster_3d', 'vector'):
            currentGrassDb, currentProject, currentSubproject = self._isCurrent(gisenv())
            if not currentSubproject:
                event.Veto()

    def OnEditLabel(self, node, event):
        """End label editing"""
        if event.IsEditCancelled():
            return

        old_name = node.data['name']
        Debug.msg(1, "End label edit {name}".format(name=old_name))
        new_name = event.GetLabel()

        if node.data['type'] in ('raster', 'raster_3d', 'vector'):
            self.Rename(old_name, new_name)

        elif node.data['type'] == 'subproject':
            message = get_subproject_name_invalid_reason(
                            self.selected_grassdb[0].data['name'],
                            self.selected_project[0].data['name'],
                            new_name)
            if message:
                GError(parent=self, message=message,
                       caption=_("Cannot rename subproject"),
                       showTraceback=False)
                event.Veto()
                return
            rename_subproject(self.selected_grassdb[0].data['name'],
                          self.selected_project[0].data['name'],
                          self.selected_subproject[0].data['name'],
                          new_name)
            self._renameNode(self.selected_subproject[0], new_name)
            label = _(
                "Renaming subproject <{oldsubproject}> to <{newsubproject}> completed").format(
                oldsubproject=old_name, newsubproject=new_name)
            self.showNotification.emit(message=label)

        elif node.data['type'] == 'project':
            message = get_project_name_invalid_reason(
                            self.selected_grassdb[0].data['name'],
                            new_name)
            if message:
                GError(parent=self, message=message,
                       caption=_("Cannot rename project"),
                       showTraceback=False)
                event.Veto()
                return
            rename_project(self.selected_grassdb[0].data['name'],
                            self.selected_project[0].data['name'],
                            new_name)
            self._renameNode(self.selected_project[0], new_name)
            label = _(
                "Renaming project <{oldproject}> to <{newproject}> completed").format(
                oldproject=old_name, newproject=new_name)
            self.showNotification.emit(message=label)

    def Rename(self, old, new):
        """Rename layer"""
        string = old + ',' + new
        gisrc, env = gscript.create_environment(
            self.selected_grassdb[0].data['name'],
            self.selected_project[0].data['name'],
            self.selected_subproject[0].data['name'])
        label = _("Renaming map <{name}>...").format(name=string)
        self.showNotification.emit(message=label)
        if self.selected_layer[0].data['type'] == 'vector':
            renamed, cmd = self._runCommand('g.rename', vector=string, env=env)
        elif self.selected_layer[0].data['type'] == 'raster':
            renamed, cmd = self._runCommand('g.rename', raster=string, env=env)
        else:
            renamed, cmd = self._runCommand(
                'g.rename', raster3d=string, env=env)
        gscript.try_remove(gisrc)
        if renamed == 0:
            self.showNotification.emit(
                message=_("{cmd} -- completed").format(cmd=cmd))
            Debug.msg(1, "LAYER RENAMED TO: " + new)
            self._giface.grassdbChanged.emit(grassdb=self.selected_grassdb[0].data['name'],
                                             project=self.selected_project[0].data['name'],
                                             subproject=self.selected_subproject[0].data['name'],
                                             map=old,
                                             element=self.selected_layer[0].data['type'],
                                             newname=new,
                                             action='rename')

    def OnPasteMap(self, event):
        # copying between subprojects of one project
        if not self.copy_layer:
            if self.copy_mode:
                GMessage(_("No map selected for copying."), parent=self)
            else:
                GMessage(_("No map selected for moving."), parent=self)
            return

        for i in range(len(self.copy_layer)):
            gisrc, env = gscript.create_environment(self.selected_grassdb[0].data['name'],
                                                    self.selected_project[0].data['name'],
                                                    self.selected_subproject[0].data['name'])
            gisrc2, env2 = gscript.create_environment(self.copy_grassdb[i].data['name'],
                                                      self.copy_project[i].data['name'],
                                                      self.copy_subproject[i].data['name'])
            new_name = self.copy_layer[i].data['name']
            if self.selected_project[0] == self.copy_project[i]:
                # within one subproject
                if self.selected_subproject[0] == self.copy_subproject[i]:
                    # ignore when just moves map
                    if self.copy_mode is False:
                        return
                    new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].data['name']),
                                                   _('Select new name'),
                                                   self.copy_layer[i].data['name'], env=env,
                                                   subproject=self.selected_subproject[0].data['name'],
                                                   element=self.copy_layer[i].data['type'])
                    if not new_name:
                        return
                # within one project, different subprojects
                else:
                    if map_exists(new_name, element=self.copy_layer[i].data['type'], env=env,
                                  subproject=self.selected_subproject[0].data['name']):
                        new_name = self._getNewMapName(_('New name for <{n}>').format(n=self.copy_layer[i].data['name']),
                                                       _('Select new name'),
                                                       self.copy_layer[i].data['name'], env=env,
                                                       subproject=self.selected_subproject[0].data['name'],
                                                       element=self.copy_layer[i].data['type'])
                        if not new_name:
                            return

                string = self.copy_layer[i].data['name'] + '@' + self.copy_subproject[i].data['name'] + ',' + new_name
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
                    Debug.msg(1, "COPIED TO: " + new_name)
                    if self.copy_mode:
                        self.showNotification.emit(message=_("g.copy completed"))
                    else:
                        self.showNotification.emit(message=_("g.copy completed"))
                    self._giface.grassdbChanged.emit(grassdb=self.selected_grassdb[0].data['name'],
                                                     project=self.selected_project[0].data['name'],
                                                     subproject=self.selected_subproject[0].data['name'],
                                                     map=new_name,
                                                     element=node,
                                                     action='new')
                    # remove old
                    if not self.copy_mode:
                        self._removeMapAfterCopy(self.copy_layer[i], self.copy_subproject[i], env2)

                gscript.try_remove(gisrc)
                gscript.try_remove(gisrc2)
                # expand selected subproject
            else:
                if self.copy_layer[i].data['type'] == 'raster_3d':
                    GError(_("Reprojection is not implemented for 3D rasters"), parent=self)
                    return
                if map_exists(new_name, element=self.copy_layer[i].data['type'], env=env,
                              subproject=self.selected_subproject[0].data['name']):
                    new_name = self._getNewMapName(_('New name'), _('Select new name'),
                                                   self.copy_layer[i].data['name'], env=env,
                                                   subproject=self.selected_subproject[0].data['name'],
                                                   element=self.copy_layer[i].data['type'])
                    if not new_name:
                        continue
                callback = lambda gisrc2=gisrc2, gisrc=gisrc, cLayer=self.copy_layer[i], \
                                  cSubproject=self.copy_subproject[i], cMode=self.copy_mode, \
                                  sSubproject=self.selected_subproject[0], name=new_name: \
                                  self._onDoneReprojection(env2, gisrc2, gisrc, cLayer, cSubproject, cMode, sSubproject, name)
                dlg = CatalogReprojectionDialog(self, self._giface,
                                                self.copy_grassdb[i].data['name'],
                                                self.copy_project[i].data['name'],
                                                self.copy_subproject[i].data['name'],
                                                self.copy_layer[i].data['name'],
                                                env2,
                                                self.selected_grassdb[0].data['name'],
                                                self.selected_project[0].data['name'],
                                                self.selected_subproject[0].data['name'],
                                                new_name,
                                                self.copy_layer[i].data['type'],
                                                env, callback)
                if dlg.ShowModal() == wx.ID_CANCEL:
                    return
        self.ExpandNode(self.selected_subproject[0], recursive=True)
        self._resetCopyVariables()

    def _onDoneReprojection(self, iEnv, iGisrc, oGisrc, cLayer, cSubproject, cMode, sSubproject, name):
        self._giface.grassdbChanged.emit(grassdb=sSubproject.parent.parent.data['name'],
                                         project=sSubproject.parent.data['name'],
                                         subproject=sSubproject.data['name'],
                                         element=cLayer.data['type'],
                                         map=name,
                                         action='new')
        if not cMode:
            self._removeMapAfterCopy(cLayer, cSubproject, iEnv)
        gscript.try_remove(iGisrc)
        gscript.try_remove(oGisrc)
        self.ExpandNode(sSubproject, recursive=True)

    def _removeMapAfterCopy(self, cLayer, cSubproject, env):
        removed, cmd = self._runCommand('g.remove', type=cLayer.data['type'],
                                        name=cLayer.data['name'], flags='f', env=env)
        if removed == 0:
            Debug.msg(1, "LAYER " + cLayer.data['name'] + " DELETED")
            self.showNotification.emit(message=_("g.remove completed"))
            self._giface.grassdbChanged.emit(grassdb=cSubproject.parent.parent.data['name'],
                                             project=cSubproject.parent.data['name'],
                                             subproject=cSubproject.data['name'],
                                             map=cLayer.data['name'],
                                             element=cLayer.data['type'],
                                             action='delete')

    def InsertLayer(self, name, subproject_node, element_name):
        """Insert layer into model and refresh tree"""
        self._model.AppendNode(parent=subproject_node,
                               data=dict(type=element_name, name=name))
        self._model.SortChildren(subproject_node)
        self.RefreshNode(subproject_node, recursive=True)

    def InsertSubproject(self, name, project_node):
        """Insert new subproject into model and refresh tree.
        Assumes subproject is empty."""
        subproject_path = os.path.join(project_node.parent.data['name'],
                                   project_node.data['name'],
                                   name)
        subproject_node = self._model.AppendNode(parent=project_node,
                                             data=dict(type='subproject',
                                                       name=name,
                                                       current=False,
                                                       lock=is_subproject_locked(subproject_path),
                                                       is_different_owner=is_different_subproject_owner(subproject_path),
                                                       owner=get_subproject_owner(subproject_path)))
        self._model.SortChildren(project_node)
        self.RefreshNode(project_node, recursive=True)
        return subproject_node

    def InsertProject(self, name, grassdb_node):
        """Insert new project into model and refresh tree"""
        project_node = self._model.AppendNode(parent=grassdb_node,
                                               data=dict(type='project', name=name))
        # reload new project since it has a subproject
        self._reloadProjectNode(project_node)
        self._model.SortChildren(grassdb_node)
        self.RefreshNode(grassdb_node, recursive=True)
        return project_node

    def InsertGrassDb(self, name):
        """
        Insert new grass db into model, update user setting and refresh tree.
        Check if not already added.
        """
        grassdb_node = self._model.SearchNodes(name=name,
                                               type='grassdb')
        if not grassdb_node:
            grassdb_node = self._model.AppendNode(parent=self._model.root,
                                                  data=dict(type="grassdb", name=name))
            self._reloadGrassDBNode(grassdb_node)
            self.RefreshItems()

            # Update user's settings
            self.grassdatabases.append(name)
            self._saveGrassDBs()
        return grassdb_node

    def OnDeleteMap(self, event):
        """Delete layer or subproject"""
        names = [self.selected_layer[i].data['name'] + '@' + self.selected_subproject[i].data['name']
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
                    self.selected_project[i].data['name'],
                    self.selected_subproject[i].data['name'])
                removed, cmd = self._runCommand(
                        'g.remove', flags='f', type=self.selected_layer[i].data['type'],
                        name=self.selected_layer[i].data['name'], env=env)
                gscript.try_remove(gisrc)
                if removed == 0:
                    self._giface.grassdbChanged.emit(grassdb=self.selected_grassdb[i].data['name'],
                                                     project=self.selected_project[i].data['name'],
                                                     subproject=self.selected_subproject[i].data['name'],
                                                     element=self.selected_layer[i].data['type'],
                                                     map=self.selected_layer[i].data['name'],
                                                     action='delete')
                    Debug.msg(1, "LAYER " + self.selected_layer[i].data['name'] + " DELETED")

            self.UnselectAll()
            self.showNotification.emit(message=_("g.remove completed"))

    def OnDeleteSubproject(self, event):
        """
        Delete selected subproject or subprojects
        """
        subprojects = []
        changes = []
        for i in range(len(self.selected_subproject)):
            # Append to the list of tuples
            subprojects.append((
                self.selected_grassdb[i].data['name'],
                self.selected_project[i].data['name'],
                self.selected_subproject[i].data['name']
            ))
            changes.append(dict(grassdb=self.selected_grassdb[i].data['name'],
                                project=self.selected_project[i].data['name'],
                                subproject=self.selected_subproject[i].data['name'],
                                action='delete',
                                element='subproject'))
        if delete_subprojects_interactively(self, subprojects):
            for change in changes:
                self._giface.grassdbChanged.emit(**change)

    def OnDeleteProject(self, event):
        """
        Delete selected project or projects
        """
        projects = []
        changes = []
        for i in range(len(self.selected_project)):
            # Append to the list of tuples
            projects.append((
                self.selected_grassdb[i].data['name'],
                self.selected_project[i].data['name']
            ))
            changes.append(dict(grassdb=self.selected_grassdb[i].data['name'],
                                project=self.selected_project[i].data['name'],
                                action='delete',
                                element='project'))
        if delete_projects_interactively(self, projects):
            for change in changes:
                self._giface.grassdbChanged.emit(**change)

    def DownloadProject(self, grassdb_node):
        """
        Download new project interactively.
        """
        grassdatabase, project, subproject = (
            download_project_interactively(self, grassdb_node.data['name'])
        )
        if project:
            self._reloadGrassDBNode(grassdb_node)
            self.UpdateCurrentDbProjectSubprojectNode()
            self.RefreshItems()

    def OnDownloadProject(self, event):
        """
        Download project online
        """
        self.DownloadProject(self.selected_grassdb[0])

    def DeleteGrassDb(self, grassdb_node):
        """
        Delete grassdb from disk.
        """
        grassdb = grassdb_node.data['name']
        if (delete_grassdb_interactively(self, grassdb)):
            self.RemoveGrassDB(grassdb_node)

    def OnDeleteGrassDb(self, event):
        """
        Delete grassdb from disk.
        """
        self.DeleteGrassDb(self.selected_grassdb[0])

    def OnRemoveGrassDb(self, event):
        """
        Remove grassdb node from data catalogue.
        """
        self.RemoveGrassDB(self.selected_grassdb[0])

    def RemoveGrassDB(self, grassdb_node):
        """
        Remove grassdb node from tree
        and updates settings. Doesn't check if it's current db.
        """
        self.grassdatabases.remove(grassdb_node.data['name'])
        self._model.RemoveNode(grassdb_node)
        self.RefreshItems()

        # Update user's settings
        self._saveGrassDBs()

    def OnDisplayLayer(self, event):
        """
        Display layer in current graphics view
        """
        self.DisplayLayer()

    def DisplayLayer(self):
        """Display selected layer in current graphics view"""
        all_names = []
        names = {'raster': [], 'vector': [], 'raster_3d': []}
        for i in range(len(self.selected_layer)):
            name = self.selected_layer[i].data['name'] + '@' + self.selected_subproject[i].data['name']
            names[self.selected_layer[i].data['type']].append(name)
            all_names.append(name)
        #if self.selected_project[0].data['name'] == gisenv()['LOCATION_NAME'] and self.selected_subproject[0]:
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

        if self.selected_project and None in self.selected_subproject and \
           None in self.selected_layer:
            GMessage(_("Move or copy project isn't allowed"))
            event.Veto()
            return
        elif self.selected_project and self.selected_subproject and \
             None in self.selected_layer:
            GMessage(_("Move or copy subproject isn't allowed"))
            event.Veto()
            return

        if self.selected_layer and not (self._restricted and gisenv()[
                                        'LOCATION_NAME'] != self.selected_project[0].data['name']):
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
            if None not in self.selected_subproject:
                if self._restricted and gisenv()['MAPSET'] != self.selected_subproject[0].data['name']:
                    GMessage(_("To move or copy maps to other subprojects, unlock editing of other subprojects"),
                             parent=self)
                    event.Veto()
                    return

                event.Allow()
                Debug.msg(1, "DROP DONE")
                self.OnPasteMap(event)
            else:
                GMessage(_("To move or copy maps to other project, "
                           "please drag them to a subproject in the "
                           "destination project"),
                         parent=self)
                event.Veto()
                return

    def OnSwitchSubproject(self, event):
        """Switch to project and subproject"""
        genv = gisenv()
        grassdb = self.selected_grassdb[0].data['name']
        project = self.selected_project[0].data['name']
        subproject = self.selected_subproject[0].data['name']

        if can_switch_subproject_interactive(self, grassdb, project, subproject):
            # Switch to subproject in the same project
            if (grassdb == genv['GISDBASE'] and project == genv['LOCATION_NAME']):
                switch_subproject_interactively(self, self._giface, None, None, subproject)
            # Switch to subproject in the same grassdb
            elif grassdb == genv['GISDBASE']:
                switch_subproject_interactively(self, self._giface, None, project, subproject)
            # Switch to subproject in a different grassdb
            else:
                switch_subproject_interactively(self, self._giface, grassdb, project, subproject)

    def _updateAfterGrassdbChanged(self, action, element, grassdb, project, subproject=None,
                                   map=None, newname=None):
        """Update tree after grassdata changed"""
        if element == 'subproject':
            if action == 'new':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project)
                if node:
                    self.InsertSubproject(name=subproject, project_node=node)
            elif action == 'delete':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project)
                if node:
                    self._reloadProjectNode(node)
                    self.UpdateCurrentDbProjectSubprojectNode()
                    self.RefreshNode(node, recursive=True)
            elif action == 'rename':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project,
                                      subproject=subproject)
                if node:
                    self._renameNode(node, newname)
        elif element == 'project':
            if action == 'new':
                node = self.GetDbNode(grassdb=grassdb)
                if not node:
                    node = self.InsertGrassDb(name=grassdb)
                if node:
                    self.InsertProject(project, node)
            elif action == 'delete':
                node = self.GetDbNode(grassdb=grassdb)
                if node:
                    self._reloadGrassDBNode(node)
                    self.RefreshNode(node, recursive=True)
            elif action == 'rename':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project)
                if node:
                    self._renameNode(node, newname)
        elif element in ('raster', 'vector', 'raster_3d'):
            # when watchdog is used, it watches current subproject,
            # so we don't process any signals here,
            # instead the watchdog handler takes care of refreshing tree
            if (watchdog_used and grassdb == self.current_grassdb_node.data['name']
               and project == self.current_project_node.data['name']
               and subproject == self.current_subproject_node.data['name']):
                return
            if action == 'new':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project,
                                      subproject=subproject)
                if node:
                    if map:
                        # check if map already exists
                        if not self._model.SearchNodes(parent=node, name=map, type=element):
                            self.InsertLayer(name=map, subproject_node=node,
                                             element_name=element)
                    else:
                        # we know some maps created
                        self._reloadSubprojectNode(node)
                        self.RefreshNode(node)
            elif action == 'delete':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project,
                                      subproject=subproject,
                                      map=map,
                                      map_type=element)
                if node:
                    self._model.RemoveNode(node)
                    self.RefreshNode(node.parent, recursive=True)
            elif action == 'rename':
                node = self.GetDbNode(grassdb=grassdb,
                                      project=project,
                                      subproject=subproject,
                                      map=map,
                                      map_type=element)
                if node:
                    self._renameNode(node, newname)

    def _updateAfterSubprojectChanged(self):
        """Update tree after current subproject has changed"""
        self.UpdateCurrentDbProjectSubprojectNode()
        self.ExpandCurrentSubproject()
        self.RefreshItems()
        self.ScheduleWatchCurrentSubproject()

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
            cmd.append('map=%s@%s' % (self.selected_layer[i].data['name'], self.selected_subproject[i].data['name']))

            gisrc, env = gscript.create_environment(
                self.selected_grassdb[i].data['name'],
                self.selected_project[i].data['name'],
                self.selected_subproject[i].data['name'])
            # print output to command log area
            # temp gisrc file must be deleted onDone
            self._giface.RunCmd(cmd, env=env, onDone=done, userData=gisrc)

    def OnCopyName(self, event):
        """Copy layer name to clipboard"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            text = []
            for i in range(len(self.selected_layer)):
                text.append('%s@%s' % (self.selected_layer[i].data['name'], self.selected_subproject[i].data['name']))
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
        self.UpdateCurrentDbProjectSubprojectNode()
        self.RefreshItems()
        self.ExpandCurrentSubproject()

    def _getNewMapName(self, message, title, value, element, subproject, env):
        """Dialog for simple text entry"""
        dlg = NameEntryDialog(parent=self, message=message, caption=title,
                              element=element, env=env, subproject=subproject)
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
            currentSubproject = currentProject = currentGrassDb = True
            for i in range(len(self.selected_grassdb)):
                if self.selected_grassdb[i].data['name'] != genv['GISDBASE']:
                    currentGrassDb = False
                    currentProject = False
                    currentSubproject = False
                    break
            if currentProject and self.selected_project[0]:
                for i in range(len(self.selected_project)):
                    if self.selected_project[i].data['name'] != genv['LOCATION_NAME']:
                        currentProject = False
                        currentSubproject = False
                        break
            if currentSubproject and self.selected_subproject[0]:
                for i in range(len(self.selected_subproject)):
                    if self.selected_subproject[i].data['name'] != genv['MAPSET']:
                        currentSubproject = False
                        break
            return currentGrassDb, currentProject, currentSubproject
        else:
            return True, True, True

    def _popupMenuLayer(self):
        """Create popup menu for layers"""
        menu = Menu()
        genv = gisenv()
        currentGrassDb, currentProject, currentSubproject = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Cut"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMoveMap, item)
        if not currentSubproject:
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
        if not(currentSubproject and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteMap, item)
        item.Enable(currentSubproject)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameMap, item)
        item.Enable(currentSubproject and len(self.selected_layer) == 1)

        menu.AppendSeparator()

        if not isinstance(self._giface, StandaloneGrassInterface):
            if all([each.data['name'] == genv['LOCATION_NAME'] for each in self.selected_project]):
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

    def _popupMenuSubproject(self):
        """Create popup menu for subprojects"""
        menu = Menu()
        genv = gisenv()
        currentGrassDb, currentProject, currentSubproject = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        if not(currentSubproject and self.copy_layer):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Switch subproject"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnSwitchSubproject, item)
        if (
            self.selected_grassdb[0].data['name'] == genv['GISDBASE']
            and self.selected_project[0].data['name'] == genv['LOCATION_NAME']
            and self.selected_subproject[0].data['name'] == genv['MAPSET']
        ):
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete subproject"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteSubproject, item)
        if self._restricted:
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename subproject"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameSubproject, item)
        if self._restricted:
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuProject(self):
        """Create popup menu for projects"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Create subproject"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCreateSubproject, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete project"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteProject, item)
        if self._restricted:
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Rename project"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameProject, item)
        if self._restricted:
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuGrassDb(self):
        """Create popup menu for grass db"""
        menu = Menu()
        genv = gisenv()
        currentGrassDb, currentProject, currentSubproject = self._isCurrent(genv)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Create new project"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnCreateProject, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Download sample project"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDownloadProject, item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Remove GRASS database from data catalog"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRemoveGrassDb, item)
        if self.selected_grassdb[0].data['name'] == genv['GISDBASE']:
            item.Enable(False)

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete GRASS database from disk"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteGrassDb, item)
        if self._restricted:
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuElement(self):
        """Create popup menu for elements"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("&Paste"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnPasteMap, item)
        genv = gisenv()
        currentGrassDb, currentProject, currentSubproject = self._isCurrent(genv)
        if not(currentSubproject and self.copy_layer):
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuMultipleProjects(self):
        """Create popup menu for multiple selected projects"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete projects"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteProject, item)
        if self._restricted:
            item.Enable(False)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuMultipleSubprojects(self):
        """Create popup menu for multiple selected subprojects"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete subprojects"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteSubproject, item)
        if self._restricted:
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
