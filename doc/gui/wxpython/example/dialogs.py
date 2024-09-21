"""!
@package example.dialogs

@brief Dialogs used in Example tool

Classes:
 - dialogs::ExampleMapDialog

(C) 2011-2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.
# NOTE: in this particular case, we don't really need the grass library;
# NOTE: we import it just for the side effects of gettext.install()
import os
import sys
import wx
import gettext
from core import globalvar
from gui_core.dialogs import SimpleDialog
from gui_core import gselect

# this enables to run application standalone (> python example/frame.py )
if __name__ == "__main__":
    sys.path.append(os.path.join(os.environ["GISBASE"], "etc", "gui", "wxpython"))

# Instead of importing grass, we directly import the gettext function
gettext.install("grasswxpy", localedir=None, unicode=True)


class ExampleMapDialog(SimpleDialog):
    """!Dialog for adding raster map.

    Dialog can be easily changed to enable to choose vector,
    imagery groups, or other elements.
    """

    def __init__(self, parent, title=_("Choose raster map")):
        """!Calls super class constructor.

        @param parent gui parent
        @param title dialog window title
        @param id id
        """
        SimpleDialog.__init__(self, parent, title)

        # here is the place to determine element type
        self.element = gselect.Select(
            parent=self.panel, type="raster", size=globalvar.DIALOG_GSELECT_SIZE
        )

        self._layout()

        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(
            wx.StaticText(parent=self.panel, label=_("Name of raster map:")),
            proportion=0,
            flag=wx.ALL,
            border=1,
        )
        self.dataSizer.Add(
            self.element, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetRasterMap(self):
        """!Returns selected raster map"""
        return self.element.GetValue()
