"""
@package startup.guiutils

@brief General GUI-dependent utilities for GUI startup of GRASS GIS

(C) 2018 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>
@author Linda Kladivova <l.kladivova@seznam.cz>

This is for code which depend on something from GUI (wx or wxGUI).
"""


import os
import sys
import wx

from grass.grassdb.checks import (
    is_subproject_locked,
    get_subproject_lock_info,
    is_subproject_name_valid,
    is_project_name_valid,
    get_subproject_name_invalid_reason,
    get_project_name_invalid_reason,
    get_reason_subproject_not_removable,
    get_reasons_subprojects_not_removable,
    get_reasons_project_not_removable,
    get_reasons_projects_not_removable,
    get_reasons_grassdb_not_removable
)

from grass.grassdb.create import create_subproject, get_default_subproject_name
from grass.grassdb.manage import (
    delete_subproject,
    delete_project,
    delete_grassdb,
    rename_subproject,
    rename_project,
)

from core import globalvar
from core.gcmd import GError, GMessage, DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from project_wizard.dialogs import RegionDef
from gui_core.widgets import GenericValidator


def SetSessionSubproject(database, project, subproject):
    """Sets database, project and subproject for the current session"""
    RunCommand("g.gisenv", set="GISDBASE=%s" % database)
    RunCommand("g.gisenv", set="LOCATION_NAME=%s" % project)
    RunCommand("g.gisenv", set="MAPSET=%s" % subproject)


class SubprojectDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None, message=None, caption=None,
                 database=None, project=None):
        self.database = database
        self.project = project

        validator = GenericValidator(self._isSubprojectNameValid,
                                     self._showSubprojectNameInvalidReason)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _showSubprojectNameInvalidReason(self, ctrl):
        message = get_subproject_name_invalid_reason(self.database,
                                                 self.project,
                                                 ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Invalid subproject name"))

    def _isSubprojectNameValid(self, text):
        """Check whether user's input project is valid or not."""
        return is_subproject_name_valid(self.database, self.project, text)


class ProjectDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None, message=None, caption=None,
                 database=None):
        self.database = database

        validator = GenericValidator(self._isProjectNameValid,
                                     self._showProjectNameInvalidReason)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _showProjectNameInvalidReason(self, ctrl):
        message = get_project_name_invalid_reason(self.database,
                                                   ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Invalid project name"))

    def _isProjectNameValid(self, text):
        """Check whether user's input project is valid or not."""
        return is_project_name_valid(self.database, text)


# TODO: similar to (but not the same as) read_gisrc function in grass.py
def read_gisrc():
    """Read variables from a current GISRC file

    Returns a dictionary representation of the file content.
    """
    grassrc = {}

    gisrc = os.getenv("GISRC")

    if gisrc and os.path.isfile(gisrc):
        try:
            rc = open(gisrc, "r")
            for line in rc.readlines():
                try:
                    key, val = line.split(":", 1)
                except ValueError as e:
                    sys.stderr.write(
                        _('Invalid line in GISRC file (%s):%s\n' % (e, line)))
                grassrc[key.strip()] = DecodeString(val.strip())
        finally:
            rc.close()

    return grassrc


def GetVersion():
    """Gets version and revision

    Returns tuple `(version, revision)`. For standard releases revision
    is an empty string.

    Revision string is currently wrapped in parentheses with added
    leading space. This is an implementation detail and legacy and may
    change anytime.
    """
    versionFile = open(os.path.join(globalvar.ETCDIR, "VERSIONNUMBER"))
    versionLine = versionFile.readline().rstrip('\n')
    versionFile.close()
    try:
        grassVersion, grassRevision = versionLine.split(' ', 1)
        if grassVersion.endswith('dev'):
            grassRevisionStr = ' (%s)' % grassRevision
        else:
            grassRevisionStr = ''
    except ValueError:
        grassVersion = versionLine
        grassRevisionStr = ''
    return (grassVersion, grassRevisionStr)


def create_subproject_interactively(guiparent, grassdb, project):
    """
    Create new subproject
    """
    dlg = SubprojectDialog(
        parent=guiparent,
        default=get_default_subproject_name(),
        message=_("Name for the new subproject:"),
        caption=_("Create new subproject"),
        database=grassdb,
        project=project,
    )

    subproject = None
    if dlg.ShowModal() == wx.ID_OK:
        subproject = dlg.GetValue()
        try:
            create_subproject(grassdb, project, subproject)
        except OSError as err:
            subproject = None
            GError(
                parent=guiparent,
                message=_("Unable to create new subproject: {}").format(err),
                showTraceback=False,
            )
    dlg.Destroy()
    return subproject


def create_project_interactively(guiparent, grassdb):
    """
    Create new project using Project Wizard.

    Returns tuple (database, project, subproject) where subproject is "PERMANENT"
    by default or another subproject a user created and may want to switch to.
    """
    from project_wizard.wizard import ProjectWizard

    gWizard = ProjectWizard(parent=guiparent,
                             grassdatabase=grassdb)

    if gWizard.project is None:
        gWizard_output = (None, None, None)
        # Returns Nones after Cancel
        return gWizard_output

    if gWizard.georeffile:
        message = _(
            "Do you want to import {}"
            "to the newly created project?"
        ).format(gWizard.georeffile)
        dlg = wx.MessageDialog(parent=guiparent,
                               message=message,
                               caption=_("Import data?"),
                               style=wx.YES_NO | wx.YES_DEFAULT |
                               wx.ICON_QUESTION)
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_YES:
            import_file(guiparent, gWizard.georeffile)
        dlg.Destroy()

    if gWizard.default_region:
        defineRegion = RegionDef(guiparent, project=gWizard.project)
        defineRegion.CenterOnParent()
        defineRegion.ShowModal()
        defineRegion.Destroy()

    if gWizard.user_subproject:
        subproject = create_subproject_interactively(guiparent,
                                             gWizard.grassdatabase,
                                             gWizard.project)
        # Returns database and project created by user
        # and a subproject user may want to switch to
        gWizard_output = (gWizard.grassdatabase, gWizard.project,
                          subproject)
    else:
        # Returns PERMANENT subproject when user subproject not defined
        gWizard_output = (gWizard.grassdatabase, gWizard.project,
                          "PERMANENT")
    return gWizard_output


def rename_subproject_interactively(guiparent, grassdb, project, subproject):
    """Rename subproject with user interaction.

    Exceptions during renaming are handled in get_reason_subproject_not_removable
    function.

    Returns newsubproject if there was a change or None if the subproject cannot be
    renamed (see reasons given by get_reason_subproject_not_removable
    function) or if another error was encountered.
    """
    newsubproject = None

    # Check selected subproject
    message = get_reason_subproject_not_removable(grassdb, project, subproject,
                                              check_permanent=True)
    if message:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot rename subproject <{subproject}> for the following reason:\n\n"
                "{reason}\n\n"
                "No subproject will be renamed."
            ).format(subproject=subproject, reason=message),
            caption=_("Unable to rename selected subproject"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return newsubproject

    # Display question dialog
    dlg = SubprojectDialog(
        parent=guiparent,
        default=subproject,
        message=_("Current name: {}\n\nEnter new name:").format(subproject),
        caption=_("Rename selected subproject"),
        database=grassdb,
        project=project,
    )
    if dlg.ShowModal() == wx.ID_OK:
        newsubproject = dlg.GetValue()
        try:
            rename_subproject(grassdb, project, subproject, newsubproject)
        except OSError as err:
            newsubproject = None
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename subproject.\n\n{}").format(err),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return newsubproject


def rename_project_interactively(guiparent, grassdb, project):
    """Rename project with user interaction.

    Exceptions during renaming are handled in get_reasons_project_not_removable
    function.

    Returns newproject if there was a change or None if the project cannot be
    renamed (see reasons given by get_reasons_project_not_removable
    function) or if another error was encountered.
    """
    newproject = None

    # Check selected project
    messages = get_reasons_project_not_removable(grassdb, project)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot rename project <{project}> for the following reasons:\n\n"
                "{reasons}\n\n"
                "No project will be renamed."
            ).format(project=project, reasons="\n".join(messages)),
            caption=_("Unable to rename selected project"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return newproject

    # Display question dialog
    dlg = ProjectDialog(
        parent=guiparent,
        default=project,
        message=_("Current name: {}\n\nEnter new name:").format(project),
        caption=_("Rename selected project"),
        database=grassdb,
    )
    if dlg.ShowModal() == wx.ID_OK:
        newproject = dlg.GetValue()
        try:
            rename_project(grassdb, project, newproject)
        except OSError as err:
            newproject = None
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename project.\n\n{}").format(err),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return newproject


def download_project_interactively(guiparent, grassdb):
    """
    Download new project using Project Wizard.

    Returns tuple (database, project, subproject) where subproject is "PERMANENT"
    by default or in future it could be the subproject the user may want to
    switch to.
    """
    from startup.locdownload import ProjectDownloadDialog

    result = (None, None, None)
    loc_download = ProjectDownloadDialog(parent=guiparent,
                                          database=grassdb)
    loc_download.Centre()
    loc_download.ShowModal()

    if loc_download.GetProject() is not None:
        # Returns database and project created by user
        # and a subproject user may want to switch to
        result = (grassdb, loc_download.GetProject(), "PERMANENT")
    loc_download.Destroy()
    return result


def delete_subproject_interactively(guiparent, grassdb, project, subproject):
    """Delete one subproject with user interaction.

    This is currently just a convenience wrapper for delete_subprojects_interactively().
    """
    subprojects = [(grassdb, project, subproject)]
    return delete_subprojects_interactively(guiparent, subprojects)


def delete_subprojects_interactively(guiparent, subprojects):
    """Delete multiple subprojects with user interaction.

    Parameter *subprojects* is a list of tuples (database, project, subproject).

    Exceptions during deletation are handled in get_reasons_subprojects_not_removable
    function.

    Returns True if there was a change, i.e., all subprojects were successfuly
    deleted or at least one subproject was deleted.
    Returns False if one or more subprojects cannot be deleted (see reasons given
    by get_reasons_subprojects_not_removable function) or if an error was
    encountered when deleting the first subproject in the list.
    """
    deletes = []
    modified = False

    # Check selected subprojects
    messages = get_reasons_subprojects_not_removable(subprojects, check_permanent=True)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot delete one or more subprojects for the following reasons:\n\n"
                "{reasons}\n\n"
                "No subprojects will be deleted."
            ).format(reasons="\n".join(messages)),
            caption=_("Unable to delete selected subprojects"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return modified

    # No error occurs, create list of subprojects for deleting
    for grassdb, project, subproject in subprojects:
        subproject_path = os.path.join(grassdb, project, subproject)
        deletes.append(subproject_path)

    # Display question dialog
    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting"
            " one or more of the following subprojects?\n\n"
            "{deletes}\n\n"
            "All maps included in these subprojects will be permanently deleted!"
        ).format(deletes="\n".join(deletes)),
        caption=_("Delete selected subprojects"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )
    if dlg.ShowModal() == wx.ID_YES:
        try:
            for grassdb, project, subproject in subprojects:
                delete_subproject(grassdb, project, subproject)
                modified = True
            dlg.Destroy()
            return modified
        except OSError as error:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error when deleting subprojects"),
                message=_(
                    "The following error occured when deleting subproject <{path}>:"
                    "\n\n{error}\n\n"
                    "Deleting of subprojects was interrupted."
                ).format(
                    path=os.path.join(grassdb, project, subproject),
                    error=error,
                ),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return modified


