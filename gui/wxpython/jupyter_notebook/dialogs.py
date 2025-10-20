"""
@package jupyter_notebook.dialog

@brief Integration of Jupyter Notebook to GUI.

Classes:
 - dialog::JupyterStartDialog

(C) 2025 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from pathlib import Path
import grass.script as gs

import wx


class JupyterStartDialog(wx.Dialog):
    """Dialog for selecting working directory and Jupyter startup options."""

    def __init__(self, parent):
        wx.Dialog.__init__(
            self, parent, title=_("Start Jupyter Notebook"), size=(500, 300)
        )
        env = gs.gisenv()
        mapset_path = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
        self.default_dir = mapset_path / "notebooks"

        self.selected_dir = self.default_dir
        self.create_template = True

        sizer = wx.BoxSizer(wx.VERTICAL)

        # Working directory section
        dir_box = wx.StaticBox(self, label=_("Notebook working directory"))
        dir_sizer = wx.StaticBoxSizer(dir_box, wx.VERTICAL)

        self.radio_default = wx.RadioButton(
            self,
            label=_("Use default: {}").format(self.default_dir),
            style=wx.RB_GROUP,
        )
        self.radio_custom = wx.RadioButton(self, label=_("Select another directory:"))

        self.dir_picker = wx.DirPickerCtrl(
            self,
            message=_("Choose a working directory"),
            style=wx.DIRP_USE_TEXTCTRL | wx.DIRP_DIR_MUST_EXIST,
        )
        self.dir_picker.Enable(False)

        dir_sizer.Add(self.radio_default, 0, wx.ALL, 5)
        dir_sizer.Add(self.radio_custom, 0, wx.ALL, 5)
        dir_sizer.Add(self.dir_picker, 0, wx.EXPAND | wx.ALL, 5)
        sizer.Add(dir_sizer, 0, wx.EXPAND | wx.ALL, 10)

        # Jupyter startup section
        options_box = wx.StaticBox(self, label=_("Options"))
        options_sizer = wx.StaticBoxSizer(options_box, wx.VERTICAL)

        self.checkbox_template = wx.CheckBox(self, label=_("Create example notebook"))
        self.checkbox_template.SetValue(True)
        self.checkbox_template.SetToolTip(
            _(
                "If selected, a welcome notebook (welcome.ipynb) will be created,\n"
                "but only if the selected directory contains no .ipynb files."
            )
        )
        options_sizer.Add(self.checkbox_template, 0, wx.ALL, 5)

        info = wx.StaticText(
            self,
            label=_(
                "Note: Template will be created only if the directory contains no .ipynb files."
            ),
        )

        options_sizer.Add(info, 0, wx.LEFT | wx.RIGHT | wx.BOTTOM, 8)

        sizer.Add(options_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 10)

        # OK / Cancel buttons
        btns = self.CreateSeparatedButtonSizer(wx.OK | wx.CANCEL)
        sizer.Add(btns, 0, wx.EXPAND | wx.ALL, 10)

        self.SetSizer(sizer)

        self.radio_default.Bind(wx.EVT_RADIOBUTTON, self.OnRadioToggle)
        self.radio_custom.Bind(wx.EVT_RADIOBUTTON, self.OnRadioToggle)

        self.Fit()
        self.Layout()
        self.SetMinSize(self.GetSize())
        self.CentreOnParent()

    def OnRadioToggle(self, event):
        """Enable/disable directory picker based on user choice."""
        self.dir_picker.Enable(self.radio_custom.GetValue())

    def GetValues(self):
        """Return selected working directory and template preference."""
        if self.radio_custom.GetValue():
            self.selected_dir = Path(self.dir_picker.GetPath())
        else:
            self.selected_dir = self.default_dir

        return {
            "directory": self.selected_dir,
            "create_template": self.checkbox_template.GetValue(),
        }
