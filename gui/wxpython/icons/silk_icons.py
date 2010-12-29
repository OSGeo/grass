"""
Silk icon set, v1.3
http://www.famfamfam.com/lab/icons/silk/

"""
__author__ = "Mark James"
__version__ = "1.3"

import os

import wx

import globalvar

iconpath = os.path.join(globalvar.ETCDIR, "gui", "icons", "silk")

IconsSilk = {
    # map display
    "displaymap" : 'map_go.png',
    "rendermap"  : 'arrow_refresh.png',
    "erase"      : 'cross.png',
    "pointer"    : 'cursor.png',
    "query"      : 'information.png',
    "savefile"   : 'picture_save.png',
    "printmap"   : 'printer.png',
    "pan"        : 'arrow_out.png', 
    # zoom (mapdisplay)
    "zoom_in"    : 'zoom_in.png',
    "zoom_out"   : 'zoom_out.png',
    "zoom_back"  : 'zoom_back.png',
    "zoommenu"   : 'zoom.png',
    "zoom_extent" : 'zoom_extent.png',
    # analyze raster (mapdisplay)
    "analyze"    : 'application_lightning.png',
    "measure"    : 'sum.png',
    "profile"    : 'wand.png',
    "histogram"  : 'chart_bar.png',
    "font"       : 'font.png',
    # overlay (mapdisplay)
    "overlay"    : 'overlays.png',
    "addtext"    : 'textfield_add.png',
    "addbarscale": 'page_white_picture.png',
    "addlegend"  : 'page_green.png',
    "quit"       : 'door_in.png',
    # digit
    ## add feature
    "digAddPoint": 'bullet_add.png',
    "digAddLine" : 'vector_add.png',
    "digAddBoundary": 'shape_handles.png',
    "digAddCentroid": 'shape_square_add.png',
    ## vertex
    "digAddVertex" : 'chart_line_add.png',
    "digMoveVertex" : 'chart_line.png',
    "digRemoveVertex" : 'chart_line_delete.png',
    "digSplitLine" : 'chart_line_link.png',
    ## edit feature
    "digEditLine" : 'chart_line_edit.png',
    "digMoveLine" : 'bullet_go.png',
    "digDeleteLine" : 'vector_delete.png',
    ## cats
    "digDispCats" : 'chart_organisation.png',
    "digCopyCats" : 'chart_organisation_add.png',
    ## attributes
    "digDispAttr" : 'table.png',
    ## general
    "digUndo" : 'arrow_undo.png',
    "digAdditionalTools" : 'plugin.png',
    # layer manager
    "newdisplay" : 'application_add.png',
    "fileNew"    : 'page_white.png',
    "fileLoad"   : 'page_white_get.png',
    "fileOpen"   : 'folder.png',
    "fileSave"   : 'page_save.png',
    "fileImport" : 'page_white_get.png',
    "addrast"    : 'image_add.png',
    "rastmisc"   : 'picture_empty.png',
    "addrast3d"  : 'bricks.png',
    "addshaded"  : 'picture_empty.png',
    "addrarrow"  : 'arrow_inout.png',
    "addrnum"    : 'color_swatch.png',
    "addvect"    : 'map_add.png',
    "vectmisc"   : 'thematic.png',
    "addcmd"     : 'cog_add.png',
    "addgrp"     : 'folder_add.png',
    "addovl"     : 'images.png',
    "addgrid"    : 'application_view_icons.png',
    "addlabels"  : 'tag_blue_add.png',
    "delcmd"     : 'bin_closed.png',
    "attrtable"  : 'application_view_columns.png',
    "addrgb"     : 'rgb.png',
    "addhis"     : 'his.png',
    "addthematic": 'thematic.png',
    "addchart"   : 'chart_bar.png',
    "layeropts"  : 'map_edit.png',
    "modeler"    : wx.ART_ERROR,
    # profile analysis
    "transect"   : 'image_edit.png',
    "profiledraw"  : 'arrow_refresh.png',
    "profileopt"   : 'color_swatch.png',
    # georectify
    "grGcpSet"     : 'bullet_add.png',
    'grGcpClear'   : 'cross.png',
    'grGeorect'    : 'application_lightning.png',
    'grGcpRms'     : 'error.png',
    "grGcpSave"    : 'picture_save.png',
    "grGcpAdd"     : 'bullet_add.png', 
    "grGcpDelete"  : 'bullet_delete.png',
    "grGcpReload"  : 'arrow_refresh.png',
    # modeler
    "modelActionAdd" : wx.ART_ERROR,
    "modelDataAdd"   : wx.ART_ERROR,
    "modelRelation"  : wx.ART_ERROR,
    "modelRun"       : wx.ART_ERROR,
    "modelValidate"  : wx.ART_ERROR,
    "imageSave"      : wx.ART_ERROR,
    "pythonSave"     : wx.ART_ERROR,
    "modelProperties" : wx.ART_ERROR,
    "modelVariables" : wx.ART_ERROR,
    # 3d view
    "nvizView"       : wx.ART_ERROR,
    "nvizSurface"    : wx.ART_ERROR,
    "nvizVector"     : wx.ART_ERROR,
    "nvizVolume"     : wx.ART_ERROR,
    "nvizLight"      : wx.ART_ERROR,
    "nvizFringe"     : wx.ART_ERROR,
    # various
    "settings"       : 'color_swatch.png',
    "redraw"         : 'arrow_refresh.png',
    "help"           : wx.ART_ERROR,
    }
