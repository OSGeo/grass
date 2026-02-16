"""
@package jupyter_notebook.dialogs

@brief Startup dialog for integration of Jupyter Notebook to GUI.

Classes:
 - dialogs::JupyterStartDialog

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from pathlib import Path
import wx

from .utils import get_default_jupyter_workdir


class JupyterStartDialog(wx.Dialog):
    """Dialog for selecting Jupyter startup options."""

    def __init__(self, parent):
        super().__init__(parent, title=_("Start Jupyter Notebook"), size=(500, 300))

        self.default_dir = get_default_jupyter_workdir()

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
            self, message=_("Choose a working directory"), style=wx.DIRP_USE_TEXTCTRL
        )
        self.dir_picker.Enable(False)

        dir_sizer.Add(self.radio_default, 0, wx.ALL, 5)
        dir_sizer.Add(self.radio_custom, 0, wx.ALL, 5)
        dir_sizer.Add(self.dir_picker, 0, wx.EXPAND | wx.ALL, 5)
        sizer.Add(dir_sizer, 0, wx.EXPAND | wx.ALL, 10)

        # Template preference section
        options_box = wx.StaticBox(self, label=_("Options"))
        options_sizer = wx.StaticBoxSizer(options_box, wx.VERTICAL)

        self.checkbox_template = wx.CheckBox(self, label=_("Create welcome notebook"))
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
                "Note: The welcome notebook will be created only if the directory contains no .ipynb files."
            ),
        )

        options_sizer.Add(info, 0, wx.LEFT | wx.RIGHT | wx.BOTTOM, 8)

        sizer.Add(options_sizer, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 10)

        # Buttons section
        btn_sizer = wx.BoxSizer(wx.HORIZONTAL)

        btn_cancel = wx.Button(self, wx.ID_CANCEL, label=_("Cancel"))
        btn_sizer.Add(btn_cancel, 0, wx.ALL, 5)
        btn_browser = wx.Button(self, label=_("Open Notebook in Browser"))
        btn_sizer.Add(btn_browser, 0, wx.ALL, 5)
        btn_integrated = wx.Button(self, label=_("Open Integrated Notebook"))
        btn_sizer.Add(btn_integrated, 0, wx.ALL, 5)

        sizer.Add(btn_sizer, 0, wx.ALIGN_CENTER | wx.ALL, 10)

        # Bind events
        self.radio_default.Bind(wx.EVT_RADIOBUTTON, self.OnRadioToggle)
        self.radio_custom.Bind(wx.EVT_RADIOBUTTON, self.OnRadioToggle)

        btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btn_browser.Bind(wx.EVT_BUTTON, self.OnOpenInBrowser)
        btn_integrated.Bind(wx.EVT_BUTTON, self.OnOpenIntegrated)

        self.SetSizer(sizer)
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
            path = Path(self.dir_picker.GetPath())

            try:
                # create directory if missing
                path.mkdir(parents=True, exist_ok=True)

                # write permission test
                test_file = path / ".grass_write_test"
                test_file.touch()
                test_file.unlink()

            except OSError:
                wx.MessageBox(
                    _("Cannot create or write to the selected directory."),
                    _("Error"),
                    wx.ICON_ERROR,
                )
                return None

            self.selected_dir = path
        else:
            self.selected_dir = Path(self.default_dir)

        return {
            "directory": self.selected_dir,
            "create_template": self.checkbox_template.GetValue(),
        }

    def OnCancel(self, event):
        self.EndModal(wx.ID_CANCEL)

    def OnOpenIntegrated(self, event):
        if not self.GetValues():
            return
        self.action = "integrated"
        self.EndModal(wx.ID_OK)

    def OnOpenInBrowser(self, event):
        if not self.GetValues():
            return
        self.action = "browser"
        self.EndModal(wx.ID_OK)
