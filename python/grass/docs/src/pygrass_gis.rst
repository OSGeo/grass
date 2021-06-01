.. _GRASSdatabase-label:

GRASS database management
=========================

PyGRASS implements the classes described bellow:

* :class:`~pygrass.gis.Gisdbase`
* :class:`~pygrass.gis.Location`
* :class:`~pygrass.gis.Mapset`
* :class:`~pygrass.gis.VisibleMapset`

These classes are used to manage the infrastructure of GRASS database:
GIS data directory, Location and Mapset. Details about the GRASS GIS
database management (locations and mapsets) can be found in the `GRASS
GIS 7 User's Manual: GRASS GIS Quickstart
<<<<<<< HEAD
<https://grass.osgeo.org/grass-devel/manuals/helptext.html>`_.
=======
<https://grass.osgeo.org/grass80/manuals/helptext.html>`_.
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610))

.. _Region-label:

Region management
=================

The :class:`~pygrass.gis.region.Region` class it is useful to obtain
information about the computational region and to change it. Details
about the GRASS GIS computational region management can be found in
the `GRASS GIS Wiki: Computational region
<https://grasswiki.osgeo.org/wiki/Computational_region>`_.

The classes are part of the :mod:`~pygrass.gis` module.
