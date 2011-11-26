"""!
@package gcp.toolbars

@brief Georectification module - toolbars

Classes:
 - toolbars::GCPMapToolbar
 - toolbars::GCPDisplayToolbar

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Markus Metz
"""

import os
import sys

import wx

from core              import globalvar
from gui_core.toolbars import BaseToolbar

sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
from icon              import Icons
    
class GCPManToolbar(BaseToolbar):
    """!Toolbar for managing ground control points

    @param parent reference to GCP widget
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        icons = Icons['georectify']
        return self._getToolbarData((('gcpSave', icons["gcpSave"],
                                      self.parent.SaveGCPs),
                                     ('gcpReload', icons["gcpReload"],
                                      self.parent.ReloadGCPs),
                                     (None, ),
                                     ('gcpAdd', icons["gcpAdd"],
                                      self.parent.AddGCP),
                                     ('gcpDelete', icons["gcpDelete"],
                                      self.parent.DeleteGCP),
                                     ('gcpClear', icons["gcpClear"],
                                      self.parent.ClearGCP),
                                     (None, ),
                                     ('rms', icons["gcpRms"],
                                      self.parent.OnRMS),
                                     ('georect', icons["georectify"],
                                      self.parent.OnGeorect))
                                    )
    
class GCPDisplayToolbar(BaseToolbar):
    """!GCP Display toolbar
    """
    def __init__(self, parent):
        """!GCP Display toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # add tool to toggle active map window
        self.togglemapid = wx.NewId()
        self.togglemap = wx.Choice(parent = self, id = self.togglemapid,
                                   choices = [_('source'), _('target')],
                                   style = wx.CB_READONLY)

        self.InsertControl(10, self.togglemap)

        self.SetToolShortHelp(self.togglemapid, '%s %s %s' % (_('Set map canvas for '),
                                                              Icons['displayWindow']["zoomBack"].GetLabel(),
                                                              _(' / Zoom to map')))

        # realize the toolbar
        self.Realize()
        
        self.action = { 'id' : self.gcpset }
        self.defaultAction = { 'id' : self.gcpset,
                               'bind' : self.parent.OnPointer }
        
        self.OnTool(None)
        
        self.EnableTool(self.zoomback, False)
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['displayWindow']
        return self._getToolbarData((("displaymap", icons["display"],
                                      self.parent.OnDraw),
                                     ("rendermap", icons["render"],
                                      self.parent.OnRender),
                                     ("erase", icons["erase"],
                                      self.parent.OnErase),
                                     (None, ),
                                     ("gcpset", Icons["georectify"]["gcpSet"],
                                      self.parent.OnPointer),
                                     ("pan", icons["pan"],
                                      self.parent.OnPan),
                                     ("zoomin", icons["zoomIn"],
                                      self.parent.OnZoomIn),
                                     ("zoomout", icons["zoomOut"],
                                      self.parent.OnZoomOut),
                                     ("zoommenu", icons["zoomMenu"],
                                      self.parent.OnZoomMenuGCP),
                                     (None, ),
                                     ("zoomback", icons["zoomBack"],
                                      self.parent.OnZoomBack),
                                     ("zoomtomap", icons["zoomExtent"],
                                      self.parent.OnZoomToMap),
                                     (None, ),
                                     ('settings', Icons["georectify"]["settings"],
                                      self.parent.OnSettings),
                                     ('help', Icons["misc"]["help"],
                                      self.parent.OnHelp),
                                     (None, ),
                                     ('quit', Icons["georectify"]["quit"],
                                      self.parent.OnQuit))
                                    )
