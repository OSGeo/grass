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

import grass.script as gs

from core import globalvar
from core.gcmd import GError, DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from gui_core.widgets import GenericMultiValidator


def SetSessionMapset(database, location, mapset):
    """Sets database, location and mapset for the current session"""
    RunCommand("g.gisenv", set="GISDBASE=%s" % database)
    RunCommand("g.gisenv", set="LOCATION_NAME=%s" % location)
    RunCommand("g.gisenv", set="MAPSET=%s" % mapset)


class NewMapsetDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None,
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
            message=_("Name for the new mapset:"),
            caption=_("Create new mapset"),
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
            "Mapset '{}' already exists. Please consider to use "
            "another name for your location.").format(ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Existing mapset path"))


class NewGrassDbDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None):

        # list of tuples consisting of conditions and callbacks
        checks = [(gs.legal_name, self._nameValidationFailed),
                  (self._checkDbNotExists, self._dbAlreadyExists),
                  (self._checkOGR, self._reservedDbName)]
        validator = GenericMultiValidator(checks)

        TextEntryDialog.__init__(
            self, parent=parent,
            message=_("Name for the grass database:"),
            caption=_("Create new grass database"),
            defaultValue=default,
            validator=validator,
        )

    def _nameValidationFailed(self, ctrl):
        message = _(
            "Name '{}' is not a valid name for grass database. "
            "Please use only ASCII characters excluding characters {} "
            "and space.").format(ctrl.GetValue(), '/"\'@,=*~')
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _checkOGR(self, text):
        """Check user's input for reserved grass database name."""
        if text.lower() == 'ogr':
            return False
        return True

    def _reservedDbName(self, ctrl):
        message = _(
            "Name '{}' is reserved for direct "
            "read access to OGR layers. Please use "
            "another name for your grass database.").format(ctrl.GetValue())
        GError(parent=self, message=message,
               caption=_("Reserved grass database name"))

    def _checkDbNotExists(self, text):
        """Check whether user's input grass database exists or not."""
        if grassdb_exists(text):
            return False
        return True

    def _dbAlreadyExists(self, ctrl):
        message = _(
            "Grass database '{}' already exists. Please consider to use "
            "another name for your grass database.").format(ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Existing grass database path"))


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


def mapset_exists(database, location, mapset):
    """Returns True whether mapset path exists."""
    location_path = os.path.join(database, location)
    mapset_path = os.path.join(location_path, mapset)
    if os.path.exists(mapset_path):
        return True
    return False


def grassdb_exists(text):
    """Returns True whether db path in common paths exists."""
    home = os.path.expanduser('~')
    candidates = [
        home,
        os.path.join(home, "Documents"),
    ]
    # find possible database path
    for candidate in candidates:
        if os.path.exists(os.path.join(candidate, text)):
            return True
        return False
