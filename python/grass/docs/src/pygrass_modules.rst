Interface to GRASS GIS modules
==============================

In :mod:`~pygrass.modules` module, GRASS GIS modules are represented
by :class:`~pygrass.modules.interface.module.Module` class
objects. These objects are generated based on the XML module
description that is used also for the generation of the graphical user
interface (GUI). ::

    >>> from grass.pygrass.modules import Module
    >>> slope_aspect = Module("r.slope.aspect", elevation='elevation',
    ...                        slope='slp',  aspect='asp',
    ...                        format='percent', overwrite=True)


It is possible to create a run-able module object and run it later (this
is also needed for registering GRASS GIS commands with more than one dot
in their name, e.g. r.slope.aspect):

    >>> slope_aspect = Module("r.slope.aspect", elevation='elevation',
    ...                        slope='slp',  aspect='asp',
    ...                        format='percent', overwrite=True, run_=False)

Then we can run the module with: ::

    >>> slope_aspect()

or using the run method: ::

   >>> slope_aspect.run()


It is possible to initialize a module, and give the parameters later: ::

    >>> slope_aspect = Module("r.slope.aspect")
    >>> slope_aspect(elevation='elevation', slope='slp',  aspect='asp',
    ...              format='percent', overwrite=True)


Create the module object input step by step and run later: ::

    >>> slope_aspect = Module("r.slope.aspect")
    >>> slope_aspect.inputs['elevation']
    Parameter <elevation> (required:yes, type:raster, multiple:no)
    >>> slope_aspect.inputs["elevation"].value = "elevation"
    >>> slope_aspect.inputs["format"]
    Parameter <format> (required:no, type:string, multiple:no)
    >>> print slope_aspect.inputs["format"].__doc__
    format: 'degrees', optional, string
        Format for reporting the slope
        Values: 'degrees', 'percent'
    >>> slope_aspect.inputs["format"].value = 'percents'
    Traceback (most recent call last):
        ...
    ValueError: The Parameter <format>, must be one of: ['degrees', 'percent']
    >>> slope_aspect.inputs["format"].value = 'percent'
    >>> slope_aspect.flags = "g"
    Traceback (most recent call last):
        ...
    ValueError: Flag not valid, valid flag are: ['a']
    >>> slope_aspect.flags = "a"
    >>> slope_aspect.flags_dict['overwrite']
    Flag <overwrite> (Allow output files to overwrite existing files)
    >>> slope_aspect.flags_dict['overwrite'].value = True
    >>> slope_aspect()



It is possible to access the module descriptions (name, one line description,
keywords, label) with:

    >>> slope_aspect.name
    'r.slope.aspect'
    >>> slope_aspect.description
    'Aspect is calculated counterclockwise from east.'
    >>> slope_aspect.keywords
    'raster, terrain'
    >>> slope_aspect.label
    'Generates raster maps of slope, aspect, curvatures and partial derivatives from a elevation raster map.'

and get the module documentation with: ::

    >>> print slope_aspect.__doc__
    r.slope.aspect(elevation=elevation, slope=None, aspect=None
                   format=percent, prec=None, pcurv=None
                   tcurv=None, dx=None, dy=None
                   dxx=None, dyy=None, dxy=None
                   zfactor=None, min_slp_allowed=None)
    <BLANKLINE>
    Parameters
    ----------
    <BLANKLINE>
    <BLANKLINE>
    elevation: required, string
        Name of input elevation raster map
    slope: optional, string
        Name for output slope raster map
    aspect: optional, string
        Name for output aspect raster map
    format: 'degrees', optional, string
        Format for reporting the slope
        Values: 'degrees', 'percent'
    prec: 'float', optional, string
        Type of output aspect and slope maps
        Values: 'default', 'double', 'float', 'int'
    pcurv: optional, string
        Name for output profile curvature raster map
    tcurv: optional, string
        Name for output tangential curvature raster map
    dx: optional, string
        Name for output first order partial derivative dx (E-W slope) raster map
    dy: optional, string
        Name for output first order partial derivative dy (N-S slope) raster map
    dxx: optional, string
        Name for output second order partial derivative dxx raster map
    dyy: optional, string
        Name for output second order partial derivative dyy raster map
    dxy: optional, string
        Name for output second order partial derivative dxy raster map
    zfactor: 1.0, optional, float
        Multiplicative factor to convert elevation units to meters
    min_slp_allowed: optional, float
        Minimum slope val. (in percent) for which aspect is computed
    <BLANKLINE>
    Flags
    ------
    <BLANKLINE>
    a: None
        Do not align the current region to the elevation layer
    overwrite: None
        Allow output files to overwrite existing files
    verbose: None
        Verbose module output
    quiet: None
        Quiet module output



