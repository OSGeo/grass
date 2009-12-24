#!/usr/bin/env python

"""
MODULE:    v.krige

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Performs ordinary or block kriging

DEPENDS:   R 2.x, packages gstat and spgrass6, optional: automap

COPYRIGHT: (C) 2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

## g.parser informations

#%module
#% description: Performs ordinary or block kriging.
#% keywords: vector, interpolation, kriging
#%end

#%option
#% key: input
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of point vector map containing sample data
#% required: yes
#%end
#%option
#% key: column
#% type: string
#% gisprompt: old,dbcolumn,dbcolumn
#% description: Name of attribute column with numerical value to be interpolated
#% required : yes
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new,cell,raster
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
#% options: Exp,Sph,Gau,Mat,Lin
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
#%option
#% key: output_var
#% type: string
#% gisprompt: new,cell,raster
#% label: Name for output variance raster map
#% description: If omitted, will be <input name>_kriging_var
#% required : no
#%end

import os, sys
from tempfile import gettempdir
import time
import thread

GUIModulesPath = os.path.join(os.getenv("GISBASE"), "etc", "wxpython", "gui_modules")
sys.path.append(GUIModulesPath)
GUIPath = os.path.join(os.getenv("GISBASE"), "etc", "wxpython")
sys.path.append(GUIPath)

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import gselect
import goutput
import menuform
from preferences import globalSettings as UserSettings
#import help

import wx
import wx.lib.flatnotebook as FN
import wx.lib.plot as plot # for plotting the variogram.

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

### dependencies to be checked once, as they are quite time-consuming. cfr. grass.parser.
# GRASS binding
try:
    import grass.script as grass
except ImportError:
    sys.exit(_("No GRASS-python library found."))
    
# move other checks in functions, as R?

# globals
maxint = 1e6 # instead of sys.maxint, not working with SpinCtrl on 64bit [reported by Bob Moskovitz]
Region = grass.region()
Command = None
InputData = None
Variogram = None
VariogramFunction = None
robjects = None
rinterface = None

#classes in alphabetical order. methods in logical order :)

class Controller():
    """ Executes analysis. For the moment, only with gstat functions."""
    
    def ImportMap(self, map, column):
        """ Adds x,y columns to the GRASS map and then imports it in R.
        Checks for NULL values in the provided column and exits if they are present."""
        #@NOTE: it alters original data. Is it correct? Shall I remove those columns
        # if they were absent from original data?
        cols = grass.vector_columns(map = map, layer = 1)
        if not cols.has_key('x') and not cols.has_key('y'):
            grass.run_command('v.db.addcolumn', map = map,
                              columns = 'x double precision, y double precision')
            grass.run_command('v.to.db', map = map, option = 'coor', col = 'x,y')
        nulls = int(grass.parse_command('v.univar', map = map, column = column, type = 'point', \
                                            parse = (grass.parse_key_val, \
                                     {'sep':': '}))['number of NULL attributes'])
        if nulls > 0: 
            grass.fatal(_("%d NULL value(s) in the selected column - unable to perform kriging.") % nulls)
        return robjects.r.readVECT6(map, type =  'point')
    
    def CreateGrid(self, inputdata):
        Grid = robjects.r.gmeta2grd()

        # addition of coordinates columns into dataframe.
        coordinatesDF = robjects.r['as.data.frame'](robjects.r.coordinates(Grid))
        data = robjects.r['data.frame'](x = coordinatesDF.r['s1'][0],
                                        y = coordinatesDF.r['s2'][0],
                                        k = robjects.r.rep(1, Region['cols']*Region['rows']))
        GridPredicted = robjects.r.SpatialGridDataFrame(Grid,
                                                        data,
                                                        proj4string =  robjects.r.CRS(robjects.r.proj4string(inputdata)))
        return GridPredicted
    
    def ComposeFormula(self, column, isblock, inputdata):
        if isblock is True:
            predictor = 'x+y'
        else:
            predictor = 1
        Formula = robjects.r['as.formula'](robjects.r.paste(column, "~", predictor))
        #print Formula
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
            Variograms['datavariogram'] = VariogramModel.r['exp_var'][0]
            Variograms['variogrammodel'] = VariogramModel.r['var_model'][0]
            # obtain the model name. *Too* complicated to get the string instead of level, unlike R does.
            VariogramAsDF = robjects.r['as.data.frame'](VariogramModel.r['var_model'][0]) # force conversion
            ModelDF = VariogramAsDF.r['model'][0]
            Variograms['model'] = robjects.r.levels(ModelDF).subset(ModelDF[1])[0]
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
        logger.message(_("Processing %d cells. Computing time raises exponentially with resolution." % Region['cells']))
        logger.message(_("Importing data..."))
        if globals()["InputData"] is None:
            globals()["InputData"] = self.ImportMap(input, column)
        # and from here over, InputData refers to the global variable
        #print(robjects.r.slot(InputData, 'data').names)
        logger.message(_("Data successfully imported."))
        
        GridPredicted = self.CreateGrid(InputData)
        
        logger.message(_("Fitting variogram..."))
        isblock = block is not ''
        Formula = self.ComposeFormula(column, isblock, InputData)
        if globals()["Variogram"] is None:
            globals()["Variogram"] = self.FitVariogram(Formula,
                                          InputData,
                                          model = model,
                                          sill = sill,
                                          nugget = nugget,
                                          range = range)
        logger.message(_("Variogram fitted."))
        
        logger.message(_("Kriging..."))
        KrigingResult = self.DoKriging(Formula, InputData, GridPredicted, Variogram['variogrammodel'], block) # using global ones
        logger.message(_("Kriging performed."))
        
        self.ExportMap(map = KrigingResult,
                       column='var1.pred',
                       name = output,
                       overwrite = overwrite,
                       command = command,
                       variograms = Variogram)
        if output_var is not '':
            self.ExportMap(map = KrigingResult,
                           column='var1.var',
                           name = output_var,
                           overwrite = overwrite,
                           command = command,
                           variograms = Variogram)
        
class KrigingPanel(wx.Panel):
    """ Main panel. Contains all widgets except Menus and Statusbar. """
    def __init__(self, parent, *args, **kwargs):
        wx.Panel.__init__(self, parent, *args, **kwargs)
        
        self.parent = parent 
        self.border = 4
        
        #    1. Input data 
        InputBoxSizer = wx.StaticBoxSizer(wx.StaticBox(self, id = wx.ID_ANY, label = _("Input Data")), 
                                          orient = wx.HORIZONTAL)
        
        flexSizer = wx.FlexGridSizer(cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(1)

        flexSizer.Add(item = wx.StaticText(self, id = wx.ID_ANY, label = _("Point dataset:")),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        self.InputDataMap = gselect.VectorSelect(parent = self,
                                                 ftype = 'points',
                                                 updateOnPopup = False)
        self.InputDataMap.SetFocus()
        flexSizer.Add(item = self.InputDataMap, flag = wx.ALIGN_CENTER_VERTICAL)
        
        RefreshButton = wx.Button(self, id = wx.ID_REFRESH)
        RefreshButton.Bind(wx.EVT_BUTTON, self.OnButtonRefresh)
        flexSizer.Add(item = RefreshButton, flag = wx.ALIGN_CENTER_VERTICAL)
        
        flexSizer.Add(item = wx.StaticText(self, id = wx.ID_ANY, label = _("Numeric column:")),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        self.InputDataColumn = gselect.ColumnSelect(self, id = wx.ID_ANY)
        self.InputDataColumn.SetSelection(0)
        flexSizer.Add(item = self.InputDataColumn)
        
        self.InputDataMap.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnInputDataChanged)
        
        InputBoxSizer.Add(item = flexSizer)
        
        #    2. Kriging. In book pages one for each R package. Includes variogram fit.
        KrigingSizer = wx.StaticBoxSizer(wx.StaticBox(self, id = wx.ID_ANY, label = _("Kriging")), wx.HORIZONTAL)

        self.RPackagesBook = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                        style = FN.FNB_BOTTOM |
                                        FN.FNB_NO_NAV_BUTTONS |
                                        FN.FNB_FANCY_TABS | FN.FNB_NO_X_BUTTON)
        
        for Rpackage in ["gstat"]: # , "geoR"]: #@TODO: enable it if/when it'll be implemented.
            self.CreatePage(package = Rpackage)
        
        ## Command output. From menuform module, cmdPanel class
        self.goutput = goutput.GMConsole(parent = self, margin = False,
                                         pageid = self.RPackagesBook.GetPageCount(),
                                         notebook = self.RPackagesBook)
        self.goutputId = self.RPackagesBook.GetPageCount()
        self.outpage = self.RPackagesBook.AddPage(self.goutput, text = _("Command output"))
        
        self.RPackagesBook.SetSelection(0)
        KrigingSizer.Add(self.RPackagesBook, proportion = 1, flag = wx.EXPAND)
        
        #    3. Output Parameters.
        OutputSizer = wx.StaticBoxSizer(wx.StaticBox(self, id = wx.ID_ANY, label = _("Output")), wx.HORIZONTAL)
        
        OutputParameters = wx.GridBagSizer(hgap = 5, vgap = 5)
        OutputParameters.AddGrowableCol(1)
        OutputParameters.Add(item = wx.StaticText(self, id = wx.ID_ANY, label = _("Name for the output raster map:")),
                             flag = wx.ALIGN_CENTER_VERTICAL,
                             pos = (0, 0))
        self.OutputMapName = gselect.Select(parent = self, id = wx.ID_ANY,
                                            type = 'raster',
                                            mapsets = [grass.gisenv()['MAPSET']])
        OutputParameters.Add(item = self.OutputMapName, flag = wx.EXPAND | wx.ALL,
                             pos = (0, 1))
        self.VarianceRasterCheckbox = wx.CheckBox(self, id = wx.ID_ANY, label = _("Export variance map as well: "))
        self.VarianceRasterCheckbox.SetValue(state = True)
        OutputParameters.Add(item = self.VarianceRasterCheckbox,
                             flag = wx.ALIGN_CENTER_VERTICAL,
                             pos = (1, 0))
        self.OutputVarianceMapName = gselect.Select(parent = self, id = wx.ID_ANY,
                                            type = 'raster',
                                            mapsets = [grass.gisenv()['MAPSET']])
        self.VarianceRasterCheckbox.Bind(wx.EVT_CHECKBOX, self.OnVarianceCBChecked)
        OutputParameters.Add(item = self.OutputVarianceMapName, flag = wx.EXPAND | wx.ALL,
                             pos = (1, 1))
        
        self.OverwriteCheckBox = wx.CheckBox(self, id = wx.ID_ANY,
                                             label = _("Allow output files to overwrite existing files"))
        self.OverwriteCheckBox.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        OutputParameters.Add(item = self.OverwriteCheckBox,
                             pos = (2, 0), span = (1, 2))
        
        OutputSizer.Add(OutputParameters, proportion = 0, flag = wx.EXPAND | wx.ALL, border = self.border)
        
        #    4. Run Button and Quit Button
        ButtonSizer = wx.BoxSizer(wx.HORIZONTAL)
        HelpButton = wx.Button(self, id = wx.ID_HELP)
        HelpButton.Bind(wx.EVT_BUTTON, self.OnHelpButton)
        QuitButton = wx.Button(self, id = wx.ID_EXIT)
        QuitButton.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.RunButton = wx.Button(self, id = wx.ID_ANY, label = _("&Run")) # no stock ID for Run button.. 
        self.RunButton.Bind(wx.EVT_BUTTON, self.OnRunButton)
        self.RunButton.Enable(False) # disable it on loading the interface, as input map is not set
        ButtonSizer.Add(HelpButton, proportion = 0, flag = wx.ALIGN_LEFT | wx.ALL, border = self.border)
        ButtonSizer.Add(QuitButton, proportion = 0, flag = wx.ALIGN_RIGHT | wx.ALL, border = self.border)
        ButtonSizer.Add(self.RunButton, proportion = 0, flag = wx.ALIGN_RIGHT | wx.ALL, border = self.border)
        
        #    Main Sizer. Add each child sizer as soon as it is ready.
        Sizer = wx.BoxSizer(wx.VERTICAL)
        Sizer.Add(InputBoxSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = self.border)
        Sizer.Add(KrigingSizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = self.border)
        Sizer.Add(OutputSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = self.border)
        Sizer.Add(ButtonSizer, proportion = 0, flag = wx.ALIGN_RIGHT | wx.ALL, border = self.border)
        self.SetSizerAndFit(Sizer)
        
        # last action of __init__: update imput data list.
        # it's performed in the few seconds gap while user examines interface before clicking anything.
        #@TODO: implement a splashcreen IF the maps cause a noticeable lag [markus' suggestion]
        self.InputDataMap.GetElementList()
        
    def CreatePage(self, package):
        """ Creates the three notebook pages, one for each R package """
        for package in ["gstat"]: 
            classobj = eval("RBook"+package+"Panel")
            setattr(self, "RBook"+package+"Panel", (classobj(self, id = wx.ID_ANY)))
            self.RPackagesBook.AddPage(page = getattr(self, "RBook"+package+"Panel"), text = package)

    def OnButtonRefresh(self, event):
        """ Forces refresh of list of available layers. """
        self.InputDataMap.GetElementList()

    def OnCloseWindow(self, event):
        """ Cancel button pressed"""
        self.parent.Close()
        event.Skip()

    def OnHelpButton(self, event):
        # file = os.path.join(os.getenv("GISBASE"), "docs", "html", "v.krige.html")
        # file = os.path.join(os.path.curdir, "description.html")
        # @TODO fix HelpWindow
        # helpFrame = help.HelpWindow(parent=self, id=wx.ID_ANY,
        #                            title=_("GRASS - Help page for v.krige"),
        #                            size=(640, 480),
        #                            file=file)
        # helpFrame.Show(True)

        grass.run_command('g.manual', entry = 'v.krige')
        
        event.Skip()

    def OnInputDataChanged(self, event):
        """ Refreshes list of columns and fills output map name TextCtrl """
        MapName = event.GetString()
        self.InputDataColumn.InsertColumns(vector = MapName,
                                   layer = 1, excludeKey = True,
                                   type = ['integer', 'double precision'])
        self.InputDataColumn.SetSelection(0)
        self.RunButton.Enable(self.InputDataColumn.GetSelection() is not -1)
        self.RBookgstatPanel.PlotButton.Enable(self.InputDataColumn.GetSelection() is not -1)
        
        if self.InputDataColumn.GetSelection() is not -1:
            self.OutputMapName.SetValue(MapName.split("@")[0]+"_kriging")
            self.OutputVarianceMapName.SetValue(MapName.split("@")[0]+"_kriging_var")
        else:
            self.OutputMapName.SetValue('')
            self.OutputVarianceMapName.SetValue('')
        
    def OnRunButton(self,event):
        """ Execute R analysis. """
        #@FIXME: send data to main method instead of running it here.
        
        #-1: get the selected notebook page. The user shall know that [s]he can modify settings in all
        # pages, but only the selected one will be executed when Run is pressed.
        SelectedPanel = self.RPackagesBook.GetCurrentPage()
        
        if self.RPackagesBook.GetPageText(self.RPackagesBook.GetSelection()) == 'Command output':
            self.goutput.WriteError("No parameters for running. Please select \"gstat\" tab, check parameters and re-run.")
            return False # no break invoked by above function
        
        # mount command string as it would have been written on CLI
        command = ["v.krige", "input=" + self.InputDataMap.GetValue(),
                   "column=" + self.InputDataColumn.GetValue(),
                   "output=" + self.OutputMapName.GetValue(), 
                   "package=" + '%s' % self.RPackagesBook.GetPageText(self.RPackagesBook.GetSelection())]
        
        if not hasattr(SelectedPanel, 'VariogramCheckBox') or not SelectedPanel.VariogramCheckBox.IsChecked():
            command.append("model=" + '%s' % SelectedPanel.ModelChoicebox.GetStringSelection().split(" ")[0])
            
        for i in ['Sill', 'Nugget', 'Range']:
            if getattr(SelectedPanel, i+"ChextBox").IsChecked():
                command.append(i.lower() + "=" + '%s' % getattr(SelectedPanel, i+'Ctrl').GetValue())
        
        if SelectedPanel.KrigingRadioBox.GetStringSelection() == "Block kriging":
            command.append("block=" + '%s' % SelectedPanel.BlockSpinBox.GetValue())
        if self.OverwriteCheckBox.IsChecked():
            command.append("--overwrite")
        if self.VarianceRasterCheckbox.IsChecked():
            command.append("output_var=" + self.OutputVarianceMapName.GetValue())
        
        # give it to the output console
        #@FIXME: it runs the command as a NEW instance. Reimports data, recalculates variogram fit..
        #otherwise I can use Controller() and mimic RunCmd behaviour.
        self.goutput.RunCmd(command, switchPage = True)
    
    def OnVarianceCBChecked(self, event):
        self.OutputVarianceMapName.Enable(event.IsChecked())

class KrigingModule(wx.Frame):
    """ Kriging module for GRASS GIS. Depends on R and its packages gstat and geoR. """
    def __init__(self, parent, *args, **kwargs):
        wx.Frame.__init__(self, parent, *args, **kwargs)
        # setting properties and all widgettery
        self.SetTitle(_("Kriging Module"))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_dialog.ico'), wx.BITMAP_TYPE_ICO))
        self.log = Log(self) 
        self.CreateStatusBar()
        self.log.message(_("Ready."))
        
        self.Panel = KrigingPanel(self)
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.GetBestSize())
    
class Log:
    """ The log output is redirected to the status bar of the containing frame. """
    def __init__(self, parent):
        self.parent = parent

    def message(self, text_string):
        """ Updates status bar """
        self.parent.SetStatusText(text_string.strip())

class RBookPanel(wx.Panel):
    """ Generic notebook page with shared widgets and empty kriging functions. """
    def __init__(self, parent, *args, **kwargs):
        wx.Panel.__init__(self, parent, *args, **kwargs)
        
        self.parent = parent
        
        self.VariogramSizer = wx.StaticBoxSizer(wx.StaticBox(self,
                                                             id = wx.ID_ANY, 
                                                             label = _("Variogram fitting")),
                                                wx.HORIZONTAL)
        self.LeftSizer = wx.BoxSizer(wx.VERTICAL)
        self.RightSizer = wx.BoxSizer(wx.VERTICAL)
        self.ParametersSizer = wx.GridBagSizer(vgap = 5, hgap = 5)

        self.VariogramSizer.Add(self.LeftSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        self.VariogramSizer.Add(self.RightSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        
        # left side of Variogram fitting. The checkboxes and spinctrls.
        self.PlotButton = wx.Button(self, id = wx.ID_ANY, label = _("Plot/refresh variogram")) # no stock ID for Run button.. 
        self.PlotButton.Bind(wx.EVT_BUTTON, self.OnPlotButton)
        self.PlotButton.Enable(False) # grey it out until a suitable layer is available
        self.LeftSizer.Add(self.PlotButton, proportion = 0, flag =  wx.ALL, border = parent.border)
        self.LeftSizer.Add(self.ParametersSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        
        self.ParametersList = ["Sill", "Nugget", "Range"]
        MinValues = [0,0,1]
        for n in self.ParametersList:
            setattr(self, n+"ChextBox", wx.CheckBox(self,
                                                    id = self.ParametersList.index(n),
                                                    label = _(n + ":")))
            setattr(self, n+"Ctrl", (wx.SpinCtrl(self,
                                                 id = wx.ID_ANY,
                                                 min = MinValues[self.ParametersList.index(n)],
                                                 max = maxint)))
            getattr(self, n+"ChextBox").Bind(wx.EVT_CHECKBOX,
                                             self.UseValue,
                                             id = self.ParametersList.index(n))
            setattr(self, n+"Sizer", (wx.BoxSizer(wx.HORIZONTAL)))
            self.ParametersSizer.Add(getattr(self, n+"ChextBox"),
                                     flag = wx.ALIGN_CENTER_VERTICAL,
                                     pos = (self.ParametersList.index(n),0))
            self.ParametersSizer.Add(getattr(self, n+"Ctrl"),
                                     flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL,
                                     pos = (self.ParametersList.index(n),1))
        
        # right side of the Variogram fitting. The plot area.
        #Plot = wx.StaticText(self, id= wx.ID_ANY, label = "Check Plot Variogram to interactively fit model.")
        #PlotPanel = wx.Panel(self)
        #self.PlotArea = plot.PlotCanvas(PlotPanel)
        #self.PlotArea.SetInitialSize(size = (250,250))
        #self.RightSizer.Add(PlotPanel, proportion=0, flag= wx.EXPAND|wx.ALL, border=parent.border)
        
        self.KrigingSizer = wx.StaticBoxSizer(wx.StaticBox(self,
                                                             id = wx.ID_ANY,
                                                             label = _("Kriging techniques")),
                                                wx.VERTICAL)
        
        KrigingList = ["Ordinary kriging", "Block kriging"]#, "Universal kriging"] #@FIXME: i18n on the list?
        self.KrigingRadioBox = wx.RadioBox(self,
                                           id = wx.ID_ANY,
                                           choices = KrigingList,
                                           majorDimension = 1,
                                           style = wx.RA_SPECIFY_COLS)
        self.KrigingRadioBox.Bind(wx.EVT_RADIOBOX, self.HideBlockOptions)
        self.KrigingSizer.Add(self.KrigingRadioBox, proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        
        # block kriging parameters. Size.
        BlockSizer = wx.BoxSizer(wx.HORIZONTAL)
        BlockLabel = wx.StaticText(self, id = wx.ID_ANY, label = _("Block size:"))
        self.BlockSpinBox = wx.SpinCtrl(self, id = wx.ID_ANY, min = 1, max = maxint)
        self.BlockSpinBox.Enable(False) # default choice is Ordinary kriging so block param is disabled
        BlockSizer.Add(BlockLabel, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = parent.border)
        BlockSizer.Add(self.BlockSpinBox, flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = parent.border)
        self.KrigingSizer.Add(BlockSizer, flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = parent.border)
        
        self.Sizer = wx.BoxSizer(wx.VERTICAL)
        self.Sizer.Add(self.VariogramSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        self.Sizer.Add(self.KrigingSizer,  proportion = 0, flag = wx.EXPAND | wx.ALL, border = parent.border)
        
    def HideBlockOptions(self, event):
        self.BlockSpinBox.Enable(event.GetInt() == 1)
    
    def OnPlotButton(self,event):
        """ Plots variogram with current options. """
        pass
    
    def UseValue(self, event):
        """ Enables/Disables the SpinCtrl in respect of the checkbox. """
        n = self.ParametersList[event.GetId()]
        getattr(self, n+"Ctrl").Enable(event.IsChecked())

class RBookgstatPanel(RBookPanel):
    """ Subclass of RBookPanel, with specific gstat options and kriging functions. """
    def __init__(self, parent, *args, **kwargs):
        RBookPanel.__init__(self, parent, *args, **kwargs)
        
        if robjects.r.require('automap')[0]:
            self.VariogramCheckBox = wx.CheckBox(self, id = wx.ID_ANY, label = _("Auto-fit variogram"))
            self.LeftSizer.Insert(0,
                                  self.VariogramCheckBox,
                                  proportion = 0,
                                  flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                                  border = 4)
            self.SetSizerAndFit(self.Sizer)
            self.VariogramCheckBox.Bind(wx.EVT_CHECKBOX, self.HideOptions)
            self.VariogramCheckBox.SetValue(state = True) # check it by default

        ModelFactor = robjects.r.vgm().r['long']
        ModelList = robjects.r.levels(ModelFactor[0])
        #@FIXME: no other way to let the Python pick it up..
        # and this is te wrong place where to load this list. should be at the very beginning.
        self.ModelChoicebox = wx.Choice(self, id = wx.ID_ANY, choices = ModelList)
        
        # disable model parameters' widgets by default
        for n in ["Sill", "Nugget", "Range"]:
            getattr(self, n+"Ctrl").Enable(False)
        self.ModelChoicebox.Enable(False)
        
        VariogramSubSizer = wx.BoxSizer(wx.HORIZONTAL)
        VariogramSubSizer.Add(item = wx.StaticText(self,
                                                 id =  wx.ID_ANY,
                                                 label = _("Model: ")),
                              flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                              border = 4)
        VariogramSubSizer.Add(item = self.ModelChoicebox,
                              flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                              border = 4)
        
        self.LeftSizer.Insert(2, item = VariogramSubSizer)
        
        self.SetSizerAndFit(self.Sizer)
    
    def HideOptions(self, event):
        self.ModelChoicebox.Enable(not event.IsChecked())
        for n in ["Sill", "Nugget", "Range"]:
            if not event.IsChecked():
                getattr(self, n+"Ctrl").Enable(True)
                getattr(self, n+ "ChextBox").SetValue(True)
                getattr(self, n+ "ChextBox").Enable(False) # grey it out keeping it checked.. improvable
            else:
                getattr(self, n+"Ctrl").Enable(False)
                getattr(self, n+ "ChextBox").SetValue(False)
                getattr(self, n+ "ChextBox").Enable(True)
        #@FIXME: was for n in self.ParametersSizer.GetChildren(): n.Enable(False) but doesn't work
        
    def OnPlotButton(self,event):
        """ Plots variogram with current options. """
        ## BIG WARNING: smell of code duplication. Fix this asap.
        controller = Controller()
        map = self.parent.InputDataMap.GetValue()
        column = self.parent.InputDataColumn.GetValue()
        
        # import data or pick them up
        if globals()["InputData"] is None:
            globals()["InputData"] = controller.ImportMap(map = map,
                                                          column = column)
        # fit the variogram or pick it up
        Formula = controller.ComposeFormula(column = column,
                                            isblock = self.KrigingRadioBox.GetStringSelection() == "Block kriging",
                                            inputdata = globals()['InputData'])
        #if globals()["Variogram"] is None:
        if hasattr(self, 'VariogramCheckBox') and self.VariogramCheckBox.IsChecked():
            self.model = ''
            for each in ("Sill","Nugget","Range"):
                if getattr(self, each+'ChextBox').IsChecked():
                    setattr(self, each.lower(), getattr(self, each+"Ctrl").GetValue())
                else:
                    setattr(self, each.lower(), robjects.r('''NA'''))
        else:
            self.model = self.ModelChoicebox.GetStringSelection().split(" ")[0]
            for each in ("Sill","Nugget","Range"):
                if getattr(self, each+'ChextBox').IsChecked(): #@FIXME will be removed when chextboxes will be frozen
                    setattr(self, each.lower(), getattr(self, each+"Ctrl").GetValue())
            
        globals()["Variogram"] = controller.FitVariogram(Formula,
                                                         InputData,
                                                         model = self.model,
                                                         sill = self.sill,
                                                         nugget = self.nugget,
                                                         range = self.range)
        
        ## A: use wx.plot library. Does not draw lines from f(x), damn.
        #
        ## convert R dataframe (data variogram) - points
        ##print Variogram['datavariogram']
        #PointList = []
        #for n in range(len(Variogram['datavariogram'].r['dist'][0])): #@FIXME: so less Pythonic!!
        #    dist = Variogram['datavariogram'].r['dist'][0][n]
        #    gamma = Variogram['datavariogram'].r['gamma'][0][n]
        #    PointList.append((dist,gamma))
        #DataPoints = plot.PolyMarker(PointList, legend='points', colour='black', marker='circle',size=1)
        #
        ## convert R dataframe (model variogram) - line
        #LineList = []
        #textplot = robjects.r.plot(Variogram['datavariogram'], Variogram['variogrammodel'])
        #print textplot
        ##for n in range(len(Variogram['variogrammodel'].r['dist'][0])): #@FIXME: so less Pythonic!!
        ##    dist = Variogram['datavariogram'].r['dist'][0][n]
        ##    gamma = Variogram['datavariogram'].r['gamma'][0][n]
        ##    LineList.append((dist,gamma))
        ##
        ### give them to plot.PolyPoints and plot.PolyLine
        ## plot it
        #VariogramPlot = plot.PlotGraphics([DataPoints])#,
        #                          #title = "Wa",
        #                          #xlabel = "Distance",
        #                          #ylabel = "Gamma") #([DataPoints, line], title, xlabel, ylabel)
        #self.PlotArea.Draw(VariogramPlot)
        
        # B: use R plot function, in a separate window.
        thread.start_new_thread(self.plot, ())
        #self.plot()
        
    def plot(self):
        #robjects.r.X11()
        #robjects.r.png("variogram.png")
        textplot = robjects.r.plot(Variogram['datavariogram'], Variogram['variogrammodel'])
        print textplot
        self.refresh()
        #robjects.r['dev.off']()

    def refresh(self):
        while True:
            rinterface.process_revents()
            time.sleep(0.1)
        
class RBookgeoRPanel(RBookPanel):
    """ Subclass of RBookPanel, with specific geoR options and kriging functions. """
    def __init__(self, parent, *args, **kwargs):
        RBookPanel.__init__(self, parent, *args, **kwargs)
        #@TODO: change these two lines as soon as geoR f(x)s are integrated.
        for n in self.GetChildren():
            n.Hide()
        self.Sizer.Add(wx.StaticText(self, id = wx.ID_ANY, label = _("Work in progress! No functionality provided.")))
        self.SetSizerAndFit(self.Sizer)
    
def main(argv = None):    
    #@FIXME: solve this double ifelse. the control should not be done twice.
    
    if argv is None:
        importR()
        argv = sys.argv[1:] #stripping first item, the full name of this script
        # wxGUI call.
        app = wx.App()
        KrigingFrame = KrigingModule(parent = None)
        KrigingFrame.Centre()
        KrigingFrame.Show()
        app.MainLoop()
        
    else:
        #CLI
        options, flags = argv
        #@TODO: Work on verbosity. Sometimes it's too verbose (R), sometimes not enough.
        if grass.find_file(options['input'], element = 'vector')['fullname'] is '':
            grass.fatal(_("option: <input>: Vector map not found."))
        
        #@TODO: elaborate input string, if contains mapset or not.. thanks again to Bob for testing on 64bit.
        
        # create output map name, if not specified
        if options['output'] is '':
            try: # to strip mapset name from fullname. Ugh.
                options['input'] = options['input'].split("@")[0]
            except:
                pass
            options['output'] = options['input'] + '_kriging'

        # check for output map with same name. g.parser can't handle this, afaik.
        if grass.find_file(options['output'], element = 'cell')['fullname'] and os.getenv("GRASS_OVERWRITE") == None:
            grass.fatal(_("option: <output>: Raster map already exists."))
        if options['output_var'] is not '' and (grass.find_file(options['output_var'], element = 'cell')['fullname'] and os.getenv("GRASS_OVERWRITE") == None):
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
        #print command
        
        # re-cast integers from strings, as parser() cast everything to string.
        for each in ("sill","nugget","range"):
            if options[each] is not '':
                options[each] = int(options[each])
            else:
                options[each] = robjects.r('''NA''')
        
        controller = Controller()
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
    grass.message(_('Loading packages, please wait...'))
    try:
        import rpy2.robjects as robjects
        import rpy2.rinterface as rinterface #to speed up kriging? for plots.
        haveRpy2 = True
    except ImportError:
        print >> sys.stderr, _("Rpy2 not found. Please install it and re-run.") # ok for other OSes?
        haveRpy2 = False
        if not haveRpy2:
            sys.exit(1)
        
    # R packages check.
    # @FIXME: it leaves a Rtmpxxxx folder into the make tempfolder and causes make complain. [markus]
    for each in ["gstat", "spgrass6"]:
        if not robjects.r.require(each, quietly = True)[0]:
            sys.exit(_("R package % is missing. Install it and re-run v.krige.") % each)
    
if __name__ == '__main__':
    if len(sys.argv) > 1:
        sys.exit(main(argv = grass.parser()))
    else:
        main()
