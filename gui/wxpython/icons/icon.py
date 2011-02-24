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
    # map display
    "displaymap" : MetaIcon (img=iconSet["displaymap"],
                             label=_("Display map"),
                             desc = _("Re-render modified map layers")),
    "rendermap"  : MetaIcon (img=iconSet["rendermap"],
                             label=_("Re-render map"),
                             desc=_("Force re-rendering all map layers")),
    "erase"      : MetaIcon (img=iconSet["erase"],
                             label=_("Erase display")),
    "pointer"    : MetaIcon (img=iconSet["pointer"],
                             label=_("Pointer")),
    "zoom_in"    : MetaIcon (img=iconSet["zoom_in"],
                             label=_("Zoom in"),
                             desc=_("Drag or click mouse to zoom")),
    "zoom_out"   : MetaIcon (img=iconSet["zoom_out"],
                             label=_("Zoom out"),
                             desc=_("Drag or click mouse to unzoom")),
    "pan"        : MetaIcon (img=iconSet["pan"],
                             label=_("Pan"),
                             desc=_("Drag with mouse to pan")),
    "query" : MetaIcon (img=iconSet["query"],
                        label=_("Query raster/vector map(s)"),
                        desc=_("Query selected raster/vector map(s)")),
    "zoom_back"  : MetaIcon (img=iconSet["zoom_back"],
                             label=_("Return to previous zoom")),
    "zoommenu"   : MetaIcon (img=iconSet["zoommenu"],
                             label=_("Zoom options"),
                             desc=_("Display zoom management")),
    "zoom_extent" : MetaIcon (img=iconSet["zoom_extent"],
                             label=_("Zoom to selected map layer(s)")),
    "overlay"    : MetaIcon (img=iconSet["overlay"],
                             label=_("Add map elements"),
                             desc=_("Overlay elements like scale and legend onto map")),
    "addbarscale": MetaIcon (img=iconSet["addbarscale"],
                             label=_("Add scalebar and north arrow")),
    "addlegend"  : MetaIcon (img=iconSet["addlegend"],
                             label=_("Add legend")),
    "savefile"   : MetaIcon (img=iconSet["savefile"],
                             label=_("Save display to graphic file")),
    "printmap"   : MetaIcon (img=iconSet["printmap"],
                             label=_("Print display")),
    # layer manager
    "newdisplay" : MetaIcon (img=iconSet["newdisplay"],
                             label=_("Start new map display")),
    "workspaceNew" : MetaIcon (img=iconSet["fileNew"],
                               label=_("Create new workspace (Ctrl+N)")),
    "workspaceLoad" : MetaIcon (img=iconSet["fileLoad"],
                                label=_("Load map layers into workspace (Ctrl+L)")),
    "workspaceOpen" : MetaIcon (img=iconSet["fileOpen"],
                                label=_("Open existing workspace file (Ctrl+O)")),
    "workspaceSave" : MetaIcon (img=iconSet["fileSave"],
                                label=_("Save current workspace to file (Ctrl+S)")),
    "rastImport" : MetaIcon (img=iconSet["fileImport"],
                             label=_("Import raster data")),
    "rastLink" : MetaIcon (img=iconSet["fileImport"],
                             label=_("Link external raster data")),
    "vectImport" : MetaIcon (img=iconSet["fileImport"],
                             label=_("Import vector data")),
    "vectLink" : MetaIcon (img=iconSet["fileImport"],
                             label=_("Link external vector data")),
    "addrast"    : MetaIcon (img=iconSet["addrast"],
                             label=_("Add raster map layer (Ctrl+R)")),
    "rastmisc" : MetaIcon (img=iconSet["rastmisc"],
                             label=_("Add various raster map layers (RGB, HIS, shaded relief...)")),
    "addvect"    : MetaIcon (img=iconSet["addvect"],
                             label=_("Add vector map layer (Ctrl+V)")),
    "vectmisc" : MetaIcon (img=iconSet["vectmisc"],
                             label=_("Add various vector map layers (thematic, chart...)")),
    "addcmd"     : MetaIcon (img=iconSet["addcmd"],
                             label=_("Add command layer")),
    "addgrp"     : MetaIcon (img=iconSet["addgrp"],
                             label=_("Add group")),
    "addovl"     : MetaIcon (img=iconSet["addovl"],
                             label=_("Add grid or vector labels overlay")),
    "delcmd"     : MetaIcon (img=iconSet["delcmd"],
                             label=_("Delete selected map layer")),
    "quit"       : MetaIcon (img=iconSet["quit"],
                             label=_("Quit")),
    "attrtable"  : MetaIcon (img=iconSet["attrtable"],
                             label=_("Show attribute table")),
    "vdigit"     : MetaIcon (img=iconSet["vdigit"],
                             label=_("Edit vector maps")),
    "addrgb"     : MetaIcon (img=iconSet["addrgb"],
                             label=_("Add RGB map layer")),
    "addhis"     : MetaIcon (img=iconSet["addhis"],
                             label=_("Add HIS map layer")),
    "addshaded"  : MetaIcon (img=iconSet["addshaded"],
                             label=_("Add shaded relief map layer")),
    "addrarrow"  : MetaIcon (img=iconSet["addrarrow"],
                             label=_("Add raster flow arrows")),
    "addrnum"    : MetaIcon (img=iconSet["addrnum"],
                             label=_("Add raster cell numbers")),
    "addthematic": MetaIcon (img=iconSet["addthematic"],
                             label=_("Add thematic area (choropleth) map layer")),
    "addchart"   : MetaIcon (img=iconSet["addchart"],
                             label=_("Add thematic chart layer")),
    "addgrid"    : MetaIcon (img=iconSet["addgrid"],
                             label=_("Add grid layer")),
    "addgeodesic": MetaIcon (img=iconSet["addgeodesic"],
                             label=_("Add geodesic line layer")),
    "addrhumb"   : MetaIcon (img=iconSet["addrhumb"],
                             label=_("Add rhumbline layer")),
    "addlabels"  : MetaIcon (img=iconSet["addlabels"],
                             label=_("Add labels")),
    "addtext"    : MetaIcon (img=iconSet["addtext"],
                             label=_("Add text layer")),
    "addrast3d"  : MetaIcon (img=iconSet["addrast3d"],
                             label=_("Add 3D raster map layer"),
                             desc = _("Note that 3D raster data are rendered only in 3D view mode")),
    "settings"   : MetaIcon (img=iconSet["settings"],
                             label=_("Show GUI settings")),
    # digit
    "digAddPoint": MetaIcon (img=iconSet["digAddPoint"],
                             label=_("Digitize new point"),
                             desc=_("Left: new point")),
    "digAddLine" : MetaIcon (img=iconSet["digAddLine"],
                             label=_("Digitize new line"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddBoundary": MetaIcon (img=iconSet["digAddBoundary"],
                                label=_("Digitize new boundary"),
                                desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddCentroid": MetaIcon (img=iconSet["digAddCentroid"],
                                label=_("Digitize new centroid"),
                             desc=_("Left: new point")),
    "digAddArea": MetaIcon (img=iconSet["digAddArea"],
                                label=_("Digitize new area (composition of bondaries without category and one centroid with category)"),
                             desc=_("Left: new point")),
    "digAddVertex": MetaIcon (img=iconSet["digAddVertex"],
                              label=_("Add new vertex"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digCopyCats": MetaIcon (img=iconSet["digCopyCats"],
                             label=_("Copy categories"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDeleteLine": MetaIcon (img=iconSet["digDeleteLine"],
                               label=_("Delete feature(s)"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDispAttr": MetaIcon (img=iconSet["digDispAttr"],
                             label=_("Display/update attributes"),
                             desc=_("Left: Select")),
    "digDispCats": MetaIcon (img=iconSet["digDispCats"],
                             label=_("Display/update categories"),
                             desc=_("Left: Select")),
    "digEditLine": MetaIcon (img=iconSet["digEditLine"],
                             label=_("Edit line/boundary"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digMoveLine": MetaIcon (img=iconSet["digMoveLine"],
                             label=_("Move feature(s)"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digMoveVertex": MetaIcon (img=iconSet["digMoveVertex"],
                               label=_("Move vertex"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digRemoveVertex": MetaIcon (img=iconSet["digRemoveVertex"],
                                 label=_("Remove vertex"),
                                 desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digSettings": MetaIcon (img=iconSet["settings"],
                             label=_("Settings"),
                             desc=_("Settings dialog for digitization tool")),
    "digSplitLine": MetaIcon (img=iconSet["digSplitLine"],
                              label=_("Split line/boundary"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digExit"    : MetaIcon (img=iconSet["quit"],
                             label=_("Quit digitizing tool")),
    "digAdditionalTools" : MetaIcon (img=iconSet["digAdditionalTools"],
                                     label=_("Additional tools " \
                                                 "(copy, flip, connect, etc.)"),
                                     desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digUndo" : MetaIcon (img=iconSet["digUndo"],
                          label=_("Undo"),
                          desc=_("Undo previous changes")),
    # analyze raster
    "analyze"    : MetaIcon (img=iconSet["analyze"],
                             label=_("Analyze map")),
    "measure"    : MetaIcon (img=iconSet["measure"],
                             label=_("Measure distance")),
    "transect"   : MetaIcon (img=iconSet["transect"],
                             label=_("Draw transect in map display window to profile")),
    "profile"    : MetaIcon (img=iconSet["profile"],
                             label=_("Profile surface map")),
    "profiledraw": MetaIcon (img=iconSet["profiledraw"],
                             label=_("Draw/re-draw profile")),
    "profileopt" : MetaIcon (img=iconSet["settings"],
                             label=_("Profile options")),
    "datasave"   : MetaIcon (img=iconSet["fileSave"],
                             label=_("Save profile data to csv file")),
    "histogram"  : MetaIcon (img=iconSet["histogram"],
                             label=_("Create histogram of image or raster file")),
    "font"       : MetaIcon (img=iconSet["font"],
                             label=_("Select font")),
#    "color"      : MetaIcon (img=iconSet["color"],
#                             label=_("Select color")),
    "layeropts"  : MetaIcon (img=iconSet["layeropts"],
                             label=_("Set options")),
    "analyze"    : MetaIcon (img=iconSet["analyze"],
                             label=_("Analyze")),
    # georectify
    'grGcpSet'     : MetaIcon (img=iconSet["grGcpSet"],
                             label=_("Set GCP"),
                             desc=_("Define GCP (Ground Control Points)")),
    'grGeorect'    : MetaIcon (img=iconSet["grGeorect"],
                             label=_("Georectify")),
    'grGcpRms'        : MetaIcon (img=iconSet["grGcpRms"],
                                  label=_("Recalculate RMS error")),
    'grGcpSave' : MetaIcon (img=iconSet["grGcpSave"],
                            label=_("Save GCPs to POINTS file")),
    'grGcpAdd' : MetaIcon (img=iconSet["grGcpAdd"],
                           label=_("Add new GCP")),
    'grGcpDelete' : MetaIcon (img=iconSet["grGcpDelete"],
                              label=_("Delete selected GCP")),
    'grGcpClear' : MetaIcon (img=iconSet["grGcpClear"],
                             label=_("Clear selected GCP")),
    'grGcpReload' : MetaIcon (img=iconSet["grGcpReload"],
                              label=_("Reload GCPs from POINTS file")),
    'grGcpQuit' : MetaIcon (img=iconSet["quit"],
                            label=_("Quit georectification module")),
    "grSettings": MetaIcon (img=iconSet["settings"],
                            label=_("Settings"),
                            desc=_("Settings dialog for georectification tool")),
    "grHelp": MetaIcon (img=iconSet["help"],
                        label=_('Show help'),
                        desc = _('Display GCP Manager manual page')),
    # nviz
    "nvizView": MetaIcon (img=iconSet["nvizView"],
                          label=_("Switch to view control page"),
                          desc=_("Change view settings")),
    "nvizSurface": MetaIcon (img=iconSet["nvizSurface"],
                             label=_("Switch to surface (raster) control page"),
                             desc=_("Change surface (loaded raster maps) settings")),
    "nvizVector": MetaIcon (img=iconSet["nvizVector"],
                            label=_("Switch to vector (2D/3D) control page"),
                            desc=_("Change 2D/3D vector settings")),
    "nvizVolume": MetaIcon (img=iconSet["nvizVolume"],
                            label=_("Switch to volume (3D raster) control page"),
                            desc=_("Change volume (loaded 3D raster maps) settings")),
    "nvizLight": MetaIcon (img=iconSet["nvizLight"],
                           label=_("Switch to lighting control page"),
                           desc=_("Change lighting settings")),
    "nvizFringe": MetaIcon (img=iconSet["nvizFringe"],
                            label=_("Switch to fringe control page"),
                            desc=_("Switch on/off fringes")),
    "nvizSettings": MetaIcon (img=iconSet["settings"],
                              label=_("3D view mode tools"),
                              desc=_("Show/hide 3D view mode settings dialog")),
    "nvizHelp"   : MetaIcon (img=iconSet["help"],
                             label=_("Show help"),
                             desc = _("Display 3D view mode manual page")),
    "nvizQuit": MetaIcon (img=iconSet["quit"],
                          label=_("Quit 3D view mode"),
                          desc=_("Switch back to 2D view mode")),
    # modeler
    "modeler" : MetaIcon (img=iconSet["modeler"],
                          label=_("Start Graphical Modeler")),
    "modelNew" : MetaIcon (img=iconSet["fileNew"],
                           label=_("Create new model (Ctrl+N)")),
    "modelOpen" : MetaIcon (img=iconSet["fileOpen"],
                                label=_("Load model from file (Ctrl+O)")),
    "modelSave" : MetaIcon (img=iconSet["fileSave"],
                                label=_("Save current model to file (Ctrl+S)")),
    "modelToImage" : MetaIcon (img=iconSet["imageSave"],
                                label=_("Export model to image")),
    "modelToPython" : MetaIcon (img=iconSet["pythonSave"],
                                label=_("Export model to Python script")),
    "modelActionAdd" : MetaIcon (img=iconSet["modelActionAdd"],
                                 label=_("Add action (GRASS module) to model")),
    "modelDataAdd" : MetaIcon (img=iconSet["modelDataAdd"],
                                 label=_("Add data item to model")),
    "modelRelation" : MetaIcon (img=iconSet["modelRelation"],
                                label=_("Define relation between data and action items")),
    "modelRun" : MetaIcon (img=iconSet["modelRun"],
                           label=_("Run model")),
    "modelValidate" : MetaIcon (img=iconSet["modelValidate"],
                                label=_("Validate model")),
    "modelSettings" : MetaIcon (img=iconSet["settings"],
                                label=_("Show modeler settings")),
    "modelProperties" : MetaIcon (img=iconSet["modelProperties"],
                                  label=_("Show model properties")),
    "modelVariables" : MetaIcon (img=iconSet["modelVariables"],
                                 label=_("Manage model variables")),
    "modelRedraw" : MetaIcon (img=iconSet["redraw"],
                              label=_("Redraw model canvas")),
    "modelHelp"   : MetaIcon (img=iconSet["help"],
                             label=_("Show help"),
                             desc = _("Display Graphical Modeler manual page")),
    # ps.map
    "psScript" : MetaIcon (img=iconSet["psScript"],
                           label=_("Generate instruction file")),
    "psExport" : MetaIcon (img=iconSet["psExport"],
                           label=_("Generate PostScript output")),
    }

# testing ...
if __name__ == "__main__":
    for k, v in Icons.iteritems():
        print v.GetImageName()
