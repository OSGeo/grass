.. _GRASSdatabase-label:

GRASS database management
=========================

PyGRASS implements the classes described below:

* :class:`~grass.pygrass.gis.Gisdbase`
* :class:`~grass.pygrass.gis.Location`
* :class:`~grass.pygrass.gis.Mapset`
* :class:`~grass.pygrass.gis.VisibleMapset`

These classes are used to manage the infrastructure of GRASS database:
GIS data directory, Location and Mapset. Details about the GRASS
database management (locations and mapsets) can be found in the `GRASS
User's Manual: GRASS Quickstart
<https://grass.osgeo.org/grass-devel/manuals/helptext.html>`_.

.. _Region-label:

Region management
=================

The :class:`~grass.pygrass.gis.region.Region` class it is useful to obtain
information about the computational region and to change it. Details
about the GRASS computational region management can be found in
the `GRASS Wiki: Computational region
<https://grasswiki.osgeo.org/wiki/Computational_region>`_.

The classes are part of the :mod:`~grass.pygrass.gis` module.
