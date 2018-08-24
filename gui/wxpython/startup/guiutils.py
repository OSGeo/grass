"""
@package startup.guiutils

@brief General GUI-dependent utilities for GUI startup of GRASS GIS

(C) 2018 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>

This is for code which depend on something from GUI (wx or wxGUI). 
"""


import wx

import grass.script as gs

from core.gcmd import RunCommand
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
