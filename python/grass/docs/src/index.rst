Getting Started
==========================================

Python, a widely used general-purpose, high-level programming language
provides a powerful scripting interface. Being easy-to-use yet
powerful, it enables users to efficiently exploit the capabilities
of the `GRASS` software. Python scripts for GRASS can be written at
high level (GRASS modules) as well as at low level (GRASS
libraries) through a dedicated interface. The graphical user interface
and the GRASS Temporal Framework are entirely written in Python.

A set of packages is provided to the user in order to provide functionality
at various levels:

* `grass.tools package <grass.tools.html>`_ provides Python interface to launch GRASS tools in scripts
* `grass.script package <script_intro.html>`_ provides additional tooling to use GRASS in scripts
* `PyGRASS <pygrass_index.html>`_ is an object-oriented Python Application
  Programming Interface (API) for GRASS which uses the GRASS C API as
  backend but additionally offers a convenient interface to the GRASS
  modules
* `GRASS Temporal Framework <temporal_framework.html>`_ implements the temporal GIS functionality
  of GRASS and provides an API to implement spatio-temporal processing modules
* `grass.jupyter package <grass.jupyter.html>`_ offers classes and setup functions for
  running GRASS in Jupyter Notebooks
* `Testing GRASS source code and modules <gunittest_testing.html>`_ using gunittest package
* `exceptions package <exceptions.html>`_ contains exceptions used by other packages
* `imaging package <imaging.html>`_ is a library to create animated images and films
* `pydispatch package <pydispatch.html>`_ is a library for signal-dispatching

--------------------
Additional Resources
--------------------
* `GRASS Python introduction <manuals/python_intro.html>`_ provides a general overview of
  the Python interface to GRASS.
* `GRASS Jupyter notebooks introduction <manuals/jupyter_intro.html>`_ provides an overview of how to use the **grass.jupyter** module.

.. _GRASS: https://grass.osgeo.org/

--------------------
Modules and Packages
--------------------
.. toctree::
   :maxdepth: 1

   index
   grass.tools
   script_intro
   pygrass_index
   grass.jupyter
   exceptions
   imaging
   pydispatch
   gunittest_testing