def delete_project_interactively(guiparent, grassdb, project):
    """Delete one project with user interaction.

    This is currently just a convenience wrapper for delete_projects_interactively().
    """
    projects = [(grassdb, project)]
    return delete_projects_interactively(guiparent, projects)


def delete_projects_interactively(guiparent, projects):
    """Delete multiple projects with user interaction.

    Parameter *projects* is a list of tuples (database, project).

    Exceptions during deletation are handled in get_reasons_projects_not_removable
    function.

    Returns True if there was a change, i.e., all projects were successfuly
    deleted or at least one project was deleted.
    Returns False if one or more projects cannot be deleted (see reasons given
    by get_reasons_projects_not_removable function) or if an error was
    encountered when deleting the first project in the list.
    """
    deletes = []
    modified = False

    # Check selected projects
    messages = get_reasons_projects_not_removable(projects)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot delete one or more projects for the following reasons:\n\n"
                "{reasons}\n\n"
                "No projects will be deleted."
            ).format(reasons="\n".join(messages)),
            caption=_("Unable to delete selected projects"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return modified

    # No error occurs, create list of projects for deleting
    for grassdb, project in projects:
        project_path = os.path.join(grassdb, project)
        deletes.append(project_path)

    # Display question dialog
    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting"
            " one or more of the following projects?\n\n"
            "{deletes}\n\n"
            "All subprojects included in these projects will be permanently deleted!"
        ).format(deletes="\n".join(deletes)),
        caption=_("Delete selected projects"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )
    if dlg.ShowModal() == wx.ID_YES:
        try:
            for grassdb, project in projects:
                delete_project(grassdb, project)
                modified = True
            dlg.Destroy()
            return modified
        except OSError as error:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error when deleting projects"),
                message=_(
                    "The following error occured when deleting project <{path}>:"
                    "\n\n{error}\n\n"
                    "Deleting of projects was interrupted."
                ).format(
                    path=os.path.join(grassdb, project),
                    error=error,
                ),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return modified


