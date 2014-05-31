.. PyGrass documentation master file, created by
   sphinx-quickstart2 on Sat Jun 16 18:53:32 2012.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

PyGRASS documentation
===================================

Python is a programming language which is more powerful than shell scripting but easier and more forgiving than C. ``PyGRASS`` is an object-oriented Python Application Programming Interface (API) for GRASS GIS. ``PyGRASS`` offers interfaces to GRASS modules and functionality, as well as to vector and raster data. For details, see Zambelli et al. (2013) in the references below. ``PyGRASS`` improves the integration between GRASS GIS and Python, making the use of Python under GRASS more consistent with the language itself. Furthermore, it simplifies GRASS scripting and programming and more natural for the user.

Background: In 2006, GRASS GIS developers started to adopt Python for the new GUI. Due to this Python became more and more important and developers converted all shell scripts from GRASS GIS 6 to Python for GRASS GIS 7.

To work with ``PyGRASS`` you need an up-to-date version of GRASS GIS 7. The only action before starting to work with ``PyGRASS`` is to launch GRASS GIS 7 and from the console launch ``python`` or ``ipython`` (the second one is the recommended way).

Contents:

.. toctree::
   :maxdepth: 2

   intro
   gis
   raster
   raster_elements
   vector
   vector_attributes
   vector_features
   vector_utils
   modules
   modules_grid
   messages


References
============

* Zambelli P, Gebbert S, Ciolli M., 2013. *Pygrass: An Object Oriented Python Application Programming Interface (API) for Geographic Resources Analysis Support System (GRASS) Geographic Information System (GIS)*. ISPRS International Journal of Geo-Information. 2(1):201-219. `doi:10.3390/ijgi2010201 <http://dx.doi.org/10.3390/ijgi2010201>`_
* `Python related articles in the GRASS GIS Wiki <http://grasswiki.osgeo.org/wiki/Category:Python>`_

This project has been funded with support from the `Google Summer of Code 2012 <http://trac.osgeo.org/grass/wiki/GSoC#PythonhighlevelmapinteractionforGRASSGIS>`_


..
    Indices and tables
    ==================

    * :ref:`genindex`
    * :ref:`modindex`
    * :ref:`search`

