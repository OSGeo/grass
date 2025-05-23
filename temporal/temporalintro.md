---
description: Temporal data processing in GRASS GIS
index: temporal
---

# Temporal data processing in GRASS GIS

The temporal enabled GRASS introduces three new data types that are
designed to handle time series data:

- *Space time raster datasets* (strds) are designed to manage raster map
  time series. Modules that process strds have the naming prefix
  *t.rast*.
- *Space time 3D raster datasets* (str3ds) are designed to manage 3D
  raster map time series. Modules that process str3ds have the naming
  prefix *t.rast3d*.
- *Space time vector datasets* (stvds) are designed to manage vector map
  time series. Modules that process stvds have the naming prefix
  *t.vect*.

These new data types can be managed, analyzed and processed with
temporal modules that are based on the GRASS GIS temporal framework.

## Temporal data management in general

Space time datasets are stored in a temporal database. A core principle
of the temporal framework is that temporal databases are mapset
specific. A new temporal database is created when a temporal command is
invoked in a mapset that does not contain any temporal databases yet.
For example, when a mapset was recently created.

**Therefore, as space-time datasets are mapset specific, they can only
register raster, 3D raster or vector maps from the same mapset.**

By default, space-time datasets can not register maps from other
mapsets. This is a security measure, since the registration of maps in a
space-time dataset will always modify the metadata of the registered
map. This is critical if:

- The user has no write access to the maps from other mapsets he/she
  wants to register
- If registered maps are removed from other mapsets, the temporal
  database will not be updated and will contain ghost maps

SQLite3 or PostgreSQL are supported as temporal database backends.
Temporal databases stored in other mapsets can be accessed as long as
those other mapsets are in the user's current mapset search path
(managed with [g.mapsets](g.mapsets.md)). Access to space-time datasets
from other mapsets is read-only. They can not be modified or removed.

Connection settings are performed with [t.connect](t.connect.md). By
default, a SQLite3 database is created in the current mapset to store
all space-time datasets and registered time series maps in that mapset.

New space-time datasets are created in the temporal database with
[t.create](t.create.md). The name of the new dataset, the type (strds,
str3ds, stvds), the title and the description must be provided for
creation. Optionally, the temporal type (absolute, relative) and the
semantic information can be provided.

The module [t.register](t.register.md) is designed to register raster,
3D raster and vector maps in the temporal database and in the space-time
datasets. It supports different input options. Maps to register can be
provided as a comma separated string at the command line, or in an input
file. The module supports the definition of time stamps (time instances
or intervals) for each map in the input file. With
[t.unregister](t.unregister.md) maps can be unregistered from space-time
datasets and from the temporal database.

**Important**  
*Use only temporal commands like [t.register](t.register.md) to attach a
time stamp to raster, 3D raster and vector maps. The commands
r.timestamp, r3.timestamp and v.timestamp should not be used because
they only modify the metadata of the map in the spatial database, but
they do not register maps in the temporal database. However, maps with
timestamps attached by means of \*.timestamp modules can be registered
in space-time datasets using the existing timestamp.*

The module [t.remove](t.remove.md) will remove the space-time datasets
from the temporal database and optionally all registered maps. It will
take care of multiple map registration, hence if maps are registered in
several space-time datasets in the current mapset. Use
[t.support](t.support.md) to modify the metadata of space time datasets
or to update the metadata that is derived from registered maps. This
module also checks for removed and modified maps and updates the
space-time datasets accordingly. Rename a space-time dataset with
[t.rename](t.rename.md).

To print information about space-time datasets or registered maps, the
module [t.info](t.info.md) can be used. [t.list](t.list.md) will list
all space-time datasets and registered maps in the temporal database.

The module [t.topology](t.topology.md) was designed to compute and check
the temporal topology of space-time datasets. Moreover, the module
[t.sample](t.sample.md) samples input space-time dataset(s) with a
sample space-time dataset and prints the result to standard output.
Different sampling methods are supported and can be combined.

List of general management modules:

- [t.connect](t.connect.md)
- [t.create](t.create.md)
- [t.rename](t.rename.md)
- [t.remove](t.remove.md)
- [t.register](t.register.md)
- [t.unregister](t.unregister.md)
- [t.info](t.info.md)
- [t.list](t.list.md)
- [t.sample](t.sample.md)
- [t.support](t.support.md)
- [t.topology](t.topology.md)

## Modules to visualize space-time datasets and temporal data

- [g.gui.animation](g.gui.animation.md)
- [g.gui.timeline](g.gui.timeline.md)
- [g.gui.mapswipe](g.gui.mapswipe.md)
- [g.gui.tplot](g.gui.tplot.md)

## Modules to process space-time raster datasets

The focus of the temporal GIS framework is the processing and analysis
of raster time series. Hence, the majority of the temporal modules are
designed to process space-time raster datasets (strds). However, there
are several modules to process space-time 3D raster datasets and
space-time vector datasets as well.

### Querying and map calculation

Maps registered in a space-time raster dataset can be listed using
[t.rast.list](t.rast.list.md). This module supports several methods to
list maps and uses SQL queries to determine how these maps are selected
and sorted. Subsets of space-time raster datasets can be extracted with
[t.rast.extract](t.rast.extract.md) that allows performing additional
mapcalc operations on the selected raster maps.

Several modules in the temporal framework have a *where* option. This
option allows performing different selections of maps registered in the
temporal database and in space-time datasets. The columns that can be
used to perform these selections are: *id, name, creator, mapset,
temporal_type, creation_time, start_time, end_time, north, south, west,
east, nsres, ewres, cols, rows, number_of_cells, min and max*. Note that
for vector time series, i.e. stvds, some of the columns that can be
queried to list/select vector maps differ from those for space-time
raster datasets (check with `t.vect.list --help`).

