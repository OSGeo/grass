"""
New GRASS icon set
http://robert.szczepanek.pl/icons.php
https://svn.osgeo.org/osgeo/graphics/toolbar-icons/24x24/
"""
__author__ = "Robert Szczepanek"

import os

import globalvar

iconPath = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass")

iconSet = {
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
    "zoom_in"     : 'zoom-in.png',
    "zoom_out"    : 'zoom-out.png',
    "zoom_back"   : 'zoom-last.png',
    "zoommenu"    : 'zoom-more.png',
    "zoom_extent" : 'zoom-extent.png',
    # analyze raster (mapdisplay)
    "analyze"    : 'layer-raster-analyze.png',
    "measure"    : 'measure-length.png',
    "profile"    : 'layer-raster-profile.png',
    "histogram"  : 'layer-raster-histogram.png',
    "font"       : 'font.png',
    # overlay (mapdisplay)
    "overlay"    : 'overlay-add.png',
    "addtext"    : 'text-add.png',
    "addbarscale": 'scalebar-add.png',
    "addlegend"  : 'legend-add.png',
    "quit"       : 'quit.png',
    # digit
    ## add feature
    "digAddPoint": 'point-create.png',
    "digAddLine" : 'line-create.png',
    "digAddBoundary": 'boundary-create.png',
    "digAddCentroid": 'centroid-create.png',
    "digAddArea": 'polygon-create.png',
    ## vertex
    "digAddVertex" : 'vertex-create.png',
    "digMoveVertex" : 'vertex-move.png',
    "digRemoveVertex" : 'vertex-delete.png',
    "digSplitLine" : 'line-split.png',
    ## edit feature
    "digEditLine" : 'line-edit.png',
    "digMoveLine" : 'line-move.png',
    "digDeleteLine" : 'line-delete.png',
    ## cats
    "digDispCats" : 'cats-display.png',
    "digCopyCats" : 'cats-copy.png',
    ## attributes
    "digDispAttr" : 'attributes-display.png',
    ## general
    "digUndo" : 'undo.png',
    "digAdditionalTools" : 'tools.png',
    # layer manager
    "newdisplay" : 'monitor-create.png',
    "fileNew"    : 'create.png',
    "fileLoad"   : 'layer-open.png',
    "fileOpen"   : 'open.png',
    "fileSave"   : 'save.png',
    "fileImport" : 'layer-import.png',
    "addrast"    : 'layer-raster-add.png',
    "rastmisc"   : 'layer-raster-more.png',
    "addrast3d"  : 'layer-raster3d-add.png',
    "addshaded"  : 'layer-shaded-relief-add.png',
    "addrarrow"  : 'layer-aspect-arrow-add.png',
    "addrnum"    : 'layer-cell-cats-add.png',
    "addvect"    : 'layer-vector-add.png',
    "vectmisc"   : 'layer-vector-more.png',
    "addcmd"     : 'layer-command-add.png',
    "addgrp"     : 'layer-group-add.png',
    "addovl"     : 'layer-more.png',
    "addgrid"    : 'layer-grid-add.png',
    "addlabels"  : 'layer-label-add.png',
    "delcmd"     : 'layer-remove.png',
    "attrtable"  : 'table.png',
    "vdigit"     : 'edit.png',
    "addrgb"     : 'layer-rgb-add.png',
    "addhis"     : 'layer-his-add.png',
    "addthematic": 'layer-vector-thematic-add.png',
    "addchart"   : 'layer-vector-chart-add.png',
    "addgeodesic" : 'options.png',
    "addrhumb"    : 'options.png',
    "layeropts"  : 'options.png',
    "modeler"    : 'modeler-main.png',
    # profile analysis
    "transect"     : 'layer-raster-profile.png',
    "profiledraw"  : 'show.png',
    # georectify
    "grGcpSet"     : 'gcp-create.png',
    'grGcpClear'   : 'gcp-remove.png',
    'grGeorect'    : 'georectify.png',
    'grGcpRms'     : 'gcp-rms.png',
    "grGcpSave"    : 'gcp-save.png',
    "grGcpAdd"     : 'gcp-add.png',
    "grGcpDelete"  : 'gcp-delete.png',
    "grGcpReload"  : 'reload.png',
    # modeler
    "modelActionAdd" : 'module-add.png',
    "modelDataAdd"   : 'data-add.png',
    "modelRelation"  : 'relation-create.png',
    "modelRun"       : 'execute.png',
    "modelValidate"  : 'check.png',
    "imageSave"      : 'image-export.png',
    "pythonSave"     : 'python-export.png',
    "modelProperties" : 'options.png',
    "modelVariables" : 'modeler-variables.png',
    # 3d view
    "nvizView"       : '3d-view.png',
    "nvizSurface"    : '3d-raster.png',
    "nvizVector"     : '3d-vector.png',
    "nvizVolume"     : '3d-volume.png',
    "nvizLight"      : '3d-light.png',
    "nvizFringe"     : '3d-fringe.png',
    # various
    "settings"       : 'settings.png',
    "redraw"         : 'redraw.png',
    "help"           : 'help.png',
    "psScript"       : 'ps-script.png',
    "psExport"       : 'ps-export.png',
    }