def delete_grassdb_interactively(guiparent, grassdb):
    """
    Delete grass database if could be deleted.

    If current grass database found, desired operation cannot be performed.

    Exceptions during deleting are handled in this function.

    Returns True if grass database is deleted from the disk. Returns None if
    cannot be deleted (see above the possible reasons).
    """

    deleted = False

    # Check selected grassdb
    messages = get_reasons_grassdb_not_removable(grassdb)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot delete GRASS database from disk for the following reason:\n\n"
                "{reasons}\n\n"
                "GRASS database will not be deleted."
            ).format(reasons="\n".join(messages)),
            caption=_("Unable to delete selected GRASS database"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
    else:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Do you want to delete"
                " the following GRASS database from disk?\n\n"
                "{grassdb}\n\n"
                "The directory will be permanently deleted!"
            ).format(grassdb=grassdb),
            caption=_("Delete selected GRASS database"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
        )
        if dlg.ShowModal() == wx.ID_YES:
            try:
                delete_grassdb(grassdb)
                deleted = True
                dlg.Destroy()
                return deleted
            except OSError as error:
                wx.MessageBox(
                    parent=guiparent,
                    caption=_("Error when deleting GRASS database"),
                    message=_(
                        "The following error occured when deleting database <{path}>:"
                        "\n\n{error}\n\n"
                        "Deleting of GRASS database was interrupted."
                    ).format(
                        path=grassdb,
                        error=error,
                    ),
                    style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
                )
    dlg.Destroy()
    return deleted


