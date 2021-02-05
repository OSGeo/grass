"""
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
import re

import wx
import grass.script as grass

from core import globalvar
from core.gcmd import GError, RunCommand
from core.giface import StandaloneGrassInterface
from gui_core.gselect import Select
from gui_core.widgets import IntegerValidator
from gui_core.wrap import Button, ClearButton, CloseButton, TextCtrl, \
    StaticText, StaticBox
from core.settings import UserSettings


class MapCalcFrame(wx.Frame):
    """Mapcalc Frame class. Calculator-style window to create and run
    r(3).mapcalc statements.
    """

    def __init__(self, parent, giface, cmd, id=wx.ID_ANY,
                 style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER, **kwargs):
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
            title = _('Raster Map Calculator')
        if self.cmd == 'r3.mapcalc':
            self.rast3d = True
            title = _('3D Raster Map Calculator')

        wx.Frame.__init__(self, parent, id=id, title=title, **kwargs)
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    globalvar.ICONDIR,
                    'grass.ico'),
                wx.BITMAP_TYPE_ICO))

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.CreateStatusBar()

        #
        # variables
        #
        self.heading = _('mapcalc statement')
        self.funct_dict = {
            'abs(x)': 'abs()',
            'acos(x)': 'acos()',
            'asin(x)': 'asin()',
            'atan(x)': 'atan()',
            'atan(x,y)': 'atan( , )',
            'cos(x)': 'cos()',
            'double(x)': 'double()',
            'eval([x,y,...,]z)': 'eval()',
            'exp(x)': 'exp()',
            'exp(x,y)': 'exp( , )',
            'float(x)': 'float()',
            'graph(x,x1,y1[x2,y2..])': 'graph( , , )',
            'if(x)': 'if()',
            'if(x,a)': 'if( , )',
            'if(x,a,b)': 'if( , , )',
            'if(x,a,b,c)': 'if( , , , )',
            'int(x)': 'int()',
            'isnull(x)': 'isnull()',
            'log(x)': 'log(',
            'log(x,b)': 'log( , )',
            'max(x,y[,z...])': 'max( , )',
            'median(x,y[,z...])': 'median( , )',
            'min(x,y[,z...])': 'min( , )',
            'mode(x,y[,z...])': 'mode( , )',
            'nmax(x,y[,z...])': 'nmax( , )',
            'nmedian(x,y[,z...])': 'nmedian( , )',
            'nmin(x,y[,z...])': 'nmin( , )',
            'nmode(x,y[,z...])': 'nmode( , )',
            'not(x)': 'not()',
            'pow(x,y)': 'pow( , )',
            'rand(a,b)': 'rand( , )',
            'round(x)': 'round()',
            'round(x,y)': 'round( , )',
            'round(x,y,z)': 'round( , , )',
            'sin(x)': 'sin()',
            'sqrt(x)': 'sqrt()',
            'tan(x)': 'tan()',
            'xor(x,y)': 'xor( , )',
            'row()': 'row()',
            'col()': 'col()',
            'nrows()': 'nrows()',
            'ncols()': 'ncols()',
            'x()': 'x()',
            'y()': 'y()',
            'ewres()': 'ewres()',
            'nsres()': 'nsres()',
            'area()': 'area()',
            'null()': 'null()'
        }

        if self.rast3d:
            self.funct_dict['z()'] = 'z()'
            self.funct_dict['tbres()'] = 'tbres()'
            element = 'raster_3d'
        else:
            element = 'cell'

        # characters which can be in raster map name but the map name must be
        # then quoted
        self.charactersToQuote = '+-&!<>%~?^|'
        # stores last typed map name in Select widget to distinguish typing
        # from selection
        self.lastMapName = ''

        self.operatorBox = StaticBox(parent=self.panel, id=wx.ID_ANY,
                                     label=" %s " % _('Operators'))
        self.outputBox = StaticBox(parent=self.panel, id=wx.ID_ANY,
                                   label=" %s " % _('Output'))
        self.operandBox = StaticBox(parent=self.panel, id=wx.ID_ANY,
                                    label=" %s " % _('Operands'))
        self.expressBox = StaticBox(parent=self.panel, id=wx.ID_ANY,
                                    label=" %s " % _('Expression'))

        #
        # Buttons
        #
        self.btn_clear = ClearButton(parent=self.panel)
        self.btn_help = Button(parent=self.panel, id=wx.ID_HELP)
        self.btn_run = Button(
            parent=self.panel,
            id=wx.ID_ANY,
            label=_("&Run"))
        self.btn_run.SetDefault()
        self.btn_close = CloseButton(parent=self.panel)
        self.btn_save = Button(parent=self.panel, id=wx.ID_SAVE)
        self.btn_save.SetToolTip(_('Save expression to file'))
        self.btn_load = Button(parent=self.panel, id=wx.ID_ANY,
                               label=_("&Load"))
        self.btn_load.SetToolTip(_('Load expression from file'))
        self.btn_copy = Button(
            parent=self.panel, id=wx.ID_ANY, label=_("Copy"))
        self.btn_copy.SetToolTip(
            _("Copy the current command string to the clipboard"))

        self.btn = dict()
        self.btn['pow'] = Button(parent=self.panel, id=wx.ID_ANY, label="^")
        self.btn['pow'].SetToolTip(_('exponent'))
        self.btn['div'] = Button(parent=self.panel, id=wx.ID_ANY, label="/")
        self.btn['div'].SetToolTip(_('divide'))
        self.btn['add'] = Button(parent=self.panel, id=wx.ID_ANY, label="+")
        self.btn['add'].SetToolTip(_('add'))
        self.btn['minus'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="-")
        self.btn['minus'].SetToolTip(_('subtract'))
        self.btn['mod'] = Button(parent=self.panel, id=wx.ID_ANY, label="%")
        self.btn['mod'].SetToolTip(_('modulus'))
        self.btn['mult'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="*")
        self.btn['mult'].SetToolTip(_('multiply'))

        self.btn['parenl'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="(")
        self.btn['parenr'] = Button(
            parent=self.panel, id=wx.ID_ANY, label=")")
        self.btn['lshift'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="<<")
        self.btn['lshift'].SetToolTip(_('left shift'))
        self.btn['rshift'] = Button(
            parent=self.panel, id=wx.ID_ANY, label=">>")
        self.btn['rshift'].SetToolTip(_('right shift'))
        self.btn['rshiftu'] = Button(
            parent=self.panel, id=wx.ID_ANY, label=">>>")
        self.btn['rshiftu'].SetToolTip(_('right shift (unsigned)'))
        self.btn['gt'] = Button(parent=self.panel, id=wx.ID_ANY, label=">")
        self.btn['gt'].SetToolTip(_('greater than'))
        self.btn['gteq'] = Button(
            parent=self.panel, id=wx.ID_ANY, label=">=")
        self.btn['gteq'].SetToolTip(_('greater than or equal to'))
        self.btn['lt'] = Button(parent=self.panel, id=wx.ID_ANY, label="<")
        self.btn['lt'].SetToolTip(_('less than'))
        self.btn['lteq'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="<=")
        self.btn['lteq'].SetToolTip(_('less than or equal to'))
        self.btn['eq'] = Button(parent=self.panel, id=wx.ID_ANY, label="==")
        self.btn['eq'].SetToolTip(_('equal to'))
        self.btn['noteq'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="!=")
        self.btn['noteq'].SetToolTip(_('not equal to'))

        self.btn['compl'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="~")
        self.btn['compl'].SetToolTip(_('one\'s complement'))
        self.btn['not'] = Button(parent=self.panel, id=wx.ID_ANY, label="!")
        self.btn['not'].SetToolTip(_('NOT'))
        self.btn['andbit'] = Button(
            parent=self.panel, id=wx.ID_ANY, label='&&')
        self.btn['andbit'].SetToolTip(_('bitwise AND'))
        self.btn['orbit'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="|")
        self.btn['orbit'].SetToolTip(_('bitwise OR'))
        self.btn['and'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="&&&&")
        self.btn['and'].SetToolTip(_('logical AND'))
        self.btn['andnull'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="&&&&&&")
        self.btn['andnull'].SetToolTip(_('logical AND (ignores NULLs)'))
        self.btn['or'] = Button(parent=self.panel, id=wx.ID_ANY, label="||")
        self.btn['or'].SetToolTip(_('logical OR'))
        self.btn['ornull'] = Button(
            parent=self.panel, id=wx.ID_ANY, label="|||")
        self.btn['ornull'].SetToolTip(_('logical OR (ignores NULLs)'))
        self.btn['cond'] = Button(
            parent=self.panel,
            id=wx.ID_ANY,
            label="a ? b : c")
        self.btn['cond'].SetToolTip(_('conditional'))

        #
        # Text area
        #
        self.text_mcalc = TextCtrl(
            parent=self.panel, id=wx.ID_ANY, size=(-1, 100),
            style=wx.TE_MULTILINE)
        wx.CallAfter(self.text_mcalc.SetFocus)

        #
        # Map and function insertion text and ComboBoxes
        self.newmaplabel = StaticText(parent=self.panel, id=wx.ID_ANY)
        if self.rast3d:
            self.newmaplabel.SetLabel(
                _('Name for new 3D raster map to create'))
        else:
            self.newmaplabel.SetLabel(_('Name for new raster map to create'))
        # As we can write only to current mapset, names should not be fully qualified
        # to not confuse end user about writing in other mapset
        self.newmaptxt = Select(
            parent=self.panel, id=wx.ID_ANY, size=(
                250, -1), type=element, multiple=False,
                fullyQualified=False)
        self.mapsellabel = StaticText(parent=self.panel, id=wx.ID_ANY)
        if self.rast3d:
            self.mapsellabel.SetLabel(_('Insert existing 3D raster map'))
        else:
            self.mapsellabel.SetLabel(_('Insert existing raster map'))
        self.mapselect = Select(
            parent=self.panel, id=wx.ID_ANY, size=(
                250, -1), type=element, multiple=False)
        self.functlabel = StaticText(parent=self.panel, id=wx.ID_ANY,
                                     label=_('Insert mapcalc function'))
        self.function = wx.ComboBox(
            parent=self.panel, id=wx.ID_ANY, size=(250, -1),
            choices=sorted(self.funct_dict.keys()),
            style=wx.CB_DROPDOWN | wx.CB_READONLY | wx.TE_PROCESS_ENTER)

        self.overwrite = wx.CheckBox(
            parent=self.panel, id=wx.ID_ANY,
            label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(
            UserSettings.Get(
                group='cmd',
                key='overwrite',
                subkey='enabled'))

        self.randomSeed = wx.CheckBox(
            parent=self.panel,
            label=_("Generate random seed for rand()"))
        self.randomSeedStaticText = StaticText(
            parent=self.panel, label=_("Seed:"))
        self.randomSeedText = TextCtrl(parent=self.panel, size=(100, -1),
                                       validator=IntegerValidator())
        self.randomSeedText.SetToolTip(
            _("Integer seed for rand() function"))
        self.randomSeed.SetValue(True)
        self.randomSeedStaticText.Disable()
        self.randomSeedText.Disable()

        self.addbox = wx.CheckBox(
            parent=self.panel,
            label=_('Add created raster map into layer tree'),
            style=wx.NO_BORDER)
        self.addbox.SetValue(
            UserSettings.Get(
                group='cmd',
                key='addNewLayer',
                subkey='enabled'))
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
        self.btn_copy.Bind(wx.EVT_BUTTON, self.OnCopyCommand)

        self.mapselect.Bind(wx.EVT_TEXT, self.OnSelect)
        self.function.Bind(wx.EVT_COMBOBOX, self._return_funct)
        self.function.Bind(wx.EVT_TEXT_ENTER, self.OnSelect)
        self.newmaptxt.Bind(wx.EVT_TEXT, self.OnUpdateStatusBar)
        self.text_mcalc.Bind(wx.EVT_TEXT, self.OnUpdateStatusBar)
        self.overwrite.Bind(wx.EVT_CHECKBOX, self.OnUpdateStatusBar)
        self.randomSeed.Bind(wx.EVT_CHECKBOX, self.OnUpdateStatusBar)
        self.randomSeed.Bind(wx.EVT_CHECKBOX, self.OnSeedFlag)
        self.randomSeedText.Bind(wx.EVT_TEXT, self.OnUpdateStatusBar)

        # bind closing to ESC
        self.Bind(wx.EVT_MENU, self.OnClose, id=wx.ID_CANCEL)
        accelTableList = [(wx.ACCEL_NORMAL, wx.WXK_ESCAPE, wx.ID_CANCEL)]
        accelTable = wx.AcceleratorTable(accelTableList)
        self.SetAcceleratorTable(accelTable)

        self._layout()

        self.SetMinSize(self.panel.GetBestSize())
        # workaround for http://trac.wxwidgets.org/ticket/13628
        self.SetSize(self.panel.GetBestSize())

    def _return_funct(self, event):
        i = event.GetString()
        self._addSomething(self.funct_dict[i])

        # reset
        win = self.FindWindowById(event.GetId())
        win.SetValue('')

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)

        controlSizer = wx.BoxSizer(wx.HORIZONTAL)
        operatorSizer = wx.StaticBoxSizer(self.operatorBox, wx.HORIZONTAL)
        outOpeSizer = wx.BoxSizer(wx.VERTICAL)

        buttonSizer1 = wx.GridBagSizer(5, 1)
        buttonSizer1.Add(self.btn['add'], pos=(0, 0))
        buttonSizer1.Add(self.btn['minus'], pos=(0, 1))
        buttonSizer1.Add(self.btn['mod'], pos=(5, 0))
        buttonSizer1.Add(self.btn['mult'], pos=(1, 0))
        buttonSizer1.Add(self.btn['div'], pos=(1, 1))
        buttonSizer1.Add(self.btn['pow'], pos=(5, 1))
        buttonSizer1.Add(self.btn['gt'], pos=(2, 0))
        buttonSizer1.Add(self.btn['gteq'], pos=(2, 1))
        buttonSizer1.Add(self.btn['eq'], pos=(4, 0))
        buttonSizer1.Add(self.btn['lt'], pos=(3, 0))
        buttonSizer1.Add(self.btn['lteq'], pos=(3, 1))
        buttonSizer1.Add(self.btn['noteq'], pos=(4, 1))

        buttonSizer2 = wx.GridBagSizer(5, 1)
        buttonSizer2.Add(self.btn['and'], pos=(0, 0))
        buttonSizer2.Add(self.btn['andbit'], pos=(1, 0))
        buttonSizer2.Add(self.btn['andnull'], pos=(2, 0))
        buttonSizer2.Add(self.btn['or'], pos=(0, 1))
        buttonSizer2.Add(self.btn['orbit'], pos=(1, 1))
        buttonSizer2.Add(self.btn['ornull'], pos=(2, 1))
        buttonSizer2.Add(self.btn['lshift'], pos=(3, 0))
        buttonSizer2.Add(self.btn['rshift'], pos=(3, 1))
        buttonSizer2.Add(self.btn['rshiftu'], pos=(4, 0))
        buttonSizer2.Add(self.btn['cond'], pos=(5, 0))
        buttonSizer2.Add(self.btn['compl'], pos=(5, 1))
        buttonSizer2.Add(self.btn['not'], pos=(4, 1))

        outputSizer = wx.StaticBoxSizer(self.outputBox, wx.VERTICAL)
        outputSizer.Add(self.newmaplabel,
                        flag=wx.ALIGN_CENTER | wx.TOP, border=5)
        outputSizer.Add(self.newmaptxt,
                        flag=wx.EXPAND | wx.ALL, border=5)

        operandSizer = wx.StaticBoxSizer(self.operandBox, wx.HORIZONTAL)

        buttonSizer3 = wx.GridBagSizer(7, 1)
        buttonSizer3.Add(self.functlabel, pos=(0, 0),
                         span=(1, 2), flag=wx.ALIGN_CENTER | wx.EXPAND)
        buttonSizer3.Add(self.function, pos=(1, 0),
                         span=(1, 2))
        buttonSizer3.Add(self.mapsellabel, pos=(2, 0),
                         span=(1, 2), flag=wx.ALIGN_CENTER)
        buttonSizer3.Add(self.mapselect, pos=(3, 0),
                         span=(1, 2))
        threebutton = wx.GridBagSizer(1, 2)
        threebutton.Add(self.btn['parenl'], pos=(0, 0),
                        span=(1, 1), flag=wx.ALIGN_LEFT)
        threebutton.Add(self.btn['parenr'], pos=(0, 1),
                        span=(1, 1), flag=wx.ALIGN_CENTER)
        threebutton.Add(self.btn_clear, pos=(0, 2),
                        span=(1, 1), flag=wx.ALIGN_RIGHT)
        buttonSizer3.Add(threebutton, pos=(4, 0),
                         span=(1, 1), flag=wx.ALIGN_CENTER)

        buttonSizer4 = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer4.Add(self.btn_load,
                         flag=wx.ALL, border=5)
        buttonSizer4.Add(self.btn_save,
                         flag=wx.ALL, border=5)
        buttonSizer4.Add(self.btn_copy,
                         flag=wx.ALL, border=5)
        buttonSizer4.AddSpacer(30)
        buttonSizer4.Add(self.btn_help,
                         flag=wx.ALL, border=5)
        buttonSizer4.Add(self.btn_run,
                         flag=wx.ALL, border=5)
        buttonSizer4.Add(self.btn_close,
                         flag=wx.ALL, border=5)

        operatorSizer.Add(buttonSizer1, proportion=0,
                          flag=wx.ALL | wx.EXPAND, border=5)
        operatorSizer.Add(
            buttonSizer2,
            proportion=0,
            flag=wx.TOP | wx.BOTTOM | wx.RIGHT | wx.EXPAND,
            border=5)

        operandSizer.Add(buttonSizer3, proportion=0,
                         flag=wx.ALL, border=5)

        controlSizer.Add(operatorSizer, proportion=1,
                         flag=wx.RIGHT | wx.EXPAND, border=5)
        outOpeSizer.Add(outputSizer, proportion=0,
                        flag=wx.EXPAND)
        outOpeSizer.Add(operandSizer, proportion=1,
                        flag=wx.EXPAND | wx.TOP, border=5)
        controlSizer.Add(outOpeSizer, proportion=0,
                         flag=wx.EXPAND)

        expressSizer = wx.StaticBoxSizer(self.expressBox, wx.HORIZONTAL)
        expressSizer.Add(self.text_mcalc, proportion=1,
                         flag=wx.EXPAND)

        sizer.Add(controlSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL,
                  border=5)
        sizer.Add(expressSizer, proportion=1,
                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
                  border=5)
        sizer.Add(buttonSizer4, proportion=0,
                  flag=wx.ALIGN_RIGHT | wx.ALL, border=3)

        randomSizer = wx.BoxSizer(wx.HORIZONTAL)
        randomSizer.Add(self.randomSeed, proportion=0,
                        flag=wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, border=20)
        randomSizer.Add(self.randomSeedStaticText, proportion=0,
                        flag=wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, border=5)
        randomSizer.Add(self.randomSeedText, proportion=0)
        sizer.Add(randomSizer, proportion=0,
                  flag=wx.LEFT | wx.RIGHT,
                  border=5)

        sizer.Add(self.overwrite, proportion=0,
                  flag=wx.LEFT | wx.RIGHT,
                  border=5)
        if self.addbox.IsShown():
            sizer.Add(self.addbox, proportion=0,
                      flag=wx.LEFT | wx.RIGHT,
                      border=5)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.Layout()

    def AddMark(self, event):
        """Sends operators to insertion method
        """
        if event.GetId() == self.btn['compl'].GetId():
            mark = "~"
        elif event.GetId() == self.btn['not'].GetId():
            mark = "!"
        elif event.GetId() == self.btn['pow'].GetId():
            mark = "^"
        elif event.GetId() == self.btn['div'].GetId():
            mark = "/"
        elif event.GetId() == self.btn['add'].GetId():
            mark = "+"
        elif event.GetId() == self.btn['minus'].GetId():
            mark = "-"
        elif event.GetId() == self.btn['mod'].GetId():
            mark = "%"
        elif event.GetId() == self.btn['mult'].GetId():
            mark = "*"
        elif event.GetId() == self.btn['lshift'].GetId():
            mark = "<<"
        elif event.GetId() == self.btn['rshift'].GetId():
            mark = ">>"
        elif event.GetId() == self.btn['rshiftu'].GetId():
            mark = ">>>"
        elif event.GetId() == self.btn['gt'].GetId():
            mark = ">"
        elif event.GetId() == self.btn['gteq'].GetId():
            mark = ">="
        elif event.GetId() == self.btn['lt'].GetId():
            mark = "<"
        elif event.GetId() == self.btn['lteq'].GetId():
            mark = "<="
        elif event.GetId() == self.btn['eq'].GetId():
            mark = "=="
        elif event.GetId() == self.btn['noteq'].GetId():
            mark = "!="
        elif event.GetId() == self.btn['andbit'].GetId():
            mark = "&"
        elif event.GetId() == self.btn['orbit'].GetId():
            mark = "|"
        elif event.GetId() == self.btn['or'].GetId():
            mark = "||"
        elif event.GetId() == self.btn['ornull'].GetId():
            mark = "|||"
        elif event.GetId() == self.btn['and'].GetId():
            mark = "&&"
        elif event.GetId() == self.btn['andnull'].GetId():
            mark = "&&&"
        elif event.GetId() == self.btn['cond'].GetId():
            mark = " ? : "
        elif event.GetId() == self.btn['parenl'].GetId():
            mark = "("
        elif event.GetId() == self.btn['parenr'].GetId():
            mark = ")"
        self._addSomething(mark)

    # unused
    # def OnSelectTextEvt(self, event):
    #     """Checks if user is typing or the event was emited by map selection.
    #     Prevents from changing focus.
    #     """
    #     item = self.mapselect.GetValue().strip()
    #     if not (abs(len(item) - len(self.lastMapName)) == 1 and \
    #         self.lastMapName in item or item in self.lastMapName):
    #         self.OnSelect(event)

    #     self.lastMapName = item

    def OnSelect(self, event):
        """Gets raster map or function selection and send it to
        insertion method.

        Checks for characters which can be in raster map name but
        the raster map name must be then quoted.
        """
        win = self.FindWindowById(event.GetId())
        item = win.GetValue().strip()
        if any((char in item) for char in self.charactersToQuote):
            item = '"' + item + '"'
        self._addSomething(item)

        win.ChangeValue('')  # reset
        # Map selector likes to keep focus. Set it back to expression input area
        wx.CallAfter(self.text_mcalc.SetFocus)

    def OnUpdateStatusBar(self, event):
        """Update statusbar text"""
        command = self._getCommand()
        self.SetStatusText(command)
        event.Skip()

    def OnSeedFlag(self, event):
        checked = self.randomSeed.IsChecked()
        self.randomSeedText.Enable(not checked)
        self.randomSeedStaticText.Enable(not checked)

        event.Skip()

    def _getCommand(self):
        """Returns entire command as string."""
        expr = self.text_mcalc.GetValue().strip().replace("\n", " ")
        cmd = 'r.mapcalc'
        if self.rast3d:
            cmd = 'r3.mapcalc'
        overwrite = ''
        if self.overwrite.IsChecked():
            overwrite = ' --overwrite'
        seed_flag = seed = ''
        if re.search(pattern="rand *\(.+\)", string=expr):
            if self.randomSeed.IsChecked():
                seed_flag = ' -s'
            else:
                seed = " seed={val}".format(
                    val=self.randomSeedText.GetValue().strip())

        return ('{cmd} expression="{new} = {expr}"{seed}{seed_flag}{overwrite}'
                .format(cmd=cmd, expr=expr, new=self.newmaptxt.GetValue(),
                        seed_flag=seed_flag, seed=seed, overwrite=overwrite))

    def _addSomething(self, what):
        """Inserts operators, map names, and functions into text area
        """
        mcalcstr = self.text_mcalc.GetValue()
        position = self.text_mcalc.GetInsertionPoint()

        newmcalcstr = mcalcstr[:position]

        position_offset = 0
        try:
            if newmcalcstr[-1] != ' ':
                newmcalcstr += ' '
                position_offset += 1
        except:
            pass

        newmcalcstr += what
        
        # Do not add extra space if there is already one
        try:
            if newmcalcstr[-1] != ' ' and mcalcstr[position] != ' ':
                newmcalcstr += ' '
        except:
            newmcalcstr += ' '
        
        newmcalcstr += mcalcstr[position:]
        
        self.text_mcalc.SetValue(newmcalcstr)
        if len(what) > 0:
            match = re.search(pattern="\(.*\)", string=what)
            if match:
                position_offset += match.start() + 1
            else:
                position_offset += len(what)
                try:
                    if newmcalcstr[position + position_offset] == ' ':
                        position_offset += 1
                except:
                    pass

        self.text_mcalc.SetInsertionPoint(position + position_offset)
        self.text_mcalc.Update()
        self.text_mcalc.SetFocus()

    def OnMCalcRun(self, event):
        """Builds and runs r.mapcalc statement
        """
        name = self.newmaptxt.GetValue().strip()
        if not name:
            GError(parent=self,
                   message=_("You must enter the name of "
                             "a new raster map to create."))
            return

        if not(name[0] == '"' and name[-1] == '"') and any((char in name)
                                                           for char in self.charactersToQuote):
            name = '"' + name + '"'

        expr = self.text_mcalc.GetValue().strip().replace("\n", " ")
        if not expr:
            GError(parent=self,
                   message=_("You must enter an expression "
                             "to create a new raster map."))
            return

        seed_flag = seed = None
        if re.search(pattern="rand *\(.+\)", string=expr):
            if self.randomSeed.IsChecked():
                seed_flag = '-s'
            else:
                seed = self.randomSeedText.GetValue().strip()
        if self.log:
            cmd = [self.cmd]
            if seed_flag:
                cmd.append('-s')
            if seed:
                cmd.append("seed={val}".format(val=seed))
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')
            cmd.append(str('expression=%s = %s' % (name, expr)))

            self.log.RunCmd(cmd, onDone=self.OnDone)
            self.parent.Raise()
        else:
            if self.overwrite.IsChecked():
                overwrite = True
            else:
                overwrite = False
            params = dict(expression="%s=%s" % (name, expr),
                          overwrite=overwrite)
            if seed_flag:
                params['flags'] = 's'
            if seed:
                params['seed'] = seed

            RunCommand(self.cmd,
                       **params)

    def OnDone(self, event):
        """Add create map to the layer tree

        Sends the mapCreated signal from the grass interface.
        """
        if event.returncode != 0:
            return
        name = self.newmaptxt.GetValue().strip(
            ' "') + '@' + grass.gisenv()['MAPSET']
        ltype = 'raster'
        if self.rast3d:
            ltype = 'raster_3d'
        self._giface.mapCreated.emit(
            name=name, ltype=ltype, add=self.addbox.IsChecked())
        gisenv = grass.gisenv()
        self._giface.grassdbChanged.emit(grassdb=gisenv['GISDBASE'],
                                         location=gisenv['LOCATION_NAME'],
                                         mapset=gisenv['MAPSET'],
                                         action='new',
                                         map=name.split('@')[0],
                                         element=ltype)

    def OnSaveExpression(self, event):
        """Saves expression to file
        """
        mctxt = self.newmaptxt.GetValue() + ' = ' + self.text_mcalc.GetValue() + os.linesep

        # dialog
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose a file name to save the expression"),
            wildcard=_("Expression file (*)|*"),
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT)
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
        """Load expression from file
        """
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose a file name to load the expression"),
            wildcard=_("Expression file (*)|*"),
            style=wx.FD_OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return

            try:
                fobj = open(path, 'r')
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

    def OnCopyCommand(self, event):
        command = self._getCommand()
        cmddata = wx.TextDataObject()
        cmddata.SetText(command)
        if wx.TheClipboard.Open():
            wx.TheClipboard.SetData(cmddata)
            wx.TheClipboard.Close()
            self.SetStatusText(
                _("'{cmd}' copied to clipboard").format(
                    cmd=command))

    def OnClear(self, event):
        """Clears text area
        """
        self.text_mcalc.SetValue('')

    def OnHelp(self, event):
        """Launches r.mapcalc help
        """
        RunCommand('g.manual', parent=self, entry=self.cmd)

    def OnClose(self, event):
        """Close window"""
        self.Destroy()

if __name__ == "__main__":

    app = wx.App(0)
    frame = MapCalcFrame(
        parent=None,
        cmd='r.mapcalc',
        giface=StandaloneGrassInterface())
    frame.Show()
    app.MainLoop()
