"""
New GRASS icon set
http://robert.szczepanek.pl/icons.php
"""
__author__ = "Robert Szczepanek"

import os

import globalvar

iconpath = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass2")

IconsGrass2 = {
    # map display
    "displaymap" : 'show.png',
    "rendermap"  : 'layer-redraw.png',
    "erase"      : 'erase.png',
    "pointer"    : 'pointer.png',
    "query"      : 'info.png',
    "savefile"   : 'map-export.png',
    "printmap"   : 'print.png',
    "pan"        : 'pan.png', 
    # zoom (mapdisplay)
    "zoom_in"    : 'zoom-in.png',
    "zoom_out"   : 'zoom-out.png',
    "zoom_back"  : 'zoom-last.png',
    "zoommenu"   : 'zoom-more.png',
    # analyze raster (mapdisplay)
    "analyze"    : 'layer-raster-analyze.png',
    "measure"    : 'measure-length.png',
    "profile"    : 'layer-raster-profile.png',
    "histogram"  : 'layer-raster-histogram.png',
    "font"       : None,
    # overlay (mapdisplay)
    "overlay"    : 'element-add.png',
    "addtext"    : 'text-add.png',
    "addbarscale": 'scalebar-add.png',
    "addlegend"  : 'legend-add.png',
    "quit"       : 'quit.png',
    # digit
    ## add feature
    "digAddPoint": None,
    "digAddLine" : None,
    "digAddBoundary": None,
    "digAddCentroid": None,
    ## vertex
    "digAddVertex" : None,
    "digMoveVertex" : None,
    "digRemoveVertex" : None,
    "digSplitLine" : None,
    ## edit feature
    "digEditLine" : None,
    "digMoveLine" : None,
    "digDeleteLine" : None,
    ## cats
    "digDispCats" : None,
    "digCopyCats" : None,
    ## attributes
    "digDispAttr" : None,
    ## general
    "digUndo" : None,
    "digSettings" : None,
    "digAdditionalTools" : None,
    # layer manager
    "newdisplay" : 'monitor-create.png',
    "workspaceNew" : 'create.png',
    "workspaceLoad" : 'layer-open.png',
    "workspaceOpen" : 'open.png',
    "workspaceSave" : 'save.png',
    "addrast"    : 'layer-raster-add.png',
    "addrast3d"  : None,
    "addshaded"  : 'layer-shaded-relief-add.png',
    "addrarrow"  : 'layer-aspect-arrow-add.png',
    "addrnum"    : None,
    "addvect"    : 'layer-vector-add.png',
    "addcmd"     : 'layer-command-add.png',
    "addgrp"     : 'layer-group-add.png',
    "addovl"     : 'layer-grid-add.png',
    "addgrid"    : 'layer-grid-add.png',
    "addlabels"  : 'layer-label-add.png',
    "delcmd"     : 'layer-remove.png',
    "attrtable"  : 'table.png',
    "addrgb"     : 'layer-rgb-add.png',
    "addhis"     : 'layer-his-add.png',
    "addthematic": None,
    "addchart"   : None,
    "layeropts"  : None,
    # profile analysis
    "transect"   : 'layer-raster-profile.png',
    "profiledraw"  : None,
    "profileopt"   : None,
    # georectify
    "grGcpSet"     : None,
    'grGcpClear'   : None,
    'grGeorect'    : None,
    'grGcpRms'     : None,
    'grGcpRefresh' : None,
    "grGcpSave"    : None,
    "grGcpAdd"     : None,
    "grGcpDelete"  : None,
    "grGcpReload"  : None,
    "grSettings"   : None,
    # nviz
    "nvizSettings"   : None,
    }
