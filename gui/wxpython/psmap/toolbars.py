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
from core.utils import _
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon        import MetaIcon
from core.globalvar    import ETCIMGDIR

class PsMapToolbar(BaseToolbar):
    def __init__(self, parent, toolSwitcher):
        """!Toolbar Cartographic Composer (psmap.py)
        
        @param parent parent window
        """
        BaseToolbar.__init__(self, parent, toolSwitcher)
        
        self.InitToolbar(self._toolbarData())
        self._default = self.pointer
        
        for tool in (self.pointer, self.pan, self.zoomin, self.zoomout,
                     self.drawGraphics, self.addMap):
            self.toolSwitcher.AddToolToGroup(group='mouseUse', toolbar=self, tool=tool)

        # custom button for graphics mode selection
        # TODO: could this be somehow generalized?
        self.arrowButton = self.CreateSelectionButton()
        self.arrowButtonId = self.InsertControl(18, self.arrowButton)
        self.arrowButton.Bind(wx.EVT_BUTTON, self.OnDrawGraphicsMenu)

        self.drawGraphicsAction = None
        self.OnAddPoint(event = None)

        self.Realize()

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
            'pointAdd'    : MetaIcon(img = 'point-add',
                                     label = _('Point')),
            'lineAdd'     : MetaIcon(img = 'line-add',
                                     label = _('Line')),
            'rectangleAdd': MetaIcon(img = 'rectangle-add',
                                     label = _('Rectangle')),
            'overlaysAdd': MetaIcon(img = 'layer-more',
                                    label = _("Add overlays")),
            'labelsAdd': MetaIcon(img = 'layer-label-add',
                                    label = _("Add labels"))
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
                                     ('overlaysAdd', icons['overlaysAdd'],
                                      self.OnAddOverlays), 
                                     ("delete", icons["deleteObj"],
                                      self.parent.OnDelete),
                                     ("dec", BaseIcons["overlay"],
                                      self.OnDecoration),
                                     ("drawGraphics", icons['pointAdd'],
                                      self.OnDrawGraphics, wx.ITEM_CHECK),
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

    def OnAddOverlays(self, event):
        self._onMenu(((self.icons['labelsAdd'], self.parent.OnAddLabels), ))

    def OnDrawGraphics(self, event):
        """!Graphics tool activated."""
        # we need the previous id
        if self.drawGraphicsAction == 'pointAdd':
            self.parent.OnAddPoint(event)
        elif self.drawGraphicsAction == 'lineAdd':
            self.parent.OnAddLine(event)
        elif self.drawGraphicsAction == 'rectangleAdd':
            self.parent.OnAddRectangle(event)

    def OnDrawGraphicsMenu(self, event):
        """!Simple geometry features (point, line, rectangle) overlay menu
        """
        self._onMenu(((self.icons["pointAdd"],      self.OnAddPoint),
                      (self.icons["lineAdd"],       self.OnAddLine),
                      (self.icons["rectangleAdd"],  self.OnAddRectangle),
                    ))

    def OnAddPoint(self, event):
        """!Point mode selected.

        Graphics drawing tool is activated. Tooltip changed.
        """
        self.SetToolNormalBitmap(self.drawGraphics, self.icons["pointAdd"].GetBitmap())
        self.SetToolShortHelp(self.drawGraphics, _("Add simple graphics: points"))
        self.drawGraphicsAction = 'pointAdd'
        if event:
            self.ToggleTool(self.drawGraphics, True)
            self.parent.OnAddPoint(event)

    def OnAddLine(self, event):
        """!Line mode selected.

        Graphics drawing tool is activated. Tooltip changed.
        """
        self.SetToolNormalBitmap(self.drawGraphics, self.icons["lineAdd"].GetBitmap())
        self.SetToolShortHelp(self.drawGraphics, _("Add simple graphics: lines"))
        self.ToggleTool(self.drawGraphics, True)
        if event:
            self.drawGraphicsAction = 'lineAdd'
            self.parent.OnAddLine(event)

    def OnAddRectangle(self, event):
        """!Rectangle mode selected.

        Graphics drawing tool is activated. Tooltip changed.
        """
        self.SetToolNormalBitmap(self.drawGraphics, self.icons["rectangleAdd"].GetBitmap())
        self.SetToolShortHelp(self.drawGraphics, _("Add simple graphics: rectangles"))
        self.ToggleTool(self.drawGraphics, True)
        if event:
            self.drawGraphicsAction = 'rectangleAdd'
            self.parent.OnAddRectangle(event)

    def CreateSelectionButton(self):
        """!Add button to toolbar for selection of graphics drawing mode.

        Button must be custom (not toolbar tool) to set smaller width.
        """
        arrowPath = os.path.join(ETCIMGDIR, 'small_down_arrow.png')
        if os.path.isfile(arrowPath) and os.path.getsize(arrowPath):
            bitmap = wx.Bitmap(name = arrowPath)
        else:
            bitmap = wx.ArtProvider.GetBitmap(id = wx.ART_MISSING_IMAGE, client = wx.ART_TOOLBAR)
        button =  wx.BitmapButton(parent = self, id = wx.ID_ANY, size = ((-1, self.GetSize()[1])),
                                  bitmap = bitmap, style = wx.NO_BORDER)
        button.SetToolTipString(_("Select graphics tool"))

        return button
