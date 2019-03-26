"""
@package nviz.preferences

@brief Nviz (3D view) preferences window

Classes:
 - preferences::NvizPreferencesDialog

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <KratochAnna seznam.cz> (Google SoC 2011)
"""

import os
import copy

import wx
import wx.lib.colourselect as csel

from core import globalvar
from core.settings import UserSettings
from gui_core.preferences import PreferencesBaseDialog
from gui_core.wrap import SpinCtrl, Button, CheckBox, StaticText, \
    StaticBox


class NvizPreferencesDialog(PreferencesBaseDialog):
    """Nviz preferences dialog"""

    def __init__(self, parent, giface, title=_("3D view default settings"),
                 settings=UserSettings):
        PreferencesBaseDialog.__init__(
            self,
            parent=parent,
            title=title,
            giface=giface,
            settings=settings)
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    globalvar.ICONDIR,
                    'grass_nviz.ico'),
                wx.BITMAP_TYPE_ICO))

        self.toolWin = self.parent.nviz

        # create notebook pages
        self._createViewPage(self.notebook)
        self._createFlyPage(self.notebook)
        self._createLightPage(self.notebook)
        self._createSurfacePage(self.notebook)
        self._createVectorPage(self.notebook)

        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)
        self.btnDefault.SetToolTip(
            _("Revert settings to default, changes are not applied"))

    def _createViewPage(self, notebook):
        """Create notebook page for view settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)

        notebook.AddPage(page=panel,
                         text=" %s " % _("View"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)
        row = 0
        # perspective
        pvals = UserSettings.Get(group='nviz', key='view', subkey='persp')
        ipvals = UserSettings.Get(
            group='nviz',
            key='view',
            subkey='persp',
            settings_type='internal')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Perspective:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("value:")), pos=(
                row, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        pval = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                        initial=pvals['value'],
                        min=ipvals['min'],
                        max=ipvals['max'])
        self.winId['nviz:view:persp:value'] = pval.GetId()
        gridSizer.Add(pval, pos=(row, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("step:")), pos=(
                row, 3), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        pstep = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                         initial=pvals['step'],
                         min=ipvals['min'],
                         max=ipvals['max'] - 1)
        self.winId['nviz:view:persp:step'] = pstep.GetId()
        gridSizer.Add(pstep, pos=(row, 4),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        row += 1

        # position
        posvals = UserSettings.Get(group='nviz', key='view', subkey='position')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Position:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("x:")), pos=(
                row, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        px = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                      initial=posvals['x'] * 100,
                      min=0,
                      max=100)
        self.winId['nviz:view:position:x'] = px.GetId()
        gridSizer.Add(px, pos=(row, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label="y:"), pos=(
                row, 3), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        py = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                      initial=posvals['y'] * 100,
                      min=0,
                      max=100)
        self.winId['nviz:view:position:y'] = py.GetId()
        gridSizer.Add(py, pos=(row, 4),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        row += 1

        # height is computed dynamically

        # twist
        tvals = UserSettings.Get(group='nviz', key='view', subkey='twist')
        itvals = UserSettings.Get(
            group='nviz',
            key='view',
            subkey='twist',
            settings_type='internal')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Twist:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("value:")), pos=(
                row, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        tval = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                        initial=tvals['value'],
                        min=itvals['min'],
                        max=itvals['max'])
        self.winId['nviz:view:twist:value'] = tval.GetId()
        gridSizer.Add(tval, pos=(row, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        row += 1

        # z-exag
        zvals = UserSettings.Get(group='nviz', key='view', subkey='z-exag')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Z-exag:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("value:")), pos=(
                row, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        zval = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                        initial=zvals['value'],
                        min=-1e6,
                        max=1e6)
        self.winId['nviz:view:z-exag:value'] = zval.GetId()
        gridSizer.Add(zval, pos=(row, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=3)

        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Image Appearance")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # background color
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Background color:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        color = csel.ColourSelect(
            panel,
            id=wx.ID_ANY,
            colour=UserSettings.Get(
                group='nviz',
                key='view',
                subkey=[
                    'background',
                    'color']),
            size=globalvar.DIALOG_COLOR_SIZE)
        color.SetName('GetColour')
        self.winId['nviz:view:background:color'] = color.GetId()
        gridSizer.Add(color, pos=(0, 1))

        gridSizer.AddGrowableCol(0)
        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel

    def _createFlyPage(self, notebook):
        """Create notebook page for view settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)

        notebook.AddPage(page=panel,
                         text=" %s " % _("Fly-through"))
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        # fly throuhg mode
        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Fly-through mode")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # move exag
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Move exag:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        moveExag = SpinCtrl(
            panel, id=wx.ID_ANY, min=1, max=20, initial=UserSettings.Get(
                group='nviz', key='fly', subkey=[
                    'exag', 'move']), size=(
                65, -1))
        self.winId['nviz:fly:exag:move'] = moveExag.GetId()
        gridSizer.Add(moveExag, pos=(0, 1))

        # turn exag
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Turn exag:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        turnExag = SpinCtrl(
            panel, id=wx.ID_ANY, min=1, max=20, initial=UserSettings.Get(
                group='nviz', key='fly', subkey=[
                    'exag', 'turn']), size=(
                65, -1))
        self.winId['nviz:fly:exag:turn'] = turnExag.GetId()
        gridSizer.Add(turnExag, pos=(1, 1))

        gridSizer.AddGrowableCol(0)
        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel

    def _createLightPage(self, notebook):
        """Create notebook page for light settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)

        notebook.AddPage(page=panel,
                         text=" %s " % _("Lighting"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Light")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # position
        posvals = UserSettings.Get(
            group='nviz', key='light', subkey='position')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Position:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("x:")), pos=(
                0, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        px = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                      initial=posvals['x'] * 100,
                      min=-100,
                      max=100)
        self.winId['nviz:light:position:x'] = px.GetId()
        gridSizer.Add(px, pos=(0, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label="y:"), pos=(
                0, 3), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        py = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                      initial=posvals['y'] * 100,
                      min=-100,
                      max=100)
        self.winId['nviz:light:position:y'] = py.GetId()
        gridSizer.Add(py, pos=(0, 4),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(
            StaticText(
                parent=panel, id=wx.ID_ANY, label=_("z:")), pos=(
                0, 5), flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        pz = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                      initial=posvals['z'],
                      min=0,
                      max=100)
        self.winId['nviz:light:position:z'] = pz.GetId()
        gridSizer.Add(pz, pos=(0, 6),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # brightness
        brightval = UserSettings.Get(
            group='nviz', key='light', subkey='bright')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Brightness:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        bright = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                          initial=brightval,
                          min=0,
                          max=100)
        self.winId['nviz:light:bright'] = bright.GetId()
        gridSizer.Add(bright, pos=(1, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # ambient
        ambval = UserSettings.Get(group='nviz', key='light', subkey='ambient')
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Ambient:")),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        amb = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                       initial=ambval,
                       min=0,
                       max=100)
        self.winId['nviz:light:ambient'] = amb.GetId()
        gridSizer.Add(amb, pos=(2, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # light color
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Color:")),
                      pos=(3, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        color = csel.ColourSelect(
            panel, id=wx.ID_ANY, colour=UserSettings.Get(
                group='nviz', key='light', subkey='color'),
            size=globalvar.DIALOG_COLOR_SIZE)
        color.SetName('GetColour')
        self.winId['nviz:light:color'] = color.GetId()
        gridSizer.Add(color, pos=(3, 2))

        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel

    def _createSurfacePage(self, notebook):
        """Create notebook page for surface settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)

        notebook.AddPage(page=panel,
                         text=" %s " % _("Surface"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        # draw

        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # mode
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Mode:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                0,
                0))
        mode = wx.Choice(parent=panel, id=wx.ID_ANY, size=(-1, -1),
                         choices=[_("coarse"),
                                  _("fine"),
                                  _("both")])
        self.winId['nviz:surface:draw:mode'] = mode.GetId()
        mode.SetName('GetSelection')
        mode.SetSelection(UserSettings.Get(group='nviz', key='surface',
                                           subkey=['draw', 'mode']))
        gridSizer.Add(mode, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 1))

        # fine
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Fine mode:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                1,
                0))
        res = UserSettings.Get(
            group='nviz', key='surface', subkey=[
                'draw', 'res-fine'])
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("resolution:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                1,
                1))
        fine = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                        initial=res,
                        min=1,
                        max=100)
        self.winId['nviz:surface:draw:res-fine'] = fine.GetId()

        gridSizer.Add(fine, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 2))

        # coarse
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("Coarse mode:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                2,
                0))
        res = UserSettings.Get(
            group='nviz', key='surface', subkey=[
                'draw', 'res-coarse'])
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("resolution:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                2,
                1))
        coarse = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                          initial=res,
                          min=1,
                          max=100)
        self.winId['nviz:surface:draw:res-coarse'] = coarse.GetId()

        gridSizer.Add(coarse, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 2))
        # style
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("style:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                3,
                1))
        style = wx.Choice(parent=panel, id=wx.ID_ANY, size=(-1, -1),
                          choices=[_("wire"),
                                   _("surface")])
        self.winId['nviz:surface:draw:style'] = style.GetId()
        style.SetName('GetSelection')
        style.SetSelection(UserSettings.Get(group='nviz', key='surface',
                                            subkey=['draw', 'style']))
        self.winId['nviz:surface:draw:style'] = style.GetId()

        gridSizer.Add(style, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(3, 2))
        # wire color
        gridSizer.Add(
            StaticText(
                parent=panel,
                id=wx.ID_ANY,
                label=_("wire color:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                4,
                1))
        color = csel.ColourSelect(
            panel, id=wx.ID_ANY, colour=UserSettings.Get(
                group='nviz', key='surface', subkey=[
                    'draw', 'wire-color']), size=globalvar.DIALOG_COLOR_SIZE)
        color.SetName('GetColour')
        self.winId['nviz:surface:draw:wire-color'] = color.GetId()
        gridSizer.Add(color, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(4, 2))

        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel

    def _createVectorPage(self, notebook):
        """Create notebook page for vector settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)

        notebook.AddPage(page=panel,
                         text=" %s " % _("Vector"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        # vector lines
        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        row = 0
        # icon size
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Width:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        iwidth = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                          initial=12,
                          min=1,
                          max=100)
        self.winId['nviz:vector:lines:width'] = iwidth.GetId()
        iwidth.SetValue(UserSettings.Get(group='nviz', key='vector',
                                         subkey=['lines', 'width']))
        gridSizer.Add(iwidth, pos=(row, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # icon color
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Color:")),
                      pos=(row, 4), flag=wx.ALIGN_CENTER_VERTICAL)
        icolor = csel.ColourSelect(panel, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_COLOR_SIZE)
        icolor.SetName('GetColour')
        self.winId['nviz:vector:lines:color'] = icolor.GetId()
        icolor.SetColour(UserSettings.Get(group='nviz', key='vector',
                                          subkey=['lines', 'color']))
        gridSizer.Add(icolor, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 5))
        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        # vector points
        box = StaticBox(parent=panel, id=wx.ID_ANY,
                        label=" %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=5)

        row = 0
        # icon size
        autosize = CheckBox(parent=panel, label=_("Automatic size"))
        autosize.SetToolTip(_("Icon size is set automatically based on landscape dimensions."))
        gridSizer.Add(autosize, pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self.winId['nviz:vector:points:autosize'] = autosize.GetId()
        autosize.SetValue(UserSettings.Get(group='nviz', key='vector',
                                           subkey=['points', 'autosize']))

        row += 1
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Size:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        isize = SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                         initial=100,
                         min=1,
                         max=1e6)
        self.winId['nviz:vector:points:size'] = isize.GetId()
        isize.SetValue(UserSettings.Get(group='nviz', key='vector',
                                        subkey=['points', 'size']))
        gridSizer.Add(isize, pos=(row, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # icon symbol
        row += 1
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Marker:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice(
            parent=panel, id=wx.ID_ANY, size=(100, -1),
            choices=UserSettings.Get(
                group='nviz', key='vector', subkey=['points', 'marker'],
                settings_type='internal'))
        isym.SetName("GetSelection")
        self.winId['nviz:vector:points:marker'] = isym.GetId()
        isym.SetSelection(UserSettings.Get(group='nviz', key='vector',
                                           subkey=['points', 'marker']))
        gridSizer.Add(isym, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))

        # icon color
        row += 1
        gridSizer.Add(StaticText(parent=panel, id=wx.ID_ANY,
                                 label=_("Color:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        icolor = csel.ColourSelect(panel, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_COLOR_SIZE)
        icolor.SetName('GetColour')
        self.winId['nviz:vector:points:color'] = icolor.GetId()
        icolor.SetColour(UserSettings.Get(group='nviz', key='vector',
                                          subkey=['points', 'color']))
        gridSizer.Add(icolor, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))

        boxSizer.Add(gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=5)
        pageSizer.Add(boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel

    def OnDefault(self, event):
        """Button 'Set to default' pressed"""
        self.settings.userSettings = copy.deepcopy(
            self.settings.defaultSettings)

        # update widgets
        for gks in self.winId.keys():
            subkey1 = None
            try:
                group, key, subkey = gks.split(':')
                value = self.settings.Get(group, key, subkey)
            except ValueError:
                group, key, subkey, subkey1 = gks.split(':')
                value = self.settings.Get(group, key, [subkey, subkey1])
            if subkey == 'position':
                if subkey1 in ('x', 'y'):
                    value = float(value) * 100
            win = self.FindWindowById(self.winId[gks])
            if win.GetName() == 'GetSelection':
                value = win.SetSelection(value)
            else:
                value = win.SetValue(value)

    def OnApply(self, event):
        """Apply Nviz settings for current session"""
        for item in self.winId.keys():
            try:
                group, key, subkey = item.split(':')
                subkey1 = None
            except ValueError:
                group, key, subkey, subkey1 = item.split(':')

            id = self.winId[item]
            win = self.FindWindowById(id)
            if win.GetName() == 'GetSelection':
                value = win.GetSelection()
            elif win.GetName() == 'GetColour':
                value = tuple(win.GetValue())
            else:
                value = win.GetValue()

            if subkey == 'position':
                if subkey1 in ('x', 'y'):
                    value = float(value) / 100
            if subkey1:
                self.settings.Set(group, value, key, [subkey, subkey1])
            else:
                self.settings.Set(group, value, key, subkey)

        self.toolWin.LoadSettings()

    def OnSave(self, event):
        """Save button pressed

        Apply changes and save settings to configuration file
        """
        self.OnApply(None)
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['nviz'] = UserSettings.Get(group='nviz')

        UserSettings.SaveToFile(fileSettings)
        self.parent._gconsole.WriteLog(
            _('3D view settings saved to file <%s>.') % UserSettings.filePath)

        self.Destroy()
