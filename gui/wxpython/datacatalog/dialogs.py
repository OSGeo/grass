# -*- coding: utf-8 -*-
"""
@package datacatalog.dialogs

@brief Dialogs used in data catalog

Classes:
 - dialogs::CatalogReprojectionDialog

(C) 2017 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""


import wx
from gui_core.widgets import FloatValidator, IntegerValidator
from core.giface import Notification
from core.gcmd import RunCommand
from gui_core.wrap import Button, StaticText, TextCtrl

from grass.script import parse_key_val, region_env


class CatalogReprojectionDialog(wx.Dialog):
    def __init__(self, parent, giface, inputGisdbase, inputLocation,
                 inputMapset, inputLayer, inputEnv,
                 outputGisdbase, outputLocation, outputMapset, outputLayer,
                 etype, outputEnv, callback,
                 id=wx.ID_ANY, title=_("Reprojection"),
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):

        self.parent = parent
        self._giface = giface

        wx.Dialog.__init__(self, parent, id, title, style=style,
                           name="ReprojectionDialog")

        self.panel = wx.Panel(parent=self)
        self.iGisdbase = inputGisdbase
        self.iLocation = inputLocation
        self.iMapset = inputMapset
        self.iLayer = inputLayer
        self.iEnv = inputEnv
        self.oGisdbase = outputGisdbase
        self.oLocation = outputLocation
        self.oMapset = outputMapset
        self.oLayer = outputLayer
        self.etype = etype
        self.oEnv = outputEnv
        self.callback = callback

        self._widgets()
        self._doLayout()

        if self.etype == 'raster':
            self._estimateResampling()
            self._estimateResolution()

    def _widgets(self):
        if self.etype == 'raster':
            self.resolution = TextCtrl(self.panel, validator=FloatValidator())
            self.resampling = wx.Choice(self.panel, size=(200, -1),
                                        choices=['nearest', 'bilinear', 'bicubic', 'lanczos',
                                                 'bilinear_f', 'bicubic_f', 'lanczos_f'])
        else:
            self.vsplit = TextCtrl(self.panel, validator=IntegerValidator())
            self.vsplit.SetValue('10000')

        #
        # buttons
        #
        self.btn_close = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.SetEscapeId(self.btn_close.GetId())

        # run
        self.btn_run = Button(parent=self.panel, id=wx.ID_OK, label=_("Reproject"))
        if self.etype == 'raster':
            self.btn_run.SetToolTip(_("Reproject raster"))
        elif self.etype == 'vector':
            self.btn_run.SetToolTip(_("Reproject vector"))
        self.btn_run.SetDefault()
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnReproject)

    def _doLayout(self):
        """Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        optionsSizer = wx.GridBagSizer(5, 5)

        label = _("Map layer <{ml}> needs to be reprojected.\n"
                  "Please review and modify reprojection parameters:").format(ml=self.iLayer)
        dialogSizer.Add(StaticText(self.panel, label=label),
                        flag=wx.ALL | wx.EXPAND, border=10)
        if self.etype == 'raster':
            optionsSizer.Add(StaticText(self.panel, label=_("Estimated resolution:")),
                             pos=(0, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
            optionsSizer.Add(self.resolution, pos=(0, 1), flag=wx.EXPAND)
            optionsSizer.Add(StaticText(self.panel, label=_("Resampling method:")),
                             pos=(1, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
            optionsSizer.Add(self.resampling, pos=(1, 1), flag=wx.EXPAND)
        else:
            optionsSizer.Add(StaticText(self.panel, label=_("Maximum segment length:")),
                             pos=(1, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
            optionsSizer.Add(self.vsplit, pos=(1, 1), flag=wx.EXPAND)
        optionsSizer.AddGrowableCol(1)
        dialogSizer.Add(optionsSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=10)
        helptext = StaticText(self.panel,
                              label="For more reprojection options,"
                              " please see {module}".format(module='r.proj' if self.etype == 'raster'
                                                            else 'v.proj'))
        dialogSizer.Add(helptext, proportion=0, flag=wx.ALL | wx.EXPAND, border=10)
        #
        # buttons
        #
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btn_run)
        btnStdSizer.AddButton(self.btn_close)
        btnStdSizer.Realize()
        dialogSizer.Add(btnStdSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        self.Layout()
        self.SetSize(self.GetBestSize())

    def _estimateResolution(self):
        output = RunCommand('r.proj', flags='g', quiet=False, read=True, input=self.iLayer,
                            dbase=self.iGisdbase, location=self.iLocation, mapset=self.iMapset,
                            env=self.oEnv).strip()
        params = parse_key_val(output, vsep=' ')
        output = RunCommand('g.region', flags='ug', quiet=False, read=True, env=self.oEnv,
                            parse=lambda x: parse_key_val(x, val_type=float), **params)
        cell_ns = (output['n'] - output['s']) / output['rows']
        cell_ew = (output['e'] - output['w']) / output['cols']
        estimate = (cell_ew + cell_ns) / 2.
        self.resolution.SetValue(str(estimate))
        self.params = params

    def _estimateResampling(self):
        output = RunCommand('r.info', flags='g', quiet=False, read=True, map=self.iLayer,
                            env=self.iEnv, parse=parse_key_val)
        if output['datatype'] == 'CELL':
            self.resampling.SetStringSelection('nearest')
        else:
            self.resampling.SetStringSelection('bilinear')

    def OnReproject(self, event):
        cmd = []
        if self.etype == 'raster':
            cmd.append('r.proj')
            cmd.append('dbase=' + self.iGisdbase)
            cmd.append('location=' + self.iLocation)
            cmd.append('mapset=' + self.iMapset)
            cmd.append('input=' + self.iLayer)
            cmd.append('output=' + self.oLayer)
            cmd.append('method=' + self.resampling.GetStringSelection())

            self.oEnv['GRASS_REGION'] = region_env(n=self.params['n'], s=self.params['s'],
                                                   e=self.params['e'], w=self.params['w'],
                                                   flags='a', res=float(self.resolution.GetValue()),
                                                   env=self.oEnv)
        else:
            cmd.append('v.proj')
            cmd.append('dbase=' + self.iGisdbase)
            cmd.append('location=' + self.iLocation)
            cmd.append('mapset=' + self.iMapset)
            cmd.append('input=' + self.iLayer)
            cmd.append('output=' + self.oLayer)
            cmd.append('smax=' + self.vsplit.GetValue())

        self._giface.RunCmd(cmd, env=self.oEnv, compReg=False, addLayer=False,
                            onDone=self._onDone, userData=None,
                            notification=Notification.MAKE_VISIBLE)

        event.Skip()

    def _onDone(self, event):
        self.callback()
