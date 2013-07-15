#!/usr/bin/env python
"""
MODULE:    v.krige

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Performs ordinary or block kriging

DEPENDS:   R 2.x, packages gstat, maptools and spgrass6, optional: automap

COPYRIGHT: (C) 2009, 2012 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

## g.parser information

#%module
#% description: Performs ordinary or block kriging for vector maps.
#% keywords: vector
#% keywords: interpolation
#% keywords: raster
#% keywords: kriging
#%end

#%option G_OPT_V_INPUT
#% description: Name of point vector map containing sample data
#%end
#%option G_OPT_DB_COLUMN
#% description: Name of attribute column with numerical value to be interpolated
#% required: yes
#%end
#%option G_OPT_R_OUTPUT
#% label: Name for output raster map
#% description: If omitted, will be <input name>_kriging
#% required : no
#%end
#%option
#% key: package
#% type: string
#% options: gstat
#% answer: gstat
#% description: R package to use
#% required: no
#%end
#%option
#% key: model
#% type: string
#% options: Nug,Exp,Sph,Gau,Exc,Mat,Ste,Cir,Lin,Bes,Pen,Per,Hol,Log,Pow,Spl,Leg,Err,Int
#% multiple: yes
#% label: Variogram model(s)
#% description: Leave empty to test all models (requires automap)
#% required: no
#%end
#%option
#% key: block
#% type: integer
#% multiple: no
#% label: Block size (square block)
#% description: Block size. Used by block kriging.
#% required: no
#%end
#%option
#% key: range
#% type: integer
#% label: Range value
#% description: Automatically fixed if not set
#% required : no
#%end
#%option
#% key: nugget
#% type: integer
#% label: Nugget value
#% description: Automatically fixed if not set
#% required : no
#%end
#%option
#% key: sill
#% type: integer
#% label: Sill value
#% description: Automatically fixed if not set
#% required : no
#%end
#%option G_OPT_R_OUTPUT
#% key: output_var
#% label: Name for output variance raster map
#% description: If omitted, will be <input name>_kriging.var
#% required : no
#%end

import os, sys
from tempfile import gettempdir
import time
import thread

if __name__ == "__main__":
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython'))
from core import globalvar

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

GUIModulesPath = os.path.join(os.getenv("GISBASE"), "etc", "gui", "wxpython")
sys.path.append(GUIModulesPath)

GUIPath = os.path.join(os.getenv("GISBASE"), "etc", "gui", "wxpython", "scripts")
sys.path.append(GUIPath)

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

### dependencies to be checked once, as they are quite time-consuming. cfr. grass.parser.
# GRASS binding
try:
    import grass.script as grass
except ImportError:
    sys.exit(_("No GRASS-python library found"))
    
# move other checks in functions, as R?

# globals

#~ Command = None
#~ InputData = None
#~ Variogram = None
#~ VariogramFunction = None
#~ robjects = None
#~ rinterface = None

#classes in alphabetical order. methods in logical order :)

# <2.5 class definition, without () - please test 
class Controller:
    """ Executes analysis. For the moment, only with gstat functions."""
    # moved here the global variables
    def __init__(self):
        #~ self.Command = None
        self.InputData = None
        self.Variogram = None
        #~ VariogramFunction = None
        #~ robjects = None
        #~ rinterface = None
    
    def ImportMap(self, map, column):
        """ Imports GRASS map as SpatialPointsDataFrame and adds x/y columns to attribute table.
        Checks for NULL values in the provided column and exits if they are present."""

        #@NOTE: new way with R - as it doesn't alter original data
        Rpointmap = robjects.r.readVECT6(map, type =  'point')
        # checks if x,y columns are present in dataframe. If they do are present, but with different names,
        # they'll be duplicated.
        if "x" not in robjects.r.names(Rpointmap): 
            # extract coordinates with S4 method
            coordinatesPreDF = robjects.r['as.data.frame'](robjects.r.coordinates(Rpointmap))
            coordinatesDF = robjects.r['data.frame'](x = coordinatesPreDF.rx('coords.x1')[0],
                                                     y = coordinatesPreDF.rx('coords.x2')[0])
            # match coordinates with data slot of SpatialPointsDataFrame - maptools function
            # match is done on row.names
            Rpointmap = robjects.r.spCbind(Rpointmap, coordinatesDF)
            
        # GRASS checks for null values in the chosen column. R can hardly handle column as a variable,
        # looks for a hardcoded string.
        cols = grass.vector_columns(map=map, layer=1)
        nulls = int(grass.parse_command('v.univar',
                                        map=map,
                                        column=column,
                                        type='point',
                                        parse = (grass.parse_key_val,
                                                 {'sep':': '}
                                                 )
                                        )['number of NULL attributes'])
        if nulls > 0: 
            grass.fatal(_("%d NULL value(s) in the selected column - unable to perform kriging.") % nulls)
        return Rpointmap
    
    def CreateGrid(self, inputdata):
        Region = grass.region()
        Grid = robjects.r.gmeta2grd()

        # addition of coordinates columns into dataframe.
        coordinatesDF = robjects.r['as.data.frame'](robjects.r.coordinates(Grid))
        data = robjects.r['data.frame'](x = coordinatesDF.rx('s1')[0],
                                        y = coordinatesDF.rx('s2')[0],
                                        k = robjects.r.rep(1, Region['cols']*Region['rows']))
        GridPredicted = robjects.r.SpatialGridDataFrame(Grid,
                                                        data,
                                                        proj4string =  robjects.r.CRS(robjects.r.proj4string(inputdata)))
        return GridPredicted
    
    def ComposeFormula(self, column, isblock):
        if isblock is True:
            predictor = 'x+y'
        else:
            predictor = '1'
        print column + "~" + predictor
        Formula = robjects.Formula(column + "~" + predictor)
        return Formula
    
    def FitVariogram(self, formula, inputdata, sill, nugget, range, model = ''):
        """ Fits variogram either automagically either specifying all parameters.
        Returns a list containing data and model variograms. """
        
        Variograms = {}
        
        if model is '':
            robjects.r.require('automap')
            DottedParams = {}
            #print (nugget.r_repr(), sill, range)
            DottedParams['fix.values'] = robjects.r.c(nugget, range, sill)
            
            VariogramModel = robjects.r.autofitVariogram(formula, inputdata, **DottedParams)
            #print robjects.r.warnings()
            Variograms['datavariogram'] = VariogramModel.rx('exp_var')[0]
            Variograms['variogrammodel'] = VariogramModel.rx('var_model')[0]
            # obtain the model name. *Too* complicated to get the string instead of level, unlike R does.
            VariogramAsDF = robjects.r['as.data.frame'](VariogramModel.rx('var_model')[0]) # force conversion
            ModelDF = VariogramAsDF.rx('model')[0]
            Variograms['model'] = ModelDF.levels[ModelDF[1] - 1]
        else:
            DataVariogram = robjects.r['variogram'](formula, inputdata)
            VariogramModel = robjects.r['fit.variogram'](DataVariogram,
                                                         model = robjects.r.vgm(psill = sill,
                                                                                model = model,
                                                                                nugget = nugget,
                                                                                range = range))
            Variograms['datavariogram'] = DataVariogram
            Variograms['variogrammodel'] = VariogramModel
            Variograms['model'] = model
        return Variograms
    
    def DoKriging(self, formula, inputdata, grid, model, block):
        DottedParams = {'debug.level': -1} # let krige() print percentage status
        if block is not '': #@FIXME(anne): but it's a string!! and krige accepts it!!
            DottedParams['block'] = block
        #print DottedParams
        KrigingResult = robjects.r.krige(formula, inputdata, grid, model, **DottedParams)
        return KrigingResult
 
    def ExportMap(self, map, column, name, overwrite, command, variograms):
        # add kriging parameters to raster map history
        robjects.r.writeRAST6(map, vname = name, zcol = column, overwrite = overwrite)
        grass.run_command('r.support',
                          map = name,
                          title = 'Kriging output',
                          history = 'Issued from command v.krige ' + command)
        if command.find('model') is -1: # if the command has no model option, add automap chosen model
                    grass.run_command('r.support',
                                      map = name,
                                      history = 'Model chosen by automatic fitting: ' + variograms['model'])
        
    def Run(self, input, column, output, package, sill, nugget, range, logger, \
            overwrite, model, block, output_var, command, **kwargs):
        """ Wrapper for all functions above. """

        logger.message(_("Processing %d cells. Computing time raises "
                         "exponentially with resolution." % grass.region()['cells']))
        logger.message(_("Importing data..."))

        if self.InputData is None:
            self.InputData = self.ImportMap(input, column)
        # and from here over, InputData refers to the global variable
        #print(robjects.r.slot(InputData, 'data').names)
        logger.message(_("Data successfully imported."))
        
        GridPredicted = self.CreateGrid(self.InputData)
        
        logger.message(_("Fitting variogram..."))

        if block is not '':
            self.predictor = 'x+y'
        else:
            self.predictor = '1'
        if self.Variogram is None:
            self.Variogram = self.FitVariogram(robjects.Formula(column + "~" + self.predictor),
                                          self.InputData,
                                          model = model,
                                          sill = sill,
                                          nugget = nugget,
                                          range = range)
        logger.message(_("Variogram fitting complete."))
        
        logger.message(_("Kriging..."))
        KrigingResult = self.DoKriging(Formula, self.InputData,
                 GridPredicted, self.Variogram['variogrammodel'], block) # using global ones
        logger.message(_("Kriging complete."))
        
        self.ExportMap(map = KrigingResult,
                       column='var1.pred',
                       name = output,
                       overwrite = overwrite,
                       command = command,
                       variograms = self.Variogram)
        if output_var is not '':
            self.ExportMap(map = KrigingResult,
                           column='var1.var',
                           name = output_var,
                           overwrite = overwrite,
                           command = command,
                           variograms = self.Variogram)
        
def main(argv = None):
    """ Main. Calls either GUI or CLI, depending on arguments provided. """
    #@FIXME: solve this double ifelse. the control should not be done twice.
    
    controller = Controller()
    
    if argv is None:
        importR()
        argv = sys.argv[1:] # stripping first item, the full name of this script
        # wxGUI call
        if not os.getenv("GRASS_WXBUNDLED"):
            globalvar.CheckForWx()
        import vkrige as GUI
        
        import wx
        
        app = wx.App()
        KrigingFrame = GUI.KrigingModule(parent = None,
                                         Rinstance = robjects,
                                         controller = controller)
        KrigingFrame.Centre()
        KrigingFrame.Show()
        app.MainLoop()
        
    else:
        #CLI
        options, flags = argv
        
        #@TODO: Work on verbosity. Sometimes it's too verbose (R), sometimes not enough.
        if grass.find_file(options['input'], element = 'vector')['fullname'] is '':
            grass.fatal(_("option: <input>: Vector map not found.")) #TODO cosmetics, insert real map name
        
        #@TODO: elaborate input string, if contains mapset or not.. thanks again to Bob for testing on 64bit.
        
        # create output map name, if not specified
        if options['output'] is '':
            try: # to strip mapset name from fullname. Ugh.
                options['input'] = options['input'].split("@")[0]
            except:
                pass
            options['output'] = options['input'] + '_kriging'

        # check for output map with same name. g.parser can't handle this, afaik.
        if grass.find_file(options['output'], element = 'cell')['fullname'] \
           and os.getenv("GRASS_OVERWRITE") == None:
            grass.fatal(_("option: <output>: Raster map already exists."))

        if options['output_var'] is not '' \
           and (grass.find_file(options['output_var'], element = 'cell')['fullname'] \
           and os.getenv("GRASS_OVERWRITE") == None):
            grass.fatal(_("option: <output>: Variance raster map already exists."))

        importR()        
        if options['model'] is '':
            try:
                robjects.r.require("automap")
            except ImportError, e:
                grass.fatal(_("R package automap is missing, no variogram autofit available."))
        else:
            if options['sill'] is '' or options['nugget'] is '' or options['range'] is '':
                grass.fatal(_("You have specified model, but forgot at least one of sill, nugget and range."))
        
        #@TODO: let GRASS remount its commandstring. Until then, keep that 4 lines below.
        #print grass.write_command(argv)
        command = ""
        notnulloptions = {}
        for k, v in options.items():
            if v is not '':
                notnulloptions[k] = v
        command = command.join("%s=%s " % (k, v) for k, v in notnulloptions.items())
        
        # re-cast integers from strings, as parser() cast everything to string.
        for each in ("sill","nugget","range"):
            if options[each] is not '':
                options[each] = int(options[each])
            else:
                options[each] = robjects.r('''NA''')
        
        #controller = Controller()
        controller.Run(input = options['input'],
                       column = options['column'],
                       output = options['output'],
                       overwrite = os.getenv("GRASS_OVERWRITE") == 1,
                       package = options['package'],
                       model = options['model'],
                       block = options['block'],
                       sill = options['sill'],
                       nugget = options['nugget'],
                       range = options['range'],
                       output_var = options['output_var'],
                       command = command,
                       logger = grass)
    
def importR():
    # R
    # unuseful since rpy2 will complain adequately.
    # try:
    #    #@FIXME: in Windows, it launches R terminal
    #    grass.find_program('R')
    # except:
    #    sys.exit(_("R is not installed. Install it and re-run, or modify environment variables."))
    
    # rpy2
    global robjects
    global rinterface
    grass.message(_('Loading dependencies, please wait...'))
    try:
        import rpy2.robjects as robjects
        import rpy2.rinterface as rinterface #to speed up kriging? for plots.
    except ImportError:
        # ok for other OSes?
        grass.fatal(_("Python module 'Rpy2' not found. Please install it and re-run v.krige."))
        
    # R packages check. Will create one error message after check of all packages.
    missingPackagesList = []
    for each in ["rgeos", "gstat", "spgrass6", "maptools"]:
        if not robjects.r.require(each, quietly = True)[0]:
            missingPackagesList.append(each)
    if missingPackagesList:
        errorString = _("R package(s) ") + \
                      ", ".join(map(str, missingPackagesList)) + \
                      _(" missing. Install it/them and re-run v.krige.")
        grass.fatal(errorString)
    
if __name__ == '__main__':
    if len(sys.argv) > 1:
        sys.exit(main(argv = grass.parser()))
    else:
        main()
