"""
Created on Mon Nov 26 11:57:54 2012

@author: lucadelu
"""

import wx
import os
from pathlib import Path

from core import globalvar, gcmd
from grass.script.utils import try_remove
from rlisetup.functions import retRLiPath
from rlisetup.wizard import RLIWizard
import locale
import codecs
from gui_core.wrap import Button, StaticBox, TextCtrl


class ViewFrame(wx.Frame):
    def __init__(
        self,
        parent,
        conf,
        giface=None,
        id=wx.ID_ANY,
        title=_("Modify the configuration file"),
        style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        # VARIABLES
        self.parent = parent
        self.rlipath = retRLiPath()
        self.confile = conf
        self.pathfile = os.path.join(self.rlipath, conf)
        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.confilesBox = StaticBox(
            parent=self.panel,
            id=wx.ID_ANY,
            label=_("View and modify the configuration file '{name}'").format(
                name=self.confile
            ),
        )
        self.textCtrl = TextCtrl(
            parent=self.panel, id=wx.ID_ANY, style=wx.TE_MULTILINE, size=(-1, 75)
        )
        self.textCtrl.Bind(wx.EVT_TEXT, self.OnFileText)
        with open(self.pathfile) as f:
            self.textCtrl.SetValue("".join(f.readlines()))
        # BUTTONS      #definition
        self.btn_close = Button(parent=self, id=wx.ID_EXIT)
        self.btn_ok = Button(parent=self, id=wx.ID_SAVE)
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnOk)
        self._layout()
        try:
            # Python >= 3.11
            self.enc = locale.getencoding()
        except AttributeError:
            self.enc = locale.getdefaultlocale()[1]

    def _layout(self):
        """Set the layout"""
        panelsizer = wx.GridBagSizer(1, 1)
        mainsizer = wx.BoxSizer(wx.VERTICAL)
        # CONFILES
        confilesSizer = wx.StaticBoxSizer(self.confilesBox, wx.HORIZONTAL)
        confilesSizer.Add(self.textCtrl, proportion=1, flag=wx.EXPAND)
        # END CONFILES
        # BUTTONS
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer.Add(self.btn_ok, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_close, flag=wx.ALL, border=5)
        # END BUTTONS
        # add listbox to staticbox
        panelsizer.Add(confilesSizer, pos=(0, 0), flag=wx.EXPAND, border=3)
        # add panel and buttons
        mainsizer.Add(self.panel, proportion=1, flag=wx.EXPAND, border=3)
        mainsizer.Add(buttonSizer, proportion=0, flag=wx.EXPAND, border=3)
        panelsizer.AddGrowableRow(0)
        panelsizer.AddGrowableCol(0)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizerAndFit(panelsizer)
        self.SetSizer(mainsizer)
        self.Layout()

    def OnClose(self, event):
        """Close window"""
        self.Destroy()

    def OnOk(self, event):
        """Launches help"""
        dlg = wx.MessageDialog(
            parent=self.parent,
            message=_(
                "Are you sure that you want modify"
                " r.li configuration file {name}?"
                "\nYou could broke the configuration"
                " file..."
            ).format(name=self.confile),
            caption=_("WARNING"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_WARNING,
        )

        if dlg.ShowModal() == wx.ID_YES:
            with codecs.open(
                self.pathfile, encoding=self.enc, mode="w", errors="replace"
            ) as f:
                f.write(self.text + os.linesep)
        dlg.Destroy()
        self.Destroy()

    def OnFileText(self, event):
        """File input interactively entered"""
        self.text = event.GetString()


class RLiSetupFrame(wx.Frame):
    def __init__(
        self,
        parent,
        giface=None,
        id=wx.ID_ANY,
        title=_("Setup for r.li modules"),
        style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        # VARIABLES
        self.parent = parent
        #        self.cmd = "r.li.setup"
        self.rlipath = retRLiPath()
        self.listfiles = self.ListFiles()
        # END VARIABLES
        # init of frame
        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        # box for select configuration file
        self.confilesBox = StaticBox(
            parent=self.panel,
            id=wx.ID_ANY,
            label=_("Available sampling area configuration files"),
        )
        self.listfileBox = wx.ListBox(
            parent=self.panel, id=wx.ID_ANY, choices=self.listfiles
        )

        # BUTTONS      #definition
        self.btn_close = Button(parent=self, id=wx.ID_CLOSE)
        self.btn_help = Button(parent=self, id=wx.ID_HELP)
        self.btn_remove = Button(parent=self, id=wx.ID_ANY, label=_("Remove"))
        self.btn_remove.SetToolTip(_("Remove a configuration file"))
        self.btn_new = Button(parent=self, id=wx.ID_ANY, label=_("Create"))
        self.btn_new.SetToolTip(_("Create a new configuration file"))
        self.btn_rename = Button(parent=self, id=wx.ID_ANY, label=_("Rename"))
        self.btn_rename.SetToolTip(_("Rename a configuration file"))
        self.btn_view = Button(parent=self, id=wx.ID_ANY, label=_("View/Edit"))
        self.btn_view.SetToolTip(_("View and edit a configuration file"))
        # set action for button
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.btn_remove.Bind(wx.EVT_BUTTON, self.OnRemove)
        self.btn_new.Bind(wx.EVT_BUTTON, self.OnNew)
        self.btn_rename.Bind(wx.EVT_BUTTON, self.OnRename)
        self.btn_view.Bind(wx.EVT_BUTTON, self.OnView)
        self._layout()
        # END BUTTONS
        # SIZE FRAME
        self.SetMinSize(self.GetBestSize())
        # Please check this because without this the size it is not the min
        self.SetClientSize(self.GetBestSize())
        # END SIZE

    def _layout(self):
        """Set the layout"""
        panelsizer = wx.GridBagSizer(1, 1)
        mainsizer = wx.BoxSizer(wx.VERTICAL)
        # CONFILES
        confilesSizer = wx.StaticBoxSizer(self.confilesBox, wx.HORIZONTAL)
        confilesSizer.Add(self.listfileBox, proportion=1, flag=wx.EXPAND)
        # END CONFILES
        # BUTTONS
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer.Add(self.btn_new, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_rename, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_view, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_remove, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_help, flag=wx.ALL, border=5)
        buttonSizer.Add(self.btn_close, flag=wx.ALL, border=5)
        # END BUTTONS
        # add listbox to staticbox
        panelsizer.Add(confilesSizer, pos=(0, 0), flag=wx.EXPAND, border=3)

        # add panel and buttons
        mainsizer.Add(self.panel, proportion=1, flag=wx.EXPAND, border=3)
        mainsizer.Add(buttonSizer, proportion=0, flag=wx.EXPAND, border=3)

        panelsizer.AddGrowableRow(0)
        panelsizer.AddGrowableCol(0)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizerAndFit(panelsizer)
        self.SetSizer(mainsizer)
        self.Layout()

    def ListFiles(self):
        """Check the configuration files inside the path"""
        # list of configuration file
        # return all the configuration files in self.rlipath, check if there are
        # link or directory and doesn't add them
        listfiles = [
            rli_conf.name
            for rli_conf in Path(self.rlipath).iterdir()
            if rli_conf.is_file()
        ]
        return sorted(listfiles)

    def OnClose(self, event):
        """Close window"""
        self.Destroy()

    def OnHelp(self, event):
        """Launches help"""
        gcmd.RunCommand("g.manual", parent=self, entry="wxGUI.rlisetup")

    def OnRemove(self, event):
        """Remove configuration file from path and update the list"""
        try:
            confile = self.listfiles[self.listfileBox.GetSelections()[0]]
        except IndexError:
            gcmd.GMessage(
                parent=self, message=_("You have to select a configuration file")
            )
            return
        dlg = wx.MessageDialog(
            parent=self.parent,
            message=_("Do you want remove r.li configuration file <%s>?") % confile,
            caption=_("Remove new r.li configuration file?"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
        )

        if dlg.ShowModal() == wx.ID_YES:
            self.listfileBox.Delete(self.listfileBox.GetSelections()[0])
            try_remove(os.path.join(self.rlipath, confile))
            self.listfiles = self.ListFiles()
        dlg.Destroy()
        return

    def OnNew(self, event):
        """Remove configuration file from path and update the list"""
        RLIWizard(self)
        self.listfiles = self.ListFiles()
        self.listfileBox.Clear()
        self.listfileBox.Set(self.listfiles)

    def OnRename(self, event):
        """Rename an existing configuration file"""
        try:
            confile = self.listfiles[self.listfileBox.GetSelections()[0]]
        except IndexError:
            gcmd.GMessage(
                parent=self, message=_("You have to select a configuration file")
            )
            return
        dlg = wx.TextEntryDialog(
            parent=self.parent,
            message=_(
                'Set the new name for %s " \
                                           "configuration file'
            )
            % confile,
            caption=_("Rename configuration file"),
        )
        if dlg.ShowModal() == wx.ID_OK:
            res = dlg.GetValue()
            newname = "%s%s%s" % (self.rlipath, os.sep, res)
            os.rename(os.path.join(self.rlipath, confile), newname)
            self.listfiles = self.ListFiles()
            self.listfileBox.Clear()
            self.listfileBox.Set(self.listfiles)

    def OnView(self, event):
        """Show and edit a configuration file"""
        try:
            confile = self.listfiles[self.listfileBox.GetSelections()[0]]
        except IndexError:
            gcmd.GMessage(
                parent=self, message=_("You have to select a configuration file")
            )
            return
        frame = ViewFrame(self, conf=confile)
        frame.Show()
