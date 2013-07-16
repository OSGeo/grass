"""
MODULE:    vkrige

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Dedicated GUI for v.krige script.

DEPENDS:   R 2.x, packages gstat, maptools and spgrass6, optional: automap

COPYRIGHT: (C) 2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

#@TODO move here imports related to wxGUI

### generic imports
import os, sys
from tempfile import gettempdir
import time
import thread
## i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

### dependencies to be checked once, as they are quite time-consuming. cfr. grass.parser.
# GRASS binding
try:
    import grass.script as grass
except ImportError:
    sys.exit(_("No GRASS-python library found."))
### wxGUI imports

GUIModulesPath = os.path.join(os.getenv("GISBASE"), "etc", "gui", "wxpython")
if GUIModulesPath not in sys.path:
    sys.path.append(GUIModulesPath)

from core import globalvar
from gui_core import gselect
from core import gconsole
from gui_core import goutput
from core.settings import UserSettings
from gui_core.widgets import GNotebook
#import help

import wx
#import wx.lib.plot as plot # for plotting the variogram.
import rpy2.robjects as robjects
import rpy2.rinterface as rinterface

# global variables
maxint = 1e6 # instead of sys.maxint, not working with SpinCtrl on 64bit [reported by Bob Moskovitz]

#@TODO move away functions not regarding the GUI

class KrigingPanel(wx.Panel):
    """ Main panel. Contains all widgets except Menus and Statusbar. """
    def __init__(self, parent, Rinstance, controller, *args, **kwargs):
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
        flexSizer.Add(item = self.InputDataColumn)
        
        self.InputDataMap.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnInputMapChanged)
        self.InputDataColumn.GetChildren()[0].Bind(wx.EVT_TEXT, self.OnInputColumnChanged)
        
        InputBoxSizer.Add(item = flexSizer)
        
        #    2. Kriging. In book pages one for each R package. Includes variogram fit.
        KrigingSizer = wx.StaticBoxSizer(wx.StaticBox(self, id = wx.ID_ANY, label = _("Kriging")), wx.HORIZONTAL)

        self.RPackagesBook = GNotebook(parent = self, style = globalvar.FNPageDStyle)
        
        for Rpackage in ["gstat"]: # , "geoR"]: #@TODO: enable it if/when it'll be implemented.
            self.CreatePage(package = Rpackage, Rinstance = Rinstance, controller = controller)
        
        ## Command output. From menuform module, cmdPanel class
        self._gconsole = gconsole.GConsole(guiparent = self)
        self.goutput = goutput.GConsoleWindow(parent = self, gconsole = self._gconsole, margin = False)
        self.goutputId = self.RPackagesBook.GetPageCount()
        self.outpage = self.RPackagesBook.AddPage(page = self.goutput, text = _("Command output"), name = 'output')
        self._gconsole.Bind(gconsole.EVT_CMD_RUN,
                            lambda event:
                                self._switchPageHandler(event = event, priority = 2))
        self._gconsole.Bind(gconsole.EVT_CMD_DONE,
                            lambda event:
                                self._switchPageHandler(event = event, priority = 3))
        self.RPackagesBook.SetSelection(0)
        KrigingSizer.Add(self.RPackagesBook, proportion = 1, flag = wx.EXPAND)
        
        #    3. Output Parameters.
        OutputSizer = wx.StaticBoxSizer(wx.StaticBox(self, id = wx.ID_ANY, label = _("Output")), wx.HORIZONTAL)
        
        OutputParameters = wx.GridBagSizer(hgap = 5, vgap = 5)
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
        
        OutputParameters.AddGrowableCol(1)
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
        
    def CreatePage(self, package, Rinstance, controller):
        """ Creates the three notebook pages, one for each R package """
        for package in ["gstat"]: #@TODO add here other packages when they will be implemented
            classobj = eval("RBook"+package+"Panel")
            setattr(self, "RBook"+package+"Panel", (classobj(self,
                                                             id = wx.ID_ANY,
                                                             Rinstance = Rinstance,
                                                             controller = controller)))
            self.RPackagesBook.AddPage(page = getattr(self, "RBook"+package+"Panel"), text = package)

    def OnButtonRefresh(self, event):
        """ Forces refresh of list of available layers. """
        self.InputDataMap.GetElementList()

    def OnCloseWindow(self, event):
        """ Cancel button pressed"""
        self.parent.Close()
        event.Skip()

    def OnHelpButton(self, event):
        grass.run_command('g.manual', entry = 'v.krige')
        event.Skip()

    def OnInputMapChanged(self, event):
        """ Refreshes list of columns."""
        MapName = event.GetString()
        self.InputDataColumn.InsertColumns(vector = MapName,
                                   layer = 1, excludeKey = False,
                                   type = ['integer', 'double precision'])

    def OnInputColumnChanged(self, event):
        """Fills output map name TextCtrl """
        MapName = self.InputDataMap.GetValue()
        enable = bool(self.InputDataColumn.GetValue())
        self.RunButton.Enable(enable)
        self.RBookgstatPanel.PlotButton.Enable(enable)
        
        if enable:
            self.OutputMapName.SetValue(MapName.split("@")[0]+"_kriging")
            self.OutputVarianceMapName.SetValue(MapName.split("@")[0]+"_kriging.var")
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
            self._gconsole.WriteError("No parameters for running. Please select \"gstat\" tab, check parameters and re-run.")
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
        self._gconsole.RunCmd(command, switchPage = True)
    
    def OnVarianceCBChecked(self, event):
        self.OutputVarianceMapName.Enable(event.IsChecked())

    def _switchPageHandler(self, event, priority):
        self._switchPage(priority = priority)
        event.Skip()

    def _switchPage(self, priority):
        """!Manages @c 'output' notebook page according to priority."""
        if priority == 1:
            self.RPackagesBook.HighlightPageByName('output')
        if priority >= 2:
            self.RPackagesBook.SetSelectionByName('output')
        if priority >= 3:
            self.SetFocus()
            self.Raise()

class KrigingModule(wx.Frame):
    """ Kriging module for GRASS GIS. Depends on R and its packages gstat and geoR. """
    def __init__(self, parent, Rinstance, controller, *args, **kwargs):
        wx.Frame.__init__(self, parent, *args, **kwargs)
        # setting properties and all widgettery
        self.SetTitle(_("Kriging Module"))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_dialog.ico'), wx.BITMAP_TYPE_ICO))
        self.log = Log(self) 
        self.CreateStatusBar()
        self.log.message(_("Ready."))
        
        self.Panel = KrigingPanel(self, Rinstance, controller)
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
    def __init__(self, parent, Rinstance, controller, *args, **kwargs):
        RBookPanel.__init__(self, parent, *args, **kwargs)
        
        # assigns Rinstance, that comes from the GUI call of v.krige.py.
        robjects = Rinstance
        self.controller = controller
        
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
        
        ModelFactor = robjects.r.vgm().rx('long')
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
        ## BIG WARNING: smell of code duplication. Fix this asap. emminchia!
        #controller = Controller() # sed, if needed,
        #controller = self.controller
        map = self.parent.InputDataMap.GetValue()
        column = self.parent.InputDataColumn.GetValue()
        
        # import data or pick them up
        if self.controller.InputData is None:
            self.controller.InputData = self.controller.ImportMap(map = map,
                                                          column = column)
        # fit the variogram or pick it up
        #~ Formula = self.controller.ComposeFormula(column = column,
                                            #~ isblock = self.KrigingRadioBox.GetStringSelection() == "Block kriging")
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
                    
        isblock = self.KrigingRadioBox.GetStringSelection() == "Block kriging"
        if isblock is not '':
            self.predictor = 'x+y'
        else:
            self.predictor = '1'
        
        self.controller.Variogram = self.controller.FitVariogram(robjects.Formula(str(column) + "~" + self.predictor),
                                                         self.controller.InputData,
                                                         model = self.model,
                                                         sill = self.sill,
                                                         nugget = self.nugget,
                                                         range = self.range)

        # use R plot function, in a separate window.
        thread.start_new_thread(self.plot, ())
        
    def plot(self):
        #robjects.r.X11()
        #robjects.r.png("variogram.png")
        textplot = robjects.r.plot(self.controller.Variogram['datavariogram'], 
                                   self.controller.Variogram['variogrammodel'])
        print textplot
        self.refresh()
        #robjects.r['dev.off']()

    def refresh(self):
        while True:
            rinterface.process_revents()
            time.sleep(0.2)
        
class RBookgeoRPanel(RBookPanel):
    """ Subclass of RBookPanel, with specific geoR options and kriging functions. """
    def __init__(self, parent, *args, **kwargs):
        RBookPanel.__init__(self, parent, *args, **kwargs)
        #@TODO: change these two lines as soon as geoR f(x)s are integrated.
        for n in self.GetChildren():
            n.Hide()
        self.Sizer.Add(wx.StaticText(self, id = wx.ID_ANY, label = _("Work in progress! No functionality provided.")))
        self.SetSizerAndFit(self.Sizer)
