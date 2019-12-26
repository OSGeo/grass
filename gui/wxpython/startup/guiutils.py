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
from core.gcmd import DecodeString, RunCommand
from gui_core.dialogs import TextEntryDialog
from gui_core.widgets import GenericValidator


def SetSessionMapset(database, location, mapset):
    """Sets database, location and mapset for the current session"""
    RunCommand("g.gisenv", set="GISDBASE=%s" % database)
    RunCommand("g.gisenv", set="LOCATION_NAME=%s" % location)
    RunCommand("g.gisenv", set="MAPSET=%s" % mapset)



class NewMapsetDialog(TextEntryDialog):
    def __init__(self, parent=None, default=None,
                 validation_failed_handler=None, help_hanlder=None):
        if help_hanlder:
            style = wx.OK | wx.CANCEL | wx.HELP
        else:
            style = wx.OK | wx.CANCEL
        if validation_failed_handler:
            validator=GenericValidator(
                gs.legal_name, validation_failed_handler)
        else:
            validator = None
        TextEntryDialog.__init__(
            self, parent=parent,
            message=_("Name for the new mapset:"),
            caption=_("Create new mapset"),
            defaultValue=default,
            validator=validator,
            style=style
        )
        if help_hanlder:
            help_button = self.FindWindowById(wx.ID_HELP)
            help_button.Bind(wx.EVT_BUTTON, help_hanlder)


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
