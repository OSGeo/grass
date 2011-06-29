"""!
@package icon

@brief Icon themes

@code
from icons import Icons as Icons
@endcode

Classes:
 - MetaIcon

(C) 2007-2008, 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <anna.kratochvilova fsv.cvut.cz>
"""

import os
import sys

sys.path.append(os.path.join(os.getenv("GISBASE"), "etc", "wxpython", "gui_modules"))

import wx

from gui_modules.preferences import globalSettings as UserSettings

# default icon set
import grass_icons
iconSetDefault  = grass_icons.iconSet
iconPathDefault = grass_icons.iconPath

iconTheme = UserSettings.Get(group = 'appearance', key = 'iconTheme', subkey = 'type')
if iconTheme != 'grass':
    sys.stderr.write(_("Unknown iconset '%s', using default 'grass'...\n") % (iconTheme))

iconSet  = iconSetDefault
iconPath = iconPathDefault

# join paths
try:
    if iconPath and not os.path.exists(iconPath):
        raise OSError
    
    for key, img in iconSet.iteritems():
        if key not in iconSet or \
                iconSet[key] is None: # add key
            iconSet[key] = img
        
        iconSet[key] = os.path.join(iconPath, iconSet[key])
except StandardError, e:
    sys.exit(_("Unable to load icon theme. Reason: %s. Quiting wxGUI...") % e)
    
class MetaIcon:
    """!Handle icon metadata (image path, tooltip, ...)
    """
    def __init__(self, img, label, desc = None):
        self.imagepath = img
        if not self.imagepath:
            self.type = 'unknown'
        else:
            if self.imagepath.find ('wxART_') > -1:
                self.type = 'wx'
            else:
                self.type = 'img'
        
        self.label = label
        
        if desc:
            self.description = desc
        else:
            self.description = ''
        
    def __str__(self):
        """!Debugging"""
        return "label=%s, img=%s, type=%s" % (self.label, self.imagepath, self.type)

    def GetBitmap(self, size = None):
        """!Get bitmap"""
        bmp = None
        
        if self.type == 'wx':
            bmp = wx.ArtProvider.GetBitmap(id = self.imagepath, client = wx.ART_TOOLBAR, size = size)
        elif self.type == 'img':
            if os.path.isfile(self.imagepath) and os.path.getsize(self.imagepath):
                if size and len(size) == 2:
                    image = wx.Image(name = self.imagepath)
                    image.Rescale(size[0], size[1])
                    bmp = image.ConvertToBitmap()
                elif self.imagepath:
                    bmp = wx.Bitmap(name = self.imagepath)
        
        return bmp
    
    def GetLabel(self):
        return self.label
    
    def GetDesc(self):
        return self.description
    
    def GetImageName(self):
        return os.path.basename(self.imagepath)

