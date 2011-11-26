"""!
@package psmap.toolbars

@brief wxPsMap toolbars classes

Classes:
 - toolbars::PsMapToolbar

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys

import wx

from core              import globalvar
from gui_core.toolbars import BaseToolbar

sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
from icon              import Icons

class PsMapToolbar(BaseToolbar):
    def __init__(self, parent):
        """!Toolbar Cartographic Composer (psmap.py)
        
        @param parent parent window
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        self.Realize()
        
        self.action = { 'id' : self.pointer }
        self.defaultAction = { 'id' : self.pointer,
                               'bind' : self.parent.OnPointer }
        self.OnTool(None)
        
        from psmap.frame import havePILImage
        if not havePILImage:
            self.EnableTool(self.preview, False)
        
    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['psMap']
        return self._getToolbarData((('loadFile', icons['scriptLoad'],
                                      self.parent.OnLoadFile),                                    
                                     ('instructionFile', icons['scriptSave'],
                                      self.parent.OnInstructionFile),
                                     (None, ),
                                     ('pagesetup', icons['pageSetup'],
                                      self.parent.OnPageSetup),
                                     (None, ),
                                     ("pointer", Icons["displayWindow"]["pointer"],
                                      self.parent.OnPointer, wx.ITEM_CHECK),
                                     ('pan', Icons["displayWindow"]['pan'],
                                      self.parent.OnPan, wx.ITEM_CHECK),
                                     ("zoomin", Icons["displayWindow"]["zoomIn"],
                                      self.parent.OnZoomIn, wx.ITEM_CHECK),
                                     ("zoomout", Icons["displayWindow"]["zoomOut"],
                                      self.parent.OnZoomOut, wx.ITEM_CHECK),
                                     ('zoomAll', icons['fullExtent'],
                                      self.parent.OnZoomAll),
                                     (None, ),
                                     ('addMap', icons['addMap'],
                                      self.parent.OnAddMap, wx.ITEM_CHECK),
                                     ('addRaster', icons['addRast'],
                                      self.parent.OnAddRaster),
                                     ('addVector', icons['addVect'],
                                      self.parent.OnAddVect),
                                     ("dec", Icons["displayWindow"]["overlay"],
                                      self.parent.OnDecoration),
                                     ("delete", icons["deleteObj"],
                                      self.parent.OnDelete),
                                     (None, ),
                                     ("preview", icons["preview"],
                                      self.parent.OnPreview),
                                     ('generatePS', icons['psExport'],
                                      self.parent.OnPSFile),
                                     ('generatePDF', icons['pdfExport'],
                                      self.parent.OnPDFFile),
                                     (None, ),
                                     ("help", Icons['misc']['help'],
                                      self.parent.OnHelp),
                                     ('quit', icons['quit'],
                                      self.parent.OnCloseWindow))
                                    )
