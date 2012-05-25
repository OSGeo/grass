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
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon        import MetaIcon

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
        icons = {
            'scriptSave' : MetaIcon(img = 'script-save',
                                    label = _('Generate text file with mapping instructions')),
            'scriptLoad' : MetaIcon(img = 'script-load',
                                    label = _('Load text file with mapping instructions')),                           
            'psExport'   : MetaIcon(img = 'ps-export',
                                    label = _('Generate PostScript output')),
            'pdfExport'  : MetaIcon(img = 'pdf-export',
                                    label = _('Generate PDF output')),
            'pageSetup'  : MetaIcon(img = 'page-settings',
                                    label = _('Page setup'),
                                    desc = _('Specify paper size, margins and orientation')),
            'fullExtent' : MetaIcon(img = 'zoom-extent',
                                    label = _("Full extent"),
                                    desc = _("Zoom to full extent")),
            'addMap'     : MetaIcon(img = 'layer-add',
                                    label = _("Map frame"),
                                    desc = _("Click and drag to place map frame")),
            'deleteObj'  : MetaIcon(img = 'layer-remove',
                                    label = _("Delete selected object")),
            'preview'    : MetaIcon(img = 'execute',
                                    label = _("Show preview")),
            'quit'       : MetaIcon(img = 'quit',
                                    label = _('Quit Cartographic Composer')),
            'addText'    : MetaIcon(img = 'text-add',
                                    label = _('Text')),
            'addMapinfo' : MetaIcon(img = 'map-info',
                                    label = _('Map info')),
            'addLegend'  : MetaIcon(img = 'legend-add',
                                    label = _('Legend')),
            'addScalebar' : MetaIcon(img = 'scalebar-add',
                                     label = _('Scale bar')),
            'addImage'   : MetaIcon(img = 'image-add',
                                    label = _('Image')),
            'addNorthArrow': MetaIcon(img = 'north-arrow-add',
                                      label = _('North Arrow')),
            'drawGraphics': MetaIcon(img = 'edit',
                                     label = _('Add simple graphics')),
            'pointAdd'    : MetaIcon(img = 'point-add',
                                     label = _('Point')),
            'lineAdd'     : MetaIcon(img = 'line-add',
                                     label = _('Line')),
            'rectangleAdd': MetaIcon(img = 'rectangle-add',
                                     label = _('Rectangle')),
            }
        self.icons = icons
        
        return self._getToolbarData((('loadFile', icons['scriptLoad'],
                                      self.parent.OnLoadFile),                                    
                                     ('instructionFile', icons['scriptSave'],
                                      self.parent.OnInstructionFile),
                                     (None, ),
                                     ('pagesetup', icons['pageSetup'],
                                      self.parent.OnPageSetup),
                                     (None, ),
                                     ("pointer", BaseIcons["pointer"],
                                      self.parent.OnPointer, wx.ITEM_CHECK),
                                     ('pan', BaseIcons['pan'],
                                      self.parent.OnPan, wx.ITEM_CHECK),
                                     ("zoomin", BaseIcons["zoomIn"],
                                      self.parent.OnZoomIn, wx.ITEM_CHECK),
                                     ("zoomout", BaseIcons["zoomOut"],
                                      self.parent.OnZoomOut, wx.ITEM_CHECK),
                                     ('zoomAll', icons['fullExtent'],
                                      self.parent.OnZoomAll),
                                     (None, ),
                                     ('addMap', icons['addMap'],
                                      self.parent.OnAddMap, wx.ITEM_CHECK),
                                     ('addRaster', BaseIcons['addRast'],
                                      self.parent.OnAddRaster),
                                     ('addVector', BaseIcons['addVect'],
                                      self.parent.OnAddVect),
                                     ("dec", BaseIcons["overlay"],
                                      self.OnDecoration),
                                     ("drawGraphics", icons["drawGraphics"],
                                      self.OnDrawGraphics, wx.ITEM_CHECK),
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
                                     ("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                     ('quit', icons['quit'],
                                      self.parent.OnCloseWindow))
                                    )

    def OnDecoration(self, event):
        """!Decorations overlay menu
        """
        self._onMenu(((self.icons["addLegend"],     self.parent.OnAddLegend),
                      (self.icons["addMapinfo"],    self.parent.OnAddMapinfo),
                      (self.icons["addScalebar"],   self.parent.OnAddScalebar),
                      (self.icons["addText"],       self.parent.OnAddText),
                      (self.icons["addImage"],      self.parent.OnAddImage),
                      (self.icons["addNorthArrow"], self.parent.OnAddNorthArrow)))

    def OnDrawGraphics(self, event):
        """!Simple geometry features (point, line, rectangle) overlay menu
        """
        # we need the previous id
        self.actionOld = self.action['id']
        self.OnTool(event)
        self.action['id'] = self.actionOld
        self._onMenu(((self.icons["pointAdd"],      self.parent.OnAddPoint),
                      (self.icons["lineAdd"],       self.parent.OnAddLine),
                      (self.icons["rectangleAdd"],  self.parent.OnAddRectangle),
                    ))
