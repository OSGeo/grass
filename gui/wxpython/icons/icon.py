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
"""

import os
import sys

sys.path.append(os.path.join(os.getenv("GISBASE"), "etc", "wxpython", "gui_modules"))

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx

from preferences import globalSettings as UserSettings

# default icon set
import grass_icons
iconSetDefault  = grass_icons.iconSet
iconPathDefault = grass_icons.iconPath

iconTheme = UserSettings.Get(group='advanced', key='iconTheme', subkey='type')
if iconTheme != 'grass':
    sys.stderr.write(_("Unknown iconset '%s', using default 'grass'...\n") % (iconTheme))

iconSet  = iconSetDefault
iconPath = iconPathDefault

# join paths
try:
    if iconPath and not os.path.exists(iconPath):
        raise OSError
    
    for key, img in iconSet.iteritems():
        if not iconSet.has_key(key) or \
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
        'display'    : MetaIcon(img = iconSet['show'],
                                label = _('Display map'),
                                desc  =  _('Re-render modified map layers only')),
        'render'     : MetaIcon(img = iconSet['layer-redraw'],
                                label = _('Render map'),
                                desc = _('Force re-rendering all map layers')),
        'erase'      : MetaIcon(img = iconSet['erase'],
                                label = _('Erase display'),
                                desc = _('Erase display canvas with given background color')),
        'pointer'    : MetaIcon(img = iconSet['pointer'],
                                label = _('Pointer')),
        'zoomIn'     : MetaIcon(img = iconSet['zoom-in'],
                                label = _('Zoom in'),
                                desc = _('Drag or click mouse to zoom')),
        'zoomOut'    : MetaIcon(img = iconSet['zoom-out'],
                                label = _('Zoom out'),
                                desc = _('Drag or click mouse to unzoom')),
        'pan'        : MetaIcon(img = iconSet['pan'],
                                label = _('Pan'),
                                desc = _('Drag with mouse to pan')),
        'query'      : MetaIcon(img = iconSet['info'],
                                label = _('Query raster/vector map(s)'),
                                desc = _('Query selected raster/vector map(s)')),
        'zoomBack'   : MetaIcon(img = iconSet['zoom-last'],
                                label = _('Return to previous zoom')),
        'zoomMenu'   : MetaIcon(img = iconSet['zoom-more'],
                                label = _('Various zoom options'),
                                desc = _('Zoom to computational, default, saved region, ...')),
        'zoomExtent' : MetaIcon(img = iconSet['zoom-extent'],
                                label = _('Zoom to selected map layer(s)')),
        'overlay'    : MetaIcon(img = iconSet['overlay-add'],
                                label = _('Add map elements'),
                                desc = _('Overlay elements like scale and legend onto map')),
        'addBarscale': MetaIcon(img = iconSet['scalebar-add'],
                                label = _('Add scalebar and north arrow')),
        'addLegend'  : MetaIcon(img = iconSet['legend-add'],
                                label = _('Add legend')),
        'saveFile'   : MetaIcon(img = iconSet['map-export'],
                                label = _('Save display to graphic file')),
        'print'      : MetaIcon(img = iconSet['print'],
                                label = _('Print display')),
        'analyze'    : MetaIcon(img = iconSet['layer-raster-analyze'],
                                label = _('Analyze map'),
                                desc = _('Measuring, profiling, histogramming, ...')),
        'measure'    : MetaIcon(img = iconSet['measure-length'],
                                label = _('Measure distance')),
        'profile'    : MetaIcon(img = iconSet['layer-raster-profile'],
                                label = _('Profile surface map')),
        'addText'    : MetaIcon(img = iconSet['text-add'],
                                label = _('Add text layer')),
        'histogram'  : MetaIcon(img = iconSet['layer-raster-histogram'],
                                label = _('Create histogram of image or raster file')),
        },
    'layerManager' : {
        'newdisplay'   : MetaIcon(img = iconSet['monitor-create'],
                                  label = _('Start new map display')),
        'workspaceNew'  : MetaIcon(img = iconSet['create'],
                                   label = _('Create new workspace (Ctrl+N)')),
        'workspaceLoad' : MetaIcon(img = iconSet['layer-open'],
                                   label = _('Load map layers into workspace (Ctrl+L)')),
        'workspaceOpen' : MetaIcon(img = iconSet['open'],
                                   label = _('Open existing workspace file (Ctrl+O)')),
        'workspaceSave' : MetaIcon(img = iconSet['save'],
                                   label = _('Save current workspace to file (Ctrl+S)')),
        'rastImport' : MetaIcon(img = iconSet['layer-import'],
                                label = _('Import raster data')),
        'rastLink'   : MetaIcon(img = iconSet['layer-import'],
                                label = _('Link external raster data')),
        'vectImport' : MetaIcon(img = iconSet['layer-import'],
                                label = _('Import vector data')),
        'vectLink'   : MetaIcon(img = iconSet['layer-import'],
                                label = _('Link external vector data')),
        'addRast'    : MetaIcon(img = iconSet['layer-raster-add'],
                                label = _('Add raster map layer (Ctrl+R)')),
        'rastMisc'   : MetaIcon(img = iconSet['layer-raster-more'],
                                label = _('Add various raster map layers (RGB, HIS, shaded relief...)')),
        'addVect'    : MetaIcon(img = iconSet['layer-vector-add'],
                                label = _('Add vector map layer (Ctrl+V)')),
        'vectMisc'   : MetaIcon(img = iconSet['layer-vector-more'],
                                label = _('Add various vector map layers (thematic, chart...)')),
        'addCmd'     : MetaIcon(img = iconSet['layer-command-add'],
                                label = _('Add command layer')),
        'addGroup'   : MetaIcon(img = iconSet['layer-group-add'],
                                label = _('Add group')),
        'addOverlay' : MetaIcon(img = iconSet['layer-more'],
                                label = _('Add grid or vector labels overlay')),
        'delCmd'     : MetaIcon(img = iconSet['layer-remove'],
                                label = _('Delete selected map layer')),
        'quit'       : MetaIcon(img = iconSet['quit'],
                                label = _('Quit')),
        'attrTable'  : MetaIcon(img = iconSet['table'],
                                label = _('Show attribute table')),
        'vdigit'     : MetaIcon(img = iconSet['edit'],
                                label = _('Edit vector maps')),
        'addRgb'     : MetaIcon(img = iconSet['layer-rgb-add'],
                                label = _('Add RGB map layer')),
        'addHis'     : MetaIcon(img = iconSet['layer-his-add'],
                                label = _('Add HIS map layer')),
        'addShaded'  : MetaIcon(img = iconSet['layer-shaded-relief-add'],
                                label = _('Add shaded relief map layer')),
        'addRArrow'  : MetaIcon(img = iconSet['layer-aspect-arrow-add'],
                                label = _('Add raster flow arrows')),
        'addRNum'    : MetaIcon(img = iconSet['layer-cell-cats-add'],
                                label = _('Add raster cell numbers')),
        'addThematic': MetaIcon(img = iconSet['layer-vector-thematic-add'],
                                label = _('Add thematic area (choropleth) map layer')),
        'addChart'   : MetaIcon(img = iconSet['layer-vector-chart-add'],
                                label = _('Add thematic chart layer')),
        'addGrid'    : MetaIcon(img = iconSet['layer-grid-add'],
                                label = _('Add grid layer')),
        'addGeodesic': MetaIcon(img = iconSet['shortest-distance'],
                                label = _('Add geodesic line layer')),
        'addRhumb'   : MetaIcon(img = iconSet['shortest-distance'],
                                label = _('Add rhumbline layer')),
        'addLabels'  : MetaIcon(img = iconSet['layer-label-add'],
                                label = _('Add labels')),
        'addRast3d'  : MetaIcon(img = iconSet['layer-raster3d-add'],
                                label = _('Add 3D raster map layer'),
                                desc  =  _('Note that 3D raster data are rendered only in 3D view mode')),
        'settings'   : MetaIcon(img = iconSet['settings'],
                                label = _('Show GUI settings')),
        'modeler'    : MetaIcon(img = iconSet['modeler-main'],
                                label = _('Start Graphical Modeler')),
        "layerOptions"  : MetaIcon(img = iconSet['options'],
                                   label = _('Set options')),
        },
    'vdigit' : {
        'addPoint'        : MetaIcon(img = iconSet['point-create'],
                                     label = _('Digitize new point'),
                                     desc = _('Left: new point')),
        'addLine'         : MetaIcon(img = iconSet['line-create'],
                                     label = _('Digitize new line'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'addBoundary'     : MetaIcon(img = iconSet['boundary-create'],
                                     label = _('Digitize new boundary'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'addCentroid'     : MetaIcon(img = iconSet['centroid-create'],
                                     label = _('Digitize new centroid'),
                                     desc = _('Left: new point')),
        'addArea'         : MetaIcon(img = iconSet['polygon-create'],
                                     label = _('Digitize new area (composition of bondaries without category and one centroid with category)'),
                                     desc = _('Left: new point')),
        'addVertex'       : MetaIcon(img = iconSet['vertex-create'],
                                     label = _('Add new vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'copyCats'        : MetaIcon(img = iconSet['cats-copy'],
                                     label = _('Copy categories'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'deleteLine'      : MetaIcon(img = iconSet['line-delete'],
                                     label = _('Delete feature(s)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'displayAttr'     : MetaIcon(img = iconSet['attributes-display'],
                                     label = _('Display/update attributes'),
                                     desc = _('Left: Select')),
        'displayCats'     : MetaIcon(img = iconSet['cats-display'],
                                     label = _('Display/update categories'),
                                     desc = _('Left: Select')),
        'editLine'        : MetaIcon(img = iconSet['line-edit'],
                                     label = _('Edit line/boundary'),
                                     desc = _('Left: new point; Ctrl+Left: undo last point; Right: close line')),
        'moveLine'        : MetaIcon(img = iconSet['line-move'],
                                     label = _('Move feature(s)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'moveVertex'      : MetaIcon(img = iconSet['vertex-move'],
                                     label = _('Move vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'removeVertex'    : MetaIcon(img = iconSet['vertex-delete'],
                                     label = _('Remove vertex'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'settings'        : MetaIcon(img = iconSet['settings'],
                                     label = _('Digitization settings')),
        'splitLine'       : MetaIcon(img = iconSet['line-split'],
                                     label = _('Split line/boundary'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'quit'            : MetaIcon(img = iconSet['quit'],
                                     label = _('Quit digitizer'),
                                     desc = _('Quit digitizer and save changes')),
        'additionalTools' : MetaIcon(img = iconSet['tools'],
                                     label = _('Additional tools ' \
                                                   '(copy, flip, connect, etc.)'),
                                     desc = _('Left: Select; Ctrl+Left: Unselect; Right: Confirm')),
        'undo'             : MetaIcon(img = iconSet['undo'],
                                      label = _('Undo'),
                                      desc = _('Undo previous changes')),
        },
    'profile' : {
        'draw'         : MetaIcon(img = iconSet['show'],
                                  label = _('Draw/re-draw profile')),
        'transect'     : MetaIcon(img = iconSet['layer-raster-profile'],
                                  label = _('Draw transect in map display window to profile')),
        'options'      : MetaIcon(img = iconSet['settings'],
                                  label = _('Profile options')),
        'save'         : MetaIcon(img = iconSet['save'],
                                  label = _('Save profile data to CSV file')),
        'quit'         : MetaIcon(img = iconSet['quit'],
                                  label = _('Quit Profile Analysis Tool'))
        },
    'georectify' : {
        'gcpSet'    : MetaIcon(img = iconSet['gcp-create'],
                               label = _('Set GCP'),
                               desc = _('Define GCP (Ground Control Points)')),
        'georectify': MetaIcon(img = iconSet['georectify'],
                               label = _('Georectify')),
        'gcpRms'    : MetaIcon(img = iconSet['gcp-rms'],
                               label = _('Recalculate RMS error')),
        'gcpSave'   : MetaIcon(img = iconSet['gcp-save'],
                               label = _('Save GCPs to POINTS file')),
        'gcpAdd'    : MetaIcon(img = iconSet['gcp-add'],
                               label = _('Add new GCP')),
        'gcpDelete' : MetaIcon(img = iconSet['gcp-delete'],
                               label = _('Delete selected GCP')),
        'gcpClear'  : MetaIcon(img = iconSet['gcp-remove'],
                                label = _('Clear selected GCP')),
        'gcpReload' : MetaIcon(img = iconSet['reload'],
                               label = _('Reload GCPs from POINTS file')),
        'quit'      : MetaIcon(img = iconSet['quit'],
                               label = _('Quit georectification')),
        'settings'  : MetaIcon(img = iconSet['settings'],
                               label = _('Settings'),
                               desc = _('Settings dialog for georectification tool')),
        },
    'nviz' : {
        'view'    : MetaIcon(img = iconSet['3d-view'],
                             label = _('Switch to view control page'),
                             desc = _('Change view settings')),
        'surface' : MetaIcon(img = iconSet['3d-raster'],
                             label = _('Switch to surface (raster) control page'),
                             desc = _('Change surface (loaded raster maps) settings')),
        'vector'  : MetaIcon(img = iconSet['3d-vector'],
                             label = _('Switch to vector (2D/3D) control page'),
                             desc = _('Change 2D/3D vector settings')),
        'volume'  : MetaIcon(img = iconSet['3d-volume'],
                             label = _('Switch to volume (3D raster) control page'),
                             desc = _('Change volume (loaded 3D raster maps) settings')),
        'light'   : MetaIcon(img = iconSet['3d-light'],
                             label = _('Switch to lighting control page'),
                             desc = _('Change lighting settings')),
        'fringe'  : MetaIcon(img = iconSet['3d-fringe'],
                             label = _('Switch to fringe control page'),
                             desc = _('Switch on/off fringes')),
        'settings': MetaIcon(img = iconSet['settings'],
                             label = _('3D view mode tools'),
                             desc = _('Show/hide 3D view mode settings dialog')),
        'quit'    : MetaIcon(img = iconSet['quit'],
                             label = _('Quit 3D view mode'),
                             desc = _('Switch back to 2D view mode')),
        },
    'modeler' : {
        'new'        : MetaIcon(img = iconSet['create'],
                                label = _('Create new model (Ctrl+N)')),
        'open'       : MetaIcon(img = iconSet['open'],
                                label = _('Load model from file (Ctrl+O)')),
        'save'       : MetaIcon(img = iconSet['save'],
                                label = _('Save current model to file (Ctrl+S)')),
        'toImage'    : MetaIcon(img = iconSet['image-export'],
                                label = _('Export model to image')),
        'toPython'   : MetaIcon(img = iconSet['python-export'],
                                label = _('Export model to Python script')),
        'actionAdd'  : MetaIcon(img = iconSet['module-add'],
                                label = _('Add action (GRASS module) to model')),
        'dataAdd'    : MetaIcon(img = iconSet['data-add'],
                                label = _('Add data item to model')),
        'relation'   : MetaIcon(img = iconSet['relation-create'],
                                label = _('Define relation between data and action items')),
        'run'        : MetaIcon(img = iconSet['execute'],
                                label = _('Run model')),
        'validate'   : MetaIcon(img = iconSet['check'],
                                label = _('Validate model')),
        'settings'   : MetaIcon(img = iconSet['settings'],
                                label = _('Show modeler settings')),
        'properties' : MetaIcon(img = iconSet['options'],
                                label = _('Show model properties')),
        'variables'  : MetaIcon(img = iconSet['modeler-variables'],
                                label = _('Manage model variables')),
        'redraw'     : MetaIcon(img = iconSet['redraw'],
                                label = _('Redraw model canvas')),
        'quit'       : MetaIcon(img = iconSet['quit'],
                                label = _('Quit Graphical Modeler')),
        },
    'misc' : {
        'font' : MetaIcon(img = iconSet['font'],
                          label = _('Select font')),
        'help' : MetaIcon(img = iconSet['help'],
                          label = _('Show help page')),
        'quit' : MetaIcon(img = iconSet['quit'],
                          label = _('Quit')),
        }
    }

# testing ...
if __name__ == '__main__':
    for k, v in Icons.iteritems():
        print v.GetImageName()
