"""
@package startup.guiutils

@brief General GUI-dependent utilities for GUI startup of GRASS GIS

(C) 2018 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>

This is for code which depend on something from GUI (wx or wxGUI). 
"""


import os

import wx

import grass.script as gs

from core import globalvar
from core.gcmd import GError, DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from gui_core.widgets import GenericMultiValidator
from startup.utils import mapset_exists


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
                  (self._checkMapsetExists, self._mapsetAlreadyExists),
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
            "Name <%(name)s> is not a valid name for location or mapset. "
            "Please use only ASCII characters excluding %(chars)s "
            "and space.") % {
            'name': ctrl.GetValue(),
            'chars': '/"\'@,=*~'}
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _checkOGR(self, text):
        """Check user's input for reserved mapset name."""
        if text.lower() == 'ogr':
            return False
        return True

    def _reservedMapsetName(self, ctrl):
            message = _(
                "Name <%s> is reserved for direct "
                "read access to OGR layers. Please use "
                "another name for your mapset.\n\n") % {
            'name': ctrl.GetValue()}
            GError(parent=self, message=message,
                   caption=_("Reserved mapset name"))

    def _checkMapsetExists(self, text):
        """Check whether user's input mapset exists or not."""
        if mapset_exists(self.database, self.location, text):
            return False
        return True

    def _mapsetAlreadyExists(self, ctrl):
        message = _(
            "Mapset <%s> already exists.Please consider to use "
            "another name for your location.\n\n") % {
            'name': ctrl.GetValue()}
        GError(parent=self, message=message, caption=_("Existing mapset path"))


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
