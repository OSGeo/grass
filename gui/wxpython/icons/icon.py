"""
@package icon

@brief Icon themes

@code
from icons import Icons as Icons
@endcode

Classes:
 - MetaIcon

(C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

import gui_modules.globalvar as globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx

gmPath = os.path.join(os.getenv("GISBASE"), "etc", "wxpython", "gui_modules")
sys.path.append(gmPath)

import globalvar
from preferences import globalSettings as UserSettings

import grass_icons # default icon set
iconpath_default = grass_icons.iconpath
iconpath_vdigit = grass_icons.iconpath_vdigit
Icons_default = grass_icons.IconsGrass

iconTheme = UserSettings.Get(group='advanced', key='iconTheme', subkey='type')
if iconTheme == 'silk':
    import silk_icons
    iconpath = silk_icons.iconpath
    Icons = silk_icons.IconsSilk
elif iconTheme == 'grass2':
    import grass2_icons
    iconpath = grass2_icons.iconpath
    Icons = grass2_icons.IconsGrass2
else:
    iconpath = iconpath_default
    Icons = grass_icons.IconsGrass

# merge icons dictionaries, join paths
try:
    if iconpath and not os.path.exists(iconpath):
        raise OSError
    
    if iconTheme != 'grass':
        # use default icons if no icon is available
        for key, img in Icons_default.iteritems():
            if not Icons.has_key(key) or \
                    Icons[key] is None: # add key
                Icons[key] = img
            
            if Icons[key] == img:
                if key[0:3] == 'dig':
                    iconpath_tmp = iconpath_vdigit
                else:
                    iconpath_tmp = iconpath_default
            else:
                iconpath_tmp = iconpath
            
            Icons[key] = os.path.join(iconpath_tmp, Icons[key])
    else:
        for key, img in Icons.iteritems():
            if img and type (Icons[key]) == type(''):
                if key[0:3] == 'dig':
                    Icons[key] = os.path.join(iconpath_vdigit, img)
                else:
                    Icons[key] = os.path.join(iconpath_default, img)
except:
    print >> sys.stderr, _("Unable to load icon theme...")
    sys.exit(1)

class MetaIcon:
    """
    Handle icon metadata (image path, tooltip, ...)
    """
    def __init__(self, img, label, desc=None):
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
        """Debugging"""
        return "label=%s, img=%s, type=%s" % (self.label, self.imagepath, self.type)

    def GetBitmap(self, size=None):
        """Get bitmap"""
        bmp = None

        if self.type == 'wx':
            bmp = wx.ArtProvider.GetBitmap(id=self.imagepath, client=wx.ART_TOOLBAR, size=size)
        elif self.type == 'img':
            if os.path.isfile(self.imagepath) and os.path.getsize(self.imagepath):
                if size and len(size) == 2:
                    image = wx.Image (name=self.imagepath)
                    image.Rescale (size[0], size[1])
                    bmp = image.ConvertToBitmap()
                elif self.imagepath:
                    bmp = wx.Bitmap (name=self.imagepath)

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
    "displaymap" : MetaIcon (img=Icons["displaymap"],
                             label=_("Display map")),
    "rendermap"  : MetaIcon (img=Icons["rendermap"],
                             label=_("Re-render map"),
                             desc=_("Force re-rendering of all layers")),
    "erase"      : MetaIcon (img=Icons["erase"],
                             label=_("Erase display")),
    "pointer"    : MetaIcon (img=Icons["pointer"],
                             label=_("Pointer")),
    "zoom_in"    : MetaIcon (img=Icons["zoom_in"],
                             label=_("Zoom in"),
                             desc=_("Drag or click mouse to zoom")),
    "zoom_out"   : MetaIcon (img=Icons["zoom_out"],
                             label=_("Zoom out"),
                             desc=_("Drag or click mouse to unzoom")),
    "pan"        : MetaIcon (img=Icons["pan"],
                             label=_("Pan"),
                             desc=_("Drag with mouse to pan")),
    "query" : MetaIcon (img=Icons["query"],
                        label=_("Query raster/vector map(s)"),
                        desc=_("Query selected raster/vector map(s)")),
    "zoom_back"  : MetaIcon (img=Icons["zoom_back"],
                             label=_("Return to previous zoom")),
    "zoommenu"   : MetaIcon (img=Icons["zoommenu"],
                             label=_("Zoom options"),
                             desc=_("Display zoom management")),
    "overlay"    : MetaIcon (img=Icons["overlay"],
                             label=_("Add overlay"),
                             desc=_("Add graphic overlays to map")),
    "addbarscale": MetaIcon (img=Icons["addbarscale"],
                             label=_("Add scalebar and north arrow")),
    "addlegend"  : MetaIcon (img=Icons["addlegend"],
                             label=_("Add legend")),
    "savefile"   : MetaIcon (img=Icons["savefile"],
                             label=_("Save display to PNG file")),
    "printmap"   : MetaIcon (img=Icons["printmap"],
                             label=_("Print display")),
    # gis manager
    "newdisplay" : MetaIcon (img=Icons["newdisplay"],
                             label=_("Start new display")),
    "workspaceNew" : MetaIcon (img=Icons["workspaceNew"],
                               label=_("Create new workspace file")),
    "workspaceLoad" : MetaIcon (img=Icons["workspaceLoad"],
                                label=_("Load map layers into workspace")),
    "workspaceOpen" : MetaIcon (img=Icons["workspaceOpen"],
                                label=_("Open existing workspace file")),
    "workspaceSave" : MetaIcon (img=Icons["workspaceSave"],
                                label=_("Save current workspace to file")),
    # TODO: "layer" is not conformant with GRASS vocabulary (vector layer: 1..x) ! 
    "addrast"    : MetaIcon (img=Icons["addrast"],
                             label=_("Add raster map layer")),
    "addvect"    : MetaIcon (img=Icons["addvect"],
                             label=_("Add vector map layer")),
    "addcmd"     : MetaIcon (img=Icons["addcmd"],
                             label=_("Add command layer")),
    "addgrp"     : MetaIcon (img=Icons["addgrp"],
                             label=_("Add layer group")),
    "addovl"     : MetaIcon (img=Icons["addovl"],
                             label=_("Add grid or vector labels overlay")),
    "delcmd"     : MetaIcon (img=Icons["delcmd"],
                             label=_("Delete selected layer")),
    "quit"       : MetaIcon (img=Icons["quit"],
                             label=_("Quit")),
    "attrtable"  : MetaIcon (img=Icons["attrtable"],
                             label=_("Show attribute table")),
    "addrgb"     : MetaIcon (img=Icons["addrgb"],
                             label=_("Add RGB layer")),
    "addhis"     : MetaIcon (img=Icons["addhis"],
                             label=_("Add HIS layer")),
    "addshaded"  : MetaIcon (img=Icons["addshaded"],
                             label=_("Add shaded relief map layer")),
    "addrarrow"  : MetaIcon (img=Icons["addrarrow"],
                             label=_("Add raster flow arrows")),
    "addrnum"    : MetaIcon (img=Icons["addrnum"],
                             label=_("Add raster cell numbers")),
    "addthematic": MetaIcon (img=Icons["addthematic"],
                             label=_("Add thematic layer")),
    "addchart"   : MetaIcon (img=Icons["addchart"],
                             label=_("Add thematic chart layer")),
    "addgrid"    : MetaIcon (img=Icons["addgrid"],
                             label=_("Add grid layer")),
    "addgeodesic": MetaIcon (img=Icons["addgeodesic"],
                             label=_("Add geodesic line layer")),
    "addrhumb"   : MetaIcon (img=Icons["addrhumb"],
                             label=_("Add rhumbline layer")),
    "addlabels"  : MetaIcon (img=Icons["addlabels"],
                             label=_("Add labels")),
    "addtext"    : MetaIcon (img=Icons["addtext"],
                             label=_("Add text layer")),
    "addrast3d"  : MetaIcon (img=Icons["addrast3d"],
                             label=_("Add 3D raster map")),
    # digit
    "digAddPoint": MetaIcon (img=Icons["digAddPoint"],
                             label=_("Digitize new point"),
                             desc=_("Left: new point")),
    "digAddLine" : MetaIcon (img=Icons["digAddLine"],
                             label=_("Digitize new line"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddBoundary": MetaIcon (img=Icons["digAddBoundary"],
                                label=_("Digitize new boundary"),
                                desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddCentroid": MetaIcon (img=Icons["digAddCentroid"],
                                label=_("Digitize new centroid"),
                             desc=_("Left: new point")),
    "digAddVertex": MetaIcon (img=Icons["digAddVertex"],
                              label=_("Add new vertex"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digCopyCats": MetaIcon (img=Icons["digCopyCats"],
                             label=_("Copy categories"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDeleteLine": MetaIcon (img=Icons["digDeleteLine"],
                               label=_("Delete feature(s)"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDispAttr": MetaIcon (img=Icons["digDispAttr"],
                             label=_("Display/update attributes"),
                             desc=_("Left: Select")),
    "digDispCats": MetaIcon (img=Icons["digDispCats"],
                             label=_("Display/update categories"),
                             desc=_("Left: Select")),
    "digEditLine": MetaIcon (img=Icons["digEditLine"],
                             label=_("Edit line/boundary"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digMoveLine": MetaIcon (img=Icons["digMoveLine"],
                             label=_("Move feature(s)"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digMoveVertex": MetaIcon (img=Icons["digMoveVertex"],
                               label=_("Move vertex"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digRemoveVertex": MetaIcon (img=Icons["digRemoveVertex"],
                                 label=_("Remove vertex"),
                                 desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digSettings": MetaIcon (img=Icons["digSettings"],
                             label=_("Settings"),
                             desc=_("Settings dialog for digitization tool")),
    "digSplitLine": MetaIcon (img=Icons["digSplitLine"],
                              label=_("Split line/boundary"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digExit"    : MetaIcon (img=Icons["quit"],
                             label=_("Quit digitizing tool")),
    "digAdditionalTools" : MetaIcon (img=Icons["digAdditionalTools"],
                                     label=_("Additional tools " \
                                                 "(copy, flip, connect, etc.)"),
                                     desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digUndo" : MetaIcon (img=Icons["digUndo"],
                          label=_("Undo"),
                          desc=_("Undo previous changes")),
    # analyze raster
    "analyze"    : MetaIcon (img=Icons["analyze"],
                             label=_("Analyze map")),
    "measure"    : MetaIcon (img=Icons["measure"],
                             label=_("Measure distance")),
    "transect"   : MetaIcon (img=Icons["transect"],
                             label=_("Draw transect in map display window to profile")),
    "profile"    : MetaIcon (img=Icons["profile"],
                             label=_("Profile surface map")),
    "profiledraw": MetaIcon (img=Icons["profiledraw"],
                             label=_("Draw/re-draw profile")),
    "profileopt" : MetaIcon (img=Icons["profileopt"],
                             label=_("Profile options")),
    "histogram"  : MetaIcon (img=Icons["histogram"],
                             label=_("Create histogram of image or raster file")),
    "font"       : MetaIcon (img=Icons["font"],
                             label=_("Select font")),
    "color"      : MetaIcon (img=Icons["color"],
                             label=_("Select color")),
    "layeropts"  : MetaIcon (img=Icons["layeropts"],
                             label=_("Set options")),
    "analyze"    : MetaIcon (img=Icons["analyze"],
                             label=_("Analyze")),
    # georectify
    'grGcpSet'     : MetaIcon (img=Icons["grGcpSet"],
                             label=_("Set GCP"),
                             desc=_("Define GCP (Ground Control Points)")),
    'grGeorect'    : MetaIcon (img=Icons["grGeorect"],
                             label=_("Georectify")),
    'grGcpRms'        : MetaIcon (img=Icons["grGcpRms"],
                                  label=_("Recalculate RMS error")),
    'grGcpRefresh' : MetaIcon (img=Icons["grGcpRefresh"],
                               label=_("Redraw GCP markers in map displays")),
    'grGcpSave' : MetaIcon (img=Icons["grGcpSave"],
                            label=_("Save GCPs to POINTS file")),
    'grGcpAdd' : MetaIcon (img=Icons["grGcpAdd"],
                           label=_("Add new GCP")),
    'grGcpDelete' : MetaIcon (img=Icons["grGcpDelete"],
                              label=_("Delete selected GCP")),
    'grGcpClear' : MetaIcon (img=Icons["grGcpClear"],
                             label=_("Clear selected GCP")),
    'grGcpReload' : MetaIcon (img=Icons["grGcpReload"],
                              label=_("Reload GCPs from selected POINTS file")),
    'grGcpQuit' : MetaIcon (img=Icons["quit"],
                            label=_("Quit georectification module")),
    "grSettings": MetaIcon (img=Icons["grSettings"],
                            label=_("Settings"),
                            desc=_("Settings dialog for georectification tool")),
    # nviz
    "nvizSettings": MetaIcon (img=Icons["nvizSettings"],
                              label=_("Settings"),
                              desc=_("Show Nviz settings dialog")),
    }

# testing ...
if __name__ == "__main__":
    for k, v in Icons.iteritems():
        print v.GetImageName()

