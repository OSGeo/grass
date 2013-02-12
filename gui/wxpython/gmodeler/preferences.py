"""!
@package gmodeler.preferences

@brief wxGUI Graphical Modeler - preferences

Classes:
 - preferences::PreferencesDialog
 - preferences::PropertiesDialog

(C) 2010-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx
import wx.lib.colourselect    as csel

from core                 import globalvar
from gui_core.preferences import PreferencesBaseDialog
from core.settings        import UserSettings

class PreferencesDialog(PreferencesBaseDialog):
    """!User preferences dialog"""
    def __init__(self, parent, giface, settings = UserSettings,
                 title = _("Modeler settings")):
        
        PreferencesBaseDialog.__init__(self, parent = parent, giface = giface, title = title,
                                       settings = settings)
        
        # create notebook pages
        self._createGeneralPage(self.notebook)
        self._createActionPage(self.notebook)
        self._createDataPage(self.notebook)
        self._createLoopPage(self.notebook)
        
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createGeneralPage(self, notebook):
        """!Create notebook page for action settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("General"))
        
        # colors
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Item properties"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Disabled:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        rColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='disabled', subkey='color'),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        rColor.SetName('GetColour')
        self.winId['modeler:disabled:color'] = rColor.GetId()
        
        gridSizer.Add(item = rColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)

        panel.SetSizer(border)
        
        return panel

    def _createActionPage(self, notebook):
        """!Create notebook page for action settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Action"))
        
        # colors
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Color"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Valid:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        vColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='action', subkey=('color', 'valid')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        vColor.SetName('GetColour')
        self.winId['modeler:action:color:valid'] = vColor.GetId()
        
        gridSizer.Add(item = vColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Invalid:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        iColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='action', subkey=('color', 'invalid')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        iColor.SetName('GetColour')
        self.winId['modeler:action:color:invalid'] = iColor.GetId()
        
        gridSizer.Add(item = iColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Running:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        rColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='action', subkey=('color', 'running')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        rColor.SetName('GetColour')
        self.winId['modeler:action:color:running'] = rColor.GetId()
        
        gridSizer.Add(item = rColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        # size
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Shape size"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)

        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Width:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        
        width = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                            min = 0, max = 500,
                            initial = self.settings.Get(group='modeler', key='action', subkey=('size', 'width')))
        width.SetName('GetValue')
        self.winId['modeler:action:size:width'] = width.GetId()
        
        gridSizer.Add(item = width,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        
        height = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                             min = 0, max = 500,
                             initial = self.settings.Get(group='modeler', key='action', subkey=('size', 'height')))
        height.SetName('GetValue')
        self.winId['modeler:action:size:height'] = height.GetId()
        
        gridSizer.Add(item = height,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
                
        panel.SetSizer(border)
        
        return panel

    def _createDataPage(self, notebook):
        """!Create notebook page for data settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Data"))
        
        # colors
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Type"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Raster:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        rColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='data', subkey=('color', 'raster')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        rColor.SetName('GetColour')
        self.winId['modeler:data:color:raster'] = rColor.GetId()
        
        gridSizer.Add(item = rColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("3D raster:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        r3Color = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                    colour = self.settings.Get(group='modeler', key='data', subkey=('color', 'raster3d')),
                                    size = globalvar.DIALOG_COLOR_SIZE)
        r3Color.SetName('GetColour')
        self.winId['modeler:data:color:raster3d'] = r3Color.GetId()
        
        gridSizer.Add(item = r3Color,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Vector:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        vColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='data', subkey=('color', 'vector')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        vColor.SetName('GetColour')
        self.winId['modeler:data:color:vector'] = vColor.GetId()
        
        gridSizer.Add(item = vColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)

        # size
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Shape size"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Width:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        
        width = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                            min = 0, max = 500,
                            initial = self.settings.Get(group='modeler', key='data', subkey=('size', 'width')))
        width.SetName('GetValue')
        self.winId['modeler:data:size:width'] = width.GetId()
        
        gridSizer.Add(item = width,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        
        height = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                             min = 0, max = 500,
                             initial = self.settings.Get(group='modeler', key='data', subkey=('size', 'height')))
        height.SetName('GetValue')
        self.winId['modeler:data:size:height'] = height.GetId()
        
        gridSizer.Add(item = height,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
        
        panel.SetSizer(border)
        
        return panel

    def _createLoopPage(self, notebook):
        """!Create notebook page for loop settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Loop"))
        
        # colors
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Color"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Valid:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        vColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                   colour = self.settings.Get(group='modeler', key='loop', subkey=('color', 'valid')),
                                   size = globalvar.DIALOG_COLOR_SIZE)
        vColor.SetName('GetColour')
        self.winId['modeler:loop:color:valid'] = vColor.GetId()
        
        gridSizer.Add(item = vColor,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        # size
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Shape size"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)

        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Width:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        
        width = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                            min = 0, max = 500,
                            initial = self.settings.Get(group='modeler', key='loop', subkey=('size', 'width')))
        width.SetName('GetValue')
        self.winId['modeler:loop:size:width'] = width.GetId()
        
        gridSizer.Add(item = width,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        
        height = wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                             min = 0, max = 500,
                             initial = self.settings.Get(group='modeler', key='loop', subkey=('size', 'height')))
        height.SetName('GetValue')
        self.winId['modeler:loop:size:height'] = height.GetId()
        
        gridSizer.Add(item = height,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
                
        panel.SetSizer(border)
        
        return panel

    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        PreferencesBaseDialog.OnApply(self, event)
        
        self.parent.GetModel().Update()
        self.parent.GetCanvas().Refresh()

    def OnSave(self, event):
        """!Button 'Save' pressed"""
        PreferencesBaseDialog.OnSave(self, event)
        
        self.parent.GetModel().Update()
        self.parent.GetCanvas().Refresh()

class PropertiesDialog(wx.Dialog):
    """!Model properties dialog
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _('Model properties'),
                 size = (350, 400),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        wx.Dialog.__init__(self, parent, id, title, size = size,
                           style = style)
        
        self.metaBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label=" %s " % _("Metadata"))
        self.cmdBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                   label=" %s " % _("Commands"))
        
        self.name = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                size = (300, 25))
        self.desc = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                style = wx.TE_MULTILINE,
                                size = (300, 50))
        self.author = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                size = (300, 25))
        
        # commands
        self.overwrite = wx.CheckBox(parent = self, id=wx.ID_ANY,
                                     label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        
        # buttons
        self.btnOk     = wx.Button(self, wx.ID_OK)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk.SetDefault()
        
        self.btnOk.SetToolTipString(_("Apply properties"))
        self.btnOk.SetDefault()
        self.btnCancel.SetToolTipString(_("Close dialog and ignore changes"))
        
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self._layout()

    def _layout(self):
        metaSizer = wx.StaticBoxSizer(self.metaBox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 3, vgap = 3)
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label = _("Name:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 0))
        gridSizer.Add(item = self.name,
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (0, 1))
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label = _("Description:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 0))
        gridSizer.Add(item = self.desc,
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (1, 1))
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label = _("Author(s):")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 0))
        gridSizer.Add(item = self.author,
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos = (2, 1))
        gridSizer.AddGrowableCol(1)
        gridSizer.AddGrowableRow(1)
        metaSizer.Add(item = gridSizer, proportion = 1, flag = wx.EXPAND)
        
        cmdSizer = wx.StaticBoxSizer(self.cmdBox, wx.VERTICAL)
        cmdSizer.Add(item = self.overwrite,
                     flag = wx.EXPAND | wx.ALL, border = 3)
        
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.AddButton(self.btnOk)
        btnStdSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=metaSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=cmdSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def OnCloseWindow(self, event):
        self.Hide()
        
    def GetValues(self):
        """!Get values"""
        return { 'name'        : self.name.GetValue(),
                 'description' : self.desc.GetValue(),
                 'author'      : self.author.GetValue(),
                 'overwrite'   : self.overwrite.IsChecked() }
    
    def Init(self, prop):
        """!Initialize dialog"""
        self.name.SetValue(prop['name'])
        self.desc.SetValue(prop['description'])
        self.author.SetValue(prop['author'])
        if 'overwrite' in prop:
            self.overwrite.SetValue(prop['overwrite'])