#
# create list of icon instances
#
Icons = {
    'displayWindow' : {
        'display'    : MetaIcon(img = iconSet.get('show', wx.ART_ERROR),
                                label = _('Display map'),
                                desc  =  _('Re-render modified map layers only')),
        'render'     : MetaIcon(img = iconSet.get('layer-redraw', wx.ART_ERROR),
                                label = _('Render map'),
                                desc = _('Force re-rendering all map layers')),
        'erase'      : MetaIcon(img = iconSet.get('erase', wx.ART_ERROR),
                                label = _('Erase display'),
                                desc = _('Erase display canvas with given background color')),
        'pointer'    : MetaIcon(img = iconSet.get('pointer', wx.ART_ERROR),
                                label = _('Pointer')),
        'zoomIn'     : MetaIcon(img = iconSet.get('zoom-in', wx.ART_ERROR),
                                label = _('Zoom in'),
                                desc = _('Drag or click mouse to zoom')),
        'zoomOut'    : MetaIcon(img = iconSet.get('zoom-out', wx.ART_ERROR),
                                label = _('Zoom out'),
                                desc = _('Drag or click mouse to unzoom')),
        'pan'        : MetaIcon(img = iconSet.get('pan', wx.ART_ERROR),
                                label = _('Pan'),
                                desc = _('Drag with mouse to pan')),
        'query'      : MetaIcon(img = iconSet.get('info', wx.ART_ERROR),
                                label = _('Query raster/vector map(s)'),
                                desc = _('Query selected raster/vector map(s)')),
        'zoomBack'   : MetaIcon(img = iconSet.get('zoom-last', wx.ART_ERROR),
                                label = _('Return to previous zoom')),
        'zoomMenu'   : MetaIcon(img = iconSet.get('zoom-more', wx.ART_ERROR),
                                label = _('Various zoom options'),
                                desc = _('Zoom to computational, default, saved region, ...')),
        'zoomExtent' : MetaIcon(img = iconSet.get('zoom-extent', wx.ART_ERROR),
                                label = _('Zoom to selected map layer(s)')),
        'overlay'    : MetaIcon(img = iconSet.get('overlay-add', wx.ART_ERROR),
                                label = _('Add map elements'),
                                desc = _('Overlay elements like scale and legend onto map')),
        'addBarscale': MetaIcon(img = iconSet.get('scalebar-add', wx.ART_ERROR),
                                label = _('Add scalebar and north arrow')),
        'addLegend'  : MetaIcon(img = iconSet.get('legend-add', wx.ART_ERROR),
                                label = _('Add legend')),
        'saveFile'   : MetaIcon(img = iconSet.get('map-export', wx.ART_ERROR),
                                label = _('Save display to graphic file')),
        'print'      : MetaIcon(img = iconSet.get('print', wx.ART_ERROR),
                                label = _('Print display')),
        'analyze'    : MetaIcon(img = iconSet.get('layer-raster-analyze', wx.ART_ERROR),
                                label = _('Analyze map'),
                                desc = _('Measuring, profiling, histogramming, ...')),
        'measure'    : MetaIcon(img = iconSet.get('measure-length', wx.ART_ERROR),
                                label = _('Measure distance')),
        'profile'    : MetaIcon(img = iconSet.get('layer-raster-profile', wx.ART_ERROR),
                                label = _('Profile surface map')),
        'addText'    : MetaIcon(img = iconSet.get('text-add', wx.ART_ERROR),
                                label = _('Add text layer')),
        'histogram'  : MetaIcon(img = iconSet.get('layer-raster-histogram', wx.ART_ERROR),
                                label = _('Create histogram of image or raster file')),
        },
    'layerManager' : {
        'newdisplay'   : MetaIcon(img = iconSet.get('monitor-create', wx.ART_ERROR),
                                  label = _('Start new map display')),
        'workspaceNew'  : MetaIcon(img = iconSet.get('create', wx.ART_ERROR),
                                   label = _('Create new workspace (Ctrl+N)')),
        'workspaceOpen' : MetaIcon(img = iconSet.get('open', wx.ART_ERROR),
                                   label = _('Open existing workspace file (Ctrl+O)')),
        'workspaceSave' : MetaIcon(img = iconSet.get('save', wx.ART_ERROR),
                                   label = _('Save current workspace to file (Ctrl+S)')),
        'addMulti'      : MetaIcon(img = iconSet.get('layer-open', wx.ART_ERROR),
                                   label = _('Add multiple raster or vector map layers (Ctrl+Shift+L)')),
        'import'        : MetaIcon(img = iconSet.get('layer-import', wx.ART_ERROR),
                                   label = _('Import/link raster or vector data')),
        'rastImport' : MetaIcon(img = iconSet.get('layer-import', wx.ART_ERROR),
                                label = _('Import raster data')),
        'rastLink'   : MetaIcon(img = iconSet.get('layer-import', wx.ART_ERROR),
                                label = _('Link external raster data')),
        'vectImport' : MetaIcon(img = iconSet.get('layer-import', wx.ART_ERROR),
                                label = _('Import vector data')),
        'vectLink'   : MetaIcon(img = iconSet.get('layer-import', wx.ART_ERROR),
                                label = _('Link external vector data')),
        'addRast'    : MetaIcon(img = iconSet.get('layer-raster-add', wx.ART_ERROR),
                                label = _('Add raster map layer (Ctrl+Shift+R)')),
        'rastMisc'   : MetaIcon(img = iconSet.get('layer-raster-more', wx.ART_ERROR),
                                label = _('Add various raster map layers (RGB, HIS, shaded relief...)')),
        'addVect'    : MetaIcon(img = iconSet.get('layer-vector-add', wx.ART_ERROR),
                                label = _('Add vector map layer (Ctrl+Shift+V)')),
        'vectMisc'   : MetaIcon(img = iconSet.get('layer-vector-more', wx.ART_ERROR),
                                label = _('Add various vector map layers (thematic, chart...)')),
        'addCmd'     : MetaIcon(img = iconSet.get('layer-command-add', wx.ART_ERROR),
                                label = _('Add command layer')),
        'addGroup'   : MetaIcon(img = iconSet.get('layer-group-add', wx.ART_ERROR),
                                label = _('Add group')),
        'addOverlay' : MetaIcon(img = iconSet.get('layer-more', wx.ART_ERROR),
                                label = _('Add grid or vector labels overlay')),
        'delCmd'     : MetaIcon(img = iconSet.get('layer-remove', wx.ART_ERROR),
                                label = _('Delete selected map layer')),
        'quit'       : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                                label = _('Quit')),
        'attrTable'  : MetaIcon(img = iconSet.get('table', wx.ART_ERROR),
                                label = _('Show attribute table')),
        'vdigit'     : MetaIcon(img = iconSet.get('edit', wx.ART_ERROR),
                                label = _('Edit vector maps')),
        'addRgb'     : MetaIcon(img = iconSet.get('layer-rgb-add', wx.ART_ERROR),
                                label = _('Add RGB map layer')),
        'addHis'     : MetaIcon(img = iconSet.get('layer-his-add', wx.ART_ERROR),
                                label = _('Add HIS map layer')),
        'addShaded'  : MetaIcon(img = iconSet.get('layer-shaded-relief-add', wx.ART_ERROR),
                                label = _('Add shaded relief map layer')),
        'addRArrow'  : MetaIcon(img = iconSet.get('layer-aspect-arrow-add', wx.ART_ERROR),
                                label = _('Add raster flow arrows')),
        'addRNum'    : MetaIcon(img = iconSet.get('layer-cell-cats-add', wx.ART_ERROR),
                                label = _('Add raster cell numbers')),
        'addThematic': MetaIcon(img = iconSet.get('layer-vector-thematic-add', wx.ART_ERROR),
                                label = _('Add thematic area (choropleth) map layer')),
        'addChart'   : MetaIcon(img = iconSet.get('layer-vector-chart-add', wx.ART_ERROR),
                                label = _('Add thematic chart layer')),
        'addGrid'    : MetaIcon(img = iconSet.get('layer-grid-add', wx.ART_ERROR),
                                label = _('Add grid layer')),
        'addGeodesic': MetaIcon(img = iconSet.get('shortest-distance', wx.ART_ERROR),
                                label = _('Add geodesic line layer')),
        'addRhumb'   : MetaIcon(img = iconSet.get('shortest-distance', wx.ART_ERROR),
                                label = _('Add rhumbline layer')),
        'addLabels'  : MetaIcon(img = iconSet.get('layer-label-add', wx.ART_ERROR),
                                label = _('Add labels')),
        'addRast3d'  : MetaIcon(img = iconSet.get('layer-raster3d-add', wx.ART_ERROR),
                                label = _('Add 3D raster map layer'),
                                desc  =  _('Note that 3D raster data are rendered only in 3D view mode')),
        'settings'   : MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                                label = _('Show GUI settings')),
        'modeler'    : MetaIcon(img = iconSet.get('modeler-main', wx.ART_ERROR),
                                label = _('Graphical Modeler')),
        'layerOptions'  : MetaIcon(img = iconSet.get('options', wx.ART_ERROR),
                                   label = _('Set options')),
        'mapOutput'  : MetaIcon(img = iconSet.get('print-compose', wx.ART_ERROR),
                                label = _('Hardcopy Map Output Utility')),
        'mapcalc'    : MetaIcon(img = iconSet.get('calculator', wx.ART_ERROR),
                                label = _('Raster Map Calculator')),
        },
    'vdigit' : {
        'addPoint'        : MetaIcon(img = iconSet.get('point-create', wx.ART_ERROR),
                                     label = _('Digitize new point'),
                                     desc = _('Left: new point')),
        'addLine'         : MetaIcon(img = iconSet.get('line-create', wx.ART_ERROR),
                                     label = _('Digitize new line'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'addBoundary'     : MetaIcon(img = iconSet.get('boundary-create', wx.ART_ERROR),
                                     label = _('Digitize new boundary'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'addCentroid'     : MetaIcon(img = iconSet.get('centroid-create', wx.ART_ERROR),
                                     label = _('Digitize new centroid'),
                                     desc = _('Left: new point')),
        'addArea'         : MetaIcon(img = iconSet.get('polygon-create', wx.ART_ERROR),
                                     label = _('Digitize new area (composition of boundaries without category and one centroid with category)'),
                                     desc = _('Left: new point')),
        'addVertex'       : MetaIcon(img = iconSet.get('vertex-create', wx.ART_ERROR),
                                     label = _('Add new vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'copyCats'        : MetaIcon(img = iconSet.get('cats-copy', wx.ART_ERROR),
                                     label = _('Copy categories'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'deleteLine'      : MetaIcon(img = iconSet.get('line-delete', wx.ART_ERROR),
                                     label = _('Delete feature(s)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'displayAttr'     : MetaIcon(img = iconSet.get('attributes-display', wx.ART_ERROR),
                                     label = _('Display/update attributes'),
                                     desc = _('Left: Select')),
        'displayCats'     : MetaIcon(img = iconSet.get('cats-display', wx.ART_ERROR),
                                     label = _('Display/update categories'),
                                     desc = _('Left: Select')),
        'editLine'        : MetaIcon(img = iconSet.get('line-edit', wx.ART_ERROR),
                                     label = _('Edit line/boundary'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'moveLine'        : MetaIcon(img = iconSet.get('line-move', wx.ART_ERROR),
                                     label = _('Move feature(s)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'moveVertex'      : MetaIcon(img = iconSet.get('vertex-move', wx.ART_ERROR),
                                     label = _('Move vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'removeVertex'    : MetaIcon(img = iconSet.get('vertex-delete', wx.ART_ERROR),
                                     label = _('Remove vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'settings'        : MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                                     label = _('Digitization settings')),
        'splitLine'       : MetaIcon(img = iconSet.get('line-split', wx.ART_ERROR),
                                     label = _('Split line/boundary'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'quit'            : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                                     label = _('Quit digitizer'),
                                     desc = _('Quit digitizer and save changes')),
        'additionalTools' : MetaIcon(img = iconSet.get('tools', wx.ART_ERROR),
                                     label = _('Additional tools ' \
                                                   '(copy, flip, connect, etc.)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'undo'             : MetaIcon(img = iconSet.get('undo', wx.ART_ERROR),
                                      label = _('Undo'),
                                      desc = _('Undo previous changes')),
        },
    'profile' : {
        'draw'         : MetaIcon(img = iconSet.get('show', wx.ART_ERROR),
                                  label = _('Draw/re-draw profile')),
        'transect'     : MetaIcon(img = iconSet.get('layer-raster-profile', wx.ART_ERROR),
                                  label = _('Draw transect in map display window to profile')),
        'options'      : MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                                  label = _('Profile options')),
        'save'         : MetaIcon(img = iconSet.get('save', wx.ART_ERROR),
                                  label = _('Save profile data to CSV file')),
        'quit'         : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                                  label = _('Quit Profile Analysis Tool'))
        },
    'georectify' : {
        'gcpSet'    : MetaIcon(img = iconSet.get('gcp-create', wx.ART_ERROR),
                               label = _('Set GCP'),
                               desc = _('Define GCP (Ground Control Points)')),
        'georectify': MetaIcon(img = iconSet.get('georectify', wx.ART_ERROR),
                               label = _('Georectify')),
        'gcpRms'    : MetaIcon(img = iconSet.get('gcp-rms', wx.ART_ERROR),
                               label = _('Recalculate RMS error')),
        'gcpSave'   : MetaIcon(img = iconSet.get('gcp-save', wx.ART_ERROR),
                               label = _('Save GCPs to POINTS file')),
        'gcpAdd'    : MetaIcon(img = iconSet.get('gcp-add', wx.ART_ERROR),
                               label = _('Add new GCP')),
        'gcpDelete' : MetaIcon(img = iconSet.get('gcp-delete', wx.ART_ERROR),
                               label = _('Delete selected GCP')),
        'gcpClear'  : MetaIcon(img = iconSet.get('gcp-remove', wx.ART_ERROR),
                                label = _('Clear selected GCP')),
        'gcpReload' : MetaIcon(img = iconSet.get('reload', wx.ART_ERROR),
                               label = _('Reload GCPs from POINTS file')),
        'quit'      : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                               label = _('Quit georectification')),
        'settings'  : MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                               label = _('Settings'),
                               desc = _('Settings dialog for georectification tool')),
        },
    'nviz' : {
        'view'    : MetaIcon(img = iconSet.get('3d-view', wx.ART_ERROR),
                             label = _('Switch to view control page'),
                             desc = _('Change view settings')),
        'surface' : MetaIcon(img = iconSet.get('3d-raster', wx.ART_ERROR),
                             label = _('Switch to surface (raster) control page'),
                             desc = _('Change surface (loaded raster maps) settings')),
        'vector'  : MetaIcon(img = iconSet.get('3d-vector', wx.ART_ERROR),
                             label = _('Switch to vector (2D/3D) control page'),
                             desc = _('Change 2D/3D vector settings')),
        'volume'  : MetaIcon(img = iconSet.get('3d-volume', wx.ART_ERROR),
                             label = _('Switch to volume (3D raster) control page'),
                             desc = _('Change volume (loaded 3D raster maps) settings')),
        'light'   : MetaIcon(img = iconSet.get('3d-light', wx.ART_ERROR),
                             label = _('Switch to lighting control page'),
                             desc = _('Change lighting settings')),
        'fringe'  : MetaIcon(img = iconSet.get('3d-fringe', wx.ART_ERROR),
                             label = _('Switch to fringe control page'),
                             desc = _('Switch on/off fringes')),
        'settings': MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                             label = _('3D view mode tools'),
                             desc = _('Show/hide 3D view mode settings dialog')),
        'quit'    : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                             label = _('Quit 3D view mode'),
                             desc = _('Switch back to 2D view mode')),
        },
    'modeler' : {
        'new'        : MetaIcon(img = iconSet.get('create', wx.ART_ERROR),
                                label = _('Create new model (Ctrl+N)')),
        'open'       : MetaIcon(img = iconSet.get('open', wx.ART_ERROR),
                                label = _('Load model from file (Ctrl+O)')),
        'save'       : MetaIcon(img = iconSet.get('save', wx.ART_ERROR),
                                label = _('Save current model to file (Ctrl+S)')),
        'toImage'    : MetaIcon(img = iconSet.get('image-export', wx.ART_ERROR),
                                label = _('Export model to image')),
        'toPython'   : MetaIcon(img = iconSet.get('python-export', wx.ART_ERROR),
                                label = _('Export model to Python script')),
        'actionAdd'  : MetaIcon(img = iconSet.get('module-add', wx.ART_ERROR),
                                label = _('Add action (GRASS module) to model')),
        'dataAdd'    : MetaIcon(img = iconSet.get('data-add', wx.ART_ERROR),
                                label = _('Add data item to model')),
        'relation'   : MetaIcon(img = iconSet.get('relation-create', wx.ART_ERROR),
                                label = _('Define relation between data and action items')),
        'run'        : MetaIcon(img = iconSet.get('execute', wx.ART_ERROR),
                                label = _('Run model')),
        'validate'   : MetaIcon(img = iconSet.get('check', wx.ART_ERROR),
                                label = _('Validate model')),
        'settings'   : MetaIcon(img = iconSet.get('settings', wx.ART_ERROR),
                                label = _('Show modeler settings')),
        'properties' : MetaIcon(img = iconSet.get('options', wx.ART_ERROR),
                                label = _('Show model properties')),
        'variables'  : MetaIcon(img = iconSet.get('modeler-variables', wx.ART_ERROR),
                                label = _('Manage model variables')),
        'redraw'     : MetaIcon(img = iconSet.get('redraw', wx.ART_ERROR),
                                label = _('Redraw model canvas')),
        'quit'       : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                                label = _('Quit Graphical Modeler')),
        },
    'misc' : {
        'font' : MetaIcon(img = iconSet.get('font', wx.ART_ERROR),
                          label = _('Select font')),
        'help' : MetaIcon(img = iconSet.get('help', wx.ART_ERROR),
                          label = _('Show manual')),
        'quit' : MetaIcon(img = iconSet.get('quit', wx.ART_ERROR),
                          label = _('Quit')),
        },
    'psMap' : {
        'scriptSave' : MetaIcon(img = iconSet['script-save'],
                                label = _('Generate text file with mapping instructions')),
        'scriptLoad' : MetaIcon(img = iconSet['script-load'],
                                label = _('Load text file with mapping instructions')),                           
        'psExport'   : MetaIcon(img = iconSet['ps-export'],
                                label = _('Generate PostScript output')),
        'pdfExport'  : MetaIcon(img = iconSet['pdf-export'],
                                label = _('Generate PDF output')),
        'pageSetup'  : MetaIcon(img = iconSet['page-settings'],
                                label = _('Page setup'),
                                desc = _('Specify paper size, margins and orientation')),
        'fullExtent' : MetaIcon(img = iconSet['zoom-extent'],
                                label = _("Full extent"),
                                desc = _("Zoom to full extent")),
        'addMap'     : MetaIcon(img = iconSet['layer-add'],
                                label = _("Map frame"),
                                desc = _("Click and drag to place map frame")),
        'addRast'    : MetaIcon(img = iconSet['layer-raster-add'],
                                label = _("Raster map"),
                                desc = _("Add raster map")),
        'addVect'    : MetaIcon(img = iconSet['layer-vector-add'],
                                label = _("Vector map"),
                                desc = _("Add vector map")),
        'deleteObj'  : MetaIcon(img = iconSet['layer-remove'],
                                label = _("Delete selected object")),
        'preview'    : MetaIcon(img = iconSet['execute'],
                                label = _("Show preview")),
        'quit'       : MetaIcon(img = iconSet['quit'],
                                label = _('Quit Hardcopy Map Utility')),
        'addText'    : MetaIcon(img = iconSet['text-add'],
                                label = _('Add text')),
        'addMapinfo' : MetaIcon(img = iconSet['map-info'],
                                label = _('Add map info')),
        'addLegend'  : MetaIcon(img = iconSet['legend-add'],
                                label = _('Add legend')),
        'addScalebar' : MetaIcon(img = iconSet['scalebar-add'],
                                 label = _('Add scale bar')),
        }
    }

# testing ...
if __name__ == '__main__':
    for k, v in Icons.iteritems():
        print v.GetImageName()
