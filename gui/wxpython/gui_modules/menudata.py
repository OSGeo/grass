"""
@package menudata.py

@brief Complex list for main menu entries for GRASS wxPython GUI.

Classes:
 - Data

COPYRIGHT:  (C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton (Arizona State University)
@author Yann Chemin
@author Martin Landa <landa.martin gmail.com>
"""

import os

class Data:
    '''Data object that returns menu descriptions to be used in wxgui.py.
    Probably could be changed to XML or *.dtd file.'''
    def GetMenu(self):
        return [(
                (_("File"), (
                        (_("Workspace"), (

                                (_("New workspace"),
                                 _("Create new workspace file (erase current workspace settings first)"),
                                 "self.OnWorkspaceNew",
                                 ""),

                                (_("Open existing workspace"),
                                 _("Open existing workspace file"),
                                 "self.OnWorkspaceOpen",
                                 ""),

                                (_("Load map layers"),
                                 _("Load map layers into layer tree"),
                                 "self.OnWorkspaceLoad",
                                 ""),

                                (_("Load GRC file (Tcl/Tk GUI)"),
                                 _("Load map layers from GRC file to layer tree (not fully implemented)"),
                                 "self.OnWorkspaceLoadGrcFile",
                                 ""),

                                (_("Save workspace"),
                                 _("Save current workspace to file"),
                                 "self.OnWorkspaceSave",
                                 ""),

                                (_("Save workspace as"),
                                 _("Save current workspace as file"),
                                 "self.OnWorkspaceSaveAs",
                                 ""),

                                (_("Close current workspace"),
                                 _("Close current workspace file"),
                                 "self.OnWorkspaceClose",
                                 ""),
                                )
                         ),
                        ("","","", ""),
                        (_("Import raster map"), (

                                (_("Import raster data using GDAL"),
                                 _("Import GDAL supported raster file into a binary raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.in.gdal"),
                                (_("Multiple raster data import using GDAL"),
                                 _("Converts selected GDAL layers to GRASS raster maps using r.in.gdal."),
                                 "self.OnImportGdalLayers",
                                 ""),

                                ("","","", ""),

                                (_("Aggregate ASCII xyz import"),
                                 _("Create a raster map from an assemblage of many coordinates using univariate statistics."),
                                 "self.OnMenuCmd",
                                 "r.in.xyz"),

                                (_("ASCII grid import"),
                                 _("Converts ASCII raster file to binary raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.in.ascii"),

                                (_("ASCII polygons and lines import"),
                                 _("Creates raster maps from ASCII polygon/line/point data files."),
                                 "self.OnMenuCmd",
                                 "r.in.poly"),
                                ("","","", ""),

                                (_("Binary file import"),
                                 _("Import a binary raster file into a GRASS raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.in.bin"),

                                (_("ESRI ASCII grid import"),
                                 _("Converts an ESRI ARC/INFO ascii raster file (GRID) into a (binary) raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.in.arc"),

                                (_("GRIDATB.FOR import"),
                                 _("Imports GRIDATB.FOR map file (TOPMODEL) into GRASS raster map"),
                                 "self.OnMenuCmd",
                                 "r.in.gridatb"),

                                (_("MAT-File (v.4) import"),
                                 _("Imports a binary MAT-File(v4) to a GRASS raster."),
                                 "self.OnMenuCmd",
                                 "r.in.mat"),

                                (_("SPOT NDVI import"),
                                 _("Import of SPOT VGT NDVI file into a raster map"),
                                 "self.OnMenuCmd",
                                 "i.in.spotvgt"),

                                (_("SRTM HGT import"),
                                 _("Import SRTM HGT files into GRASS"),
                                 "self.OnMenuCmd",
                                 "r.in.srtm"),

                                (_("Terra ASTER HDF import"),
                                 _("Georeference, rectify and import Terra-ASTER imagery and relative DEM's using gdalwarp."),
                                 "self.OnMenuCmd",
                                 "r.in.aster"),
                                ("","","", ""),

                                (_("WMS import"),
                                 _("Downloads and imports data from WMS servers."),
                                 "self.OnMenuCmd",
                                 "r.in.wms"),
                                )
                         ),
                        (_("Import vector map"), (

                                (_("Import vector data using OGR"),
                                 _("Convert OGR vector layers to GRASS vector map."),
                                 "self.OnMenuCmd",
                                 "v.in.ogr"),
                                
                                (_("Multiple vector data import using OGR"),
                                 _("Converts selected OGR layers to GRASS vector maps using v.in.ogr."),
                                 "self.OnImportOgrLayers",
                                 ""),
                                ("","","", ""),

                                (_("ASCII points/GRASS ASCII vector import"),
                                 _("Creates a vector map from ASCII points file or ASCII vector file."),
                                 "self.OnMenuCmd",
                                 "v.in.ascii"),

                                (_("Old GRASS vector import"),
                                 _("Imports older versions of GRASS vector maps."),
                                 "self.OnMenuCmd",
                                 "v.convert"),
                                ("","","", ""),

                                (_("DXF import"),
                                 _("Converts files in DXF format to GRASS vector map format."),
                                 "self.OnMenuCmd",
                                 "v.in.dxf"),

                                (_("Multiple DXF layers import"),
                                 _("Converts selected DXF layers to GRASS vector maps (using v.in.dxf)."),
                                 "self.OnImportDxfFile",
                                 ""),
                                ("","","", ""),
                                
                                (_("ESRI e00 import"),
                                 _("Import E00 file into a vector map."),
                                 "self.OnMenuCmd",
                                 "v.in.e00"),

                                (_("Garmin GPS import"),
                                 _("Download waypoints, routes, and tracks from a Garmin GPS receiver into a vector map."),
                                 "self.OnMenuCmd",
                                 "v.in.garmin"),

                                (_("GPSBabel GPS import"),
                                 _("Import waypoints, routes, and tracks from a GPS receiver or GPS download file into a vector map."),
                                 "self.OnMenuCmd",
                                 "v.in.gpsbabel"),

                                (_("GEOnet import"),
                                 _("Imports US-NGA GEOnet Names Server (GNS) country files into a GRASS vector points map."),
                                 "self.OnMenuCmd",
                                 "v.in.gns"),

                                (_("Matlab and MapGen import"),
                                 _("Import Mapgen or Matlab vector maps into GRASS."),
                                 "self.OnMenuCmd",
                                 "v.in.mapgen"),
                                )
                         ),
                        (_("Import grid 3D volume"), (

                                (_("ASCII 3D import"),
                                 _("Convert a 3D ASCII raster text file into a (binary) 3D raster map layer"),
                                 "self.OnMenuCmd",
                                 "r3.in.ascii"),

                                (_("Vis5D import"),
                                 _("import of 3-dimensional Vis5D files (i.e. the v5d file with 1 variable and 1 time step)"),
                                 "self.OnMenuCmd",
                                 "r3.in.v5d"),
                                )
                         ),
                        (_("Import database table"), (

                                (_("Multiple import formats using OGR"),
                                 _("Imports attribute tables in various formats."),
                                 "self.OnMenuCmd",
                                 "db.in.ogr"),
                                )
                         ),
                        ("","","", ""),
                        (_("Export raster map"), (

                                (_("Multiple export formats using GDAL"),
                                 _("Exports GRASS raster map into GDAL supported formats."),
                                 "self.OnMenuCmd",
                                 "r.out.gdal"),
                                ("","","", ""),

                                (_("ASCII grid export"),
                                 _("Converts a raster map layer into an ASCII text file."),
                                 "self.OnMenuCmd",
                                 "r.out.ascii"),

                                (_("ASCII x,y,z export"),
                                 _("Export a raster map to a text file as x,y,z values based on cell centers."),
                                 "self.OnMenuCmd",
                                 "r.out.xyz"),
                                ("","","", ""),

                                (_("ESRI ASCII grid export"),
                                 _("Converts a raster map layer into an ESRI ARCGRID file."),
                                 "self.OnMenuCmd",
                                 "r.out.arc"),

                                (_("GRIDATB.FOR export"),
                                 _("Exports GRASS raster map to GRIDATB.FOR map file (TOPMODEL)"),
                                 "self.OnMenuCmd",
                                 "r.out.gridatb"),

                                (_("MAT-File (v.4) export"),
                                 _("Exports a GRASS raster to a binary MAT-File."),
                                 "self.OnMenuCmd",
                                 "r.out.mat"),
                                ("","","", ""),

                                (_("Binary export"),
                                 _("Exports a GRASS raster to a binary array."),
                                 "self.OnMenuCmd",
                                 "r.out.bin"),
                                ("","","", ""),

                                (_("MPEG-1 export"),
                                 _("Raster File Series to MPEG Conversion Program."),
                                 "self.OnMenuCmd",
                                 "r.out.mpeg"),

                                (_("PNG export"),
                                 _("Export GRASS raster as non-georeferenced PNG image format."),
                                 "self.OnMenuCmd",
                                 "r.out.png"),

                                (_("PPM export"),
                                 _("Converts a GRASS raster map to a PPM image file at the pixel resolution of the currently defined region."),
                                 "self.OnMenuCmd",
                                 "r.out.ppm"),

                                (_("PPM from RGB export"),
                                 _("Converts 3 GRASS raster layers (R,G,B) to a PPM image file at the pixel resolution of the CURRENTLY DEFINED REGION."),
                                 "self.OnMenuCmd",
                                 "r.out.ppm3"),

                                (_("POV-Ray export"),
                                 _("Converts a raster map layer into a height-field file for POVRAY."),
                                 "self.OnMenuCmd",
                                 "r.out.pov"),

                                (_("TIFF export"),
                                 _("Exports a GRASS raster map to a 8/24bit TIFF image file at the pixel resolution of the currently defined region."),
                                 "self.OnMenuCmd",
                                 "r.out.tiff"),

                                (_("VRML export"),
                                 _("Export a raster map to the Virtual Reality Modeling Language (VRML)"),
                                 "self.OnMenuCmd",
                                 "r.out.vrml"),

                                (_("VTK export"),
                                 _("Converts raster maps into the VTK-Ascii format"),
                                 "self.OnMenuCmd",
                                 "r.out.vtk"),
                                )
                         ),
                        (_("Export vector map"), (

                                (_("Multiple export formats using OGR"),
                                 _("Converts to one of the supported OGR vector formats."),
                                 "self.OnMenuCmd",
                                 "v.out.ogr"),
                                ("","","", ""),

                                (_("ASCII points/GRASS ASCII vector export"),
                                 _("Converts a GRASS binary vector map to a GRASS ASCII vector map."),
                                 "self.OnMenuCmd",
                                 "v.out.ascii"),

                                (_("DXF export"),
                                 _("Exports GRASS vector map layers to DXF file format."),
                                 "self.OnMenuCmd",
                                 "v.out.dxf"),

                                (_("POV-Ray export"),
                                 _("Converts to POV-Ray format, GRASS x,y,z -> POV-Ray x,z,y"),
                                 "self.OnMenuCmd",
                                 "v.out.pov"),

                                (_("SVG export"),
                                 _("Exports a GRASS vector map to SVG."),
                                 "self.OnMenuCmd",
                                 "v.out.svg"),

                                (_("VTK export"),
                                 _("Converts a GRASS binary vector map to VTK ASCII output."),
                                 "self.OnMenuCmd",
                                 "v.out.vtk"),
                                )
                         ),
                        (_("Export grid 3D volume"), (

                                (_("ASCII 3D export"),
                                 _("Converts a 3D raster map layer into an ASCII text file"),
                                 "self.OnMenuCmd",
                                 "r3.out.ascii"),

                                (_("Vis5D export"),
                                 _("Export of GRASS 3D raster map to 3-dimensional Vis5D file."),
                                 "self.OnMenuCmd",
                                 "r3.out.v5d"),

                                (_("VTK export"),
                                 _("Converts 3D raster maps (G3D) into the VTK-Ascii format"),
                                 "self.OnMenuCmd",
                                 "r3.out.vtk"),
                                )
                         ),
                        ("","","", ""),
                        (_("Manage maps and volumes"), (

                                (_("Copy"),
                                 _("Copies available data files in the user's current mapset search path and location to the appropriate element directories under the user's current mapset."),
                                 "self.OnMenuCmd",
                                 "g.copy"),
                                ("","","", ""),

                                (_("List"),
                                 _("Lists available GRASS data base files of the user-specified data type to standard output."),
                                 "self.OnMenuCmd",
                                 "g.list"),

                                (_("List filtered"),
                                 _("Apply regular expressions and wildcards to g.list"),
                                 "self.OnMenuCmd",
                                 "g.mlist"),
                                ("","","", ""),

                                (_("Rename"),
                                 _("Renames data base element files in the user's current mapset."),
                                 "self.OnMenuCmd",
                                 "g.rename"),
                                ("","","", ""),

                                (_("Delete"),
                                 _("Removes data base element files from the user's current mapset."),
                                 "self.OnMenuCmd",
                                 "g.remove"),

                                (_("Delete filtered"),
                                 _("Apply regular expressions and wildcards to g.remove"),
                                 "self.OnMenuCmd",
                                 "g.mremove"),
                                )
                         ),
                        (_("Map type conversions"), (

                                (_("Raster to vector"),
                                 _("Converts a raster map into a vector map layer."),
                                 "self.OnMenuCmd",
                                 "r.to.vect"),

                                (_("Raster series to volume"),
                                 _("Converts 2D raster map slices to one 3D raster volume map."),
                                 "self.OnMenuCmd",
                                 "r.to.rast3"),

                                (_("Raster 2.5D to volume"),
                                 _("Creates a 3D volume map based on 2D elevation and value raster maps."),
                                 "self.OnMenuCmd",
                                 "r.to.rast3elev"),
                                ("","","", ""),

                                (_("Vector to raster"),
                                 _("Converts a binary GRASS vector map layer into a GRASS raster map layer."),
                                 "self.OnMenuCmd",
                                 "v.to.rast"),

                                (_("Vector to volume"),
                                 _("Converts a binary GRASS vector map (only points) layer into a 3D GRASS raster map layer."),
                                 "self.OnMenuCmd",
                                 "v.to.rast3"),

                                (_("Sites to vector"),
                                 _("Converts a GRASS site_lists file into a vector map."),
                                 "self.OnMenuCmd",
                                 "v.in.sites"),
                                ("","","", ""),

                                (_("Volume to raster series"),
                                 _("Converts 3D raster maps to 2D raster maps"),
                                 "self.OnMenuCmd",
                                 "r3.to.rast"),
                                )
                         ),
                        ("","","", ""),

                        (_("Georectify"),
                         _("Georectify raster and vector maps"),
                         "self.OnGeorectify",
                         ""),
                        ("","","", ""),

                        (_("NVIZ (requires Tcl/Tk)"),
                         _("nviz - Visualization and animation tool for GRASS data"),
                         "self.OnMenuCmd",
                         "nviz"),
                        ("","","", ""),

                        (_("Bearing/distance to coordinates"),
                         _("It assumes a cartesian coordinate system"),
                         "self.OnMenuCmd",
                         "m.cogo"),
                        ("","","", ""),

                        (_("Postscript plot"),
                         _("Hardcopy PostScript map output utility."),
                         "self.OnMenuCmd",
                         "ps.map"),
                        ("","","", ""),

                        (_("E&xit"),
                         _("Exit GUI"),
                         "self.OnCloseWindow",
                         ""),
                        )
                 ),
                (_("Config"), (
                        (_("Region"), (

                                (_("Display region"),
                                 _("Manages the boundary definitions for the geographic region."),
                                 "self.RunMenuCmd",
                                 "g.region -p"),

                                (_("Set region"),
                                 _("Manages the boundary definitions for the geographic region."),
                                 "self.OnMenuCmd",
                                 "g.region -p"),
                                )
                         ),
                        (_("GRASS working environment"), (

                                (_("Mapset access"),
                                 _("Set/unset access to other mapsets in current location"),
                                 "self.OnMapsets",
                                 ""),

                                (_("Change working environment"),
                                 _("Change current mapset."),
                                 "self.OnMenuCmd",
                                 "g.mapset"),

                                (_("User access"),
                                 _("Controls access to the current mapset for other users on the system."),
                                 "self.OnMenuCmd",
                                 "g.access"),

                                (_("Show settings"),
                                 _("Outputs and modifies the user's current GRASS variable settings."),
                                 "self.RunMenuCmd",
                                 "g.gisenv --v"),

                                (_("Change settings"),
                                 _("Outputs and modifies the user's current GRASS variable settings."),
                                 "self.OnMenuCmd",
                                 "g.gisenv"),

                                (_("Version"),
                                 _("Displays version and copyright information."),
                                 "self.RunMenuCmd",
                                 "g.version -c"),
                                )
                         ),
                        (_("Manage projections"), (

                                (_("Manage projections"),
                                 _("Converts co-ordinate system descriptions (i.e. projection information) between various formats (including GRASS format). Can also be used to create GRASS locations."),
                                 "self.OnMenuCmd",
                                 "g.proj"),

                                (_("Projection for current location"),
                                 _("Create/edit projection information for current location"),
                                 "self.OnXTerm",
                                 "g.setproj"),
                                ("","","", ""),

                                (_("Convert coordinates"),
                                 _("Convert coordinates from one projection to another (cs2cs frontend)."),
                                 "self.OnMenuCmd",
                                 "m.proj"),
                                )
                         ),

                        (_("Preferences"),
                         _("User GUI preferences (display font, commands, digitizer, etc.)"),
                         "self.OnPreferences",
                         ""),
                        )
                 ),
                (_("Raster"), (
                        (_("Develop raster map"), (

                                (_("Digitize raster (requires XTerm)"),
                                 _("Digitize raster map"),
                                 "self.OnXTerm",
                                 "r.digit"),
                                ("","","", ""),

                                (_("Compress/decompress"),
                                 _("Compresses and decompresses raster maps."),
                                 "self.OnMenuCmd",
                                 "r.compress"),
                                ("","","", ""),

                                (_("Region boundaries"),
                                 _("Sets the boundary definitions for a raster map."),
                                 "self.OnMenuCmd",
                                 "r.region"),

                                (_("Manage NULL values"),
                                 _("Creates explicitly the NULL-value bitmap file."),
                                 "self.OnMenuCmd",
                                 "r.null"),

                                (_("Quantization"),
                                 _("Produces the quantization file for a floating-point map."),
                                 "self.OnMenuCmd",
                                 "r.quant"),

                                (_("Timestamp"),
                                 _("Print/add/remove a timestamp for a raster map."),
                                 "self.OnMenuCmd",
                                 "r.timestamp"),
                                ("","","", ""),

                                (_("Resample using aggregate statistics"),
                                 _("Resamples raster map layers using aggregation."),
                                 "self.OnMenuCmd",
                                 "r.resamp.stats"),

                                (_("Resample using multiple methods"),
                                 _("Resamples raster map layers using interpolation."),
                                 "self.OnMenuCmd",
                                 "r.resamp.interp"),

                                (_("Resample using nearest neighbor"),
                                 _("GRASS raster map layer data resampling capability."),
                                 "self.OnMenuCmd",
                                 "r.resample"),

                                (_("Resample using spline tension"),
                                 _("Reinterpolates and optionally computes topographic analysis from input raster map to a new raster map (possibly with different resolution) using regularized spline with tension and smoothing."),
                                 "self.OnMenuCmd",
                                 "r.resamp.rst"),
                                ("","","", ""),

                                (_("Support file maintenance"),
                                 _("Allows creation and/or modification of raster map layer support files."),
                                 "self.OnMenuCmd",
                                 "r.support"),

                                (_("Update map statistics"),
                                 _("Update raster map statistics"),
                                 "self.OnMenuCmd",
                                 "r.support.stats"),
                                ("","","", ""),

                                (_("Reproject raster"),
                                 _("Re-projects a raster map from one location to the current location."),
                                 "self.OnMenuCmd",
                                 "r.proj"),

                                (_("Tiling"),
                                 _("Produces tilings of the source projection for use in the destination region and projection."),
                                 "self.OnMenuCmd",
                                 "r.tileset"),
                                )
                         ),
                        (_("Manage colors"), (

                                (_("Color tables"),
                                 _("Creates/modifies the color table associated with a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.colors"),

                                (_("Color rules"),
                                 _("Set colors interactively by entering color rules"),
                                 "self.RulesCmd",
                                 "r.colors"),
                                ("","","", ""),

                                (_("Blend 2 color rasters"),
                                 _("Blends color components of two raster maps by a given ratio."),
                                 "self.OnMenuCmd",
                                 "r.blend"),

                                (_("Create RGB"),
                                 _("Combines red, green and blue map layers into a single composite map layer."),
                                 "self.OnMenuCmd",
                                 "r.composite"),

                                (_("RGB to HIS"),
                                 _("Generates red, green and blue raster map layers combining hue, intensity and saturation (HIS) values from user-specified input raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.his"),
                                )
                         ),

                        (_("Query by coordinates"),
                         _("Queries raster map layers on their category values and category labels."),
                         "self.OnMenuCmd",
                         "r.what"),
                        ("","","", ""),

                        (_("Buffer rasters"),
                         _("Creates a raster map layer showing buffer zones surrounding cells that contain non-NULL category values."),
                         "self.OnMenuCmd",
                         "r.buffer"),

                        (_("Closest points"),
                         _("Locates the closest points between objects in two raster maps."),
                         "self.OnMenuCmd",
                         "r.distance"),

                        (_("Mask"),
                         _("Create a MASK for limiting raster operation"),
                         "self.OnMenuCmd",
                         "r.mask"),

                        (_("Map calculator"),
                         _("Map calculator for raster map algebra"),
                         "self.DispMapCalculator",
                         ""),
                        (_("Neighborhood analysis"), (

                                (_("Moving window"),
                                 _("Makes each cell category value a function of the category values assigned to the cells around it, and stores new cell values in an output raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.neighbors"),

                                (_("Neighborhood points"),
                                 _("Makes each cell value a function of the attribute values assigned to the vector points or centroids around it, and stores new cell values in an output raster map layer."),
                                 "self.OnMenuCmd",
                                 "v.neighbors"),
                                )
                         ),
                        (_("Overlay rasters"), (

                                (_("Cross product"),
                                 _("Creates a cross product of the category values from multiple raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.cross"),

                                (_("Raster series"),
                                 _("Makes each output cell value a function of the values assigned to the corresponding cells in the input raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.series"),

                                (_("Patch raster maps"),
                                 _("Creates a composite raster map layer by using known category values from one (or more) map layer(s) to fill in areas of \"no data\" in another map layer."),
                                 "self.OnMenuCmd",
                                 "r.patch"),
                                ("","","", ""),

                                (_("Statistical overlay"),
                                 _("Calculates category or object oriented statistics."),
                                 "self.OnMenuCmd",
                                 "r.statistics"),
                                )
                         ),
                        (_("Solar radiance and shadows"), (

                                (_("Solar irradiance and irradiation"),
                                 _("Computes direct (beam), diffuse and reflected solar irradiation raster maps for given day, latitude, surface and atmospheric conditions. Solar parameters (e.g. sunrise, sunset times, declination, extraterrestrial irradiance, daylight length) are saved in the map history file. Alternatively, a local time can be specified to compute solar incidence angle and/or irradiance raster maps. The shadowing effect of the topography is optionally incorporated."),
                                 "self.OnMenuCmd",
                                 "r.sun"),

                                (_("Shadows map"),
                                 _("Calculates cast shadow areas from sun position and DEM. Either A: exact sun position is specified, or B: date/time to calculate the sun position by r.sunmask itself."),
                                 "self.OnMenuCmd",
                                 "r.sunmask"),
                                )
                         ),
                        (_("Terrain analysis"), (

                                (_("Cumulative movement costs"),
                                 _("Outputs a raster map layer showing the anisotropic cumulative cost of moving between different geographic locations on an input elevation raster map layer whose cell category values represent elevation combined with an input raster map layer whose cell values represent friction cost."),
                                 "self.OnMenuCmd",
                                 "r.walk"),

                                (_("Cost surface"),
                                 _("Outputs a raster map layer showing the cumulative cost of moving between different geographic locations on an input raster map layer whose cell category values represent cost."),
                                 "self.OnMenuCmd",
                                 "r.cost"),

                                (_("Least cost route or flow"),
                                 _("Traces a flow through an elevation model on a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.drain"),
                                ("","","", ""),

                                (_("Shaded relief"),
                                 _("Creates shaded relief map from an elevation map (DEM)."),
                                 "self.OnMenuCmd",
                                 "r.shaded.relief"),
                                ("","","", ""),

                                (_("Slope and aspect"),
                                 _("Generates raster map layers of slope, aspect, curvatures and partial derivatives from a raster map layer of true elevation values. Aspect is calculated counterclockwise from east."),
                                 "self.OnMenuCmd",
                                 "r.slope.aspect"),

                                (_("Terrain parameters"),
                                 _("Uses a multi-scale approach by taking fitting quadratic parameters to any size window (via least squares)."),
                                 "self.OnMenuCmd",
                                 "r.param.scale"),

                                (_("Textural features"),
                                 _("Generate images with textural features from a raster map."),
                                 "self.OnMenuCmd",
                                 "r.texture"),
                                ("","","", ""),

                                (_("Visibility"),
                                 _("Line-of-sight raster analysis program."),
                                 "self.OnMenuCmd",
                                 "r.los"),
                                )
                         ),
                        (_("Transform features"), (

                                (_("Clump"),
                                 _("Recategorizes data in a raster map layer by grouping cells that form physically discrete areas into unique categories."),
                                 "self.OnMenuCmd",
                                 "r.clump"),

                                (_("Grow"),
                                 _("Generates a raster map layer with contiguous areas grown by one cell."),
                                 "self.OnMenuCmd",
                                 "r.grow"),

                                (_("Thin"),
                                 _("Thins non-zero cells that denote linear features in a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.thin"),
                                )
                         ),
                        ("","","", ""),
                        (_("Hydrologic modeling"), (

                                (_("Carve stream channels"),
                                 _("Takes vector stream data, transforms it to raster and subtracts depth from the output DEM."),
                                 "self.OnMenuCmd",
                                 "r.carve"),

                                (_("Fill lake"),
                                 _("Fills lake from seed at given level"),
                                 "self.OnMenuCmd",
                                 "r.lake"),
                                ("","","", ""),

                                (_("Depressionless map and flowlines"),
                                 _("Filters and generates a depressionless elevation map and a flow direction map from a given elevation layer."),
                                 "self.OnMenuCmd",
                                 "r.fill.dir"),

                                (_("Flow accumulation"),
                                 _("Flow computation for massive grids (Float version)."),
                                 "self.OnMenuCmd",
                                 "r.terraflow"),

                                (_("Flow lines"),
                                 _("Construction of slope curves (flowlines), flowpath lengths, and flowline densities (upslope areas) from a raster digital elevation model (DEM)"),
                                 "self.OnMenuCmd",
                                 "r.flow"),
                                ("","","", ""),

                                (_("SIMWE Overland flow modeling"),
                                 _("Overland flow hydrologic simulation using path sampling method (SIMWE)"),
                                 "self.OnMenuCmd",
                                 "r.sim.water"),

                                (_("SIMWE Sediment flux modeling"),
                                 _("Sediment transport and erosion/deposition simulation using path sampling method (SIMWE)"),
                                 "self.OnMenuCmd",
                                 "r.sim.sediment"),
                                ("","","", ""),

                                (_("Topographic index map"),
                                 _("Creates topographic index [ln(a/tan(beta))] map from elevation map."),
                                 "self.OnMenuCmd",
                                 "r.topidx"),

                                (_("TOPMODEL simulation"),
                                 _("Simulates TOPMODEL which is a physically based hydrologic model."),
                                 "self.OnMenuCmd",
                                 "r.topmodel"),
                                ("","","", ""),

                                (_("Watershed subbasins"),
                                 _("Generates a raster map layer showing watershed subbasins."),
                                 "self.OnMenuCmd",
                                 "r.basins.fill"),

                                (_("Watershed analysis"),
                                 _("Watershed basin analysis program."),
                                 "self.OnMenuCmd",
                                 "r.watershed"),

                                (_("Watershed basin creation"),
                                 _("Watershed basin creation program."),
                                 "self.OnMenuCmd",
                                 "r.water.outlet"),
                                )
                         ),
                        (_("Landscape structure modeling"), (

                                (_("Set up (requires XTerm)"),
                                 _("Set up sampling and analysis framework"),
                                 "self.OnXTerm",
                                 "r.le.setup"),
                                ("","","", ""),

                                (_("Analyze landscape"),
                                 _("Contains a set of measures for attributes, diversity, texture, juxtaposition, and edge."),
                                 "self.OnMenuCmd",
                                 "r.le.pixel"),

                                (_("Analyze patches"),
                                 _("Calculates attribute, patch size, core (interior) size, shape, fractal dimension, and perimeter measures for sets of patches in a landscape."),
                                 "self.OnMenuCmd",
                                 "r.le.patch"),

                                (_("Output"),
                                 _("Displays the boundary of each r.le patch and shows how the boundary is traced, displays the attribute, size, perimeter and shape indices for each patch and saves the data in an output file."),
                                 "self.OnMenuCmd",
                                 "r.le.trace"),
                                )
                         ),
                        (_("Landscape patch analysis"), (

                                (_("Set up sampling and analysis framework"),
                                 _("Configuration editor for r.li.'index'"),
                                 "self.OnMenuCmd",
                                 "r.li.setup"),
                                ("","","", ""),

                                (_("Edge density"),
                                 _("Calculates edge density index on a raster map, using a 4 neighbour algorithm"),
                                 "self.OnMenuCmd",
                                 "r.li.edgedensity"),

                                (_("Contrast weighted edge density"),
                                 _("Calculates contrast weighted edge density index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.cwed"),
                                ("","","", ""),

                                (_("Patch area mean"),
                                 _("Calculates mean patch size index on a raster map, using a 4 neighbour algorithm"),
                                 "self.OnMenuCmd",
                                 "r.li.mps"),

                                (_("Patch area range"),
                                 _("Calculates range of patch area size on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.padrange"),

                                (_("Patch area Std Dev"),
                                 _("Calculates standard deviation of patch area a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.padsd"),

                                (_("Patch area Coeff Var"),
                                 _("Calculates coefficient of variation of patch area on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.padcv"),

                                (_("Patch density"),
                                 _("Calculates patch density index on a raster map, using a 4 neighbour algorithm"),
                                 "self.OnMenuCmd",
                                 "r.li.patchdensity"),

                                (_("Patch number"),
                                 _("Calculates patch number index on a raster map, using a 4 neighbour algorithm."),
                                 "self.OnMenuCmd",
                                 "r.li.patchnum"),
                                ("","","", ""),

                                (_("Dominance's diversity"),
                                 _("Calculates dominance's diversity index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.dominance"),

                                (_("Shannon's diversity"),
                                 _("Calculates Shannon's diversity index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.shannon"),

                                (_("Simpson's diversity"),
                                 _("Calculates Simpson's diversity index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.simpson"),
                                ("","","", ""),

                                (_("Richness"),
                                 _("Calculates dominance's diversity index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.richness"),

                                (_("Shape index"),
                                 _("Calculates shape index on a raster map"),
                                 "self.OnMenuCmd",
                                 "r.li.shape"),
                                )
                         ),
                        (_("Wildfire modeling"), (

                                (_("Rate of spread"),
                                 _("Generates three, or four raster map layers showing 1) the base (perpendicular) rate of spread (ROS), 2) the maximum (forward) ROS, 3) the direction of the maximum ROS, and optionally 4) the maximum potential spotting distance."),
                                 "self.OnMenuCmd",
                                 "r.ros"),

                                (_("Least-cost spread paths"),
                                 _("Recursively traces the least cost path backwards to cells from which the cumulative cost was determined."),
                                 "self.OnMenuCmd",
                                 "r.spreadpath"),

                                (_("Anisotropic spread simulation"),
                                 _("It optionally produces raster maps to contain backlink UTM coordinates for tracing spread paths."),
                                 "self.OnMenuCmd",
                                 "r.spread"),
                                )
                         ),
                        ("","","", ""),
                        (_("Change category values and labels"), (

                                (_("Interactively edit category values"),
                                 _("Interactively edit cell values in a raster map."),
                                 "self.OnMenuCmd",
                                 "d.rast.edit"),
                                ("","","", ""),

                                (_("Reclassify by size"),
                                 _("Reclasses a raster map greater or less than user specified area size (in hectares)"),
                                 "self.OnMenuCmd",
                                 "r.reclass.area"),

                                (_("Reclassify interactively"),
                                 _("Reclassify raster categories interactively by entering reclass rules"),
                                 "self.RulesCmd",
                                 "r.reclass"),

                                (_("Reclassify using rules file"),
                                 _("Creates a new map layer whose category values are based upon a reclassification of the categories in an existing raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.reclass"),
                                ("","","", ""),

                                (_("Recode interactively"),
                                 _("Recode raster categories interactively by entering recode rules (create new raster map)"),
                                 "self.RulesCmd",
                                 "r.recode"),

                                (_("Recode using rules file"),
                                 _("r.recode.rules - Use ascii rules file to recode categories in raster map"),
                                 "self.OnMenuCmd",
                                 "r.recode.file"),
                                ("","","", ""),

                                (_("Rescale"),
                                 _("Rescales the range of category values in a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.rescale"),

                                (_("Rescale with histogram"),
                                 _("Rescales histogram equalized the range of category values in a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.rescale.eq"),
                                )
                         ),
                        ("","","", ""),

                        (_("Concentric circles"),
                         _("Creates a raster map containing concentric rings around a given point."),
                         "self.OnMenuCmd",
                         "r.circle"),
                        (_("Generate random cells"), (

                                (_("Random cells"),
                                 _("Generates random cell values with spatial dependence."),
                                 "self.OnMenuCmd",
                                 "r.random.cells"),

                                (_("Random cells and vector points"),
                                 _("Creates a raster map layer and vector point map containing randomly located sites."),
                                 "self.OnMenuCmd",
                                 "r.random"),
                                )
                         ),
                        (_("Generate surfaces"), (

                                (_("Fractal surface"),
                                 _("Creates a fractal surface of a given fractal dimension."),
                                 "self.OnMenuCmd",
                                 "r.surf.fractal"),
                                ("","","", ""),

                                (_("Gaussian kernel density surface"),
                                 _("Generates a raster density map from vector points data using a moving 2D isotropic Gaussian kernel or optionally generates a vector density map on vector network with a 1D kernel."),
                                 "self.OnMenuCmd",
                                 "v.kernel"),

                                (_("Gaussian deviates surface"),
                                 _("GRASS module to produce a raster map layer of gaussian deviates whose mean and standard deviation can be expressed by the user. It uses a gaussian random number generator."),
                                 "self.OnMenuCmd",
                                 "r.surf.gauss"),
                                ("","","", ""),

                                (_("Plane"),
                                 _("Creates raster plane map given dip (inclination), aspect (azimuth) and one point."),
                                 "self.OnMenuCmd",
                                 "r.plane"),
                                ("","","", ""),

                                (_("Random deviates surface"),
                                 _("Produces a raster map layer of uniform random deviates whose range can be expressed by the user."),
                                 "self.OnMenuCmd",
                                 "r.surf.random"),

                                (_("Random surface with spatial dependence"),
                                 _("Generates random surface(s) with spatial dependence."),
                                 "self.OnMenuCmd",
                                 "r.random.surface"),
                                )
                         ),

                        (_("Generate contour lines"),
                         _("Produces a vector map layer of specified contours from a raster map layer."),
                         "self.OnMenuCmd",
                         "r.contour"),
                        (_("Interpolate surfaces"), (

                                (_("Bilinear from raster points"),
                                 _("Bilinear interpolation utility for raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.bilinear"),

                                (_("Bilinear and bicubic from vector points"),
                                 _("Bicubic or bilinear spline interpolation with Tykhonov regularization."),
                                 "self.OnMenuCmd",
                                 "v.surf.bspline"),
                                ("","","", ""),

                                (_("IDW from raster points"),
                                 _("Surface interpolation utility for raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.surf.idw"),

                                (_("IDW from vector points"),
                                 _("Surface interpolation from vector point data by Inverse Distance Squared Weighting."),
                                 "self.OnMenuCmd",
                                 "v.surf.idw"),
                                ("","","", ""),

                                (_("Raster contours"),
                                 _("Surface generation program from rasterized contours."),
                                 "self.OnMenuCmd",
                                 "r.surf.contour"),

                                (_("Regularized spline tension"),
                                 _("Spatial approximation and topographic analysis from given point or isoline data in vector format to floating point raster format using regularized spline with tension."),
                                 "self.OnMenuCmd",
                                 "v.surf.rst"),
                                ("","","", ""),

                                (_("Fill NULL cells"),
                                 _("Fills no-data areas in raster maps using v.surf.rst splines interpolation"),
                                 "self.OnMenuCmd",
                                 "r.fillnulls"),
                                )
                         ),
                        ("","","", ""),
                        (_("Report and statistics"), (

                                (_("Basic raster metadata"),
                                 _("Output basic information about a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.info"),

                                (_("Manage category information"),
                                 _("Manages category values and labels associated with user-specified raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.category"),
                                ("","","", ""),

                                (_("General statistics"),
                                 _("Generates area statistics for raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.stats"),

                                (_("Range of category values"),
                                 _("Prints terse list of category values found in a raster map layer."),
                                 "self.OnMenuCmd",
                                 "r.describe"),

                                (_("Sum category values"),
                                 _("Sums up the raster cell values."),
                                 "self.OnMenuCmd",
                                 "r.sum"),

                                (_("Sum area by raster map and category"),
                                 _("Reports statistics for raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.report"),

                                (_("Statistics for clumped cells"),
                                 _("Calculates the volume of data \"clumps\", and (optionally) produces a GRASS vector points map containing the calculated centroids of these clumps."),
                                 "self.OnMenuCmd",
                                 "r.volume"),

                                (_("Total corrected area"),
                                 _("Surface area estimation for rasters."),
                                 "self.OnMenuCmd",
                                 "r.surf.area"),

                                (_("Univariate raster statistics"),
                                 _("Calculates univariate statistics from the non-null cells of a raster map."),
                                 "self.OnMenuCmd",
                                 "r.univar"),
                                ("","","", ""),

                                (_("Sample transects"),
                                 _("Outputs the raster map layer values lying on user-defined line(s)."),
                                 "self.OnMenuCmd",
                                 "r.profile"),

                                (_("Sample transects (bearing/distance)"),
                                 _("Outputs raster map layer values lying along user defined transect line(s)."),
                                 "self.OnMenuCmd",
                                 "r.transect"),
                                ("","","", ""),

                                (_("Covariance/correlation"),
                                 _("Outputs a covariance/correlation matrix for user-specified raster map layer(s)."),
                                 "self.OnMenuCmd",
                                 "r.covar"),

                                (_("Linear regression"),
                                 _("Calculates linear regression from two raster maps: y = a + b*x"),
                                 "self.OnMenuCmd",
                                 "r.regression.line"),

                                (_("Mutual category occurrences"),
                                 _("Tabulates the mutual occurrence (coincidence) of categories for two raster map layers."),
                                 "self.OnMenuCmd",
                                 "r.coin"),
                                )
                         ),
                        )
                 ),
                (_("Vector"), (
                        (_("Develop vector map"), (

                                (_("Create new vector map"),
                                 _("Create new empty vector map"),
                                 "self.OnNewVector",
                                 ""),
                                ("","","", ""),

                                (_("Create/rebuild topology"),
                                 _("Creates topology for GRASS vector map."),
                                 "self.OnMenuCmd",
                                 "v.build"),

                                (_("Clean vector map"),
                                 _("Toolset for cleaning topology of vector map."),
                                 "self.OnMenuCmd",
                                 "v.clean"),

                                (_("Generalization"),
                                 _("Vector based generalization."),
                                 "self.OnMenuCmd",
                                 "v.generalize"),
                                ("","","", ""),

                                (_("Convert object types"),
                                 _("Change the type of geometry elements."),
                                 "self.OnMenuCmd",
                                 "v.type.py"),
                                ("","","", ""),

                                (_("Add centroids"),
                                 _("Adds missing centroids to closed boundaries."),
                                 "self.OnMenuCmd",
                                 "v.centroids"),
                                ("","","", ""),

                                (_("Build polylines"),
                                 _("Builds polylines from lines or boundaries."),
                                 "self.OnMenuCmd",
                                 "v.build.polylines"),

                                (_("Split polylines"),
                                 _("Creates points/segments from input vector lines and positions."),
                                 "self.OnMenuCmd",
                                 "v.segment"),

                                (_("Parallel lines"),
                                 _("Create parallel line to input lines"),
                                 "self.OnMenuCmd",
                                 "v.parallel"),
                                ("","","", ""),

                                (_("Dissolve boundaries"),
                                 _("Dissolves boundaries between adjacent areas sharing a common category number or attribute."),
                                 "self.OnMenuCmd",
                                 "v.dissolve"),
                                ("","","", ""),

                                (_("Create 3D vector over raster"),
                                 _("Converts vector map to 3D by sampling of elevation raster map."),
                                 "self.OnMenuCmd",
                                 "v.drape"),

                                (_("Extrude 3D vector map"),
                                 _("Extrudes flat vector object to 3D with defined height."),
                                 "self.OnMenuCmd",
                                 "v.extrude"),
                                ("","","", ""),

                                (_("Link to OGR"),
                                 _("Available drivers: ESRI Shapefile,MapInfo File,UK .NTF,SDTS,TIGER,S57,DGN,VRT,REC,Memory,BNA,CSV,GML,GPX,KML,GeoJSON,GMT,SQLite,ODBC,PGeo,PostgreSQL,AVCBin"),
                                 "self.OnMenuCmd",
                                 "v.external"),
                                ("","","", ""),

                                (_("Create labels"),
                                 _("Creates paint labels for a vector map from attached attributes."),
                                 "self.OnMenuCmd",
                                 "v.label"),
                                ("","","", ""),

                                (_("Reposition vector map"),
                                 _("Performs an affine transformation (shift, scale and rotate, or GPCs) on vector map."),
                                 "self.OnMenuCmd",
                                 "v.transform"),

                                (_("Reproject vector map"),
                                 _("Allows projection conversion of vector maps."),
                                 "self.OnMenuCmd",
                                 "v.proj"),
                                )
                         ),
                        ("","","", ""),

                        (_("Manage colors"),
                         _("Set colors interactively by entering color rules"),
                         "self.RulesCmd",
                         "vcolors"),
                        ("","","", ""),

                        (_("Query with attributes"),
                         _("Selects vector objects from an existing vector map and creates a new map containing only the selected objects."),
                         "self.OnMenuCmd",
                         "v.extract"),

                        (_("Query with coordinate(s)"),
                         _("Queries a vector map layer at given locations."),
                         "self.OnMenuCmd",
                         "v.what"),

                        (_("Query with another vector map"),
                         _("Select features from ainput by features from binput"),
                         "self.OnMenuCmd",
                         "v.select"),
                        ("","","", ""),

                        (_("Buffer vectors"),
                         _("Creates a buffer around features of given type (areas must contain centroid)."),
                         "self.OnMenuCmd",
                         "v.buffer"),
                        (_("Lidar analysis"), (

                                (_("Detect edges"),
                                 _("Detects the object's edges from a LIDAR data set."),
                                 "self.OnMenuCmd",
                                 "v.lidar.edgedetection"),

                                (_("Detect interiors"),
                                 _("Building contour determination and Region Growing algorithm for determining the building inside"),
                                 "self.OnMenuCmd",
                                 "v.lidar.growing"),

                                (_("Correct and reclassify objects"),
                                 _("Correction of the v.lidar.growing output. It is the last of the three algorithms for LIDAR filtering."),
                                 "self.OnMenuCmd",
                                 "v.lidar.correction"),
                                )
                         ),
                        (_("Linear referencing"), (

                                (_("Create LRS"),
                                 _("Create Linear Reference System"),
                                 "self.OnMenuCmd",
                                 "v.lrs.create"),

                                (_("Create stationing"),
                                 _("Create stationing from input lines, and linear reference system"),
                                 "self.OnMenuCmd",
                                 "v.lrs.label"),

                                (_("Create points/segments"),
                                 _("Creates points/segments from input lines, linear reference system and positions read from stdin or a file."),
                                 "self.OnMenuCmd",
                                 "v.lrs.segment"),

                                (_("Find line id and offset"),
                                 _("Finds line id and real km+offset for given points in vector map using linear reference system."),
                                 "self.OnMenuCmd",
                                 "v.lrs.where"),
                                )
                         ),

                        (_("Nearest features"),
                         _("Finds the nearest element in vector map 'to' for elements in vector map 'from'."),
                         "self.OnMenuCmd",
                         "v.distance"),
                        (_("Network analysis"), (

                                (_("Allocate subnets"),
                                 _("Centre node must be opened (costs >= 0). Costs of centre node are used in calculation"),
                                 "self.OnMenuCmd",
                                 "v.net.alloc"),

                                (_("Network maintenance"),
                                 _("Network maintenance."),
                                 "self.OnMenuCmd",
                                 "v.net"),

                                (_("Visibility network"),
                                 _("Visibility graph construction."),
                                 "self.OnMenuCmd",
                                 "v.net.visibility"),

                                (_("Shortest path"),
                                 _("Finds shortest path on vector network."),
                                 "self.OnMenuCmd",
                                 "v.net.path"),

                                (_("Display shortest route (requires XTerm)"),
                                 _("Display shortest route along network between 2 nodes (visualization only, requires XTerm))"),
                                 "self.OnXTerm",
                                 "d.path"),

                                (_("Split net"),
                                 _("Splits net to bands between cost isolines (direction from centre). Centre node must be opened (costs >= 0). Costs of centre node are used in calculation."),
                                 "self.OnMenuCmd",
                                 "v.net.iso"),

                                (_("Steiner tree"),
                                 _("Note that 'Minimum Steiner Tree' problem is NP-hard and heuristic algorithm is used in this module so the result may be sub optimal"),
                                 "self.OnMenuCmd",
                                 "v.net.steiner"),

                                (_("Traveling salesman analysis"),
                                 _("Note that TSP is NP-hard, heuristic algorithm is used by this module and created cycle may be sub optimal"),
                                 "self.OnMenuCmd",
                                 "v.net.salesman"),
                                )
                         ),
                        (_("Overlay vector maps"), (

                                (_("Overlay vector maps"),
                                 _("Overlays two vector maps."),
                                 "self.OnMenuCmd",
                                 "v.overlay"),

                                (_("Patch vector maps"),
                                 _("Create a new vector map layer by combining other vector map layers."),
                                 "self.OnMenuCmd",
                                 "v.patch"),
                                )
                         ),
                        ("","","", ""),
                        (_("Change attributes"), (

                                (_("Manage or report categories"),
                                 _("Attach, delete or report vector categories to map geometry."),
                                 "self.OnMenuCmd",
                                 "v.category"),

                                (_("Reclassify objects interactively"),
                                 _("Reclassify vector objects interactively by entering SQL rules"),
                                 "self.RulesCmd",
                                 "v.reclass"),

                                (_("Reclassify objects using rules file"),
                                 _("Changes vector category values for an existing vector map according to results of SQL queries or a value in attribute table column."),
                                 "self.OnMenuCmd",
                                 "v.reclass"),
                                )
                         ),
                        ("","","", ""),

                        (_("Generate area for current region"),
                         _("Create a new vector from the current region."),
                         "self.OnMenuCmd",
                         "v.in.region"),
                        (_("Generate areas from points"), (

                                (_("Convex hull"),
                                 _("Uses a GRASS vector points map to produce a convex hull vector map."),
                                 "self.OnMenuCmd",
                                 "v.hull"),

                                (_("Delaunay triangles"),
                                 _("Creates a Delaunay triangulation from an input vector map containing points or centroids."),
                                 "self.OnMenuCmd",
                                 "v.delaunay"),

                                (_("Voronoi diagram/Thiessen polygons"),
                                 _("Creates a Voronoi diagram from an input vector map containing points or centroids."),
                                 "self.OnMenuCmd",
                                 "v.voronoi"),
                                )
                         ),

                        (_("Generate grid"),
                         _("Creates a GRASS vector map of a user-defined grid."),
                         "self.OnMenuCmd",
                         "v.mkgrid"),
                        (_("Generate points"), (

                                (_("Generate from database"),
                                 _("Creates new vector (points) map from database table containing coordinates."),
                                 "self.OnMenuCmd",
                                 "v.in.db"),

                                (_("Generate points along lines"),
                                 _("Create points along input lines in new vector with 2 layers."),
                                 "self.OnMenuCmd",
                                 "v.to.points"),

                                (_("Generate random points"),
                                 _("Randomly generate a 2D/3D vector points map."),
                                 "self.OnMenuCmd",
                                 "v.random"),

                                (_("Perturb points"),
                                 _("Random location perturbations of GRASS vector points"),
                                 "self.OnMenuCmd",
                                 "v.perturb"),
                                )
                         ),
                        ("","","", ""),

                        (_("Remove outliers in point sets"),
                         _("Removes outliers from vector point data."),
                         "self.OnMenuCmd",
                         "v.outlier"),

                        (_("Test/training point sets"),
                         _("Randomly partition points into test/train sets."),
                         "self.OnMenuCmd",
                         "v.kcv"),
                        ("","","", ""),

                        (_("Update area attributes from raster"),
                         _("Calculates univariate statistics from a GRASS raster map based on vector polygons and uploads statistics to new attribute columns."),
                         "self.OnMenuCmd",
                         "v.rast.stats"),

                        (_("Update point attributes from areas"),
                         _("Uploads vector values at positions of vector points to the table."),
                         "self.OnMenuCmd",
                         "v.what.vect"),
                        (_("Update point attributes from raster"), (

                                (_("Sample raster maps at point locations"),
                                 _("Uploads raster values at positions of vector points to the table."),
                                 "self.OnMenuCmd",
                                 "v.what.rast"),

                                (_("Sample raster neighborhood around points"),
                                 _("Samples a raster map at vector point locations."),
                                 "self.OnMenuCmd",
                                 "v.sample"),
                                )
                         ),
                        ("","","", ""),
                        (_("Reports and statistics"), (

                                (_("Basic vector metadata"),
                                 _("Outputs basic information about a user-specified vector map layer."),
                                 "self.OnMenuCmd",
                                 "v.info"),
                                ("","","", ""),

                                (_("Report topology by category"),
                                 _("Reports geometry statistics for vectors."),
                                 "self.OnMenuCmd",
                                 "v.report"),

                                (_("Upload or report topology"),
                                 _("Populate database values from vector features."),
                                 "self.OnMenuCmd",
                                 "v.to.db"),
                                ("","","", ""),

                                (_("Univariate attribute statistics"),
                                 _("Calculates univariate statistics for attribute. Variance and standard deviation is calculated only for points if specified."),
                                 "self.OnMenuCmd",
                                 "v.univar"),
                                ("","","", ""),

                                (_("Quadrat indices"),
                                 _("Indices for quadrat counts of sites lists."),
                                 "self.OnMenuCmd",
                                 "v.qcount"),

                                (_("Test normality"),
                                 _("Tests for normality for points."),
                                 "self.OnMenuCmd",
                                 "v.normal"),
                                )
                         ),
                        )
                 ),
                (_("Imagery"), (
                        (_("Develop images and groups"), (

                                (_("Create/edit group"),
                                 _("Creates, edits, and lists groups and subgroups of imagery files."),
                                 "self.OnMenuCmd",
                                 "i.group"),

                                (_("Target group"),
                                 _("Targets an imagery group to a GRASS location and mapset."),
                                 "self.OnMenuCmd",
                                 "i.target"),
                                ("","","", ""),

                                (_("Mosaic images"),
                                 _("Mosaics up to 4 images and extends colormap; creates map *.mosaic"),
                                 "self.OnMenuCmd",
                                 "i.image.mosaic"),
                                )
                         ),
                        (_("Manage image colors"), (

                                (_("Color balance for RGB"),
                                 _("Auto-balancing of colors for LANDSAT images"),
                                 "self.OnMenuCmd",
                                 "i.landsat.rgb"),

                                (_("HIS to RGB"),
                                 _("Hue-intensity-saturation (his) to red-green-blue (rgb) raster map color transformation function."),
                                 "self.OnMenuCmd",
                                 "i.his.rgb"),

                                (_("RGB to HIS"),
                                 _("Red-green-blue (rgb) to hue-intensity-saturation (his) raster map color transformation function"),
                                 "self.OnMenuCmd",
                                 "i.rgb.his"),
                                )
                         ),

                        (_("Rectify image or raster"),
                         _("Rectifies an image by computing a coordinate transformation for each pixel in the image based on the control points"),
                         "self.OnMenuCmd",
                         "i.rectify"),

                        (_("Ortho photo rectification (requires Xterm)"),
                         _("Ortho Photo rectification"),
                         "self.OnXTerm",
                         "i.ortho.photo"),
                        ("","","", ""),

                        (_("Brovey sharpening"),
                         _("Brovey transform to merge multispectral and high-res panchromatic channels"),
                         "self.OnMenuCmd",
                         "i.fusion.brovey"),
                        (_("Classify image"), (

                                (_("Clustering input for unsupervised classification"),
                                 _("The resulting signature file is used as input for i.maxlik, to generate an unsupervised image classification."),
                                 "self.OnMenuCmd",
                                 "i.cluster"),
                                ("","","", ""),

                                (_("Maximum likelihood classification (MLC)"),
                                 _("Classification is based on the spectral signature information generated by either i.cluster, i.class, or i.gensig."),
                                 "self.OnMenuCmd",
                                 "i.maxlik"),

                                (_("Sequential maximum a posteriori classification (SMAP)"),
                                 _("Performs contextual image classification using sequential maximum a posteriori (SMAP) estimation."),
                                 "self.OnMenuCmd",
                                 "i.smap"),
                                ("","","", ""),

                                (_("Interactive input for supervised classification (requires Xterm)"),
                                 _("Interactive input for supervised classification"),
                                 "self.OnXTerm",
                                 "i.class"),

                                (_("Input for supervised MLC"),
                                 _("Generates statistics for i.maxlik from raster map layer."),
                                 "self.OnMenuCmd",
                                 "i.gensig"),

                                (_("Input for supervised SMAP"),
                                 _("Generate statistics for i.smap from raster map layer."),
                                 "self.OnMenuCmd",
                                 "i.gensigset"),
                                )
                         ),
                        (_("Filter image"), (

                                (_("Edge detection"),
                                 _("Zero-crossing \"edge detection\" raster function for image processing."),
                                 "self.OnMenuCmd",
                                 "i.zc"),

                                (_("Matrix/convolving filter"),
                                 _("Raster map matrix filter."),
                                 "self.OnMenuCmd",
                                 "r.mfilter"),
                                )
                         ),

                        (_("Histogram"),
                         _("Generate histogram of image"),
                         "self.DispHistogram",
                         ""),

                        (_("Spectral response"),
                         _("displays spectral response at user specified locations in group or images"),
                         "self.OnMenuCmd",
                         "i.spectral"),

                        (_("Tasseled cap vegetation index"),
                         _("Tasseled Cap (Kauth Thomas) transformation for LANDSAT-TM data"),
                         "self.OnMenuCmd",
                         "i.tasscap"),
                        (_("Transform image"), (

                                (_("Canonical correlation"),
                                 _("Canonical components analysis (cca) program for image processing."),
                                 "self.OnMenuCmd",
                                 "i.cca"),

                                (_("Principal components"),
                                 _("Principal components analysis (pca) program for image processing."),
                                 "self.OnMenuCmd",
                                 "i.pca"),

                                (_("Fast Fourier"),
                                 _("Fast Fourier Transform (FFT) for image processing."),
                                 "self.OnMenuCmd",
                                 "i.fft"),

                                (_("Inverse Fast Fourier"),
                                 _("Inverse Fast Fourier Transform (IFFT) for image processing."),
                                 "self.OnMenuCmd",
                                 "i.ifft"),
                                )
                         ),
                        (_("Atmospheric correction"),
                         _("Performs atmospheric correction using the 6S algorithm."),
                         "self.OnMenuCmd",
                         "i.atcorr"),
                        ("","","", ""),
                        (_("Report and statistics"), (

                                (_("Bit pattern comparison "),
                                 _("Compares bit patterns with a raster map."),
                                 "self.OnMenuCmd",
                                 "r.bitpattern"),

                                (_("Kappa analysis"),
                                 _("Calculate error matrix and kappa parameter for accuracy assessment of classification result."),
                                 "self.OnMenuCmd",
                                 "r.kappa"),

                                (_("OIF for LandSat TM"),
                                 _("Calculates Optimum-Index-Factor table for LANDSAT TM bands 1-5, & 7"),
                                 "self.OnMenuCmd",
                                 "i.oif"),
                                )
                         ),
                        )
                 ),
                (_("Volumes"), (
                        (_("Develop volumes"), (

                                (_("Manage 3D NULL values"),
                                 _("Explicitly create the 3D NULL-value bitmap file."),
                                 "self.OnMenuCmd",
                                 "r3.null"),

                                (_("Manage timestamp"),
                                 _("Print/add/remove a timestamp for a 3D raster map"),
                                 "self.OnMenuCmd",
                                 "r3.timestamp"),
                                )
                         ),
                        ("","","", ""),

                        (_("3D Mask"),
                         _("Establishes the current working 3D raster mask."),
                         "self.OnMenuCmd",
                         "r3.mask"),

                        (_("3D raster map calculator"),
                         _("Map calculator for volumetric map algebra"),
                         "self.Disp3DMapCalculator",
                         ""),

                        (_("Cross section"),
                         _("Creates cross section 2D raster map from 3d raster map based on 2D elevation map"),
                         "self.OnMenuCmd",
                         "r3.cross.rast"),

                        (_("Interpolate volume from points"),
                         _("Interpolates point data to a G3D grid volume using regularized spline with tension (RST) algorithm."),
                         "self.OnMenuCmd",
                         "v.vol.rst"),
                        ("","","", ""),
                        (_("Report and Statistics"), (

                                (_("Basic volume metadata"),
                                 _("Outputs basic information about a user-specified 3D raster map layer."),
                                 "self.OnMenuCmd",
                                 "r3.info"),
                                )
                         ),
                        )
                 ),
                (_("Database"), (
                        (_("Database information"), (

                                (_("Describe table"),
                                 _("Describes a table in detail."),
                                 "self.OnMenuCmd",
                                 "db.describe"),

                                (_("List columns"),
                                 _("List all columns for a given table."),
                                 "self.OnMenuCmd",
                                 "db.columns"),

                                (_("List drivers"),
                                 _("List all database drivers."),
                                 "self.OnMenuCmd",
                                 "db.drivers"),

                                (_("List tables"),
                                 _("Lists all tables for a given database."),
                                 "self.OnMenuCmd",
                                 "db.tables"),
                                )
                         ),
                        ("","","", ""),
                        (_("Manage databases"), (

                                (_("Connect"),
                                 _("Prints/sets general DB connection for current mapset and exits."),
                                 "self.OnMenuCmd",
                                 "db.connect"),

                                (_("Login"),
                                 _("Sets user/password for driver/database."),
                                 "self.OnMenuCmd",
                                 "db.login"),
                                ("","","", ""),

                                (_("Copy table"),
                                 _("Copy a table. Either 'from_table' (optionaly with 'where') can be used or 'select' option, but not 'from_table' and 'select' at the same time."),
                                 "self.OnMenuCmd",
                                 "db.copy"),

                                (_("New table"),
                                 _("Creates and adds a new attribute table to a given layer of an existing vector map."),
                                 "self.OnMenuCmd",
                                 "v.db.addtable"),

                                (_("Remove table"),
                                 _("Drops an attribute table."),
                                 "self.OnMenuCmd",
                                 "db.droptable"),
                                ("","","", ""),

                                (_("Add columns"),
                                 _("Adds one or more columns to the attribute table connected to a given vector map."),
                                 "self.OnMenuCmd",
                                 "v.db.addcol"),

                                (_("Change values"),
                                 _("Allows to update a column in the attribute table connected to a vector map."),
                                 "self.OnMenuCmd",
                                 "v.db.update"),

                                (_("Rename column"),
                                 _("Renames a column in the attribute table connected to a given vector map."),
                                 "self.OnMenuCmd",
                                 "v.db.renamecol"),
                                ("","","", ""),

                                (_("Test"),
                                 _("Test database driver, database must exist and set by db.connect."),
                                 "self.OnMenuCmd",
                                 "db.test"),
                                )
                         ),
                        (_("Query"), (

                                (_("Query any table"),
                                 _("Selects data from table."),
                                 "self.OnMenuCmd",
                                 "db.select"),

                                (_("Query vector attribute data"),
                                 _("Prints vector map attributes."),
                                 "self.OnMenuCmd",
                                 "v.db.select"),

                                (_("SQL statement"),
                                 _("Executes any SQL statement."),
                                 "self.OnMenuCmd",
                                 "db.execute"),
                                )
                         ),
                        ("","","", ""),
                        (_("Vector database connections"), (

                                (_("Reconnect vector to database"),
                                 _("Reconnects vectors to a new database."),
                                 "self.OnMenuCmd",
                                 "v.db.reconnect.all"),

                                (_("Set vector map - database connection"),
                                 _("Prints/sets DB connection for a vector map to attribute table."),
                                 "self.OnMenuCmd",
                                 "v.db.connect"),
                                )
                         ),
                        )
                 ),
                (_("Help"), (

                        (_("GRASS GIS help"),
                         _("Display the HTML man pages of GRASS"),
                         "self.RunMenuCmd",
                         "g.manual -i"),

                        (_("GRASS GIS GUI help"),
                         _("Display the HTML man pages of GRASS"),
                         "self.RunMenuCmd",
                         "g.manual wxGUI"),

                        (_("About GRASS GIS"),
                         _("About GRASS GIS"),
                         "self.OnAboutGRASS",
                         ""),
                        )
                 ),
                )
                ]