def can_switch_subproject_interactive(guiparent, grassdb, project, subproject):
    """
    Checks if subproject is locked and offers to remove the lock file.

    Returns True if user wants to switch to the selected subproject in spite of
    removing lock. Returns False if a user wants to stay in the current
    subproject or if an error was encountered.
    """
    can_switch = True
    subproject_path = os.path.join(grassdb, project, subproject)

    if is_subproject_locked(subproject_path):
        info = get_subproject_lock_info(subproject_path)
        user = info['owner'] if info['owner'] else _('unknown')
        lockpath = info['lockpath']
        timestamp = info['timestamp']

        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_("User {user} is already running GRASS in selected subproject "
                      "<{subproject}>\n (file {lockpath} created {timestamp} "
                      "found).\n\n"
                      "Concurrent use not allowed.\n\n"
                      "Do you want to stay in the current subproject or remove "
                      ".gislock and switch to selected subproject?"
                      ).format(user=user,
                               subproject=subproject,
                               lockpath=lockpath,
                               timestamp=timestamp),
            caption=_("Subproject is in use"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
        )
        dlg.SetYesNoLabels("S&witch to selected subproject",
                           "S&tay in current subproject")
        if dlg.ShowModal() == wx.ID_YES:
            # Remove lockfile
            try:
                os.remove(lockpath)
            except IOError as e:
                wx.MessageBox(
                    parent=guiparent,
                    caption=_("Error when removing lock file"),
                    message=_("Unable to remove {lockpath}.\n\n Details: {error}."
                              ).format(lockpath=lockpath,
                                       error=e),
                    style=wx.OK | wx.ICON_ERROR | wx.CENTRE
                )
                can_switch = False
        else:
            can_switch = False
        dlg.Destroy()
    return can_switch


def import_file(guiparent, filePath):
    """Tries to import file as vector or raster.

    If successfull sets default region from imported map.
    """
    RunCommand('db.connect', flags='c')
    mapName = os.path.splitext(os.path.basename(filePath))[0]
    vectors = RunCommand('v.in.ogr', input=filePath, flags='l',
                         read=True)

    wx.BeginBusyCursor()
    wx.GetApp().Yield()
    if vectors:
        # vector detected
        returncode, error = RunCommand(
            'v.in.ogr', input=filePath, output=mapName, flags='e',
            getErrorMsg=True)
    else:
        returncode, error = RunCommand(
            'r.in.gdal', input=filePath, output=mapName, flags='e',
            getErrorMsg=True)
    wx.EndBusyCursor()

    if returncode != 0:
        GError(
            parent=guiparent,
            message=_(
                "Import of <%(name)s> failed.\n"
                "Reason: %(msg)s") % ({
                    'name': filePath,
                    'msg': error}))
    else:
        GMessage(
            message=_(
                "Data file <%(name)s> imported successfully. "
                "The project's default region was set from "
                "this imported map.") % {
                'name': filePath},
            parent=guiparent)


def switch_subproject_interactively(guiparent, giface, dbase, project, subproject):
    """Switch current subproject. Emits giface.currentSubprojectChanged signal."""
    if dbase:
        if RunCommand('g.subproject', parent=guiparent,
                      project=project,
                      subproject=subproject,
                      dbase=dbase) == 0:
            GMessage(parent=guiparent,
                     message=_("Current GRASS database is <%(dbase)s>.\n"
                               "Current project is <%(loc)s>.\n"
                               "Current subproject is <%(subproject)s>."
                               ) %
                     {'dbase': dbase, 'loc': project, 'subproject': subproject})
            giface.currentSubprojectChanged.emit(dbase=dbase,
                                             project=project,
                                             subproject=subproject)
    elif project:
        if RunCommand('g.subproject', parent=guiparent,
                      project=project,
                      subproject=subproject) == 0:
            GMessage(parent=guiparent,
                     message=_("Current project is <%(loc)s>.\n"
                               "Current subproject is <%(subproject)s>.") %
                     {'loc': project, 'subproject': subproject})
            giface.currentSubprojectChanged.emit(dbase=None,
                                             project=project,
                                             subproject=subproject)
    else:
        if RunCommand('g.subproject',
                      parent=guiparent,
                      subproject=subproject) == 0:
            GMessage(parent=guiparent,
                     message=_("Current subproject is <%s>.") % subproject)
            giface.currentSubprojectChanged.emit(dbase=None, project=None, subproject=subproject)
