"""
@package nviz_preferences.py

@brief Nviz (3D view) preferences window

Classes:
 - NvizPreferencesDialog

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <KratochAnna seznam.cz> (Google SoC 2011)
"""

import types
import copy

import wx
import wx.lib.colourselect as csel

import globalvar
from preferences import globalSettings as UserSettings
from preferences import PreferencesBaseDialog

class NvizPreferencesDialog(PreferencesBaseDialog):
    """!Nviz preferences dialog"""
    def __init__(self, parent, title = _("3D view settings"),
                 settings = UserSettings):
        PreferencesBaseDialog.__init__(self, parent = parent, title = title,
                                       settings = settings)
        self.toolWin = self.parent.GetLayerManager().nviz
        self.win = dict()
        
        # create notebook pages
        self._createViewPage(self.notebook)
        self._createLightPage(self.notebook)
        self._createSurfacePage(self.notebook)
        self._createVectorPage(self.notebook)
        
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)
        
    def _createViewPage(self, notebook):
        """!Create notebook page for general settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        
        notebook.AddPage(page = panel,
                         text = " %s " % _("View"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['general'] = {}
        self.win['view'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # perspective
        self.win['view']['persp'] = {}
        pvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'persp')
        ipvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'persp', internal = True)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Perspective:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(value)")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        pval = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = pvals['value'],
                           min = ipvals['min'],
                           max = ipvals['max'])
        self.win['view']['persp']['value'] = pval.GetId()
        gridSizer.Add(item = pval, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(step)")),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        pstep = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = pvals['step'],
                           min = ipvals['min'],
                           max = ipvals['max']-1)
        self.win['view']['persp']['step'] = pstep.GetId()
        gridSizer.Add(item = pstep, pos = (0, 4),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # position
        self.win['view']['position'] = {}
        posvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'position')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Position:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(x)")),
                      pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        px = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = posvals['x'] * 100,
                           min = 0,
                           max = 100)
        self.win['view']['position']['x'] = px.GetId()
        gridSizer.Add(item = px, pos = (1, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "(y)"),
                      pos = (1, 3), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        py = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = posvals['y'] * 100,
                           min = 0,
                           max = 100)
        self.win['view']['position']['y'] = py.GetId()
        gridSizer.Add(item = py, pos = (1, 4),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # height
        self.win['view']['height'] = {}
        hvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'height')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(step)")),
                      pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        hstep = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = hvals['step'],
                           min = 1,
                           max = 1e6)
        self.win['view']['height']['step'] = hstep.GetId()
        gridSizer.Add(item = hstep, pos = (2, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # twist
        self.win['view']['twist'] = {}
        tvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'twist')
        itvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'twist', internal = True)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Twist:")),
                      pos = (3, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(value)")),
                      pos = (3, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        tval = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = tvals['value'],
                           min = itvals['min'],
                           max = itvals['max'])
        self.win['view']['twist']['value'] = tval.GetId()
        gridSizer.Add(item = tval, pos = (3, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(step)")),
                      pos = (3, 3), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        tstep = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = tvals['step'],
                           min = itvals['min'],
                           max = itvals['max']-1)
        self.win['view']['twist']['step'] = tstep.GetId()
        gridSizer.Add(item = tstep, pos = (3, 4),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # z-exag
        self.win['view']['z-exag'] = {}
        zvals = UserSettings.Get(group = 'nviz', key = 'view', subkey = 'z-exag')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Z-exag:")),
                      pos = (4, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(value)")),
                      pos = (4, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        zval = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           min = -1e6,
                           max = 1e6)
        self.win['view']['z-exag']['value'] = zval.GetId()
        gridSizer.Add(item = zval, pos = (4, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(step)")),
                      pos = (4, 3), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        zstep = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = zvals['step'],
                           min = -1e6,
                           max = 1e6)
        self.win['view']['z-exag']['step'] = zstep.GetId()
        gridSizer.Add(item = zstep, pos = (4, 4),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)

        box = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                           label = " %s " % (_("Image Appearance")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        gridSizer.AddGrowableCol(0)
        
        # background color
        self.win['view']['background'] = {}
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Background color:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'view',
                                                            subkey = ['background', 'color']),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetName("color")
        self.win['view']['background']['color'] = color.GetId()
        gridSizer.Add(item = color, pos = (0, 1))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
    
    def _createLightPage(self, notebook):
        """!Create notebook page for general settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        
        notebook.AddPage(page = panel,
                         text = " %s " % _("Lighting"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        self.win['light'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Light")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)

        
        # position
        self.win['light']['position'] = {}
        posvals = UserSettings.Get(group = 'nviz', key = 'light', subkey = 'position')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Position:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(x)")),
                      pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        px = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = posvals['x'] * 100,
                           min = -100,
                           max = 100)
        self.win['light']['position']['x'] = px.GetId()
        gridSizer.Add(item = px, pos = (0, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = "(y)"),
                      pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        py = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = posvals['y'] * 100,
                           min = -100,
                           max = 100)
        self.win['light']['position']['y'] = py.GetId()
        gridSizer.Add(item = py, pos = (0, 4),
                      flag = wx.ALIGN_CENTER_VERTICAL)
                    
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("(z)")),
                      pos = (0, 5), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)
        
        pz = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = posvals['z'],
                           min = 0,
                           max = 100)
        self.win['light']['position']['z'] = pz.GetId()
        gridSizer.Add(item = pz, pos = (0, 6),
                      flag = wx.ALIGN_CENTER_VERTICAL)
                    
        # brightness
        brightval = UserSettings.Get(group = 'nviz', key = 'light', subkey = 'bright')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Brightness:")),
                      pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        bright = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = brightval,
                           min = 0,
                           max = 100)
        self.win['light']['bright'] = bright.GetId()
        gridSizer.Add(item = bright, pos = (1, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
                    
        # ambient
        ambval = UserSettings.Get(group = 'nviz', key = 'light', subkey = 'ambient')
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Ambient:")),
                      pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        amb = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = ambval,
                           min = 0,
                           max = 100)
        self.win['light']['ambient'] = amb.GetId()
        gridSizer.Add(item = amb, pos = (2, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
                    
        # light color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Color:")),
                      pos = (3, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'light',
                                                            subkey = 'color'),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetName("color")
        self.win['light']['color'] = color.GetId()
        gridSizer.Add(item = color, pos = (3, 2))
        
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
    
    def _createSurfacePage(self, notebook):
        """!Create notebook page for general settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        
        notebook.AddPage(page = panel,
                         text = " %s " % _("Surface"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        self.win['surface'] = {}
        
        # draw
        
        self.win['surface']['draw'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # mode
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                        label = _("Mode:")), flag = wx.ALIGN_CENTER_VERTICAL,
                        pos = (0, 0))
        mode = wx.Choice(parent = panel, id = wx.ID_ANY, size = (-1, -1),
                          choices = [_("coarse"),
                                     _("fine"),
                                     _("both")])
        self.win['surface']['draw']['mode'] = mode.GetId()
        mode.SetName('selection')
        mode.SetSelection(UserSettings.Get(group = 'nviz', key = 'surface',
                                            subkey = ['draw', 'mode']))
        gridSizer.Add(item = mode, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 1))
                    
        # fine
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                        label = _("Fine mode:")), flag = wx.ALIGN_CENTER_VERTICAL,
                        pos = (1, 0))
        res = UserSettings.Get(group = 'nviz', key = 'surface', subkey = ['draw','res-fine'])
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                        label = _("resolution:")), flag = wx.ALIGN_CENTER_VERTICAL,
                        pos = (1, 1))
        fine = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = res,
                           min = 1,
                           max = 100)
        self.win['surface']['draw']['res-fine'] = fine.GetId()
        
        gridSizer.Add(item = fine, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 2))
                    
        # coarse
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                        label = _("Coarse mode:")), flag = wx.ALIGN_CENTER_VERTICAL,
                        pos = (2, 0))
        res = UserSettings.Get(group = 'nviz', key = 'surface', subkey = ['draw','res-coarse'])
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                        label = _("resolution:")), flag = wx.ALIGN_CENTER_VERTICAL,
                        pos = (2, 1))
        coarse = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                           initial = res,
                           min = 1,
                           max = 100)
        self.win['surface']['draw']['res-coarse'] = coarse.GetId()
        
        gridSizer.Add(item = coarse, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 2))
        #style
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
            label = _("style:")), flag = wx.ALIGN_CENTER_VERTICAL,
            pos = (3, 1))
        style = wx.Choice(parent = panel, id = wx.ID_ANY, size = (-1, -1),
                          choices = [_("wire"),
                                     _("surface")])
        self.win['surface']['draw']['style'] = style.GetId()
        style.SetName('selection')
        style.SetSelection(UserSettings.Get(group = 'nviz', key = 'surface',
                                            subkey = ['draw', 'style']))
        self.win['surface']['draw']['style'] = style.GetId()
        
        gridSizer.Add(item = style, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (3, 2))
        #wire color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
            label = _("wire color:")), flag = wx.ALIGN_CENTER_VERTICAL,
            pos = (4, 1))
        color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                  colour = UserSettings.Get(group = 'nviz', key = 'surface',
                                                            subkey = ['draw', 'wire-color']),
                                  size = globalvar.DIALOG_COLOR_SIZE)
        color.SetName("color")
        self.win['surface']['draw']['wire-color'] = color.GetId() 
        gridSizer.Add(item = color, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (4, 2))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)            
        pageSizer.Add(item = boxSizer, proportion = 0,
              flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
              border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
    
    def _createVectorPage(self, notebook):
        """!Create notebook page for general settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        
        notebook.AddPage(page = panel,
                         text = " %s " % _("Vector"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        
        # vector lines
        self.win['vector'] = {}
        self.win['vector']['lines'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # show
        row = 0
        showLines = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = _("Show lines"))
        self.win['vector']['lines']['show'] = showLines.GetId()
        showLines.SetValue(UserSettings.Get(group = 'nviz', key = 'vector',
                                            subkey = ['lines', 'show']))
        gridSizer.Add(item = showLines, pos = (row, 0))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        # vector points
        self.win['vector']['points'] = {}
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                            label = " %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap = 3, hgap = 5)
        
        # show
        row = 0
        showPoints = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Show points"))
        showPoints.SetValue(UserSettings.Get(group = 'nviz', key = 'vector',
                                             subkey = ['points', 'show']))
        self.win['vector']['points']['show'] = showPoints.GetId()
        gridSizer.Add(item = showPoints, pos = (row, 0), span = (1, 8))
        
        # icon size
        row += 1 
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Size:")),
                      pos = (row, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        isize = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 100,
                            min = 1,
                            max = 1e6)
        self.win['vector']['points']['size'] = isize.GetId()
        isize.SetValue(UserSettings.Get(group = 'nviz', key = 'vector',
                                        subkey = ['points', 'size']))
        gridSizer.Add(item = isize, pos = (row, 1),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # icon width
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Width:")),
                      pos = (row, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        
        iwidth = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (65, -1),
                            initial = 2,
                            min = 1,
                            max = 1e6)
        self.win['vector']['points']['width'] = isize.GetId()
        iwidth.SetValue(UserSettings.Get(group = 'nviz', key = 'vector',
                                         subkey = ['points', 'width']))
        gridSizer.Add(item = iwidth, pos = (row, 3),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # icon symbol
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Marker:")),
                      pos = (row, 4), flag = wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice (parent = panel, id = wx.ID_ANY, size = (100, -1),
                          choices = UserSettings.Get(group = 'nviz', key = 'vector',
                                                   subkey = ['points', 'marker'], internal = True))
        isym.SetName("selection")
        self.win['vector']['points']['marker'] = isym.GetId()
        isym.SetSelection(UserSettings.Get(group = 'nviz', key = 'vector',
                                           subkey = ['points', 'marker']))
        gridSizer.Add(item = isym, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 5))
        
        # icon color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Color:")),
                      pos = (row, 6), flag = wx.ALIGN_CENTER_VERTICAL)
        icolor = csel.ColourSelect(panel, id = wx.ID_ANY)
        icolor.SetName("color")
        self.win['vector']['points']['color'] = icolor.GetId()
        icolor.SetColour(UserSettings.Get(group = 'nviz', key = 'vector',
                                          subkey = ['points', 'color']))
        gridSizer.Add(item = icolor, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 7))
        
        boxSizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        pageSizer.Add(item = boxSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border = 3)
        
        panel.SetSizer(pageSizer)
        
        return panel
    
    def _getValue(self, win):
        if win.GetName() == "selection":
            value = win.GetSelection()
        elif win.GetName() == "color":
            value = tuple(win.GetColour())
        else:
            value = win.GetValue()
            
        return value
        
    def OnDefault(self, event):
        """Restore default settings"""
        settings = copy.deepcopy(UserSettings.GetDefaultSettings()['nviz'])
        UserSettings.Set(group = 'nviz',
                         value = settings)
        
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            if subgroup != 'view':
                continue
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    win = self.FindWindowById(self.win[subgroup][subkey][subvalue])
                    val = settings[subgroup][subkey][subvalue]
                    if subkey == 'position':
                        if val < 1:
                            val = int(val * 100)
                    
                    win.SetValue(val)
        
        event.Skip()
        
    def OnApply(self, event):
        """Apply Nviz settings for current session"""
        settings = UserSettings.Get(group = 'nviz')
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                if type(value) == types.DictType:
                    for subvalue in value.keys():
                        try:
                            win = self.FindWindowById(self.win[subgroup][subkey][subvalue])
                        except KeyError, e:
                            #print "error", str(e)+ str(subgroup)+str(subkey)+str(subvalue)
                            continue
                        value = self._getValue(win)
                        if subkey == 'position':
                            if subvalue in ('x', 'y'):
                                value = float(value) / 100
                        settings[subgroup][subkey][subvalue] = value
                else:
                    try:
                        win = self.FindWindowById(self.win[subgroup][subkey])
                    except KeyError, e:
                        #print "error", str(subgroup)+" "+str(subkey)+" "+str(value)
                        continue
                    value = self._getValue(win)
                    settings[subgroup][subkey] = value
                    
        
        
    def OnLoad(self, event):
        """!Apply button pressed"""
        self.LoadSettings()
        
        if event:
            event.Skip()

    def LoadSettings(self):
        """!Load saved Nviz settings and apply to current session"""
        UserSettings.ReadSettingsFile()
        settings = copy.deepcopy(UserSettings.Get(group = 'nviz'))
        
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    if subvalue == 'step':
                        continue
                    else:
                        insetting = value[subvalue]                                                    
                    if subgroup == 'view':
                        for viewkey, viewitem in self.mapWindow.view[subkey].iteritems(): 
                            if viewkey == subvalue:
                                self.mapWindow.view[subkey][viewkey] = insetting 
                            else:
                                continue
                    else:
                        for otherkey, otheritem in self.win[subgroup][subkey].iteritems():
                            if type(otheritem) == data:
                                for endkey, enditem in otheritem.iteritems():
                                    if endkey == subvalue:
                                        paramwin = self.FindWindowById(enditem)
                                    else:
                                        continue
                            else:
                                if otherkey == subvalue:
                                    paramwin = self.FindWindowById(otheritem)
                                else:
                                    continue
                            if type(insetting) in [tuple, list] and len(insetting) > 2:
                                insetting = tuple(insetting)
                                paramwin.SetColour(insetting)
                            else:
                                try:
                                    paramwin.SetValue(insetting)
                                except:
                                    try:
                                        paramwin.SetStringSelection(insetting)
                                    except:
                                        continue
                                
        self.toolWin.UpdateSettings()
        self.FindWindowById(self.win['view']['position']).Draw()
        self.FindWindowById(self.win['view']['position']).Refresh(False)
        
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)
        
    def OnSave(self, event):
        """!Save button pressed
        
        Save settings to configuration file
        """
        self.OnApply(None)
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings = fileSettings)
        fileSettings['nviz'] = UserSettings.Get(group = 'nviz')
        
        UserSettings.SaveToFile(fileSettings)
        self.parent.GetLayerManager().goutput.WriteLog(
                _('3D view settings saved to file <%s>.') % UserSettings.filePath)
        
        self.Destroy()
        
