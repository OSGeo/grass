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

import grass.script as gs

from core import globalvar
from core.gcmd import GError, GMessage, DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from location_wizard.dialogs import RegionDef
from gui_core.widgets import GenericMultiValidator
from startup.utils import (create_mapset, delete_mapset, delete_location,
                           rename_mapset, rename_location, mapset_exists,
                           location_exists, get_default_mapset_name)


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

        # list of tuples consisting of conditions and callbacks
        checks = [(gs.legal_name, self._nameValidationFailed),
                  (self._checkMapsetNotExists, self._mapsetAlreadyExists),
                  (self._checkOGR, self._reservedMapsetName)]
        validator = GenericMultiValidator(checks)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _nameValidationFailed(self, ctrl):
        message = _(
            "Name '{}' is not a valid name for location or mapset. "
            "Please use only ASCII characters excluding characters {} "
            "and space.").format(ctrl.GetValue(), '/"\'@,=*~')
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _checkOGR(self, text):
        """Check user's input for reserved mapset name."""
        if text.lower() == 'ogr':
            return False
        return True

    def _reservedMapsetName(self, ctrl):
        message = _(
            "Name '{}' is reserved for direct "
            "read access to OGR layers. Please use "
            "another name for your mapset.").format(ctrl.GetValue())
        GError(parent=self, message=message,
               caption=_("Reserved mapset name"))

    def _checkMapsetNotExists(self, text):
        """Check whether user's input mapset exists or not."""
        if mapset_exists(self.database, self.location, text):
            return False
        return True

    def _mapsetAlreadyExists(self, ctrl):
        message = _(
            "Mapset '{}' already exists. Please consider using "
            "another name for your mapset.").format(ctrl.GetValue())
        GError(parent=self, message=message,
               caption=_("Existing mapset path"))


class LocationDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None, message=None, caption=None,
                 database=None):
        self.database = database

        # list of tuples consisting of conditions and callbacks
        checks = [(gs.legal_name, self._nameValidationFailed),
                  (self._checkLocationNotExists, self._locationAlreadyExists)]
        validator = GenericMultiValidator(checks)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=message,
            caption=caption,
            defaultValue=default,
            validator=validator,
        )

    def _nameValidationFailed(self, ctrl):
        message = _(
            "Name '{}' is not a valid name for location or mapset. "
            "Please use only ASCII characters excluding characters {} "
            "and space.").format(ctrl.GetValue(), '/"\'@,=*~')
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _checkLocationNotExists(self, text):
        """Check whether user's input location exists or not."""
        if location_exists(self.database, text):
            return False
        return True

    def _locationAlreadyExists(self, ctrl):
        message = _(
            "Location '{}' already exists. Please consider using "
            "another name for your location.").format(ctrl.GetValue())
        GError(parent=self, message=message,
               caption=_("Existing location path"))


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

    if dlg.ShowModal() == wx.ID_OK:
        mapset = dlg.GetValue()
        try:
            create_mapset(grassdb, location, mapset)
        except OSError as err:
            GError(
                parent=guiparent,
                message=_("Unable to create new mapset: %s") % err,
                showTraceback=False,
            )
    else:
        mapset = None
    dlg.Destroy()
    return mapset


def create_location_interactively(guiparent, grassdb):
    """
    Location wizard started
    """
    from location_wizard.wizard import LocationWizard

    gWizard_output = (None, None, "PERNAMENT")
    gWizard = LocationWizard(parent=guiparent,
                             grassdatabase=grassdb)

    if gWizard.location is not None:
        if gWizard.georeffile:
            message = _(
                "Do you want to import <%(name)s>"
                "to the newly created location?"
            ) % {'name': gWizard.georeffile}
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
            user_mapset = create_mapset_interactively(guiparent,
                                                      gWizard.grassdatabase,
                                                      gWizard.location)
            # Returns database and location created by user
            # and a mapset user may want to switch to
            gWizard_output = (gWizard.grassdatabase, gWizard.location,
                              user_mapset)
        gWizard_output = (gWizard.grassdatabase, gWizard.location,
                          "PERNAMENT")
    return gWizard_output