For each input and output parameter it is possible to obtain specific
information. To see all module inputs, just type: ::

    >>> slope_aspect.inputs #doctest: +NORMALIZE_WHITESPACE
    TypeDict([('elevation', Parameter <elevation> (required:yes, type:raster, multiple:no)), ('format', Parameter <format> ...)])

To get information for each parameter: ::

    >>> slope_aspect.inputs["elevation"].description
    'Name of input elevation raster map'
    >>> slope_aspect.inputs["elevation"].type
    'raster'
    >>> slope_aspect.inputs["elevation"].typedesc
    'string'
    >>> slope_aspect.inputs["elevation"].multiple
    False
    >>> slope_aspect.inputs["elevation"].required
    True

Or get a small documentation for each parameter with:

    >>> print slope_aspect.inputs["elevation"].__doc__
    elevation: required, string
        Name of input elevation raster map


User or developer can check which parameters have been set, with: ::

    if slope_aspect.outputs['aspect'].value == None:
        print "Aspect is not computed"


After we set the parameters and run the module, the execution of the module
instantiate a popen attribute to the class. The `Popen`_ class allow user
to kill/wait/ the process. ::

    >>> slope_aspect = Module('r.slope.aspect')
    >>> slope_aspect(elevation='elevation', slope='slp', aspect='asp', overwrite=True, finish_=False)
    >>> slope_aspect.popen.wait() # *.kill(), *.terminate()
    0
    >>> out, err = slope_aspect.popen.communicate()
    >>> print err #doctest: +NORMALIZE_WHITESPACE
     100%
    Aspect raster map <asp> complete
    Slope raster map <slp> complete
    <BLANKLINE>

On the above example we use a new parameter `finish_`, if is set to True, the
run method, automatically store the stdout and stderr to stdout and stderr
attributes of the class: ::

    >>> slope_aspect = Module('r.slope.aspect')
    >>> slope_aspect(elevation='elevation', slope='slp', aspect='asp', overwrite=True, finish_=True)
    >>> print slope_aspect.stderr #doctest: +NORMALIZE_WHITESPACE
     100%
    Aspect raster map <asp> complete
    Slope raster map <slp> complete
    <BLANKLINE>

Another example of use: ::

    >>> from subprocess import PIPE
    >>> info = Module("r.info", map="elevation", flags="r", stdout_=PIPE)
    >>> from grass.script.utils import parse_key_val
    >>> parse_key_val(info.outputs.stdout)
    {'max': '156.3299', 'min': '55.57879'}
    >>> info = Module("r.info", map="elevation", flags="r", finish_=False)
    >>> category = Module("r.category", map="elevation",
    ...                   stdin_=info.popen.stdout, finish_=True)

Launching GRASS GIS modules in parallel
---------------------------------------

PyGRASS implements simple mechanism for launching GRASS modules in
parallel. See
:class:`~pygrass.modules.interface.module.ParallelModuleQueue` class
for details.

Multiple GRASS modules can be joined into one object by
:class:`~pygrass.modules.interface.module.MultiModule`.

    
.. _Popen: http://docs.python.org/library/subprocess.html#Popen
