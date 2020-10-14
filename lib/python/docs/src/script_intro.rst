GRASS GIS Python scripting with grass.script package
====================================================

Parts of the API
----------------

.. toctree::
   :maxdepth: 2

   script


Syntax
------

First, try a standard command in Console tab in Layer Manager in GRASS GUI::

    r.info map=elevation -g

We are running *r.info* with an option ``map`` set to ``elevation`` Now,
switch to Python tab and type the same command but in Python syntax::

    gs.read_command('r.info', map='elevation', flags='g')

We used function ``read_command()`` from the ``grass.script`` package
which is imported under the name ``gs`` in the Python tab in GRASS
GUI. There are also other functions besides ``read_command()`` most
notably ``run_command()``, ``write_command()`` and ``parse_command()``.
The first parameter for functions from this group is the name of the
GRASS module as string. Other parameters are options of the module.
Python keyword arguments syntax is used for the options. Flags can be
passed in a parameter ``flags`` where value of the parameter is a string
containing all the flags we want to set. The general syntax is the
following::

    function_name('module.name', option1=value1, option2=..., flags='flagletters')

The function parameters are the same as module options, so you can just
use standard module manual page to learn about the interface.

Most of the GRASS functionality is available through modules and all of
them can be called using the functions above. However, in some cases, it
is more advantageous to use specialized Python functions. This is the
case for ``mapcalc()`` function (wrapper for *r.mapcalc* module) and
``list_strings()`` function (wrapper for *g.list* module).


Combining multiple modules
--------------------------

To launch a Python script from GUI, use File -> Launch Python script.

::

    import grass.script as gs

    def main():
        input_raster = 'elevation'
        output_raster = 'high_areas'
        stats = gs.parse_command('r.univar', map='elevation', flags='g')
        raster_mean = float(stats['mean'])
        raster_stddev = float(stats['stddev'])
        raster_high = raster_mean + raster_stddev
        gs.mapcalc('{r} = {a} > {m}'.format(r=output_raster, a=input_raster,
                                            m=raster_high))

    if __name__ == "__main__":
        main()


Processing many maps
--------------------

::

    import grass.script as gs

    def main():
        rasters = ['lsat7_2002_10', 'lsat7_2002_20', 'lsat7_2002_30', 'lsat7_2002_40']
        max_min = None
        for raster in rasters:
            stats = gs.parse_command('r.univar', map=raster, flags='g')
            if max_min is None or max_min < stats['min']:
                max_min = stats['min']
        print(max_min)

    if __name__ == "__main__":
        main()

Providing GRASS module interface to a script
---------------------------------------------


::

    #!/usr/bin/env python3

    #%module
    #% description: Adds the values of two rasters (A + B)
    #% keyword: raster
    #% keyword: algebra
    #% keyword: sum
    #%end
    #%option G_OPT_R_INPUT
    #% key: araster
    #% description: Name of input raster A in an expression A + B
    #%end
    #%option G_OPT_R_INPUT
    #% key: braster
    #% description: Name of input raster B in an expression A + B
    #%end
    #%option G_OPT_R_OUTPUT
    #%end


    import sys

    import grass.script as gs


    def main():
        options, flags = gs.parser()
        araster = options['araster']
        braster = options['braster']
        output = options['output']

        gs.mapcalc('{r} = {a} + {b}'.format(r=output, a=araster, b=braster))

        return 0


    if __name__ == "__main__":
        sys.exit(main())

The options which has something like ``G_OPT_R_INPUT`` after the word
``option`` are called standard options. Their list is accessible
in GRASS GIS `C API documentation`_ of ``STD_OPT`` enum from ``gis.h`` file.
Always use standard options if possible. They are not only easier to use
but also ensure consistency across the modules and easier maintanenace
in case of updates to the parameters parsing system.
Typically, you change ``description`` (and/or ``label``), sometimes ``key``
and ``answer``. There are also standard flags to be used
with ``flag`` which work in the same way.

The examples of syntax of options and flags (without the ``G_OPT...`` part)
can be obtained from any GRASS module using special ``--script`` flag.
Alternatively, you can use GRASS source code to look how different scripts
actually define and use their parameters.

Note that the previous code samples were missing some whitespace which
Python PEP8 style guide requires but this last sample fulfills all the
requirements. You should always use *pep8* tool to check your syntax and
style or set your editor to do it for you. Note also that although
a some mistakes in Python code can be discovered only when executing
the code due to the dynamic nature of Python, there is a large number
of tools such as *pep8* or *pylint* which can help you to identify problems
in you Python code.

.. _C API documentation: https://grass.osgeo.org/programming7/gis_8h.html
