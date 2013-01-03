"""!
@package vnet.toolbars

@brief Vector network analysis dialog - toolbars

Classes:
 - toolbars::PointListToolbar
 - toolbars::MainToolbar

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import wx

from icon              import MetaIcon
from gui_core.toolbars import BaseToolbar, BaseIcons
from core.gcmd         import RunCommand
class PointListToolbar(BaseToolbar):
    """!Toolbar for managing list of points

    @param parent reference to VNETDialog
    """
    def __init__(self, parent, list):
        BaseToolbar.__init__(self, parent)
        self.list = list
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()


    def _toolbarData(self):

        icons = {
            'insertPoint'  : MetaIcon(img = 'pointer',
                                    label = _('Insert points from Map Display')),
            'snapping'  : MetaIcon(img = 'move',
                                    label = _('Activate snapping to nodes')),
            'pointAdd'     : MetaIcon(img = 'point-create',
                                    label = _('Add new point')),
            'pointDelete'  : MetaIcon(img = 'gcp-delete',
                                    label = _('Delete selected point'))
            }

        return  self._getToolbarData((('insertPoint', icons['insertPoint'],
                                      self.list.dialog.OnInsertPoint,#TODO self.list.dialog
                                      wx.ITEM_CHECK),
                                      ('snapping', icons['snapping'],
                                      self.list.dialog.OnSnapping,
                                      wx.ITEM_CHECK),
                                      (None, ),
                                     ('pointAdd', icons["pointAdd"],
                                        self.list.AddItem),
                                     ('pointDelete', icons["pointDelete"],
                                        self.list.DeleteItem)))
                                    
    def GetToolId(self, toolName): #TODO can be useful in base

        return vars(self)[toolName]

class MainToolbar(BaseToolbar):
    """!Main toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())

        choices = []

        for moduleName in self.parent.vnetModulesOrder:
            choices.append(self.parent.vnetParams[moduleName]['label'])

        self.UpdateUndoRedo()
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):

        icons = {
                 'run' : MetaIcon(img = 'execute',
                                  label = _('Execute analysis')),
                 'undo' : MetaIcon(img = 'undo',
                                  label = _('Go to previous analysis result')),
                 'redo' : MetaIcon(img = 'redo',
                                  label = _('Go to next analysis result')),
                 'showResult'   : MetaIcon(img = 'layer-add',
                                    label = _("Show analysis result")),
                 'saveTempLayer' : MetaIcon(img = 'map-export',
                                             label = _('Save temporary result')),
                 'settings' : BaseIcons['settings'].SetLabel( _('Vector network analysis settings')),
                 'help'       : MetaIcon(img = 'help',
                                         label = _('Show manual'))
                }

        return self._getToolbarData((
                                     ("run", icons['run'],
                                      self.parent.OnAnalyze),
                                     (None, ),     
                                     ("undo", icons['undo'], 
                                      self.parent.OnUndo), 
                                     ("redo", icons['redo'], 
                                      self.parent.OnRedo),
                                     (None, ),
                                     ("showResult", icons['showResult'], 
                                      self.parent.OnShowResult, wx.ITEM_CHECK), 
                                     ("saveTempLayer", icons['saveTempLayer'],
                                      self.parent.OnSaveTmpLayer),
                                     (None, ),
                                     ('settings', icons["settings"],
                                      self.parent.OnSettings),  
                                     ('help', icons["help"],
                                      self.OnHelp),                                     
                                     ("quit", BaseIcons['quit'],
                                      self.parent.OnCloseDialog)
                                    ))


    def UpdateUndoRedo(self):

        id = vars(self)['showResult']
        self.ToggleTool(id =id,
                        toggle = True)

        if self.parent.history.GetCurrHistStep() >= self.parent.history.GetStepsNum():
           self.Enable("undo", False)
        else:
           self.Enable("undo", True)

        if self.parent.history.GetCurrHistStep() <= 0:
           self.Enable("redo", False)
        else:
           self.Enable("redo", True)  

    def OnHelp(self, event) :
            RunCommand('g.manual',
                       entry = 'wxGUI.VNet')

class AnalysisToolbar(BaseToolbar):
    """!Main toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())

        choices = []

        for moduleName in self.parent.vnetModulesOrder:
            choices.append(self.parent.vnetParams[moduleName]['label'])

        self.anChoice = wx.ComboBox(parent = self, id = wx.ID_ANY,
                                    choices = choices,
                                    style = wx.CB_READONLY, size = (350, 30))#FIXME
        self.anChoice.SetToolTipString(_('Availiable analyses'))
        self.anChoice.SetSelection(0)
               
        self.anChoiceId = self.AddControl(self.anChoice)
        self.parent.Bind(wx.EVT_COMBOBOX, self.parent.OnAnalysisChanged, self.anChoiceId)
                
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.anChoice.Hide()
        self.anChoice.Show()
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):

        icons = {}

        return self._getToolbarData(())