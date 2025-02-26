---
description: GRASS GIS Quickstart
---

# GRASS GIS Quickstart

When [launching](grass.md) GRASS GIS for the first time, you will open a
**default project** "world_latlog_wgs84" where you can find a map layer
called "country_boundaries" showing a world map in the WGS84 coordinate
system.

![GRASS GIS after first startup](grass_start.png)

The main component of the Data tab is the *Data Catalog* which shows the
GRASS GIS hierarchical structure consisting of database ![GRASS
Database](grassdb.png), project ![project](location.png) and
mapset ![mapset](mapset.png).

![GRASS Database](grassdb.png) **GRASS database** (directory with projects)  
Running GRASS GIS for the first time, a folder named "grassdata" is
automatically created. Depending on your operating system, you can find
it in your $HOME directory (\*nix) or My Documents (MS Windows).

![project](location.png) **project** (previously called location)  
A project is defined by its coordinate reference system (CRS). In the
case of the default project, it is a geographic coordinate reference
system WGS84 (EPSG:4326). If you have data in another CRS than WGS84,
you should create a new project corresponding to your system.

![mapset](mapset.png) **mapset** (a subproject)  
Each project can have many mapsets for managing different aspects of a
project or project's subregions. When creating a new project, GRASS GIS
automatically creates a special mapset called PERMANENT where the core
data for the project can be stored.

For more info about data hierarchy, see [GRASS GIS
Database](grass_database.md) page.

## GRASS started in the default project, now what?

First, if you would like to get to know GRASS better before importing
your own data, please download provided samples such as the "North
Carolina" dataset. You can simply reach them through "Download sample
project to current database" management icon ![Download
project](location-download.png).

To work with your own data, you typically want to first create a new
project with a [coordinate reference system
(CRS)](https://en.wikipedia.org/wiki/Spatial_reference_system) suitable
for your study area or one that matches your data's CRS. The Project
Wizard ![Add project](location-add.png) will help you with that by
guiding you through a series of dialogs to browse and select predefined
projections (also via EPSG code) or to define individual projections.

### Creating a New project with the Project Wizard

If you know the CRS of your data or study area, you can fill [EPSG
code](https://spatialreference.org/) or description and Project Wizard
finds appropriate CRS from a predefined list of projections. If you do
not know CRS of you data, you can read it from your georeferenced data
file (e.g. shapefile or GeoTiff file with the related metadata properly
included).

### Importing data

After creating a new project, you are ready to import your data. You can
use simple raster or vector data import ![Raster
import](raster-import.png), ![Vector import](vector-import.png) or
a variety of more specialized tools. If the data's CRS does not match
your project's CRS, data will be automatically reprojected. After import
your raster or vector data are added as a layer to Map Display. To
change layer properties, go to Display tab. To analyze your data, search
for a tool in the Modules tab.

## Text-based startup and project creation

GRASS GIS can be run entirely without using the graphical user
interface. See [examples](grass.md) of running GRASS GIS from a command
line.

## See also

*[GRASS GIS Reference Manual](index.md)  
[GRASS GIS startup program manual page](grass.md)  
[GRASS GIS tutorials and books](https://grass.osgeo.org/learn/)*

[List of EPSG codes](https://spatialreference.org/) (Database of
worldwide coordinate systems), [CRS Explorer - PROJ
codes](https://crs-explorer.proj.org/), and [EPSG Geodetic Parameter
Dataset](https://epsg.org/)
