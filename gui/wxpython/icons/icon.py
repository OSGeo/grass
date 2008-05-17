"""
MODULE: icon

CLASSES:
 * MetaIcon

PURPOSE: Icon themes

         from icons import Icons as Icons

AUTHORS: The GRASS Development Team
         Martin Landa <landa.martin gmail.com>

COPYRIGHT: (C) 2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import sys

import wx

gmPath = os.path.join(os.getenv("GISBASE"), "etc", "wxpython", "gui_modules")
sys.path.append(gmPath)

import grassenv
import globalvar
from preferences import globalSettings as UserSettings

# iconpath = grassenv.GetGRASSVariable('GRASS_ICONPATH')
# if not iconpath:
#    iconpath = os.getenv("GRASS_ICONPATH")
iconTheme = UserSettings.Get(group='advanced', key='iconTheme', subkey='type')
if iconTheme and iconTheme == 'silk':
    iconpath = os.path.join(os.getenv("GISBASE"), "docs", "html", "icons", "silk")
else:
    iconpath = None

iconpath_default = os.path.join(globalvar.ETCDIR, "gui", "icons")
iconpath_vdigit  = os.path.join(globalvar.ETCDIR, "v.digit")

icons_default = {
    # map display
    "displaymap" : 'gui-display.gif',
    "rendermap"  : 'gui-redraw.gif',
    "erase"      : 'gui-erase.gif',
    "pointer"    : 'gui-pointer.gif',
    "zoom_in"    : 'gui-zoom_in.gif',
    "zoom_out"   : 'gui-zoom_out.gif',
    "pan"        : 'gui-pan.gif',
    "query"      : 'gui-query.gif',
    "zoom_back"  : 'gui-zoom_back.gif',
    "zoommenu"   : 'gui-mapzoom.gif',
    "savefile"   : 'file-save.gif',
    "printmap"   : 'file-print.gif',
    "overlay"    : 'gui-overlay.gif',
    # digit
    ## add feature
    "digAddPoint": 'new.point.gif',
    "digAddLine" : 'new.line.gif',
    "digAddBoundary": 'new.boundary.gif',
    "digAddCentroid": 'new.centroid.gif',
    ## vertex
    "digAddVertex" : 'add.vertex.gif',
    "digMoveVertex" : 'move.vertex.gif',
    "digRemoveVertex" : 'rm.vertex.gif',
    "digSplitLine" : 'split.line.gif',
    ## edit feature
    "digEditLine" : 'edit.line.gif',
    "digMoveLine" : 'move.line.gif',
    "digDeleteLine" : 'delete.line.gif',
    ## cats
    "digCopyCats" : 'copy.cats.gif',
    "digDispCats" : 'display.cats.gif',
    ## attributes
    "digDispAttr" : 'display.attributes.gif',
    ## general
    "digUndo" : wx.ART_ERROR, # FIXME
    "digSettings" : 'settings.gif',
    "digAdditionalTools" : wx.ART_ERROR, # FIXME
    # gis manager
    "newdisplay" : 'gui-startmon.gif',
    "workspaceNew" : 'file-new.gif',
    "workspaceLoad" : 'file-new.gif', # change the icon if possible
    "workspaceOpen" : 'file-open.gif',
    "workspaceSave" : 'file-save.gif',
    "addrast"    : 'element-cell.gif',
    "addvect"    : 'element-vector.gif',
    "addcmd"     : 'gui-cmd.gif',
    "addgrp"     : 'gui-group.gif',
    "addovl"     : 'module-d.grid.gif',
    "delcmd"     : 'edit-cut.gif',
    "attrtable"  : 'db-values.gif',
    "addrgb"     : 'module-d.rgb.gif',
    "addhis"     : 'channel-his.gif',
    "addshaded"  : 'module-d.shadedmap.gif',
    "addrarrow"  : 'module-d.rast.arrow.gif',
    "addrnum"    : 'module-d.rast.num.gif',
    "elvect"     : 'element-vector.gif',
    "addthematic": 'module-d.vect.thematic.gif',
    "addchart"   : 'module-d.vect.chart.gif',
    "addgrid"    : 'module-d.grid.gif',
    "addgeodesic": 'module-d.geodesic.gif',
    "addrhumb"   : 'module-d.rhumbline.gif',
    "addlabels"  : 'module-d.labels.gif',
    "addtext"    : 'module-d.text.gif',
    "addbarscale": 'module-d.barscale.gif',
    "addlegend"  : 'module-d.legend.gif',
    "quit"       : 'gui-exit.gif',
    # analyze raster
    "analyze"    : 'gui-rastanalyze.gif',
    "measure"    : 'gui-measure.gif',
    "font"       : 'gui-font.gif',
    "histogram"  : 'module-d.histogram.gif',
    "color"      : 'edit-color.gif',
    "options"    :  wx.ART_ERROR, # FIXME
    # profile 
    "profile"    : 'gui-profile.gif',
    "transect"   : 'gui-profiledefine.gif',
    # "profiledraw": 'gui-profiledraw.gif',
    "profiledraw" : 'gui-redraw.gif',
    "profileopt" : 'gui-profileopt.gif',
    # georectify
    'grGcpClear'   : 'gui-gcperase.gif',
    'grGcpSet'     : 'gui-gcpset.gif',
    'grGeorect'    : 'gui-georect.gif',
    'grGcpRms'     : 'gui-rms.gif',
    'grGcpRefresh' : 'gui-display.gif',
    "grGcpSave"    : 'file-save.gif', 
    "grGcpAdd"     : wx.ART_ERROR, # FIXME
    "grGcpDelete"  : wx.ART_ERROR, # FIXME
    "grGcpReload"  : 'gui-redraw.gif',
    "grSettings"   : 'edit-color.gif', 
    }

# merge icons dictionaries, join paths
try:
    if iconpath and not os.path.exists(iconpath):
        raise OSError
    if iconpath is not None and iconpath.find('silk') > -1: # silk icon theme
        from silk import IconsSilk as icons_img
        # use default icons if needed
        for key, img in icons_default.iteritems():
            if not icons_img.has_key(key): # add key
                icons_img[key] = img
                if key[0:3] == 'dig':
                    iconpath_tmp = iconpath_vdigit
                else:
                    iconpath_tmp = iconpath_default
            else:
                iconpath_tmp = iconpath

            if icons_img[key]: # join paths
                if type (icons_img[key]) == type(''):
                    icons_img[key] = os.path.join(iconpath_tmp, icons_img[key])
except:
    print >> sys.stderr, _("Unable to load icon theme, using default icon theme...")
    iconpath = None

if iconpath is None: # default icons
    icons_img = icons_default
    for key, img in icons_img.iteritems():
        if img and type (icons_img[key]) == type(''):
            if key[0:3] == 'dig':
                icons_img[key] = os.path.join(iconpath_vdigit, img)
            else:
                icons_img[key] = os.path.join(iconpath_default, img)

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

    def GetBitmap (self, size=None):
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

    def GetLabel (self):
        return self.label

    def GetDesc (self):
        return self.description

#
# create list of icon instances
#
Icons = {
    # map display
    "displaymap" : MetaIcon (img=icons_img["displaymap"],
                             label=_("Display map")),
    "rendermap"  : MetaIcon (img=icons_img["rendermap"],
                             label=_("Re-render map"),
                             desc=_("Force re-rendering of all layers")),
    "erase"      : MetaIcon (img=icons_img["erase"],
                             label=_("Erase display")),
    "pointer"    : MetaIcon (img=icons_img["pointer"],
                             label=_("Pointer")),
    "zoom_in"    : MetaIcon (img=icons_img["zoom_in"],
                             label=_("Zoom in"),
                             desc=_("Drag or click mouse to zoom")),
    "zoom_out"   : MetaIcon (img=icons_img["zoom_out"],
                             label=_("Zoom out"),
                             desc=_("Drag or click mouse to unzoom")),
    "pan"        : MetaIcon (img=icons_img["pan"],
                             label=_("Pan"),
                             desc=_("Drag with mouse to pan")),
    "queryDisplay" : MetaIcon (img=icons_img["query"],
                               label=_("Query raster/vector map(s) (display mode)"),
                               desc=_("Query selected raster/vector map(s)")),
    "queryModify" : MetaIcon (img=icons_img["query"],
                              label=_("Query vector map (editable mode)"),
                             desc=_("Query selected vector map in editable mode")),
    "zoom_back"  : MetaIcon (img=icons_img["zoom_back"],
                             label=_("Return to previous zoom")),
    "zoommenu"   : MetaIcon (img=icons_img["zoommenu"],
                             label=_("Zoom options"),
                             desc=_("Display zoom management")),
    "overlay"    : MetaIcon (img=icons_img["overlay"],
                             label=_("Add overlay"),
                             desc=_("Add graphic overlays to map")),
    "addbarscale": MetaIcon (img=icons_img["addbarscale"],
                             label=_("Add scalebar and north arrow")),
    "addlegend"  : MetaIcon (img=icons_img["addlegend"],
                             label=_("Add legend")),
    "savefile"   : MetaIcon (img=icons_img["savefile"],
                             label=_("Save display to PNG file")),
    "printmap"   : MetaIcon (img=icons_img["printmap"],
                             label=_("Print display")),
    # gis manager
    "newdisplay" : MetaIcon (img=icons_img["newdisplay"],
                             label=_("Start new display")),
    "workspaceNew" : MetaIcon (img=icons_img["workspaceNew"],
                               label=_("Create new workspace file")),
    "workspaceLoad" : MetaIcon (img=icons_img["workspaceLoad"],
                                label=_("Load map layers into workspace")),
    "workspaceOpen" : MetaIcon (img=icons_img["workspaceOpen"],
                                label=_("Open existing workspace file")),
    "workspaceSave" : MetaIcon (img=icons_img["workspaceSave"],
                                label=_("Save current workspace to file")),
    # TODO: "layer" is not conformant with GRASS vocabulary (vector layer: 1..x) ! 
    "addrast"    : MetaIcon (img=icons_img["addrast"],
                             label=_("Add raster map layer")),
    "addvect"    : MetaIcon (img=icons_img["addvect"],
                             label=_("Add vector map layer")),
    "addcmd"     : MetaIcon (img=icons_img["addcmd"],
                             label=_("Add command layer")),
    "addgrp"     : MetaIcon (img=icons_img["addgrp"],
                             label=_("Add layer group")),
    "addovl"     : MetaIcon (img=icons_img["addovl"],
                             label=_("Add grid or vector labels overlay")),
    "delcmd"     : MetaIcon (img=icons_img["delcmd"],
                             label=_("Delete selected layer")),
    "quit"       : MetaIcon (img=icons_img["quit"],
                             label=_("Quit")),
    "attrtable"  : MetaIcon (img=icons_img["attrtable"],
                             label=_("Show attribute table")),
    "addrgb"     : MetaIcon (img=icons_img["addrgb"],
                             label=_("Add RGB layer")),
    "addhis"     : MetaIcon (img=icons_img["addhis"],
                             label=_("Add HIS layer")),
    "addshaded"  : MetaIcon (img=icons_img["addshaded"],
                             label=_("Add shaded relief map layer")),
    "addrarrow"  : MetaIcon (img=icons_img["addrarrow"],
                             label=_("Add raster flow arrows")),
    "addrnum"    : MetaIcon (img=icons_img["addrnum"],
                             label=_("Add raster cell numbers")),
    "elvect"     : MetaIcon (img=icons_img["elvect"],
                             label=_("")),
    "addthematic": MetaIcon (img=icons_img["addthematic"],
                             label=_("Add thematic layer")),
    "addchart"   : MetaIcon (img=icons_img["addchart"],
                             label=_("Add thematic chart layer")),
    "addgrid"    : MetaIcon (img=icons_img["addgrid"],
                             label=_("Add grid layer")),
    "addgeodesic": MetaIcon (img=icons_img["addgeodesic"],
                             label=_("Add geodesic line layer")),
    "addrhumb"   : MetaIcon (img=icons_img["addrhumb"],
                             label=_("Add rhumbline layer")),
    "addlabels"  : MetaIcon (img=icons_img["addlabels"],
                             label=_("Add labels")),
    "addtext"    : MetaIcon (img=icons_img["addtext"],
                             label=_("Add text layer")),
    # digit
    "digAddPoint": MetaIcon (img=icons_img["digAddPoint"],
                             label=_("Digitize new point"),
                             desc=_("Left: new point")),
    "digAddLine" : MetaIcon (img=icons_img["digAddLine"],
                             label=_("Digitize new line"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddBoundary": MetaIcon (img=icons_img["digAddBoundary"],
                                label=_("Digitize new boundary"),
                                desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digAddCentroid": MetaIcon (img=icons_img["digAddCentroid"],
                                label=_("Digitize new centroid"),
                             desc=_("Left: new point")),
    "digAddVertex": MetaIcon (img=icons_img["digAddVertex"],
                              label=_("Add new vertex"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digCopyCats": MetaIcon (img=icons_img["digCopyCats"],
                             label=_("Copy categories"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDeleteLine": MetaIcon (img=icons_img["digDeleteLine"],
                               label=_("Delete feature(s)"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digDispAttr": MetaIcon (img=icons_img["digDispAttr"],
                             label=_("Display/update attributes"),
                             desc=_("Left: Select")),
    "digDispCats": MetaIcon (img=icons_img["digDispCats"],
                             label=_("Display/update categories"),
                             desc=_("Left: Select")),
    "digEditLine": MetaIcon (img=icons_img["digEditLine"],
                             label=_("Edit line/boundary"),
                             desc=_("Left: new point; Middle: undo last point; Right: close line")),
    "digMoveLine": MetaIcon (img=icons_img["digMoveLine"],
                             label=_("Move feature(s)"),
                             desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digMoveVertex": MetaIcon (img=icons_img["digMoveVertex"],
                               label=_("Move vertex"),
                               desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digRemoveVertex": MetaIcon (img=icons_img["digRemoveVertex"],
                                 label=_("Remove vertex"),
                                 desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digSettings": MetaIcon (img=icons_img["digSettings"],
                             label=_("Settings"),
                             desc=_("Settings dialog for digitization tool")),
    "digSplitLine": MetaIcon (img=icons_img["digSplitLine"],
                              label=_("Split line/boundary"),
                              desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digExit"    : MetaIcon (img=icons_img["quit"],
                             label=_("Quit digitizing tool")),
    "digAdditionalTools" : MetaIcon (img=icons_img["digAdditionalTools"],
                                     label=_("Additional tools " \
                                                 "(copy, flip, connect, etc.)"),
                                     desc=_("Left: Select; Middle: Unselect; Right: Confirm")),
    "digUndo" : MetaIcon (img=icons_img["digUndo"],
                          label=_("Undo"),
                          desc=_("Undo previous changes")),
    # analyze raster
    "analyze"    : MetaIcon (img=icons_img["analyze"],
                             label=_("Analyze map")),
    "measure"    : MetaIcon (img=icons_img["measure"],
                             label=_("Measure distance")),
    "transect"   : MetaIcon (img=icons_img["transect"],
                             label=_("Draw transect in map display window to profile")),
    "profile"    : MetaIcon (img=icons_img["profile"],
                             label=_("Profile surface map")),
    "profiledraw": MetaIcon (img=icons_img["profiledraw"],
                             label=_("Draw/re-draw profile")),
    "profileopt" : MetaIcon (img=icons_img["profileopt"],
                             label=_("Profile options")),
    "histogram"  : MetaIcon (img=icons_img["histogram"],
                             label=_("Create histogram of image or raster file")),
    "font"       : MetaIcon (img=icons_img["font"],
                             label=_("Select font")),
    "color"      : MetaIcon (img=icons_img["color"],
                             label=_("Select color")),
    "options"    : MetaIcon (img=icons_img["options"],
                             label=_("Set histogram options")),
    "analyze"    : MetaIcon (img=icons_img["analyze"],
                             label=_("Analyze")),
    # georectify
    'grGcpSet'     : MetaIcon (img=icons_img["grGcpSet"],
                             label=_("Set GCP"),
                             desc=_("Define GCP (Ground Control Points)")),
    'grGeorect'    : MetaIcon (img=icons_img["grGeorect"],
                             label=_("Georectify")),
    'grGcpRms'        : MetaIcon (img=icons_img["grGcpRms"],
                                  label=_("Recalculate RMS error")),
    'grGcpRefresh' : MetaIcon (img=icons_img["grGcpRefresh"],
                               label=_("Redraw GCP markers in map displays")),
    'grGcpSave' : MetaIcon (img=icons_img["grGcpSave"],
                            label=_("Save GCPs to POINTS file")),
    'grGcpAdd' : MetaIcon (img=icons_img["grGcpAdd"],
                           label=_("Add new GCP")),
    'grGcpDelete' : MetaIcon (img=icons_img["grGcpDelete"],
                              label=_("Delete selected GCP")),
    'grGcpClear' : MetaIcon (img=icons_img["grGcpClear"],
                             label=_("Clear selected GCP")),
    'grGcpReload' : MetaIcon (img=icons_img["grGcpReload"],
                              label=_("Reload GCPs from selected POINTS file")),
    'grGcpQuit' : MetaIcon (img=icons_img["quit"],
                            label=_("Quit georectification module")),
    "grSettings": MetaIcon (img=icons_img["grSettings"],
                            label=_("Settings"),
                            desc=_("Settings dialog for georectification tool")),
    }

# testing ...
if __name__ == "__main__":
    for k,v in Icons.iteritems():
        print k, "/", v

