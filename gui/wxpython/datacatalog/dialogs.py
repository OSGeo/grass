# -*- coding: utf-8 -*-
"""
@package datacatalog.dialogs

@brief Dialogs used in data catalog

Classes:
 - dialogs::CatalogReprojectionDialog

(C) 2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""


import wx
import grass.script as gscript
from grass.script import task as gtask
from gui_core.forms import CmdPanel
from core.giface import Notification


class CatalogReprojectionDialog(wx.Dialog):
    """ """
    def __init__(self, parent, giface, inputGisdbase, inputLocation, inputMapset, inputLayer,
                 outputGisdbase, outputLocation, outputMapset, etype,
                 id=wx.ID_ANY, title=_("Reprojection"),
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame
        self._giface = giface  # used to add layers

        wx.Dialog.__init__(self, parent, id, title, style=style,
                           name="ReprojectionDialog")

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.iGisdbase = inputGisdbase
        self.iLocation = inputLocation
        self.iMapset = inputMapset
        self.iLayer = inputLayer
        self.oGisdbase = outputGisdbase
        self.oLocation = outputLocation
        self.oMapset = outputMapset
        self.etype = etype

        self._blackList = {
            'enabled': True,
            'items': {
                'r.proj': {
                    'params': ['location', 'mapset', 'input', 'dbase'],
                    'flags': ['l']},
                'v.proj': {
                    'params': ['location', 'mapset', 'input', 'dbase'],
                    'flags': ['l']}}}

        if self.etype == 'raster':
            grass_task = gtask.parse_interface('r.proj', blackList=self._blackList)
        elif self.etype == 'vector':
            grass_task = gtask.parse_interface('v.proj', blackList=self._blackList)

        self.settingsPanel = CmdPanel(parent=self, giface=self._giface, task=grass_task, frame=None)
        self.closeOnFinished = wx.CheckBox(self.panel, label=_("Close dialog on finish"))
        #
        # buttons
        #
        # cancel
        self.btn_close = wx.Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btn_close.Bind(wx.EVT_BUTTON, lambda evt: self.Close())

        # run
        self.btn_run = wx.Button(
            parent=self.panel,
            id=wx.ID_OK,
            label=_("Reproject"))
        if self.etype == 'raster':
            self.btn_run.SetToolTipString(_("Reproject raster"))
        elif self.etype == 'vector':
            self.btn_run.SetToolTipString(_("Reproject vector"))
        self.btn_run.SetDefault()
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnReproject)

        self.doLayout()

    def doLayout(self):
        """Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        dialogSizer.Add(wx.StaticText(self.panel, label=_("The copied layer needs to be reprojected:")),
                        flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=5)
        dialogSizer.Add(item=self.settingsPanel, proportion=1,
                        flag=wx.ALL | wx.EXPAND, border=5)
        dialogSizer.Add(item=self.closeOnFinished, flag=wx.ALL | wx.EXPAND, border=5)

        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        btnsizer.Add(item=self.btn_close, proportion=0,
                     flag=wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        btnsizer.Add(item=self.btn_run, proportion=0,
                     flag=wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        dialogSizer.Add(
            item=btnsizer,
            proportion=0,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT,
            border=5)

        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        self.Layout()
        # sizing not working properly
        self.SetMinSize(self.GetBestSize())

    def getSettingsPageCmd(self):
        return self.settingsPanel.createCmd(
            ignoreErrors=True, ignoreRequired=True)

    def OnReproject(self, event):
        cmd = self.getSettingsPageCmd()
        cmd.append('dbase=' + self.iGisdbase)
        cmd.append('location=' + self.iLocation)
        cmd.append('mapset=' + self.iMapset)
        cmd.append('input=' + self.iLayer)

        self.tmpfile, env = gscript.create_environment(self.oGisdbase, self.oLocation, self.oMapset)

        self._giface.RunCmd(cmd, env=env,
               onDone=self.OnDone, userData=None,
               notification=Notification.MAKE_VISIBLE)

    def OnDone(self, event):
        gscript.try_remove(self.tmpfile)
        if self.closeOnFinished.IsChecked() and event.returncode == 0:
            self.Close()
