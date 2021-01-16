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
    is_mapset_locked,
    get_mapset_lock_info,
    is_mapset_name_valid,
    is_location_name_valid,
    get_mapset_name_invalid_reason,
    get_location_name_invalid_reason,
    get_reason_mapset_not_removable,
    get_reasons_mapsets_not_removable,
    get_reasons_location_not_removable,
    get_reasons_locations_not_removable,
    get_reasons_grassdb_not_removable
)

from grass.grassdb.create import create_mapset, get_default_mapset_name
from grass.grassdb.manage import (
    delete_mapset,
    delete_location,
    delete_grassdb,
    rename_mapset,
    rename_location,
)

from core import globalvar
from core.gcmd import GError, GMessage, DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from location_wizard.dialogs import RegionDef
from gui_core.widgets import GenericValidator


def SetSessionMapset(database, location, mapset):
    """Sets database, location and mapset for the current session"""
    RunCommand("g.gisenv", set="GISDBASE=%s" % database)
    RunCommand("g.gisenv", set="LOCATION_NAME=%s" % location)
    RunCommand("g.gisenv", set="MAPSET=%s" % mapset)


class MapsetDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None, message=None, caption=None,
                 database=None, location=None):
        self.database = database
        self.location = location

        validator = GenericValidator(self._isMapsetNameValid,
                                     self._showMapsetNameInvalidReason)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _showMapsetNameInvalidReason(self, ctrl):
        message = get_mapset_name_invalid_reason(self.database,
                                                 self.location,
                                                 ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Invalid mapset name"))

    def _isMapsetNameValid(self, text):
        """Check whether user's input location is valid or not."""
        return is_mapset_name_valid(self.database, self.location, text)


class LocationDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None, message=None, caption=None,
                 database=None):
        self.database = database

        validator = GenericValidator(self._isLocationNameValid,
                                     self._showLocationNameInvalidReason)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _showLocationNameInvalidReason(self, ctrl):
        message = get_location_name_invalid_reason(self.database,
                                                   ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Invalid location name"))

    def _isLocationNameValid(self, text):
        """Check whether user's input location is valid or not."""
        return is_location_name_valid(self.database, text)


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


def create_mapset_interactively(guiparent, grassdb, location):
    """
    Create new mapset
    """
    dlg = MapsetDialog(
        parent=guiparent,
        default=get_default_mapset_name(),
        message=_("Name for the new mapset:"),
        caption=_("Create new mapset"),
        database=grassdb,
        location=location,
    )

    mapset = None
    if dlg.ShowModal() == wx.ID_OK:
        mapset = dlg.GetValue()
        try:
            create_mapset(grassdb, location, mapset)
        except OSError as err:
            mapset = None
            GError(
                parent=guiparent,
                message=_("Unable to create new mapset: {}").format(err),
                showTraceback=False,
            )
    dlg.Destroy()
    return mapset


def create_location_interactively(guiparent, grassdb):
    """
    Create new location using Location Wizard.

    Returns tuple (database, location, mapset) where mapset is "PERMANENT"
    by default or another mapset a user created and may want to switch to.
    """
    from location_wizard.wizard import LocationWizard

    gWizard = LocationWizard(parent=guiparent,
                             grassdatabase=grassdb)

    if gWizard.location is None:
        gWizard_output = (None, None, None)
        # Returns Nones after Cancel
        return gWizard_output

    if gWizard.georeffile:
        message = _(
            "Do you want to import {}"
            "to the newly created location?"
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
        defineRegion = RegionDef(guiparent, location=gWizard.location)
        defineRegion.CenterOnParent()
        defineRegion.ShowModal()
        defineRegion.Destroy()

    if gWizard.user_mapset:
        mapset = create_mapset_interactively(guiparent,
                                             gWizard.grassdatabase,
                                             gWizard.location)
        # Returns database and location created by user
        # and a mapset user may want to switch to
        gWizard_output = (gWizard.grassdatabase, gWizard.location,
                          mapset)
    else:
        # Returns PERMANENT mapset when user mapset not defined
        gWizard_output = (gWizard.grassdatabase, gWizard.location,
                          "PERMANENT")
    return gWizard_output


def rename_mapset_interactively(guiparent, grassdb, location, mapset):
    """Rename mapset with user interaction.

    Exceptions during renaming are handled in get_reason_mapset_not_removable
    function.

    Returns newmapset if there was a change or None if the mapset cannot be
    renamed (see reasons given by get_reason_mapset_not_removable
    function) or if another error was encountered.
    """
    newmapset = None

    # Check selected mapset
    message = get_reason_mapset_not_removable(grassdb, location, mapset,
                                              check_permanent=True)
    if message:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot rename mapset <{mapset}> for the following reason:\n\n"
                "{reason}\n\n"
                "No mapset will be renamed."
            ).format(mapset=mapset, reason=message),
            caption=_("Unable to rename selected mapset"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return newmapset

    # Display question dialog
    dlg = MapsetDialog(
        parent=guiparent,
        default=mapset,
        message=_("Current name: {}\n\nEnter new name:").format(mapset),
        caption=_("Rename selected mapset"),
        database=grassdb,
        location=location,
    )
    if dlg.ShowModal() == wx.ID_OK:
        newmapset = dlg.GetValue()
        try:
            rename_mapset(grassdb, location, mapset, newmapset)
        except OSError as err:
            newmapset = None
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename mapset.\n\n{}").format(err),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return newmapset


def rename_location_interactively(guiparent, grassdb, location):
    """Rename location with user interaction.

    Exceptions during renaming are handled in get_reasons_location_not_removable
    function.

    Returns newlocation if there was a change or None if the location cannot be
    renamed (see reasons given by get_reasons_location_not_removable
    function) or if another error was encountered.
    """
    newlocation = None

    # Check selected location
    messages = get_reasons_location_not_removable(grassdb, location)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot rename location <{location}> for the following reasons:\n\n"
                "{reasons}\n\n"
                "No location will be renamed."
            ).format(location=location, reasons="\n".join(messages)),
            caption=_("Unable to rename selected location"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return newlocation

    # Display question dialog
    dlg = LocationDialog(
        parent=guiparent,
        default=location,
        message=_("Current name: {}\n\nEnter new name:").format(location),
        caption=_("Rename selected location"),
        database=grassdb,
    )
    if dlg.ShowModal() == wx.ID_OK:
        newlocation = dlg.GetValue()
        try:
            rename_location(grassdb, location, newlocation)
        except OSError as err:
            newlocation = None
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename location.\n\n{}").format(err),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return newlocation


def download_location_interactively(guiparent, grassdb):
    """
    Download new location using Location Wizard.

    Returns tuple (database, location, mapset) where mapset is "PERMANENT"
    by default or in future it could be the mapset the user may want to
    switch to.
    """
    from startup.locdownload import LocationDownloadDialog

    result = (None, None, None)
    loc_download = LocationDownloadDialog(parent=guiparent,
                                          database=grassdb)
    loc_download.Centre()
    loc_download.ShowModal()

    if loc_download.GetLocation() is not None:
        # Returns database and location created by user
        # and a mapset user may want to switch to
        result = (grassdb, loc_download.GetLocation(), "PERMANENT")
    loc_download.Destroy()
    return result


def delete_mapset_interactively(guiparent, grassdb, location, mapset):
    """Delete one mapset with user interaction.

    This is currently just a convenience wrapper for delete_mapsets_interactively().
    """
    mapsets = [(grassdb, location, mapset)]
    return delete_mapsets_interactively(guiparent, mapsets)


def delete_mapsets_interactively(guiparent, mapsets):
    """Delete multiple mapsets with user interaction.

    Parameter *mapsets* is a list of tuples (database, location, mapset).

    Exceptions during deletation are handled in get_reasons_mapsets_not_removable
    function.

    Returns True if there was a change, i.e., all mapsets were successfuly
    deleted or at least one mapset was deleted.
    Returns False if one or more mapsets cannot be deleted (see reasons given
    by get_reasons_mapsets_not_removable function) or if an error was
    encountered when deleting the first mapset in the list.
    """
    deletes = []
    modified = False

    # Check selected mapsets
    messages = get_reasons_mapsets_not_removable(mapsets, check_permanent=True)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot delete one or more mapsets for the following reasons:\n\n"
                "{reasons}\n\n"
                "No mapsets will be deleted."
            ).format(reasons="\n".join(messages)),
            caption=_("Unable to delete selected mapsets"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return modified

    # No error occurs, create list of mapsets for deleting
    for grassdb, location, mapset in mapsets:
        mapset_path = os.path.join(grassdb, location, mapset)
        deletes.append(mapset_path)

    # Display question dialog
    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting"
            " one or more of the following mapsets?\n\n"
            "{deletes}\n\n"
            "All maps included in these mapsets will be permanently deleted!"
        ).format(deletes="\n".join(deletes)),
        caption=_("Delete selected mapsets"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )
    if dlg.ShowModal() == wx.ID_YES:
        try:
            for grassdb, location, mapset in mapsets:
                delete_mapset(grassdb, location, mapset)
                modified = True
            dlg.Destroy()
            return modified
        except OSError as error:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error when deleting mapsets"),
                message=_(
                    "The following error occured when deleting mapset <{path}>:"
                    "\n\n{error}\n\n"
                    "Deleting of mapsets was interrupted."
                ).format(
                    path=os.path.join(grassdb, location, mapset),
                    error=error,
                ),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    dlg.Destroy()
    return modified


def delete_location_interactively(guiparent, grassdb, location):
    """Delete one location with user interaction.

    This is currently just a convenience wrapper for delete_locations_interactively().
    """
    locations = [(grassdb, location)]
    return delete_locations_interactively(guiparent, locations)


def delete_locations_interactively(guiparent, locations):
    """Delete multiple locations with user interaction.

    Parameter *locations* is a list of tuples (database, location).

    Exceptions during deletation are handled in get_reasons_locations_not_removable
    function.

    Returns True if there was a change, i.e., all locations were successfuly
    deleted or at least one location was deleted.
    Returns False if one or more locations cannot be deleted (see reasons given
    by get_reasons_locations_not_removable function) or if an error was
    encountered when deleting the first location in the list.
    """
    deletes = []
    modified = False

    # Check selected locations
    messages = get_reasons_locations_not_removable(locations)
    if messages:
        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_(
                "Cannot delete one or more locations for the following reasons:\n\n"
                "{reasons}\n\n"
                "No locations will be deleted."
            ).format(reasons="\n".join(messages)),
            caption=_("Unable to delete selected locations"),
            style=wx.OK | wx.ICON_WARNING
        )
        dlg.ShowModal()
        dlg.Destroy()
        return modified

    # No error occurs, create list of locations for deleting
    for grassdb, location in locations:
        location_path = os.path.join(grassdb, location)
        deletes.append(location_path)

    # Display question dialog
    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting"
            " one or more of the following locations?\n\n"
            "{deletes}\n\n"
            "All mapsets included in these locations will be permanently deleted!"
        ).format(deletes="\n".join(deletes)),
        caption=_("Delete selected locations"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )
    if dlg.ShowModal() == wx.ID_YES:
        try:
            for grassdb, location in locations:
                delete_location(grassdb, location)
                modified = True
            dlg.Destroy()
            return modified
        except OSError as error:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error when deleting locations"),
                message=_(
                    "The following error occured when deleting location <{path}>:"
                    "\n\n{error}\n\n"
                    "Deleting of locations was interrupted."
                ).format(
                    path=os.path.join(grassdb, location),
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


def can_switch_mapset_interactive(guiparent, grassdb, location, mapset):
    """
    Checks if mapset is locked and offers to remove the lock file.

    Returns True if user wants to switch to the selected mapset in spite of
    removing lock. Returns False if a user wants to stay in the current
    mapset or if an error was encountered.
    """
    can_switch = True
    mapset_path = os.path.join(grassdb, location, mapset)

    if is_mapset_locked(mapset_path):
        info = get_mapset_lock_info(mapset_path)
        user = info['owner'] if info['owner'] else _('unknown')
        lockpath = info['lockpath']
        timestamp = info['timestamp']

        dlg = wx.MessageDialog(
            parent=guiparent,
            message=_("User {user} is already running GRASS in selected mapset "
                      "<{mapset}>\n (file {lockpath} created {timestamp} "
                      "found).\n\n"
                      "Concurrent use not allowed.\n\n"
                      "Do you want to stay in the current mapset or remove "
                      ".gislock and switch to selected mapset?"
                      ).format(user=user,
                               mapset=mapset,
                               lockpath=lockpath,
                               timestamp=timestamp),
            caption=_("Mapset is in use"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
        )
        dlg.SetYesNoLabels("S&witch to selected mapset",
                           "S&tay in current mapset")
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
                "The location's default region was set from "
                "this imported map.") % {
                'name': filePath},
            parent=guiparent)


