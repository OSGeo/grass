###############################################################
# 	File:		gmmenu.tcl
#	Purpose:	menu file for GRASS GIS Manager
#	Author:		Michael Barton, Arizona State University
#				Based originally on menu for d.m and tcltkgrass
#				Additions by Cedrick Shock and Benjamin Ducke
# 	COPYRIGHT:	(C) 2006-2007 by the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
###############################################################

# our job is simply to make a variable called descmenu

#source "$env(GISBASE)/etc/gui/menus/menu.tcl"


global execom 
global mon
global filename
global devnull

# Tear off menus (yes / no)
global tmenu
# Key to use for control (for menu accelerators)
global keyctrl
# The environment
global env



set XtnsMenu "False"
set pathlist {}
set menulist {}
set menudatlist {}

# Check for existence of xtnmenu.dat file and parse it
# into an extensions menu

lappend menudatlist "$env(GISBASE)/etc/xtnmenu.dat"
if {[info exists env(GRASS_ADDON_ETC)]} {
    set pathlist [split $env(GRASS_ADDON_ETC) ":"]
    foreach path $pathlist {
        lappend menudatlist "$path/xtnmenu.dat"
    }
}

foreach menudat $menudatlist {
    if {[file exists $menudat]} {	
        if { [lsearch $menudatlist $menudat] > 0} {lappend menulist "separator"}
        if {![catch {open $menudat r} menudef]} {
            while {[gets $menudef menuline] >= 0} {
                set menuline [string trim $menuline]
                if {[string first # $menuline] == 0 } {
                    continue}
                set menuline [split $menuline ":"]
                set menulevel [lindex $menuline 0]
                set menuitem [G_msg [lindex $menuline 1]]
                set menucmd "execute "
                append menucmd [lindex $menuline 2]
                set menuhelp [G_msg [lindex $menuline 3]]
                # add if statement to read comments here
                if {$menuitem == "separator"} {
                    lappend menulist "separator"
                } else { 
                    set line [list command $menuitem {} \
                        $menuhelp {} -command $menucmd]
    
                    lappend menulist $line
                }
            }
            if {[catch {close $menudef} error]} {
                GmLib::errmsg $error ["Error reading xtnmenu.dat file"]
            }
        }
        set XtnsMenu "True"
    }
    
}
		

# This is the menu. 

set descmenu [subst  {
 {[G_msg "&File"]} all file $tmenu {
	{cascad {[G_msg "Workspace"]} {} "" $tmenu {			
		{command {[G_msg "Open..."]} {} "Open gis.m workspace file" {} -accelerator $keyctrl-O -command { GmLib::OpenFileBox }}
		{command {[G_msg "Save"]} {} "Save gis.m workspace file" {} -accelerator $keyctrl-S -command { GmLib::SaveFileBox }}
		{command {[G_msg "Save as..."]} {} "Save gis.m workspace file as new name" {} -command { set filename($mon) "" ; GmLib::SaveFileBox }}
		{command {[G_msg "Close"]} {} "Close gis.m workspace" {} -accelerator $keyctrl-W -command { GmTree::FileClose {}}}
	}}
	{separator}
	{cascad {[G_msg "Import raster map"]} {} "" $tmenu {
		{command {[G_msg "Multiple formats using GDAL"]} {} "r.in.gdal: Import multiple formats using GDAL" {} -command { execute r.in.gdal }}
		{separator}
		{command {[G_msg "Aggregate ASCII xyz"]} {} "r.in.xyz: Import aggregate ASCII xyz data into raster grid" {} -command { execute r.in.xyz }}
		{command {[G_msg "ASCII grid"]} {} "r.in.ascii: Import ASCII grid (includes GRASS ASCII)" {} -command { execute r.in.ascii }}
		{command {[G_msg "ASCII polygons and lines"]} {} "r.in.poly: Import polygons and lines from ASCII file" {} -command { execute r.in.poly }}
		{separator}
		{command {[G_msg "Binary"]} {} "r.in.bin: Import binary file (includes GTOPO30 format)" {} -command { execute r.in.bin }}
		{command {[G_msg "ESRI grid"]} {} "r.in.arc: Import ESRI Arc/Info ASCII grid" {} -command { execute r.in.arc }}
		{command {[G_msg "GRIDATB.FOR"]} {} "r.in.gridatbk: Import GRIDATB.FOR map file (TOPMODEL)" {} -command { execute r.in.gridatb }}
		{command {[G_msg "MAT-File (v.4)"]} {} "r.in.mat: Import MAT-File (v.4) array (Matlab or Octave)" {} -command { execute r.in.mat }}
		{command {[G_msg "SPOT NDVI"]} {} "i.in.spotvgt: Import SPOT vegetation NDVI data sets" {} -command { execute i.in.spotvgt }}
		{command {[G_msg "SRTM hgt"]} {} "r.in.srtm: Import SRTM hgt files" {} -command { execute r.in.srtm }}
		{command {[G_msg "Terra ASTER"]} {} "r.in.aster: Import Terra ASTER HDF files" {} -command { execute r.in.aster }}
		{separator}
		{command {[G_msg "Web Mapping Server"]} {} "r.in.wms: Import Web Mapping Server files" {} -command { execute r.in.wms }}
	}}
	{cascad {[G_msg "Import vector map"]} {} "" $tmenu {			
		{command {[G_msg "Multiple formats using OGR"]} {} "v.in.ogr: Import multiple formats using OGR" {} -command { execute v.in.ogr }}
		{separator}
		{command {[G_msg "ASCII points or GRASS ASCII vector"]} {} "v.in.ascii: Import ASCII points file or GRASS ASCII vector file" {} -command { execute v.in.ascii }}
		{command {[G_msg "Old GRASS vector"]} {} "v.convert: Import old GRASS vector format" {} -command { execute v.convert }}
		{separator}
		{command {[G_msg "DXF"]} {} "v.in.dxf: Import DXF file" {} -command { execute v.in.dxf }}
		{command {[G_msg "ESRI e00"]} {} "v.in.e00: Import ESRI e00 format" {} -command { execute v.in.e00 }}
		{command {[G_msg "Garmin GPS"]} {} "v.in.garmin: Import Garmin GPS waypoints/routes/tracks" {} -command { execute v.in.garmin }}
		{command {[G_msg "GPSBabel GPS"]} {} "v.in.gpsbabel: Import GPS waypoints/routes/tracks using GPSBabel" {} -command { execute v.in.gpsbabel }}
		{command {[G_msg "GEOnet"]} {} "v.in.gns: Import GEOnet Name server country files (US-NGA GNS)" {} -command { execute v.in.gns }}
		{command {[G_msg "Matlab and MapGen"]} {} "v.in.mapgen: Import Matlab and MapGen files" {} -command { execute v.in.mapgen }}
	}}
	{cascad {[G_msg "Import grid 3D volume"]} {} "" $tmenu {			
		{command {[G_msg "ASCII 3D"]} {} "r3.in.ascii: Import ASCII 3D file" {} -command { execute r3.in.ascii }}
		{command {[G_msg "Vis5D"]} {} "r3.in.v5d: Import Vis5D file" {} -command { execute r3.in.v5d }}
	}}
	{separator}
	{cascad {[G_msg "Export raster map"]} {} "" $tmenu {
		{command {[G_msg "Multiple formats using GDAL"]} {} "r.out.gdal: Export multiple formats using GDAL" {} -command { execute r.out.gdal }}
		{separator}
		{command {[G_msg "ASCII grid"]} {} "r.out.ascii: Export ASCII grid (for GRASS, Surfer, Modflow, etc)" {} -command { execute r.out.ascii }}
		{command {[G_msg "ASCII x,y,z"]} {} "r.out.xyz: Export ASCII x,y,z values of cell centers" {} -command { execute r.out.xyz }}
		{separator}
		{command {[G_msg "ESRI ASCII grid"]} {} "r.out.arc" {} -command { execute r.out.arc }}
		{command {[G_msg "GRIDATB.FOR"]} {} "r.out.gridatb: Export GRIDATB.FOR map file (TOPMODEL)" {} -command { execute r.out.gridatb }}
		{command {[G_msg "MAT-File (v.4)"]} {} "r.out.mat: Export MAT-File (v.4) array (Matlab or Octave)" {} -command { execute r.out.mat }}
		{separator}
		{command {[G_msg "Binary"]} {} "r.out.bin: Export binary file" {} -command { execute r.out.bin }}
		{separator}
		{command {[G_msg "MPEG-1"]} {} "r.out.mpeg: Export MPEG-1 animations" {} -command { execute r.out.mpeg }}
		{command {[G_msg "PNG"]} {} "r.out.png: Export PNG image (not georeferenced)" {} -command { execute r.out.png }}
		{command {[G_msg "PPM"]} {} "r.out.ppm: Export PPM image (24bit)" {} -command { execute r.out.ppm }}
		{command {[G_msg "PPM from RGB"]} {} "r.out.ppm3: Export PPM image from red, green, blue raster maps" {} -command { execute r.out.ppm3 }}
		{command {[G_msg "POV-Ray"]} {} "r.out.pov: Export POV-Ray height-field" {} -command { execute r.out.pov }}
		{command {[G_msg "TIFF"]} {} "r.out.tiff: Export TIFF image (8/24bit)" {} -command { execute r.out.tiff }}
		{command {[G_msg "VRML"]} {} "r.out.vrml: Export VRML file" {} -command { execute r.out.vrml }}
		{command {[G_msg "VTK"]} {} "r.out.vtk: Export VTK ASCII file" {} -command { execute r.out.vtk }}
	}}
	{cascad {[G_msg "Export vector map"]} {} "" $tmenu {			
		{command {[G_msg "Multiple formats using OGR"]} {} "v.out.ogr: Export multiple formats using OGR (SHAPE, MapInfo etc)" {} -command { execute v.out.ogr }}
		{separator}
		{command {[G_msg "ASCII points or GRASS ASCII vector"]} {} "v.out.ascii: Export ASCII vector or point file/old GRASS ASCII vector file" {} -command { execute v.out.ascii }}
		{command {[G_msg "DXF"]} {} "v.out.dxf: Export DXF file (ASCII)" {} -command { execute v.out.dxf }}
		{command {[G_msg "POV-Ray"]} {} "v.out.pov: Export POV-Ray format" {} -command { execute v.out.pov }}
		{command {[G_msg "SVG"]} {} "v.out.svg: Export SVG file" {} -command { execute v.out.svg }}
		{command {[G_msg "VTK"]} {} "v.out.vtk: Export VTK ASCII file" {} -command { execute v.out.vtk }}
	}}
	{cascad {[G_msg "Export grid 3D volume"]} {} "" $tmenu {
		{command {[G_msg "ASCII 3D"]} {} "r3.out.ascii: Export ASCII 3D file" {} -command { execute r3.out.ascii }}
		{command {[G_msg "Vis5D"]} {} "r3.out.v5d: Export Vis5D file" {} -command { execute r3.out.v5d }}
		{command {[G_msg "VTK"]} {} "r3.out.vtk: Export VTK ASCII file" {} -command { execute r3.out.vtk }}
		}}
	{separator}
	{cascad {[G_msg "Manage maps and volumes"]} {} "" $tmenu {
		{command {[G_msg "Copy"]} {} "g.copy: Copy maps" {} -command {execute g.copy }}
		{separator}
		{command {[G_msg "List"]} {} "g.list: List maps" {} -command {execute g.list}}
		{command {[G_msg "List filtered"]} {} "g.mlist: List maps using expressions and 'wildcards'" {} -command {execute g.mlist }}
		{separator}
		{command {[G_msg "Rename"]} {} "g.rename: Rename maps" {} -command {execute g.rename }}
		{separator}
		{command {[G_msg "Delete"]} {} "g.remove: Delete maps" {} -command {execute g.remove }}
		{command {[G_msg "Delete filtered"]} {} "g.mremove: Delete maps using expressions and 'wildcards'" {} -command {execute g.mremove }}
	}}
	{cascad {[G_msg "Map type conversions"]} {} "" $tmenu {
		{command {[G_msg "Raster to vector"]} {} "r.to.vect: Convert aster to vector map" {} -command {execute r.to.vect }}
		{command {[G_msg "Raster series to volume"]} {} "r.to.rast3: Convert raster map series to volume" {} -command {execute r.to.rast3 }}
		{command {[G_msg "Raster 2.5D to volume"]} {} "r.to.rast3elev: Convert raster 2.5D map to volume" {} -command {execute r.to.rast3elev }}
		{separator}
		{command {[G_msg "Vector to raster"]} {} "v.to.rast: Convert vector to raster map" {} -command {execute v.to.rast }}
		{command {[G_msg "Vector to volume"]} {} "v.to.rast3: Convert vector 3D points to volume voxels" {} -command {execute v.to.rast3 }}
		{command {[G_msg "Sites to vector"]} {} "v.in.sites: Convert GRASS 5 sites to vector points" {} -command {execute v.in.sites }}
		{separator}
		{command {[G_msg "Volume to raster series"]} {} "r3.to.rast: Convert volume to raster map series" {} -command {execute r3.to.rast }}
	}}
	{separator}
	{command {[G_msg "Georectify"]} {} "Georectify raster map in XY location" {} -command { GRMap::startup }}
	{separator}
	{command {[G_msg "Animate raster maps"]} {} "Display a series of raster maps as an animation" {} -command { GmAnim::main }}
	{separator}
	{command {[G_msg "Bearing/distance to coordinates"]} {} "m.cogo: Convert between bearing/distance and coordinates" {} -command { execute m.cogo }}
	{separator}
	{cascad {[G_msg "3D rendering"]} {} "" $tmenu {
		{command {[G_msg "NVIZ"]} {} "nviz: Launch N-dimensional visualization" {} -command {execute nviz }}
		{command {[G_msg "NVIZ fly through path"]} {} "d.nviz: Create a fly-through path for NVIZ (requires xterm for interactive path creation)" {} -command {execute d.nviz }}
	}}
	{command {[G_msg "PostScript plot"]} {} "ps.map: Create cartographic PostScript plot" {} -command { execute ps.map }}
	{separator}
	{command {[G_msg "E&xit"]} {} "Exit GIS Manager" {} -accelerator $keyctrl-Q -command { exit } }
 }
 {[G_msg "&Config"]} all options $tmenu {
	{cascad {[G_msg "Region"]} {} "" $tmenu {
		{command {[G_msg "Display region settings"]} {} "g.region -p: Display region settings" {} -command {run_panel "g.region -p" }}
		{command {[G_msg "Change region settings"]} {} "g.region: " {} -command {execute g.region }}
	}}
	{cascad {[G_msg "GRASS working environment"]} {} "" $tmenu {			
		{command {[G_msg "Mapset access"]} {} "g.mapsets.tcl: Access other mapsets in current location" {} -command {exec $env(GRASS_WISH) $env(GISBASE)/etc/g.mapsets.tcl --tcltk &}}
		{command {[G_msg "Change working environment"]} {} "g.mapset: Change current working session to new mapset, location, or GISDBASE" {} -command {execute g.mapset }}
		{command {[G_msg "User access"]} {} "g.access: Modify access by other users to current mapset" {} -command {execute g.access }}
		{command {[G_msg "Show settings"]} {} "g.gisenv: Show current GRASS environment settings" {} -command {run_panel g.gisenv }}
		{command {[G_msg "Change settings"]} {} "g.gisenv: Set GRASS environment settings" {} -command {execute g.gisenv }}
		{command {[G_msg "Show current GRASS version"]} {} "g.version -c: Show current GRASS version" {} -command {run_panel "g.version -c" }}
	}}
	{cascad {[G_msg "Manage projections"]} {} "" $tmenu {
		{command {[G_msg "Manage projections"]} {} "g.proj: Show projection information and create projection files" {} -command {execute g.proj }}
		{command {[G_msg "Projection for current location"]} {} "g.setproj: Create/edit projection information for current location" {} -command {term g.setproj }}
		{separator}
		{command {[G_msg "Convert coordinates"]} {} "m.proj: Convert coordinates from one projection to another" {} -command {execute m.proj }}
	}}
	{command {[G_msg "Display font"]} {} "Set default display font" {} -command {Gm::defaultfont "menu" }}
 } 
 {[G_msg "&Raster"]} all options $tmenu {
	{cascad {[G_msg "Develop map"]} {} "" $tmenu {			
		{command {[G_msg "Digitize raster"]} {} "r.digit" {} -command {
			unset env(GRASS_RENDER_IMMEDIATE)
			guarantee_xmon
			term r.digit 
			set env(GRASS_RENDER_IMMEDIATE) "TRUE"}}
		{separator}
		{command {[G_msg "Compress/decompress"]} {} "r.compress: Compress/decompress raster file" {} -command {execute r.compress }}
		{separator}
		{command {[G_msg "Boundaries"]} {} "r.region: Manage boundary definitions" {} -command {execute r.region }}
		{command {[G_msg "Null values"]} {} "r.null:Manage null values" {} -command {execute r.null }}
		{command {[G_msg "Quantization"]} {} "r.quant: Quantization for floating-point maps" {} -command {execute r.quant }}
		{command {[G_msg "Timestamps"]} {} "r.timestamp: Manage timestamps for files" {} -command {execute r.timestamp }}
		{separator}
		{command {[G_msg "Resample using aggregate statistics"]} {} "r.resamp.stats: Resample (change resolution) using aggregate statistics sing regularized spline tension" {} -command {execute r.resamp.stats }}
		{command {[G_msg "Resample using multiple methods"]} {} "r.resamp.interp: Resample (change resolution) using nearest neighbor, bilinear, or bicubic interpolation" {} -command {execute r.resamp.interp }}
		{command {[G_msg "Resample using nearest neighbor"]} {} "r.resample: Resample (change resolution) using nearest neighbor interpolation" {} -command {execute r.resample }}
		{command {[G_msg "Resample using spline tension"]} {} "r.resamp.rst: Resample (change resolution)" {} -command {execute r.resamp.rst }}
		{separator}
		{command {[G_msg "Support file maintenance"]} {} "r.support: Support file maintenance" {} -command {term r.support }}
		{command {[G_msg "Update map statistics"]} {} "r.support.stats: Update map statistics" {} -command {execute r.support.stats }}
		{separator}
		{command {[G_msg "Reproject"]} {} "r.proj: Reproject raster from other location" {} -command {execute r.proj }}
		{command {[G_msg "Tiling"]} {} "r.tileset: Generate tiling for other projection" {} -command {execute r.tileset }}
	}}
	{cascad {[G_msg "Manage map colors"]} {} "" $tmenu {			
		{command {[G_msg "Color tables"]} {} "r.colors: Set colors to predefined color tables or from another raster map" {} -command {execute r.colors }}
		{command {[G_msg "Color rules"]} {} "Set colors interactively by entering color rules" {} -command {GmRules::main "r.colors" }}
		{separator}
		{command {[G_msg "Blend"]} {} "r.blend: Blend 2 color maps to produce 3 RGB files" {} -command {execute r.blend }}
		{command {[G_msg "Create RGB"]} {} "r.composite: Create color image from RGB files" {} -command {execute r.composite }}
		{command {[G_msg "HIS to RGB"]} {} "r.his: Create 3 RGB (red, green, blue) maps from 3 HIS (hue, intensity, saturation) maps" {} -command {execute r.his }}
	}}
	{separator}
	{command {[G_msg "Query by coordinate(s)"]} {} "r.what: Query by coordinate(s)" {} -command { execute r.what }}
	{separator}
	{command {[G_msg "Buffers"]} {} "r.buffer: Create raster buffers" {} -command { execute r.buffer }}
	{command {[G_msg "Closest points"]} {} "r.distance: Locate closest points between areas in 2 raster maps" {} -command { execute r.distance }}
	{command {[G_msg "MASK"]} {} "r.mask: Create raster MASK (defines map areas for GIS operations)" {} -command { execute r.mask }}
	{command {[G_msg "Map calculator"]} {} "r.mapcalculator: Map calculator" {} -command { execute r.mapcalculator }}
	{cascad  {[G_msg "Neighborhood analysis"]} {} "" $tmenu {			
		{command {[G_msg "Moving window"]} {} "r.neighbors: Moving window analysis of raster cells" {} -command { execute r.neighbors }}
		{command {[G_msg "Neighborhood points"]} {} "v.neighbors: Analyze vector points in neighborhood of raster cells" {} -command { execute v.neighbors }}
	}}
	{cascad {[G_msg "Overlay maps"]} {} "" $tmenu {			
		{command {[G_msg "Cross product"]} {} "r.cross: Cross product" {} -command {execute r.cross }}
		{command {[G_msg "Map series"]} {} "r.series: Function of map series (time series)" {} -command {execute r.series }}
		{command {[G_msg "Patch maps"]} {} "r.patch: Patch maps" {} -command {execute r.patch }}
		{separator}
		{command {[G_msg "Statistical overlay"]} {} "r.statistics: Statistical calculations for cover map over base map" {} -command {execute r.statistics }}
	}}
	{cascad {[G_msg "Solar radiance and shadows"]} {} "" $tmenu {			
		{command {[G_msg "Solar irradiance irradiation"]} {} "r.sun: Solar irradiance and daily irradiation" {} -command {execute r.sun }}
		{command {[G_msg "Shadows map"]} {} "r.sunmask: Shadows map for sun position or date/time" {} -command {execute r.sunmask }}
	}}
	{cascad {[G_msg "Terrain analysis"]} {} "" $tmenu {			
		{command {[G_msg "Cumulative movement costs"]} {} "r.walk: Calculate cumulative movement costs between locales" {} -command {execute r.walk }}
		{command {[G_msg "Cost surface"]} {} "r.cost: Cost surface" {} -command {execute r.cost }}
		{command {[G_msg "Least cost route or flow"]} {} "r.drain: Least cost route or flow" {} -command {execute r.drain }}
		{separator}
		{command {[G_msg "Shaded relief"]} {} "r.shaded.relief: Shaded relief map" {} -command {execute r.shaded.relief }}
		{separator}
		{command {[G_msg "Slope and aspect"]} {} "r.slope.aspect: Slope and aspect" {} -command {execute r.slope.aspect }}
		{command {[G_msg "Terrain parameters"]} {} "r.param.scale: Terrain parameters" {} -command {execute r.param.scale }}
		{command {[G_msg "Textural features"]} {} "r.texture: Textural features" {} -command {execute r.texture }}
		{separator}
		{command {[G_msg "Visibility"]} {} "r.los: Visibility/line of sight" {} -command {execute r.los }}
	}}
	{cascad {[G_msg "Transform features"]} {} "" $tmenu {			
		{command {[G_msg "Clump"]} {} "r.clump: Clump small areas (statistics calculated by r.volume)" {} -command {execute r.clump }}
		{command {[G_msg "Grow"]} {} "r.grow: Grow areas" {} -command {execute r.grow }}
		{command {[G_msg "Thin"]} {} "r.thin: Thin linear features" {} -command {execute r.thin }}
	}}
	{separator}
	{cascad {[G_msg "Hydrologic modeling"]} {} "" $tmenu {			
		{command {[G_msg "Carve stream channels"]} {} "r.carve: Carve stream channels into elevation map using vector streams map" {} -command {execute r.carve }}
		{command {[G_msg "Fill lake"]} {} "r.lake: Fill lake from seed point to specified level" {} -command {execute r.lake }}
		{separator}
		{command {[G_msg "Depressionless map and flowlines"]} {} "r.fill.dir: Depressionless elevation map and flowline map" {} -command {execute r.fill.dir }}
		{command {[G_msg "Flow accumulation"]} {} "r.terraflow: Flow accumulation for massive grids" {} -command {execute r.terraflow }}
		{command {[G_msg "Flow lines"]} {} "r.flow: " {} -command {execute r.flow }}
		{separator}
	    {command {[G_msg "Groundwater flow model"]} {} "r.gwflow: 2D groundwater flow model" {} -command {execute r.gwflow }}
		{separator}
		{command {[G_msg "SIMWE overland flow modeling"]} {} "r.sim.water: SIMWE overland flow modeling" {} -command {execute r.sim.water }}
		{command {[G_msg "SIMWE sediment flux modeling"]} {} "r.sim.sediment: SIMWE sediment erosion, transport, & deposition modeling" {} -command {execute r.sim.sediment }}
		{separator}
		{command {[G_msg "Topographic index map"]} {} "r.topidx: Topographic index map" {} -command {execute r.topidx }}
		{command {[G_msg "TOPMODEL simulation"]} {} "r.topmodel: TOPMODEL simulation" {} -command {execute r.topmodel }}
		{separator}
		{command {[G_msg "USLE K Factor"]} {} "r.uslek: Soil Erodibility" {} -command {execute r.uslek }}
		{command {[G_msg "USLE R Factor"]} {} "r.usler: Rainfall Erosivity" {} -command {execute r.usler }}
		{separator}
		{command {[G_msg "Watershed subbasins"]} {} "r.basins.fill: Watershed subbasins" {} -command {execute r.basins.fill }}
		{command {[G_msg "Watershed analysis"]} {} "r.watershed: Watershed analysis" {} -command {execute r.watershed }}
		{command {[G_msg "Watershed basin creation"]} {} "r.water.outlet: Watershed basin creation" {} -command {execute r.water.outlet }}
	}}
	{cascad {[G_msg "Landscape structure modeling"]} {} "" $tmenu {			
		{command {[G_msg "Set up"]} {} "r.le.setup: Set up sampling and analysis framework" {} -command {
			unset env(GRASS_RENDER_IMMEDIATE)
			guarantee_xmon
			term r.le.setup 
			set env(GRASS_RENDER_IMMEDIATE) "TRUE"}}
		{separator}
		{command {[G_msg "Analyze landscape"]} {} "r.le.pixel: Analyze landscape characteristics" {} -command {execute r.le.pixel }}
		{command {[G_msg "Analyze patches"]} {} " r.le.patch: Analyze landscape patch characteristics" {} -command {execute r.le.patch }}
		{command {[G_msg "Output"]} {} "r.le.trace: Output landscape patch information" {} -command {execute r.le.trace }}
	}}
	{cascad {[G_msg "Landscape patch analysis"]} {} "" $tmenu {			
		{command {[G_msg "Set up sampling and analysis framework"]} {} "r.li.setup: Configure and create patch map for analysis" {} -command {execute r.li.setup }}
		{separator}
		{command {[G_msg "Edge density"]} {} "r.li.edgedensity: Calculate edge density index using a 4 neighbour algorithm" {} -command {execute r.li.edgedensity }}
		{command {[G_msg "Contrast weighted edge density"]} {} "r.li.cwed: Calculate contrast weighted edge density index" {} -command {execute r.li.cwed }}
		{separator}
		{command {[G_msg "Patch size mean"]} {} "r.li.mps: Calculate mean patch size index using a 4 neighbour algorithm" {} -command {execute r.li.mps }}
 		{command {[G_msg "Patch area range"]} {} "r.li.padrange: Calculate range of patch area size" {} -command {execute r.li.padrange }}
 		{command {[G_msg "Patch area Std Dev"]} {} "r.li.padsd: Calculate standard deviation of patch area" {} -command {execute r.li.padsd }}
		{command {[G_msg "Patch area Coeff Var"]} {} "r.li.padcv: Calculate coefficient of variation of patch area" {} -command {execute r.li.padcv }}
 		{command {[G_msg "Patch density"]} {} "r.li.patchdensity: Calculate patch density index using a 4 neighbour algorithm" {} -command {execute r.li.patchdensity }}
 		{command {[G_msg "Patch number"]} {} "r.li.patchnum: Calculate patch number index using a 4 neighbour algorithm" {} -command {execute r.li.patchnum }}
		{separator}
		{command {[G_msg "Dominance's diversity"]} {} "r.li.dominance: Calculate Dominance's diversity index" {} -command {execute r.li.dominance }}
 		{command {[G_msg "Shannon's diversity"]} {} "r.li.shannon: Calculate Shannon's diversity index" {} -command {execute r.li.shannon }}
 		{command {[G_msg "Simpson's diversity"]} {} "r.li.simpson: Calculate Simpson's diversity index" {} -command {execute r.li.simpson }}
		{separator}
		{command {[G_msg "Ricness"]} {} "r.li.richness: Calculate ricness index" {} -command {execute r.li.richness }}
 		{command {[G_msg "Shape index"]} {} "r.li.shape: Calculate shape index" {} -command {execute r.li.shape }}
	}}
	{cascad {[G_msg "Wildfire modeling"]} {} "" $tmenu {			
		{command {[G_msg "Rate of spread"]} {} "r.ros: Generate rate of spread (ROS) maps" {} -command {execute r.ros }}
		{command {[G_msg "Least-cost spread paths"]} {} "r.spreadpath: Generate least-cost spread paths" {} -command {execute r.spreadpath }}
		{command {[G_msg "Anisotropic spread simulation"]} {} "r.spread: Simulate anisotropic spread phenomena" {} -command {execute r.spread }}
	}}
	{separator}
	{cascad {[G_msg "Change category values and labels"]} {} "" $tmenu {			
		{command {[G_msg "Interactively edit category values"]} {} "d.rast.edit: Edit category values of individual cells for displayed raster map" {} -command {execute d.rast.edit }}
		{separator}
		{command {[G_msg "Reclassify by size"]} {} "r.reclass.area: Reclassify categories for areas of specified sizes" {} -command {execute r.reclass.area }}
		{command {[G_msg "Reclassify interactively"]} {} "Reclassify categories interactively by entering reclass rules" {} -command {GmRules::main "r.reclass" }}
		{command {[G_msg "Reclassify using rules file"]} {} "r.reclass: Reclassify categories by inputting rules from a text file" {} -command {execute r.reclass}}
		{separator}
		{command {[G_msg "Recode interactively"]} {} "Recode categories interactively by entering recode rules (create new map)" {} -command {GmRules::main "r.recode" }}
		{command {[G_msg "Recode using rules file"]} {} "r.recode: Recode categories  by inputting rules from a text file (create new map)" {} -command {execute r.recode }}
		{separator}
		{command {[G_msg "Rescale"]} {} "r.rescale: Rescale categories (create new map)" {} -command {execute r.rescale }}
		{command {[G_msg "Rescale with histogram"]} {} "r.rescale.eq: Rescale categories with equalized histogram (create new map)" {} -command {execute r.rescale.eq }}
	}}
	{separator}
	{command {[G_msg "Concentric circles"]} {} "r.circle: Generate concentric circles around points" {} -command { execute r.circle }}
	{cascad {[G_msg "Generate random cells"]} {} "" $tmenu {			
		{command {[G_msg "Random cells"]} {} "r.random.cells: Generate random cells" {} -command {execute r.random.cells }}
		{command {[G_msg "Random cells and vector points"]} {} "r.random: Generate random cells and vector points from raster map" {} -command {execute r.random }}
	}}
	{cascad {[G_msg "Generate surfaces"]} {} "" $tmenu {			
		{command {[G_msg "Fractal surface"]} {} "r.surf.fractal: Generate fractal surface" {} -command {execute r.surf.fractal }}
		{separator}
		{command {[G_msg "Gausian kernal density surface"]} {} "v.kernel: Generate density surface using moving Gausian kernal" {} -command {execute v.kernel }}
		{command {[G_msg "Gaussian deviates surface"]} {} "r.surf.gauss: Generate gaussian deviates surface" {} -command {execute r.surf.gauss }}
		{separator}
		{command {[G_msg "Plane"]} {} "r.plane: Generate plane" {} -command {execute r.plane }}
		{separator}
		{command {[G_msg "Random deviates surface"]} {} "r.surf.random: Generate random deviates surface" {} -command {execute r.surf.random }}
		{command {[G_msg "Random surface with spatial dependence"]} {} "r.random.surface: Generate random surface with spatial dependence" {} -command {execute r.random.surface }}
	}}
	{command {[G_msg "Contour lines"]} {} "r.contour: Generate vector contour lines" {} -command { execute r.contour }}
	{cascad {[G_msg "Interpolate surfaces"]} {} "" $tmenu {			
		{command {[G_msg "Bilinear from raster points"]} {} "r.bilinear: Bilinear interpolation from raster points" {} -command { execute r.bilinear }}
		{command {[G_msg "Bilinear and bicubic from vector points"]} {} "v.surf.bspline: Bicubic and bilinear interpolation with Tykhonov regularization from vector points" {} -command { execute v.surf.bspline }}
		{separator}
		{command {[G_msg "IDW from raster points"]} {} "r.surf.idw: Inverse distance weighted interpolation from raster points" {} -command { execute r.surf.idw }}
		{command {[G_msg "IDW from vector points"]} {} "v.surf.idw: Inverse distance weighted interpolation from vector points" {} -command { execute v.surf.idw }}
		{separator}
		{command {[G_msg "Raster contours"]} {} "r.surf.contour: Interpolation from raster contours" {} -command { execute r.surf.contour }}
		{command {[G_msg "Regularized spline tension"]} {} "v.surf.rst: Regularized spline tension interpolation from vector points or contours" {} -command { execute v.surf.rst }}
		{separator}
		{command {[G_msg "Fill NULL cells"]} {} " r.fillnulls: Fill NULL cells by interpolation using regularized spline tension" {} -command {execute r.fillnulls }}
	}}
	{separator}
	{cascad {[G_msg "Reports and statistics"]} {} "" $tmenu {			
		{command {[G_msg "Report basic file information"]} {} "r.info: Report basic file information" {} -command {execute r.info }}
		{command {[G_msg "Manage category information"]} {} "r.category: Manage category labels and values" {} -command {execute r.category }}
		{separator}
		{command {[G_msg "General statistics"]} {} "r.stats: General statistics" {} -command {execute r.stats }}
		{command {[G_msg "Range of category values"]} {} "r.describe: Range of all category values" {} -command {execute r.describe }}
		{command {[G_msg "Sum cell category values"]} {} "r.sum: Sum all cell category values" {} -command {execute r.sum }}
		{command {[G_msg "Sum area by map and category"]} {} "r.report:Sum area by map and category" {} -command {execute r.report }}
		{command {[G_msg "Statistics for clumped cells (works with r.clump)"]} {} "r.volume: " {} -command {execute r.volume }}
		{command {[G_msg "Total surface area corrected for topography"]} {} "r.surf.area: Total surface area corrected for topography" {} -command {execute r.surf.area }}
		{command {[G_msg "Univariate statistics"]} {} "r.univar: Univariate statistics" {} -command {execute r.univar }}
		{separator}
		{command {[G_msg "Sample transects"]} {} "r.profile: Sample values along transects" {} -command {execute r.profile }}
		{command {[G_msg "Sample transects (bearing/distance)"]} {} " r.transect: Sample values along transects (use azimuth, distance)" {} -command {execute r.transect }}
		{separator}
		{command {[G_msg "Covariance/correlation"]} {} "r.covar: Covariance/correlation" {} -command {execute r.covar }}
		{command {[G_msg "Linear regression"]} {} "r.regression.line: Linear regression between 2 maps" {} -command {execute r.regression.line }}
		{command {[G_msg "Mutual category occurences"]} {} "r.coin: Mutual category occurences (coincidence)" {} -command {execute r.coin }}
	}}
 } 
 {[G_msg "&Vector"]} all options $tmenu {
	{cascad {[G_msg "Develop map"]} {} "" $tmenu {			
		{command {[G_msg "Digitize"]} {} "v.digit: Digitize/edit vector map" {} -command {execute v.digit }}
		{command {[G_msg "Edit features"]} {} "v.edit: Edit vector features" {} -command {execute v.edit }}
		{separator}
		{command {[G_msg "Create/rebuild topology: "]} {} "v.build: Create or rebuild topology of vector objects" {} -command {execute v.build }}
		{command {[G_msg "Clean vector"]} {} "v.clean: Clean vector objects" {} -command {execute v.clean }}
		{command {[G_msg "Generalization"]} {} "v.generalize: Smooth, simplify, displace, or generalize a vector map" {} -command {execute v.generalize }}
		{separator}
		{command {[G_msg "Convert object types"]} {} "v.type: Convert vector objects from one feature type to another" {} -command {execute $env(GISBASE)/etc/gui/scripts/v.type.py }}
		{separator}
		{command {[G_msg "Add centroids"]} {} "v.centroids: Add centroids to closed boundaries to create areas" {} -command {execute v.centroids }}
		{separator}
		{command {[G_msg "Build polylines"]} {} "v.build.polylines: Build polylines from adjacent segments" {} -command {execute v.build.polylines }}
		{command {[G_msg "Split polylines"]} {} "v.segment: Split polylines into points and segments" {} -command {execute v.segment }}
		{command {[G_msg "Parallel lines"]} {} "v.parallel: Create lines parallel to existing lines" {} -command {execute v.parallel }}
		{separator}
		{command {[G_msg "Dissolve boundaries"]} {} "v.dissolve: Dissolve common boundaries of areas" {} -command {execute v.dissolve }}
		{separator}
		{command {[G_msg "Create 3D vector over raster"]} {} "v.drape: Create 3D objects by sampling raster with 2D vector" {} -command {execute v.drape }}
		{command {[G_msg "Extrude 3D vector"]} {} "v.extrude: Extrude 3D objects from 2D vector" {} -command {execute v.extrude }}
		{separator}
		{command {[G_msg "Link to OGR"]} {} "v.external: Create new vector as link to external OGR layer" {} -command {execute v.external }}
		{separator}
		{command {[G_msg "Create labels"]} {} "v.label: Create text label file for vector objects" {} -command {execute v.label }}
		{separator}
		{command {[G_msg "Reposition vector"]} {} "v.transform: Reposition (shift, rotate, skew) vector file in coordinate space" {} -command {execute v.transform }}
		{command {[G_msg "Reproject vector"]} {} "v.proj: Reproject vector from other location" {} -command {execute v.proj }}
		{separator}
		{command {[G_msg "Metadata support"]} {} "v.support: Edit metadata for vector map" {} -command {execute v.support }}
		{separator}
	}}
	{separator}
	{command {[G_msg "Query with attributes"]} {} "v.extract: Query vector objects by attribute values" {} -command {execute v.extract }}
	{command {[G_msg "Query with coordinate(s)"]} {} "v.what: Query vector objects by coordinate(s)" {} -command { execute v.what }}
	{command {[G_msg "Query with another map"]} {} " v.select: Query objects using objects from a second map" {} -command {execute v.select }}
	{separator}
	{command {[G_msg "Buffer vectors"]} {} "v.buffer: Create vector buffers around vector objects" {} -command {execute v.buffer }}
	{cascad {[G_msg "Lidar analysis"]} {} "" $tmenu {			
		{command {[G_msg "Detect edges"]} {} "v.lidar.edgedetection: Detect object edges in Lidar data" {} -command {execute v.lidar.edgedetection }}
		{command {[G_msg "Detect interiors"]} {} "v.lidar.growing: Detect interiors of objects in Lidar data" {} -command {execute v.lidar.growing }}
		{command {[G_msg "Correct and reclassify objects"]} {} "v.lidar.correction: Correct and reclassify objects detected in Lidar data" {} -command {execute v.lidar.correction }}
	}}
	{cascad {[G_msg "Linear referencing"]} {} "" $tmenu {			
		{command {[G_msg "Create LRS"]} {} "v.lrs.create: Create linear reference system" {} -command {execute v.lrs.create }}
		{command {[G_msg "Create stationing"]} {} "v.lrs.label: Create stationing from input lines and a linear reference system" {} -command {execute v.lrs.label }}
		{command {[G_msg "Create points/segments"]} {} "v.lrs.segment: Create points and line segments along a linear reference system" {} -command {execute v.lrs.segment }}
		{command {[G_msg "Find ID and offset"]} {} "v.lrs.where: Find line ID and real km+offset for given points in vector map using linear reference system" {} -command {execute v.lrs.where }}
	}}			
	{command {[G_msg "Nearest features"]} {} "v.distance: Locate nearest features to points or centroids" {} -command {execute v.distance }}
	{cascad {[G_msg "Network analysis"]} {} "" $tmenu {			
		{command {[G_msg "Allocate subnets"]} {} "v.net.alloc: Allocate subnets for nearest centers" {} -command {execute v.net.alloc }}
		{command {[G_msg "Network maintenance"]} {} "v.net: Network maintenance" {} -command {execute v.net }}
		{command {[G_msg "Visibility network"]} {} "v.net.visibility: Create and maintain a visibility network" {} -command {execute v.net.visibility }}
		{command {[G_msg "Shortest route"]} {} "v.net.path: Calculate shortest route along network between 2 nodes" {} -command {execute v.net.path }}
		{command {[G_msg "Display shortest route"]} {} "d.path: Display shortest route along network between 2 nodes (visualization only)" {} -command {
			unset env(GRASS_RENDER_IMMEDIATE)
			guarantee_xmon
			spawn d.path.sh -b --ui
			set env(GRASS_RENDER_IMMEDIATE) "TRUE"}}
		{command {[G_msg "Split net"]} {} "v.net.iso: Split net into bands between cost isolines" {} -command {execute v.net.iso }}
		{command {[G_msg "Steiner tree"]} {} "v.net.steiner: Create Steiner tree for network and given terminals" {} -command {execute v.net.steiner }}
		{command {[G_msg "Traveling salesman analysis"]} {} "v.net.salesman: Calculate shortest route connecting given set of nodes (Traveling salesman analysis)" {} -command {execute v.net.salesman }}
	}}
	{cascad {[G_msg "Overlay maps"]} {} "" $tmenu {			
		{command {[G_msg "Overlay"]} {} "v.overlay: Boolean overlay of 2 vector maps" {} -command {execute v.overlay }}
		{command {[G_msg "Patch (combine)"]} {} "v.patch: Patch/combine multiple maps (Boolean OR)" {} -command {execute v.patch }}
	}}
	{separator}
	{cascad {[G_msg "Change attributes"]} {} "" $tmenu {			
		{command {[G_msg "Manage or report categories"]} {} "v.category: Attach, delete, or report categories" {} -command {execute v.category }}
		{command {[G_msg "Reclassify objects interactively"]} {} "Reclassify objects interactively by entering SQL rules" {} -command {GmRules::main "v.reclass" }}
		{command {[G_msg "Reclassify using rules file"]} {} "v.reclass: Reclassify objects by inputting rules from a text file" {} -command {execute v.reclass }}
	}}
	{separator}
	{command {[G_msg "Generate area for current region"]} {} "v.in.region: Generate area object for extent of current region" {} -command {execute v.in.region }}
	{cascad {[G_msg "Generate areas from points"]} {} "" $tmenu {			
		{command {[G_msg "Convex hull"]} {} "v.hull: Generate convex hull for point set" {} -command {execute v.hull }}
		{command {[G_msg "Delaunay triangles"]} {} "v.delaunay: Generate Delaunay triangles for point set" {} -command {execute v.delaunay }}
		{command {[G_msg "Voronoi diagram/Thiessen polygons"]} {} "v.voronoi: Generate Voronoi diagram/Thiessen polygons for point set" {} -command {execute v.voronoi }}
	}}
	{command {[G_msg "Generate grid"]} {} "v.mkgrid: Generate rectangular vector grid" {} -command {execute v.mkgrid }}		
	{cascad {[G_msg "Generate points"]} {} "" $tmenu {			
		{command {[G_msg "Generate points from database"]} {} "v.in.db: Generate points from database with x/y coordinates" {} -command {execute v.in.db }}
		{command {[G_msg "Generate points along lines"]} {} "v.to.points: Generate points along vector lines/boundaries" {} -command {execute v.to.points }}
		{command {[G_msg "Generate random points"]} {} "v.random: Generate random points" {} -command {execute v.random }}
		{command {[G_msg "Perturb points"]} {} "v.perturb: Random perturbations of point locations" {} -command {execute v.perturb }}
	}}
	{separator}
	{command {[G_msg "Remove outliers in point sets"]} {} "v.outlier: Remove outliers from vector point set" {} -command {execute v.outlier }}
	{command {[G_msg "Test/training sets"]} {} "v.kcv: Partition points into test/training sets for k-fold cross validatation" {} -command {execute v.kcv }}
	{separator}
	{command {[G_msg "Update area attributes from raster"]} {} "v.rast.stats: Update area attribute data from univariate statistics on raster map" {} -command {execute v.rast.stats }}
	{command {[G_msg "Update point attributes from areas"]} {} "v.what.vect: Update point attribute data from vector area map" {} -command {execute v.what.vect }}
	{cascad {[G_msg "Update point attributes from raster"]} {} "" $tmenu {			
		{command {[G_msg "Sample raster map at point locations"]} {} "v.what.rast: " {} -command {execute v.what.rast }}
		{command {[G_msg "Sample raster neighborhood around points"]} {} "v.sample: " {} -command {execute v.sample }}
	}}
	{separator}
	{cascad {[G_msg "Reports and statistics"]} {} "" $tmenu {			
		{command {[G_msg "Basic information"]} {} "v.info: Basic information" {} -command {execute v.info }}
 		{separator}
		{command {[G_msg "Report topology by category"]} {} "v.report: Report areas for vector attribute categories" {} -command {execute v.report }}
		{command {[G_msg "Upload or report topology"]} {} "v.to.db: Update database fields or create reports from vector topology" {} -command {execute v.to.db }}
 		{separator}
		{command {[G_msg "Univariate attribute statistics"]} {} "v.univar: Calculate univariate statistics for vector attributes" {} -command {execute v.univar }}
 		{separator}
		{command {[G_msg "Quadrat indices"]} {} "v.qcount: Indices of point counts in quadrats" {} -command {execute v.qcount }}
		{command {[G_msg "Test normality"]} {} "v.normal: Test normality of point distribution" {} -command {execute v.normal }}
	}}
 } 
 {[G_msg "&Imagery"]} all options $tmenu {			
	{cascad {[G_msg "Develop images and groups"]} {} "" $tmenu {			
		{command {[G_msg "Create/edit group"]} {} "i.group: Create/edit imagery group" {} -command {execute i.group }}			
		{command {[G_msg "Target group"]} {} "i.target: Target imagery group" {} -command {execute i.target }}
		{separator}
		{command {[G_msg "Mosaic images"]} {} "i.image.mosaic: Mosaic up to 4 adjacent images" {} -command {execute i.image.mosaic }}
	}}
	{cascad {[G_msg "Manage image colors"]} {} "" $tmenu {			
		{command {[G_msg "Color balance for RGB"]} {} "i.landsat.rgb: Color balance and enhance color tables of multiband imagery for rgb display" {} -command {execute i.landsat.rgb }}
		{command {[G_msg "HIS to RGB"]} {} "i.his.rgb: Transform HIS (Hue/Intensity/Saturation) color image to RGB (Red/Green/Blue)" {} -command {execute i.his.rgb }}
		{command {[G_msg "RGB to HIS"]} {} "i.rgb.his: Transform RGB (Red/Green/Blue) color image to HIS (Hue/Intensity/Saturation)" {} -command {execute i.rgb.his }}
	}}
	{command {[G_msg "Ortho photo rectification"]} {} "i.ortho.photo: Ortho photo rectification" {} -command {
		unset env(GRASS_RENDER_IMMEDIATE)
		guarantee_xmon
		term i.ortho.photo 
		set env(GRASS_RENDER_IMMEDIATE) "TRUE"}}
	{separator}
	{cascad {[G_msg "Basic RS processing"]} {} "" $tmenu {			
				{command {[G_msg "MODIS Quality flags"]} {} "i.modis.qc: Extracts Modis Quality flags as raster values." {} -command {execute i.modis.qc }}
				{command {[G_msg "Latitude/longitude maps"]} {} "i.latlong: Latitude or Longitude maps." {} -command {execute i.latlong }}
				{separator}
				{command {[G_msg "Albedo"]} {} "i.albedo: Calculates Albedo from Modis, Aster, Landsat or AVHRR" {} -command {execute i.albedo }}
				{separator}
				{command {[G_msg "Vegetation Indices"]} {} "i.vi: Calculates 14 types of Vegetation Indices" {} -command {execute i.vi }}
	}}
	{command {[G_msg "Brovey sharpening"]} {} "i.fusion.brovey: Brovey transformation and pan sharpening" {} -command {execute i.fusion.brovey }}
	{cascad {[G_msg "Classify image"]} {} "" $tmenu {			
		{command {[G_msg "Clustering input for unsupervised classification"]} {} "i.cluster: Clustering input for unsupervised classification" {} -command {execute i.cluster }}
		{separator}
		{command {[G_msg "Maximum likelyhood classification (MLC)"]} {} "i.maxlik: Maximum likelyhood classification" {} -command {execute i.maxlik }}
		{command {[G_msg "Sequential maximum a posteriory classification (SMAP)"]} {} "i.smap: Sequential maximum a posteriory classification" {} -command {execute i.smap }}
		{separator}
		{command {[G_msg "Interactive input for supervised classification"]} {} "i.class: Interactive input for supervised classification" {} -command {term i.class }}
		{command {[G_msg "Input for supervised MLC"]} {} "i.gensig: Non-interactive input for supervised classification (MLC)" {} -command {execute i.gensig }}
		{command {[G_msg "Input for supervised SMAP"]} {} "i.gensigset: Non-interactive input for supervised classification (SMAP)" {} -command {execute i.gensigset }}
	}}
	{cascad {[G_msg "Filter image"]} {} "" $tmenu {			
		{command {[G_msg "Edge detection"]} {} "i.zc: Zero edge crossing detection" {} -command {execute i.zc }}
		{command {[G_msg "Matrix/convolving filter"]} {} "r.mfilter: User defined matrix/convolving filter" {} -command {execute r.mfilter }}
	}}
	{command {[G_msg "Spectral response"]} {} "i.spectral: Spectral response" {} -command {execute i.spectral }}
	{command {[G_msg "Tassled cap vegetation index"]} {} "i.tasscap: Tassled cap vegetation index" {} -command {execute i.tasscap }}
	{cascad {[G_msg "Transform image"]} {} "" $tmenu {			
		{command {[G_msg "Canonical correlation"]} {} "i.cca: Canonical correlation (discriminant analysis)" {} -command {execute i.cca }}
		{command {[G_msg "Principal components"]} {} "i.pca: Principal components analysis" {} -command {execute i.pca }}
		{command {[G_msg "Fast Fourier"]} {} "i.fft: Fast Fourier transform" {} -command {execute i.fft }}
		{command {[G_msg "Inverse Fast Fourier"]} {} "i.ifft: Inverse fast Fourier transform" {} -command {execute i.ifft }}
	}}
	{separator}
	{cascad {[G_msg "Reports and statistics"]} {} "" $tmenu {			
		{command {[G_msg "Bit pattern comparison"]} {} "r.bitpattern: Bit pattern comparison for ID of low quality pixels" {} -command {execute r.bitpattern }}
		{command {[G_msg "Kappa analysis"]} {} "r.kappa: Kappa classification accuracy assessment" {} -command {execute r.kappa }}
		{command {[G_msg "OIF for LandSat TM"]} {} "i.oif: Optimum index factor for LandSat TM" {} -command {execute i.oif }}
	}}
 } 
 {[G_msg "&Volumes"]} all options $tmenu {
	{cascad {[G_msg "Develop grid3D volumes"]} {} "" $tmenu {			
		{command {[G_msg "Manage nulls for grid3D volume"]} {} "r3.null: Manage nulls for grid3D volume" {} -command {execute r3.null }}
		{command {[G_msg "Manage timestamp for grid3D volume"]} {} "r3.timestamp" {} -command {execute r3.timestamp }}
	}}
	{command {[G_msg "3D MASK"]} {} "r3.mask: " {} -command {execute r3.mask }}
	{command {[G_msg "3D Map calculator"]} {} "r3.mapcalculator: Map calculator for grid3D operations" {} -command {execute r3.mapcalculator }}
	{command {[G_msg "Cross section from volume"]} {} "r3.cross.rast: Create 2D raster cross section from grid3D volume" {} -command { execute r3.cross.rast }}
	{command {[G_msg "Groundwater flow model"]} {} "r3.gwflow: 3D groundwater flow model" {} -command {execute r3.gwflow }}
	{command {[G_msg "Interpolate volume from vector points"]} {} "v.vol.rst: Interpolate volume from vector points using splines" {} -command {execute v.vol.rst }}
	{cascad {[G_msg "Report and Statistics"]} {} "" $tmenu {			
		{command {[G_msg "Basic information"]} {} "r3.info: Display information about grid3D volume" {} -command {execute r3.info }}
	}}
 } 
 {[G_msg "&Databases"]} all options $tmenu {
	{cascad {[G_msg "Database information"]} {} "" $tmenu {			
		{command {[G_msg "Describe table"]} {} "db.describe: Describe table structure and attributes" {} -command {execute db.describe }}
		{command {[G_msg "List columns"]} {} "db.columns: List columns for selected table" {} -command {execute db.columns }}
		{command {[G_msg "List drivers"]} {} "db.drivers: List available database drivers" {} -command {execute db.drivers }}
		{command {[G_msg "List tables"]} {} "db.tables: List tables in database" {} -command {execute db.tables }}
	}}
	{separator}
	{cascad {[G_msg "Manage database"]} {} "" $tmenu {			
		{command {[G_msg "Connect to database"]} {} "db.connect: Connect to database" {} -command {execute db.connect }}
		{command {[G_msg "Login to database"]} {} "db.login: Login to database" {} -command {execute db.login }}
		{separator}
		{command {[G_msg "Copy table"]} {} "db.copy: Copy attribute table" {} -command {execute db.copy }}
		{command {[G_msg "New table"]} {} "v.db.addtable: Create and connect new attribute table to vector map" {} -command {execute v.db.addtable }}
		{command {[G_msg "Remove table"]} {} "v.db.droptable: Remove existing attribute table for vector map" {} -command {execute v.db.droptable }}
		{separator}
		{command {[G_msg "Add columns"]} {} "v.db.addcol: Add columns to table" {} -command {execute v.db.addcol }}
		{command {[G_msg "Change values"]} {} "v.db.update: Change values in a column" {} -command {execute v.db.update }}
		{command {[G_msg "Drop column"]} {} "v.db.dropcol: Drop column from a table" {} -command {execute v.db.dropcol }}
		{command {[G_msg "Rename a column"]} {} "v.db.renamecol: Rename a column" {} -command {execute v.db.renamecol }}
		{separator}
		{command {[G_msg "Test database"]} {} "db.test: Test database" {} -command {execute db.test }}
	}}
	{separator}
	{cascad {[G_msg "Query"]} {} "" $tmenu {			
		{command {[G_msg "Query any table"]} {} "db.select: Query data in any table" {} -command {execute db.select }}
		{command {[G_msg "Query vector attribute table"]} {} "v.db.select: Query vector attribute data" {} -command {execute v.db.select }}
		{command {[G_msg "SQL statement"]} {} "db.execute: Execute SQL statement" {} -command {execute db.execute }}
	}}
	{cascad {[G_msg "Vector<->database connections"]} {} "" $tmenu {			
		{command {[G_msg "Reconnect vector to database"]} {} "v.db.reconnect.all: Reconnect vector map to attribute database" {} -command {execute v.db.reconnect.all }}
		{command {[G_msg "Set vector - database connection"]} {} "v.db.connect: Set database connection for vector attributes" {} -command {execute v.db.connect }}
	}}
 } 
{[G_msg "&Help"]} all options $tmenu {
	{command {[G_msg "GRASS help"]} {} "g.manual" {} -command { exec g.manual -i > $devnull & } }
	{command {[G_msg "GIS Manager &help"]} {} {[G_msg "GIS Manager help"]} {} -command { exec g.manual gis.m > $devnull & } }
	{command {[G_msg "About &GRASS"]} {} {[G_msg "About GRASS"]} {} -command { source $env(GISBASE)/etc/gm/grassabout.tcl} }
	{command {[G_msg "About &System"]} {} {[G_msg "About System"]} {} -command { exec $env(GRASS_WISH) $env(GISBASE)/etc/gm/tksys.tcl --tcltk & }}
 }

}]

# Should we add an Xtns menu entry?
if { $XtnsMenu == "True" } {
	# build extension menu
	lappend descmenu [G_msg "&Xtns"]
	lappend descmenu all
	lappend descmenu options
	lappend descmenu $tmenu
	lappend descmenu $menulist
}