def rename_mapset_interactively(guiparent, grassdb, location, mapset):
    """
    Rename selected mapset
    """
    if mapset == "PERMANENT":
        GMessage(
            parent=guiparent,
            message=_(
                "Mapset <PERMANENT> is required for valid GRASS location.\n\n"
                "This mapset cannot be renamed."
            ),
        )
        return
    dlg = MapsetDialog(
        parent=guiparent,
        default=mapset,
        message=_("Current name: %s\n\nEnter new name:") % mapset,
        caption=_("Rename selected mapset"),
        database=grassdb,
        location=location,
    )

    if dlg.ShowModal() == wx.ID_OK:
        newmapset = dlg.GetValue()
        try:
            rename_mapset(grassdb, location, mapset, newmapset)
        except OSError as err:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename mapset.\n\n%s") % err,
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    else:
        newmapset = None
    dlg.Destroy()
    return newmapset


def rename_location_interactively(guiparent, grassdb, location):
    """
    Rename selected location
    """
    dlg = LocationDialog(
        parent=guiparent,
        default=location,
        message=_("Current name: %s\n\nEnter new name:") % location,
        caption=_("Rename selected location"),
        database=grassdb,
    )

    if dlg.ShowModal() == wx.ID_OK:
        newlocation = dlg.GetValue()
        try:
            rename_location(grassdb, location, newlocation)
        except OSError as err:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to rename location.\n\n%s") % err,
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )
    else:
        newlocation = None
    dlg.Destroy()
    return newlocation


def download_location_interactively(guiparent, grassdb):
    """
    Download location online
    """
    from startup.locdownload import LocationDownloadDialog

    loc_download_output = (None, None, "PERNAMENT")
    loc_download = LocationDownloadDialog(parent=guiparent,
                                          database=grassdb)
    loc_download.ShowModal()

    if loc_download.GetLocation() is not None:
        # Returns database and location created by user
        # and a mapset user may want to switch to
        loc_download_output = (grassdb,
                               loc_download.GetLocation(),
                               "PERNAMENT")
    loc_download.Destroy()
    return loc_download_output


def delete_mapset_interactively(guiparent, grassdb, location, mapset):
    """
    Delete selected mapset
    """
    if mapset == "PERMANENT":
        GMessage(
            parent=guiparent,
            message=_(
                "Mapset <PERMANENT> is required for valid GRASS location.\n\n"
                "This mapset cannot be deleted."
            ),
        )
        return

    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting mapset <%(mapset)s> "
            "from location <%(location)s>?\n\n"
            "ALL MAPS included in this mapset will be "
            "PERMANENTLY DELETED!"
        )
        % {"mapset": mapset, "location": location},
        caption=_("Delete selected mapset"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )

    if dlg.ShowModal() == wx.ID_YES:
        try:
            delete_mapset(grassdb, location, mapset)
        except OSError as err:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to delete mapset.\n\n%s") % err,
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )

    dlg.Destroy()


def delete_location_interactively(guiparent, grassdb, location):
    """
    Delete selected location
    """
    dlg = wx.MessageDialog(
        parent=guiparent,
        message=_(
            "Do you want to continue with deleting "
            "location <%s>?\n\n"
            "ALL MAPS included in this location will be "
            "PERMANENTLY DELETED!"
        )
        % (location),
        caption=_("Delete selected location"),
        style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
    )

    if dlg.ShowModal() == wx.ID_YES:
        try:
            delete_location(grassdb, location)
        except OSError as err:
            wx.MessageBox(
                parent=guiparent,
                caption=_("Error"),
                message=_("Unable to delete location.\n\n%s") % err,
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
            )

    dlg.Destroy()


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
