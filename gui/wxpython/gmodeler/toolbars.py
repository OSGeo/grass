"""!
@package gmodeler.toolbars

@brief wxGUI Graphical Modeler toolbars classes

Classes:
 - toolbars::ModelerToolbar

(C) 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

import wx

from core              import globalvar
from core.utils import _
from gui_core.toolbars import BaseToolbar, BaseIcons

from icons.icon        import MetaIcon

class ModelerToolbar(BaseToolbar):
    """!Graphical modeler toolbaro (see gmodeler.py)
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = {
            'new'        : MetaIcon(img = 'create',
                                    label = _('Create new model (Ctrl+N)')),
            'open'       : MetaIcon(img = 'open',
                                    label = _('Load model from file (Ctrl+O)')),
            'save'       : MetaIcon(img = 'save',
                                    label = _('Save current model to file (Ctrl+S)')),
            'toImage'    : MetaIcon(img = 'image-export',
                                    label = _('Export model to image')),
            'toPython'   : MetaIcon(img = 'python-export',
                                    label = _('Export model to Python script')),
            'actionAdd'  : MetaIcon(img = 'module-add',
                                    label = _('Add command (GRASS module) to model')),
            'dataAdd'    : MetaIcon(img = 'data-add',
                                    label = _('Add data to model')),
            'relation'   : MetaIcon(img = 'relation-create',
                                    label = _('Manually define relation between data and commands')),
            'loop'       : MetaIcon(img = 'loop-add',
                                    label = _('Add loop/series')),
            'run'        : MetaIcon(img = 'execute',
                                    label = _('Run model')),
            'validate'   : MetaIcon(img = 'check',
                                    label = _('Validate model')),
            'settings'   : BaseIcons['settings'].SetLabel(_('Modeler settings')),
            'properties' : MetaIcon(img = 'options',
                                    label = _('Show model properties')),
            'variables'  : MetaIcon(img = 'modeler-variables',
                                    label = _('Manage model variables')),
            'redraw'     : MetaIcon(img = 'redraw',
                                    label = _('Redraw model canvas')),
            'quit'       : BaseIcons['quit'].SetLabel(_('Quit Graphical Modeler')),
            }
        
        return self._getToolbarData((('new', icons['new'],
                                      self.parent.OnModelNew),
                                     ('open', icons['open'],
                                      self.parent.OnModelOpen),
                                     ('save', icons['save'],
                                      self.parent.OnModelSave),
                                     ('image', icons['toImage'],
                                      self.parent.OnExportImage),
                                     ('python', icons['toPython'],
                                      self.parent.OnExportPython),
                                     (None, ),
                                     ('action', icons['actionAdd'],
                                      self.parent.OnAddAction),
                                     ('data', icons['dataAdd'],
                                      self.parent.OnAddData),
                                     ('relation', icons['relation'],
                                      self.parent.OnDefineRelation),
                                     ('loop', icons['loop'],
                                      self.parent.OnDefineLoop),
                                     (None, ),
                                     ('redraw', icons['redraw'],
                                      self.parent.OnCanvasRefresh),
                                     ('validate', icons['validate'],
                                      self.parent.OnValidateModel),
                                     ('run', icons['run'],
                                      self.parent.OnRunModel),
                                     (None, ),
                                     ("variables", icons['variables'],
                                      self.parent.OnVariables),
                                     ("settings", icons['settings'],
                                      self.parent.OnPreferences),
                                     ("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                     (None, ),
                                     ('quit', icons['quit'],
                                      self.parent.OnCloseWindow))
                                    )
