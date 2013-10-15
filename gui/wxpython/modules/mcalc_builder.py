"""!
@package modules::mcalc_builder

@brief Map calculator, GUI wrapper for r.mapcalc

Classes:
 - mcalc_builder::MapCalcFrame

(C) 2008, 2011-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
@author Martin Landa <landa.martin gmail.com>
@author Tim Michelsen (load/save expression)
"""

import os
import sys
import re

import wx
import grass.script as grass

if __name__ == "__main__":
    gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if gui_wx_path not in sys.path:
        sys.path.append(gui_wx_path)

from core             import globalvar
from core.gcmd        import GError, RunCommand
from core.giface      import StandaloneGrassInterface
from core.utils import _
from gui_core.gselect import Select
from gui_core.forms   import GUI
from core.settings    import UserSettings

class MapCalcFrame(wx.Frame):
    """!Mapcalc Frame class. Calculator-style window to create and run
    r(3).mapcalc statements.
    """
    def __init__(self, parent, giface, cmd, id = wx.ID_ANY,
                 style = wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER, **kwargs):
        self.parent = parent
        self._giface = giface

        if self.parent:
            self.log = self.parent.GetLogWindow()
        else:
            self.log = None
        
        # grass command
        self.cmd = cmd

        if self.cmd == 'r.mapcalc':
            self.rast3d = False
            title = _('GRASS GIS Raster Map Calculator')
        if self.cmd == 'r3.mapcalc':
            self.rast3d = True
            title = _('GRASS GIS 3D Raster Map Calculator')
            
        wx.Frame.__init__(self, parent, id = id, title = title, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        self.CreateStatusBar()

        #
        # variables
        #
        self.heading = _('mapcalc statement')
        self.funct_dict = {
            'abs(x)':'abs()',
            'acos(x)':'acos()',
            'asin(x)':'asin()',
            'atan(x)':'atan()',
            'atan(x,y)':'atan( , )',
            'cos(x)':'cos()',
            'double(x)':'double()',
            'eval([x,y,...,]z)':'eval()',
            'exp(x)':'exp()',
            'exp(x,y)':'exp( , )',
            'float(x)':'float()',
            'graph(x,x1,y1[x2,y2..])':'graph( , , )',
            'if(x)':'if()',
            'if(x,a)':'if( , )',
            'if(x,a,b)':'if( , , )',
            'if(x,a,b,c)':'if( , , , )',
            'int(x)':'int()',
            'isnull(x)':'isnull()',
            'log(x)':'log(',
            'log(x,b)':'log( , )',
            'max(x,y[,z...])':'max( , )',
            'median(x,y[,z...])':'median( , )',
            'min(x,y[,z...])':'min( , )',
            'mode(x,y[,z...])':'mode( , )',
            'not(x)':'not()',
            'pow(x,y)':'pow( , )',
            'rand(a,b)':'rand( , )',
            'round(x)':'round()',
            'sin(x)':'sin()',
            'sqrt(x)':'sqrt()',
            'tan(x)':'tan()',
            'xor(x,y)':'xor( , )',
            'row()':'row()',
            'col()':'col()',
            'x()':'x()',
            'y()':'y()',
            'ewres()':'ewres()',
            'nsres()':'nsres()',
            'null()':'null()'
            }
        
        if self.rast3d:
            self.funct_dict['z()'] = 'z()'
            self.funct_dict['tbres()'] = 'tbres()'
            element = 'rast3d'
        else:
            element = 'cell'

        # characters which can be in raster map name but the map name must be then quoted
        self.charactersToQuote = '+-&!<>%~?^|'
        # stores last typed map name in Select widget to distinguish typing from selection
        self.lastMapName = ''
        
        self.operatorBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                        label=" %s " % _('Operators'))
        self.outputBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label=" %s " % _('Output'))
        self.operandBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                       label=" %s " % _('Operands'))
        self.expressBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                       label=" %s " % _('Expression'))
        
        #
        # Buttons
        #
        self.btn_clear = wx.Button(parent = self.panel, id = wx.ID_CLEAR)
        self.btn_help = wx.Button(parent = self.panel, id = wx.ID_HELP)
        self.btn_run = wx.Button(parent = self.panel, id = wx.ID_ANY, label = _("&Run"))
        self.btn_run.SetForegroundColour(wx.Colour(35, 142, 35))
        self.btn_run.SetDefault()
        self.btn_close = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_save = wx.Button(parent = self.panel, id = wx.ID_SAVE)
        self.btn_save.SetToolTipString(_('Save expression to file'))
        self.btn_load = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                  label = _("&Load"))
        self.btn_load.SetToolTipString(_('Load expression from file'))
        
        self.btn = dict()        
        self.btn['pow'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "^")
        self.btn['pow'].SetToolTipString(_('exponent'))
        self.btn['div'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "/")
        self.btn['div'].SetToolTipString(_('divide'))
        self.btn['add'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "+")
        self.btn['add'].SetToolTipString(_('add'))
        self.btn['minus'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "-")
        self.btn['minus'].SetToolTipString(_('subtract'))
        self.btn['mod'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "%")
        self.btn['mod'].SetToolTipString(_('modulus'))
        self.btn['mult'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "*")
        self.btn['mult'].SetToolTipString(_('multiply'))

        self.btn['parenl'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "(") 
        self.btn['parenr'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ")")
        self.btn['lshift'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "<<")
        self.btn['lshift'].SetToolTipString(_('left shift'))
        self.btn['rshift'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">>")
        self.btn['rshift'].SetToolTipString(_('right shift'))
        self.btn['rshiftu'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">>>")
        self.btn['rshiftu'].SetToolTipString(_('right shift (unsigned)'))
        self.btn['gt'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">")
        self.btn['gt'].SetToolTipString(_('greater than'))
        self.btn['gteq'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = ">=")
        self.btn['gteq'].SetToolTipString(_('greater than or equal to'))
        self.btn['lt'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "<")
        self.btn['lt'].SetToolTipString(_('less than'))
        self.btn['lteq'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "<=")
        self.btn['lteq'].SetToolTipString(_('less than or equal to'))
        self.btn['eq'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "==")
        self.btn['eq'].SetToolTipString(_('equal to'))
        self.btn['noteq'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "!=")
        self.btn['noteq'].SetToolTipString(_('not equal to'))

        self.btn['compl'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "~")
        self.btn['compl'].SetToolTipString(_('one\'s complement'))
        self.btn['not'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "!")
        self.btn['not'].SetToolTipString(_('NOT'))
        self.btn['andbit'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = '&&')
        self.btn['andbit'].SetToolTipString(_('bitwise AND'))
        self.btn['orbit'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "|")
        self.btn['orbit'].SetToolTipString(_('bitwise OR'))
        self.btn['and'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "&&&&")
        self.btn['and'].SetToolTipString(_('logical AND'))
        self.btn['andnull'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "&&&&&&")
        self.btn['andnull'].SetToolTipString(_('logical AND (ignores NULLs)'))
        self.btn['or'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "||")
        self.btn['or'].SetToolTipString(_('logical OR'))
        self.btn['ornull'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "|||")
        self.btn['ornull'].SetToolTipString(_('logical OR (ignores NULLs)'))
        self.btn['cond'] = wx.Button(parent = self.panel, id = wx.ID_ANY, label = "a ? b : c") 
        self.btn['cond'].SetToolTipString(_('conditional'))
        
        #
        # Text area
        #
        self.text_mcalc = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY, size = (-1, 75),
                                      style = wx.TE_MULTILINE)
        wx.CallAfter(self.text_mcalc.SetFocus)
        
        #
        # Map and function insertion text and ComboBoxes
        self.newmaplabel = wx.StaticText(parent = self.panel, id = wx.ID_ANY)
        if self.rast3d:
            self.newmaplabel.SetLabel(_('Name for new 3D raster map to create'))
        else:
            self.newmaplabel.SetLabel(_('Name for new raster map to create'))
        self.newmaptxt = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY, size=(250, -1))
        self.mapsellabel = wx.StaticText(parent = self.panel, id = wx.ID_ANY)
        if self.rast3d:
            self.mapsellabel.SetLabel(_('Insert existing 3D raster map'))
        else:
            self.mapsellabel.SetLabel(_('Insert existing raster map'))
        self.mapselect = Select(parent = self.panel, id = wx.ID_ANY, size = (250, -1),
                                type = element, multiple = False)
        self.functlabel = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                        label = _('Insert mapcalc function'))
        self.function = wx.ComboBox(parent = self.panel, id = wx.ID_ANY, 
                                    size = (250, -1), choices = sorted(self.funct_dict.keys()),
                                    style = wx.CB_DROPDOWN |
                                    wx.CB_READONLY | wx.TE_PROCESS_ENTER)
        
        self.overwrite = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                     label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        
        self.addbox = wx.CheckBox(parent=self.panel,
                                  label=_('Add created raster map into layer tree'), style = wx.NO_BORDER)
        self.addbox.SetValue(UserSettings.Get(group='cmd', key='addNewLayer', subkey='enabled'))
        if not self.parent or self.parent.GetName() != 'LayerManager':
            self.addbox.Hide()
        
        #
        # Bindings
        #
        for btn in self.btn.keys():
            self.btn[btn].Bind(wx.EVT_BUTTON, self.AddMark)
        
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnMCalcRun)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.btn_save.Bind(wx.EVT_BUTTON, self.OnSaveExpression)
        self.btn_load.Bind(wx.EVT_BUTTON, self.OnLoadExpression)
        
        self.mapselect.Bind(wx.EVT_TEXT, self.OnSelectTextEvt)
        self.function.Bind(wx.EVT_COMBOBOX, self._return_funct)
        self.function.Bind(wx.EVT_TEXT_ENTER, self.OnSelect)
        self.newmaptxt.Bind(wx.EVT_TEXT, self.OnUpdateStatusBar)
        self.text_mcalc.Bind(wx.EVT_TEXT, self.OnUpdateStatusBar)

        self._layout()

        self.SetMinSize(self.GetBestSize())
    
    def _return_funct(self,event):
        i = event.GetString()
        self._addSomething(self.funct_dict[i])
    
    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        controlSizer = wx.BoxSizer(wx.HORIZONTAL)
        operatorSizer = wx.StaticBoxSizer(self.operatorBox, wx.HORIZONTAL)
        outOpeSizer = wx.BoxSizer(wx.VERTICAL)
        
        buttonSizer1 = wx.GridBagSizer(5, 1)
        buttonSizer1.Add(item = self.btn['add'], pos = (0,0))
        buttonSizer1.Add(item = self.btn['minus'], pos = (0,1))
        buttonSizer1.Add(item = self.btn['mod'], pos = (5,0))
        buttonSizer1.Add(item = self.btn['mult'], pos = (1,0))
        buttonSizer1.Add(item = self.btn['div'], pos = (1,1))
        buttonSizer1.Add(item = self.btn['pow'], pos = (5,1))
        buttonSizer1.Add(item = self.btn['gt'], pos = (2,0))
        buttonSizer1.Add(item = self.btn['gteq'], pos = (2,1))
        buttonSizer1.Add(item = self.btn['eq'], pos = (4,0))
        buttonSizer1.Add(item = self.btn['lt'], pos = (3,0))
        buttonSizer1.Add(item = self.btn['lteq'], pos = (3,1))
        buttonSizer1.Add(item = self.btn['noteq'], pos = (4,1))

        buttonSizer2 = wx.GridBagSizer(5, 1)
        buttonSizer2.Add(item = self.btn['and'], pos = (0,0))
        buttonSizer2.Add(item = self.btn['andbit'], pos = (1,0))
        buttonSizer2.Add(item = self.btn['andnull'], pos = (2,0))
        buttonSizer2.Add(item = self.btn['or'], pos = (0,1))
        buttonSizer2.Add(item = self.btn['orbit'], pos = (1,1))
        buttonSizer2.Add(item = self.btn['ornull'], pos = (2,1))
        buttonSizer2.Add(item = self.btn['lshift'], pos = (3,0))
        buttonSizer2.Add(item = self.btn['rshift'], pos = (3,1))
        buttonSizer2.Add(item = self.btn['rshiftu'], pos = (4,0))
        buttonSizer2.Add(item = self.btn['cond'], pos = (5,0))
        buttonSizer2.Add(item = self.btn['compl'], pos = (5,1))
        buttonSizer2.Add(item = self.btn['not'], pos = (4,1))

        outputSizer = wx.StaticBoxSizer(self.outputBox, wx.VERTICAL)
        outputSizer.Add(item = self.newmaplabel,
                        flag = wx.ALIGN_CENTER | wx.BOTTOM | wx.TOP, border = 5)
        outputSizer.Add(item = self.newmaptxt,
                        flag = wx.EXPAND)
        
        operandSizer = wx.StaticBoxSizer(self.operandBox, wx.HORIZONTAL)
        
        buttonSizer3 = wx.GridBagSizer(7, 1)
        buttonSizer3.Add(item = self.functlabel, pos = (0,0),
                         span = (1,2), flag = wx.ALIGN_CENTER)
        buttonSizer3.Add(item = self.function, pos = (1,0),
                         span = (1,2))                         
        buttonSizer3.Add(item = self.mapsellabel, pos = (2,0),
                         span = (1,2), flag = wx.ALIGN_CENTER)
        buttonSizer3.Add(item = self.mapselect, pos = (3,0),
                         span = (1,2))
        threebutton = wx.GridBagSizer(1, 2)
        threebutton.Add(item = self.btn['parenl'], pos = (0,0),
                         span = (1,1), flag = wx.ALIGN_LEFT)
        threebutton.Add(item = self.btn['parenr'], pos = (0,1),
                         span = (1,1), flag = wx.ALIGN_CENTER)
        threebutton.Add(item = self.btn_clear, pos = (0,2),
                         span = (1,1), flag = wx.ALIGN_RIGHT)
        buttonSizer3.Add(item = threebutton, pos = (4,0),
                         span = (1,1), flag = wx.ALIGN_CENTER)

        buttonSizer4 = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer4.Add(item = self.btn_load,
                         flag = wx.ALL, border = 5)
        buttonSizer4.Add(item = self.btn_save,
                         flag = wx.ALL, border = 5)                         
        buttonSizer4.AddSpacer(30)
        buttonSizer4.Add(item = self.btn_help,
                         flag = wx.ALL, border = 5)
        buttonSizer4.Add(item = self.btn_run,
                         flag = wx.ALL, border = 5)
        buttonSizer4.Add(item = self.btn_close,
                         flag = wx.ALL, border = 5)
        
        operatorSizer.Add(item = buttonSizer1, proportion = 0,
                          flag = wx.ALL | wx.EXPAND, border = 5)
        operatorSizer.Add(item = buttonSizer2, proportion = 0,
                          flag = wx.TOP | wx.BOTTOM | wx.RIGHT | wx.EXPAND, border = 5)
        
        operandSizer.Add(item = buttonSizer3, proportion = 0,
                         flag = wx.TOP | wx.BOTTOM | wx.RIGHT, border = 5)
        
        controlSizer.Add(item = operatorSizer, proportion = 1,
                         flag = wx.RIGHT | wx.EXPAND, border = 5)
        outOpeSizer.Add(item = outputSizer, proportion = 0,
                         flag = wx.EXPAND)
        outOpeSizer.Add(item = operandSizer, proportion = 1,
                         flag = wx.EXPAND | wx.TOP, border = 5)
        controlSizer.Add(item = outOpeSizer, proportion = 0,
                         flag = wx.EXPAND)

        expressSizer = wx.StaticBoxSizer(self.expressBox, wx.HORIZONTAL)
        expressSizer.Add(item = self.text_mcalc, proportion = 1,
                         flag = wx.EXPAND)

        sizer.Add(item = controlSizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALL,
                  border = 5)        
        sizer.Add(item = expressSizer, proportion = 1,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                  border = 5)
        sizer.Add(item = buttonSizer4, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 3)
        
        sizer.Add(item = self.overwrite, proportion = 0,
                  flag = wx.LEFT | wx.RIGHT,
                  border = 5)
        if self.addbox.IsShown():
            sizer.Add(item = self.addbox, proportion = 0,
                      flag = wx.LEFT | wx.RIGHT,
                      border = 5)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()
        
    def AddMark(self,event):
        """!Sends operators to insertion method
        """
        if event.GetId() == self.btn['compl'].GetId(): mark = "~"
        elif event.GetId() == self.btn['not'].GetId(): mark = "!"
        elif event.GetId() == self.btn['pow'].GetId(): mark = "^"
        elif event.GetId() == self.btn['div'].GetId(): mark = "/"
        elif event.GetId() == self.btn['add'].GetId(): mark = "+"
        elif event.GetId() == self.btn['minus'].GetId(): mark = "-"
        elif event.GetId() == self.btn['mod'].GetId(): mark = "%"
        elif event.GetId() == self.btn['mult'].GetId(): mark = "*"
        elif event.GetId() == self.btn['lshift'].GetId(): mark = "<<"
        elif event.GetId() == self.btn['rshift'].GetId(): mark = ">>"
        elif event.GetId() == self.btn['rshiftu'].GetId(): mark = ">>>"
        elif event.GetId() == self.btn['gt'].GetId(): mark = ">"
        elif event.GetId() == self.btn['gteq'].GetId(): mark = ">="
        elif event.GetId() == self.btn['lt'].GetId(): mark = "<"
        elif event.GetId() == self.btn['lteq'].GetId(): mark = "<="
        elif event.GetId() == self.btn['eq'].GetId(): mark = "=="
        elif event.GetId() == self.btn['noteq'].GetId(): mark = "!="
        elif event.GetId() == self.btn['andbit'].GetId(): mark = "&"
        elif event.GetId() == self.btn['orbit'].GetId(): mark = "|"
        elif event.GetId() == self.btn['or'].GetId(): mark =  "||"
        elif event.GetId() == self.btn['ornull'].GetId(): mark = "|||"
        elif event.GetId() == self.btn['and'].GetId(): mark = "&&"
        elif event.GetId() == self.btn['andnull'].GetId(): mark = "&&&"
        elif event.GetId() == self.btn['cond'].GetId(): mark = " ? : "
        elif event.GetId() == self.btn['parenl'].GetId(): mark = "("
        elif event.GetId() == self.btn['parenr'].GetId(): mark = ")"
        self._addSomething(mark)
        
    def OnSelectTextEvt(self, event):
        """!Checks if user is typing or the event was emited by map selection.
        Prevents from changing focus.
        """
        item = event.GetString()
        if not (abs(len(item) - len(self.lastMapName)) == 1  and \
            self.lastMapName in item or item in self.lastMapName):
            self.OnSelect(event)
        self.lastMapName = item

    def OnSelect(self, event):
        """!Gets raster map or function selection and send it to
        insertion method. 

        Checks for characters which can be in raster map name but 
        the raster map name must be then quoted.
        """
        item = event.GetString().strip()
        if any((char in item) for char in self.charactersToQuote):
            item = '"' + item + '"'
        self._addSomething(item)

    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        expr = self.text_mcalc.GetValue().strip().replace("\n", " ")
        self.SetStatusText("r.mapcalc '%s = %s'" % (self.newmaptxt.GetValue(),
                                                    expr))
        event.Skip()
        
    def _addSomething(self, what):
        """!Inserts operators, map names, and functions into text area
        """
        mcalcstr  = self.text_mcalc.GetValue()
        position  = self.text_mcalc.GetInsertionPoint()
        
        newmcalcstr = mcalcstr[:position]
        
        position_offset = 0
        try:
            if newmcalcstr[-1] != ' ':
                newmcalcstr += ' '
                position_offset += 1
        except:
            pass
        
        newmcalcstr += what + ' ' + mcalcstr[position:]
        
        self.text_mcalc.SetValue(newmcalcstr)
        if len(what) > 0:
            match = re.search(pattern="\(.*\)", string=what)
            if match:
                position_offset += match.start() + 1
            else:
                position_offset += len(what)

        self.text_mcalc.SetInsertionPoint(position + position_offset)
        self.text_mcalc.Update()
        self.text_mcalc.SetFocus()
        
    def OnMCalcRun(self,event):
        """!Builds and runs r.mapcalc statement
        """
        name = self.newmaptxt.GetValue().strip()
        if not name:
            GError(parent = self,
                   message = _("You must enter the name of "
                               "a new raster map to create."))
            return
        
        if not (name[0] == '"' and name[-1] == '"') and any((char in name) for char in self.charactersToQuote):
            name = '"' + name + '"'

        expr = self.text_mcalc.GetValue().strip().replace("\n", " ")
        if not expr:
            GError(parent = self,
                   message = _("You must enter an expression "
                               "to create a new raster map."))
            return
        
        if self.log:
            cmd = [self.cmd, str('expression=%s = %s' % (name, expr))]
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')
            self.log.RunCmd(cmd, onDone = self.OnDone)
            self.parent.Raise()
        else:
            if self.overwrite.IsChecked():
                overwrite = True
            else:
                overwrite = False
            RunCommand(self.cmd,
                       expression = "%s=%s" % (name, expr),
                       overwrite = overwrite)
        
    def OnDone(self, cmd, returncode):
        """!Add create map to the layer tree

        Sends the mapCreated signal from the grass interface.
        """
        if returncode != 0:
            return
        name = self.newmaptxt.GetValue().strip(' "') + '@' + grass.gisenv()['MAPSET']
        ltype = 'rast'
        if self.rast3d:
            ltype = 'rast3d'
        self._giface.mapCreated.emit(name=name, ltype=ltype, add=self.addbox.IsChecked())

    def OnSaveExpression(self, event):
        """!Saves expression to file
        """
        mctxt = self.newmaptxt.GetValue() + ' = ' + self.text_mcalc.GetValue() + os.linesep
        
        #dialog
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose a file name to save the expression"),
                            wildcard = _("Expression file (*)|*"),
                            style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return
            
            try:
                fobj = open(path, 'w')
                fobj.write(mctxt)
            finally:
                fobj.close()
        
        dlg.Destroy()

    def OnLoadExpression(self, event):
        """!Load expression from file
        """
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose a file name to load the expression"),
                            wildcard = _("Expression file (*)|*"),
                            style = wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return

            try:
                fobj = open(path,'r')
                mctxt = fobj.read()
            finally:
                fobj.close()
            
            try:
                result, exp = mctxt.split('=', 1)
            except ValueError:
                result = ''
                exp = mctxt
            
            self.newmaptxt.SetValue(result.strip())
            self.text_mcalc.SetValue(exp.strip())
            self.text_mcalc.SetFocus()
            self.text_mcalc.SetInsertionPointEnd()
        
        dlg.Destroy()
                
    def OnClear(self, event):
        """!Clears text area
        """
        self.text_mcalc.SetValue('')
        
    def OnHelp(self, event):
        """!Launches r.mapcalc help
        """
        RunCommand('g.manual', parent = self, entry = self.cmd)
        
    def OnClose(self,event):
        """!Close window"""
        self.Destroy()

if __name__ == "__main__":
    
    app = wx.App(0)
    frame = MapCalcFrame(parent = None, cmd = 'r.mapcalc', giface = StandaloneGrassInterface())
    frame.Show()
    app.MainLoop()
