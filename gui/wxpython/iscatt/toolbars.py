"""!
@package iscatt.toolbars

@brief Scatter plot - toolbars

Classes:
 - toolbars::MainToolbar

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import wx

from icons.icon import MetaIcon
from gui_core.toolbars import BaseToolbar, BaseIcons
from core.gcmd import RunCommand
from core.gcmd import GException, GError, RunCommand
from iscatt.iscatt_core import idBandsToidScatt
from iscatt.dialogs import SettingsDialog

class MainToolbar(BaseToolbar):
    """!Main toolbar
    """
    def __init__(self, parent, scatt_mgr, opt_tools=None):
        BaseToolbar.__init__(self, parent)
        self.scatt_mgr = scatt_mgr
        self.opt_tools = opt_tools

        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        self.scatt_mgr.modeSet.connect(self.ModeSet)

    def _toolbarData(self):

        icons = {
                'selectGroup' : MetaIcon(img = 'layer-group-add',
                                 label = _('Select imagery group')),
                'settings'   : BaseIcons['settings'].SetLabel( _('Settings')),
                'help'       : MetaIcon(img = 'help',
                                         label = _('Show manual')),
                'add_scatt_pl'  : MetaIcon(img = 'layer-raster-analyze',
                                            label = _('Add scatter plot')),
                'selCatPol'  : MetaIcon(img = 'polygon',
                                      label = _('Select area with polygon')),
                'pan'        : MetaIcon(img = 'pan',
                                         label = _('Pan mode for scatter plots')),
                'zoomIn'     : MetaIcon(img = 'zoom-in',
                                        label = _('Zoom mode for scatter plots (left mouse button, wheel)')),
                'zoomExtent' : MetaIcon(img = 'zoom-extent',
                                       label = _('Zoom to scatter plot data extend mode (click on scatter plot for zooming to extend)')),
                'cats_mgr' : MetaIcon(img = 'table-manager',
                                          label = _('Show/hide class manager'))
                }
            
        tools = [
                    ('add_scatt', icons["add_scatt_pl"],
                    lambda event : self.scatt_mgr.AddScattPlot()),
                    (None, ),
                    ("cats_mgr", icons['cats_mgr'],
                    lambda event: self.parent.ShowCategoryPanel(event.Checked()), wx.ITEM_CHECK),
                    (None, ),
                    ("pan", icons["pan"],
                    lambda event: self.SetPloltsMode(event, 'pan'),
                    wx.ITEM_CHECK),
                    ("zoom", icons["zoomIn"],
                    lambda event: self.SetPloltsMode(event, 'zoom'),
                    wx.ITEM_CHECK),
                    ("zoom_extend", icons["zoomExtent"],
                    lambda event: self.SetPloltsMode(event, 'zoom_extend'),
                    wx.ITEM_CHECK),
                    (None, ),
                    ('sel_pol_mode', icons['selCatPol'],
                    self.ActivateSelectionPolygonMode,
                    wx.ITEM_CHECK),
                    (None, ),
                    ('settings', icons["settings"],
                    self.OnSettings),
                    ('help', icons["help"],
                     self.OnHelp)                    
                ]

        if self.opt_tools and "add_group" in self.opt_tools:
            tools.insert(0, ("selectGroup", icons['selectGroup'],
                             lambda event : self.scatt_mgr.SetData()))

        return self._getToolbarData(tools)

    def GetToolId(self, toolName): #TODO can be useful in base
        return vars(self)[toolName]            

    def SetPloltsMode(self, event, tool_name):
        self.scatt_mgr.modeSet.disconnect(self.ModeSet)
        if event.Checked()  == True:
            for i_tool_data in  self._data:
                i_tool_name = i_tool_data[0]
                if not i_tool_name or i_tool_name in ["cats_mgr", "sel_pol_mode"]:
                    continue
                if i_tool_name == tool_name:
                    continue
                i_tool_id = vars(self)[i_tool_name]
                self.ToggleTool(i_tool_id, False)

            self.scatt_mgr.SetPlotsMode(mode = tool_name)
        else:
            self.scatt_mgr.SetPlotsMode(mode = None)
        self.scatt_mgr.modeSet.connect(self.ModeSet)

    def ActivateSelectionPolygonMode(self, event):

        activated = self.scatt_mgr.ActivateSelectionPolygonMode(event.Checked())
        self.parent.ShowPlotEditingToolbar(activated)

        i_tool_id = vars(self)['sel_pol_mode']
        self.ToggleTool(i_tool_id, activated)

    def ModeSet(self, mode):
        self.UnsetMode()

    def UnsetMode(self):
        for i_tool_data in  self._data:
                i_tool_name = i_tool_data[0]
                if not i_tool_name or i_tool_name in ["cats_mgr", "sel_pol_mode"]:
                    continue
                i_tool_id = vars(self)[i_tool_name]
                self.ToggleTool(i_tool_id, False)

    def OnSettings(self, event):
        dlg = SettingsDialog(parent=self, id=wx.ID_ANY, 
                             title=_('Settings'), scatt_mgr = self.scatt_mgr)
        
        dlg.ShowModal()
        dlg.Destroy()

    def OnHelp(self, event) :
            RunCommand('g.manual',
                       entry = 'wxGUI.iscatt')

class EditingToolbar(BaseToolbar):
    """!Main toolbar
    """
    def __init__(self, parent, scatt_mgr):
        BaseToolbar.__init__(self, parent)
        self.scatt_mgr = scatt_mgr

        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        self.scatt_mgr.modeSet.connect(self.ModeSet)

    def _toolbarData(self):
        """!Toolbar data
        """
        self.icons = {
            'sel_add'         : MetaIcon(img = 'layer-add',
                                         label = _('Include selected area to class'),
                                         desc = _('Include selected area to class')),
            'sel_remove'      : MetaIcon(img = 'layer-remove',
                                         label = _('Exclude selected area from class'),
                                         desc = _('Exclude selected area from class')),
            'addVertex'       : MetaIcon(img = 'vertex-create',
                                         label = _('Add new vertex'),
                                         desc = _('Add new vertex to polygon boundary scatter plot')),
            'editLine'        : MetaIcon(img = 'polygon-create',
                                         label = _('Create selection polygon'),
                                         desc = _('Add new vertex between last and first points of the boundary')),
            'moveVertex'      : MetaIcon(img = 'vertex-move',
                                         label = _('Move vertex'),
                                         desc = _('Move boundary vertex')),
            'removeVertex'    : MetaIcon(img = 'vertex-delete',
                                         label = _('Remove vertex'),
                                         desc = _('Remove boundary vertex')),
            'delete'        : MetaIcon(img = 'polygon-delete',
                                       label = _("Remove polygon (click on scatter plot for removing it's polygon)")),
            }

        return self._getToolbarData((
                                    ("sel_add", self.icons["sel_add"],
                                     lambda event: self.scatt_mgr.ProcessSelectionPolygons('add')),
                                     ("sel_remove", self.icons['sel_remove'],
                                     lambda event: self.scatt_mgr.ProcessSelectionPolygons('remove')),
                                     (None, ),
                                     ("add_vertex", self.icons["editLine"],
                                     lambda event: self.SetMode(event, 'add_vertex'),
                                     wx.ITEM_CHECK),
                                     ("add_boundary_vertex", self.icons['addVertex'],
                                     lambda event: self.SetMode(event, 'add_boundary_vertex'),
                                     wx.ITEM_CHECK),
                                     ("move_vertex", self.icons["moveVertex"],
                                     lambda event: self.SetMode(event, 'move_vertex'),
                                     wx.ITEM_CHECK),
                                     ('delete_vertex', self.icons['removeVertex'],
                                     lambda event: self.SetMode(event, 'delete_vertex'),
                                     wx.ITEM_CHECK),
                                     ('remove_polygon', self.icons['delete'],
                                     lambda event: self.SetMode(event, 'remove_polygon'),
                                     wx.ITEM_CHECK)
                                    ))

    def SetMode(self, event, tool_name):
        self.scatt_mgr.modeSet.disconnect(self.ModeSet)
        if event.Checked() == True:
            for i_tool_data in  self._data:
                i_tool_name = i_tool_data[0]
                if not i_tool_name:
                    continue
                if i_tool_name == tool_name:
                    continue
                i_tool_id = vars(self)[i_tool_name]
                self.ToggleTool(i_tool_id, False)
            self.scatt_mgr.SetPlotsMode(tool_name)
        else:
            self.scatt_mgr.SetPlotsMode(None)
        self.scatt_mgr.modeSet.connect(self.ModeSet)

    def ModeSet(self, mode):

        if mode in ['zoom', 'pan', 'zoom_extend', None]:
            self.UnsetMode()

    def UnsetMode(self):
        for i_tool_data in  self._data:
                i_tool_name = i_tool_data[0]
                if not i_tool_name:
                    continue
                i_tool_id = vars(self)[i_tool_name]
                self.ToggleTool(i_tool_id, False)

    def GetToolId(self, toolName):
        return vars(self)[toolName]

class CategoryToolbar(BaseToolbar):
    """!Main toolbar
    """
    def __init__(self, parent, scatt_mgr, cats_list):
        BaseToolbar.__init__(self, parent)
        self.scatt_mgr = scatt_mgr
        self.cats_mgr = self.scatt_mgr.GetCategoriesManager()
        self.cats_list = cats_list

        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        self.icons = {
            'add_class'     : MetaIcon(img = 'layer-add',
                                       label = _('Add class')),
            'remove_class'  : MetaIcon(img = 'layer-remove',
                                       label = _('Remove selected class'))
            }

        return self._getToolbarData((
                                    ("add_class", self.icons["add_class"],
                                     lambda event: self.cats_mgr.AddCategory()),
                                     ("remove_class", self.icons['remove_class'],
                                     lambda event: self.cats_list.DeleteCategory()),
                                    ))