- [t.rast.extract](t.rast.extract.md)
- [t.rast.gapfill](t.rast.gapfill.md)
- [t.rast.mapcalc](t.rast.mapcalc.md)
- [t.rast.colors](t.rast.colors.md)
- [t.rast.neighbors](t.rast.neighbors.md)

Similar to the *where* option, selected modules like
[t.rast.univar](t.rast.univar.md) support a *region_relation* option to
limit computations to maps of the space time raster dataset that have a
given spatial relation to the current computational region. Supported
spatial relations are:

- "overlaps": process only maps that spatially overlap ("intersect")
  with the current computational region
- "is_contained": process only maps that are fully within the current
  computational region
- "contains": process only maps that contain (fully cover) the current
  computational region

Space time raster datasets may contain raster maps with varying spatial
extent like for example series of scenes of satellite images. For such
cases the *region_relation* option can be useful.

Moreover, there is [v.what.strds](v.what.strds.md), that uploads
space-time raster dataset values at positions of vector points, to the
attribute table of the vector map.

### Aggregation and accumulation analysis

The temporal framework supports the aggregation of space-time raster
datasets. It provides three modules to perform aggregation using
different approaches. To aggregate a space-time raster dataset using a
temporal granularity like 4 months, 7 days and so on, use
[t.rast.aggregate](t.rast.aggregate.md). The module
[t.rast.aggregate.ds](t.rast.aggregate.ds.md) allows aggregating a
space-time raster dataset using the time intervals of the maps of
another space-time dataset (raster, 3D raster and vector). A simple
interface to [r.series](r.series.md) is the module
[t.rast.series](t.rast.series.md) that processes the whole input
space-time raster dataset or a subset of it.

- [t.rast.aggregate](t.rast.aggregate.md)
- [t.rast.aggregate.ds](t.rast.aggregate.ds.md)
- [t.rast.series](t.rast.series.md)
- [t.rast.accumulate](t.rast.accumulate.md)
- [t.rast.accdetect](t.rast.accdetect.md)

### Export/import conversion

Space-time raster datasets can be exported with
[t.rast.export](t.rast.export.md) as a compressed tar archive. Such
archives can be then imported using [t.rast.import](t.rast.import.md).

The module [t.rast.to.rast3](t.rast.to.rast3.md) converts space-time
raster datasets into space-time voxel cubes. All 3D raster modules can
be used to process such voxel cubes. This conversion allows the export
of space-time raster datasets as netCDF files that include time as one
dimension.

- [t.rast.export](t.rast.export.md)
- [t.rast.import](t.rast.import.md)
- [t.rast.out.vtk](t.rast.out.vtk.md)
- [t.rast.to.rast3](t.rast.to.rast3.md)
- [r3.out.netcdf](r3.out.netcdf.md)

### Statistics and gap filling

- [t.rast.univar](t.rast.univar.md)
- [t.rast.gapfill](t.rast.gapfill.md)

## Modules to manage, process and analyze STR3DS and STVDS

Several space-time vector dataset modules were developed to allow the
handling of vector time series data.

- [t.vect.extract](t.vect.extract.md)
- [t.vect.import](t.vect.import.md)
- [t.vect.export](t.vect.export.md)
- [t.vect.observe.strds](t.vect.observe.strds.md)
- [t.vect.univar](t.vect.univar.md)
- [t.vect.what.strds](t.vect.what.strds.md)
- [t.vect.db.select](t.vect.db.select.md)

The space-time 3D raster dataset modules are doing exactly the same as
their raster pendants, but with 3D raster map layers:

- [t.rast3d.list](t.rast3d.list.md)
- [t.rast3d.extract](t.rast3d.extract.md)
- [t.rast3d.mapcalc](t.rast3d.mapcalc.md)
- [t.rast3d.univar](t.rast3d.univar.md)

### See also

- Gebbert, S., Pebesma, E. 2014. *TGRASS: A temporal GIS for field based
  environmental modeling*. Environmental Modelling & Software 53, 1-12
  ([DOI](https://doi.org/10.1016/j.envsoft.2013.11.001)) - [preprint
  PDF](http://ifgi.uni-muenster.de/~epebe_01/tgrass.pdf)
- Gebbert, S., Pebesma, E. 2017. *The GRASS GIS temporal framework*.
  International Journal of Geographical Information Science 31,
  1273-1292 ([DOI](https://doi.org/10.1080/13658816.2017.1306862))
- Gebbert, S., Leppelt, T., Pebesma, E., 2019. *A topology based
  spatio-temporal map algebra for big data analysis*. Data 4, 86.
  ([DOI](https://doi.org/10.3390/data4020086))
- [Temporal data
  processing](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)
  (Wiki)
- Vaclav Petras, Anna Petrasova, Helena Mitasova, Markus Neteler,
  **FOSS4G 2014 workshop**:  
  [Spatio-temporal data handling and visualization in GRASS
  GIS](http://fatra.cnr.ncsu.edu/temporal-grass-workshop/)
- [GEOSTAT 2012 GRASS
  Course](http://www.geostat-course.org/Topic_Gebbert)

<!-- -->

- [Introduction into raster data processing](rasterintro.md)
- [Introduction into 3D raster data (voxel)
  processing](raster3dintro.md)
- [Introduction into vector data processing](vectorintro.md)
- [Introduction into image processing](imageryintro.md)
- [Database management](databaseintro.md)
- [Projections and spatial transformations](projectionintro.md)
