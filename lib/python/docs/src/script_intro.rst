GRASS GIS Python scripting with script package
==============================================

Parts of the API
----------------

.. toctree::
   :maxdepth: 2

   script


Syntax
------

First, try a standard command in Command console in Layer Manager::

    r.info map=elevation -g

We are running *r.info* with an option ``map`` set to ``elevation`` Now,
switch to Python shell and type the same command but in Python syntax::

    grass.read_command('r.info', map='elevation', flags='g')

We used function ``read_command()`` from the ``grass.script`` package
which is imported under the name ``grass`` in the Python shell in GRASS
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

    import grass.script as gscript

    def main():
        input_raster = 'elevation'
        output_raster = 'high_areas'
        stats = gscript.parse_command('r.univar', map='elevation', flags='g')
        raster_mean = float(stats['mean'])
        raster_stddev = float(stats['stddev'])
        raster_high = raster_mean + raster_stddev
        gscript.mapcalc('{r} = {a} > {m}'.format(r=output_raster, a=input_raster,
                                                 m=raster_high))

    if __name__ == "__main__":
        main()


Processing many maps
--------------------

::

    import grass.script as gscript

    def main():
        rasters = ['lsat7_2002_10', 'lsat7_2002_20', 'lsat7_2002_30', 'lsat7_2002_40']
        max_min = None
        for raster in rasters:
            stats = gscript.parse_command('r.univar', map=raster, flags='g')
            if max_min is None or max_min < stats['min']:
                max_min = stats['min']
        print max_min

    if __name__ == "__main__":
        main()

Providing GRASS module interface to a script
---------------------------------------------


::

    #!/usr/bin/env python

    #%module
    #% description: Adds the values of two rasters (A + B)
    #% keywords: raster
    #% keywords: algebra
    #% keywords: sum
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

    import grass.script as gscript


    def main():
        options, flags = gscript.parser()
        araster = options['araster']
        braster = options['braster']
        output = options['output']

        gscript.mapcalc('{r} = {a} + {b}'.format(r=output, a=araster, b=braster))

        return 0


    if __name__ == "__main__":
        sys.exit(main())

Note that the previous code samples were missing some whitespace which
Python PEP8 style guide requires but this last sample fulfills all the
requirements. You should always use *pep8* tool to check your syntax and
style or set your editor to do it for you.