def switch_mapset_interactively(guiparent, giface, dbase, location, mapset,
                                show_confirmation=False):
    """Switch current mapset. Emits giface.currentMapsetChanged signal."""
    if dbase:
        if RunCommand('g.mapset', parent=guiparent,
                      location=location,
                      mapset=mapset,
                      dbase=dbase) == 0:
            if show_confirmation:
                GMessage(parent=guiparent,
                         message=_("Current GRASS database is <%(dbase)s>.\n"
                                   "Current location is <%(loc)s>.\n"
                                   "Current mapset is <%(mapset)s>."
                                   ) %
                        {'dbase': dbase, 'loc': location, 'mapset': mapset})
            giface.currentMapsetChanged.emit(dbase=dbase,
                                             location=location,
                                             mapset=mapset)
    elif location:
        if RunCommand('g.mapset', parent=guiparent,
                      location=location,
                      mapset=mapset) == 0:
            if show_confirmation:
                GMessage(parent=guiparent,
                         message=_("Current location is <%(loc)s>.\n"
                                   "Current mapset is <%(mapset)s>.") %
                        {'loc': location, 'mapset': mapset})
            giface.currentMapsetChanged.emit(dbase=None,
                                             location=location,
                                             mapset=mapset)
    else:
        if RunCommand('g.mapset',
                      parent=guiparent,
                      mapset=mapset) == 0:
            if show_confirmation:
                GMessage(parent=guiparent,
                         message=_("Current mapset is <%s>.") % mapset)
            giface.currentMapsetChanged.emit(dbase=None, location=None, mapset=mapset)
