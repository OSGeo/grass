Introduction
=============

To work with ``pygrass`` you need a up-to-date version of GRASS 7.
You can obtain a recent version following the information on the
`main web site <http://grass.osgeo.org/download/software.php#g70x>`_
of GRASS, and you can read more about compilation on the 
`GRASS wiki <http://grass.osgeo.org/wiki/Compile_and_Install>`_

Now you can download the ``pygrass`` source using ``git``

::

  git clone https://lucadeluge@code.google.com/p/pygrass/ 

If you have not ``git`` you can install it from the `git website <http://git-scm.com/downloads>`_
or download the source code of ``pygrass`` from `<http://code.google.com/p/pygrass/downloads/list>`_

At this point you have to install ``pygrass``, so enter in the right 
folder and launch with administration permission

::

  python setup.py install
  
The last action before start to work with ``pygrass`` is to run 
GRASS 7 and from the console launch ``python`` or ``ipython`` 
(the second one is the the suggested)

Read more about how to work with :doc:`raster`, :doc:`vector`, :doc:`modules`.