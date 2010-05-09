"""
@package mcalc_builder.py

@brief Map calculator, wrapper for r.mapcalc

Classes:
 - MapCalcFrame

(C) 2008, 2010 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import time

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx

import gcmd
import gselect
try:
    import subprocess
except:
    sys.path.append(os.path.join(globalvar.ETCWXDIR, "compat"))
    import subprocess
from preferences import globalSettings as UserSettings

class MapCalcFrame(wx.Frame):
    """!Mapcalc Frame class. Calculator-style window to create and run
    r(3).mapcalc statements
    """
    def __init__(self, parent, id = wx.ID_ANY, title = _('Map calculator'), 
                 rast3d = False, **kwargs):
        self.parent = parent
        if self.parent:
            self.log = self.parent.GetLogWindow()
        else:
            self.log = None
        
        self.rast3d = rast3d
        wx.Frame.__init__(self, parent, id = id, title = title, **kwargs)

        #
        # variables
        #
        self.heading = _('mapcalc statement')
        self.funct_list = [
            'abs(x)',
            'acos(x)',
            'asin(x)',
            'atan(x)',
            'atan(x,y)',
            'cos(x)',
            'double(x)',
            'eval([x,y,...,]z)',
            'exp(x)',
            'exp(x,y)',
            'float(x)',
            'graph(x,x1,y1[x2,y2..])',
            'if(x)',
            'if(x,a)',
            'if(x,a,b)',
            'if(x,a,b,c)',
            'int(x)',
            'isnull(x)',
            'log(x)',
            'log(x,b)',
            'max(x,y[,z...])',
            'median(x,y[,z...])',
            'min(x,y[,z...])',
            'mode(x,y[,z...])',
            'not(x)',
            'pow(x,y)',
            'rand(a,b)',
            'round(x)',
            'sin(x)',
            'sqrt(x)',
            'tan(x)',
            'xor(x,y)',
            'row()',
            'col()',
            'x()',
            'y()',
            'ewres()',
            'nsres()',
            'null()'
            ]
        
        if self.rast3d:
            indx = self.funct_list.index('y()') +1
            self.funct_list.insert(indx, 'z()')
            indx = self.funct_list.index('nsres()') +1
            self.funct_list.insert(indx, 'tbres()')
            maplabel = _('3D raster map')
            element = 'rast3d'
        else:
            maplabel = _('raster map')
            element = 'cell'

        self.operatorBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                        label=" %s " % _('Operators'))
        self.operandBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                       label=" %s " % _('Operands'))
        self.expressBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                       label=" %s " % _('Expression'))
        
        #
        # Buttons
        #
        self.btn_clear = wx.Button(parent = self, id = wx.ID_CLEAR)
        self.btn_help = wx.Button(parent = self, id = wx.ID_HELP)
        self.btn_run = wx.Button(parent = self, id = wx.ID_ANY, label = _("&Run"))
        self.btn_run.SetDefault()
        self.btn_close = wx.Button(parent = self, id = wx.ID_CLOSE)

        self.btn = dict()        
        self.btn['pow'] = wx.Button(parent = self, id = wx.ID_ANY, label = "^")
        self.btn['pow'].SetToolTipString(_('exponent'))
        self.btn['div'] = wx.Button(parent = self, id = wx.ID_ANY, label = "/")
        self.btn['div'].SetToolTipString(_('divide'))
        self.btn['add'] = wx.Button(parent = self, id = wx.ID_ANY, label = "+")
        self.btn['add'].SetToolTipString(_('add'))
        self.btn['minus'] = wx.Button(parent = self, id = wx.ID_ANY, label = "-")
        self.btn['minus'].SetToolTipString(_('subtract'))
        self.btn['mod'] = wx.Button(parent = self, id = wx.ID_ANY, label = "%")
        self.btn['mod'].SetToolTipString(_('modulus'))
        self.btn['mult'] = wx.Button(parent = self, id = wx.ID_ANY, label = "*")
        self.btn['mult'].SetToolTipString(_('multiply'))

        self.btn['paren'] = wx.Button(parent = self, id = wx.ID_ANY, label = "( )") 

        self.btn['lshift'] = wx.Button(parent = self, id = wx.ID_ANY, label = "<<")
        self.btn['lshift'].SetToolTipString(_('left shift'))
        self.btn['rshift'] = wx.Button(parent = self, id = wx.ID_ANY, label = ">>")
        self.btn['rshift'].SetToolTipString(_('right shift'))
        self.btn['rshiftu'] = wx.Button(parent = self, id = wx.ID_ANY, label = ">>>")
        self.btn['rshiftu'].SetToolTipString(_('right shift (unsigned)'))
        self.btn['gt'] = wx.Button(parent = self, id = wx.ID_ANY, label = ">")
        self.btn['gt'].SetToolTipString(_('greater than'))
        self.btn['gteq'] = wx.Button(parent = self, id = wx.ID_ANY, label = ">=")
        self.btn['gteq'].SetToolTipString(_('greater than or equal to'))
        self.btn['lt'] = wx.Button(parent = self, id = wx.ID_ANY, label = "<")
        self.btn['lt'].SetToolTipString(_('less than or equal to'))
        self.btn['lteq'] = wx.Button(parent = self, id = wx.ID_ANY, label = "<=")
        self.btn['lteq'].SetToolTipString(_('less than'))
        self.btn['eq'] = wx.Button(parent = self, id = wx.ID_ANY, label = "==")
        self.btn['eq'].SetToolTipString(_('equal to'))
        self.btn['noteq'] = wx.Button(parent = self, id = wx.ID_ANY, label = "!=")
        self.btn['noteq'].SetToolTipString(_('not equal to'))

        self.btn['compl'] = wx.Button(parent = self, id = wx.ID_ANY, label = "~")
        self.btn['compl'].SetToolTipString(_('one\'s complement'))
        self.btn['not'] = wx.Button(parent = self, id = wx.ID_ANY, label = "!")
        self.btn['not'].SetToolTipString(_('NOT'))
        self.btn['andbit'] = wx.Button(parent = self, id = wx.ID_ANY, label = '&')
        self.btn['andbit'].SetToolTipString(_('bitwise AND'))
        self.btn['orbit'] = wx.Button(parent = self, id = wx.ID_ANY, label = "|")
        self.btn['orbit'].SetToolTipString(_('bitwise OR'))
        self.btn['and'] = wx.Button(parent = self, id = wx.ID_ANY, label = "&&&")
        self.btn['and'].SetToolTipString(_('logical AND'))
        self.btn['andnull'] = wx.Button(parent = self, id = wx.ID_ANY, label = "&&&&&")
        self.btn['andnull'].SetToolTipString(_('logical AND (ignores NULLs'))
        self.btn['or'] = wx.Button(parent = self, id = wx.ID_ANY, label = "||")
        self.btn['or'].SetToolTipString(_('logical OR'))
        self.btn['ornull'] = wx.Button(parent = self, id = wx.ID_ANY, label = "|||")
        self.btn['ornull'].SetToolTipString(_('logical OR (ignores NULLs'))
        self.btn['cond'] = wx.Button(parent = self, id = wx.ID_ANY, label = "?:") 
        self.btn['cond'].SetToolTipString(_('conditional'))
        
        #
        # Text area
        #
        self.text_mcalc = wx.TextCtrl(parent = self, id = wx.ID_ANY, size = (-1, 75),
                                      style = wx.TE_MULTILINE)
        
        #
        # Map and function insertion text and ComboBoxes
        self.newmaplabel = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label= _('Name for new %s to create') % maplabel)
        self.newmaptxt = wx.TextCtrl(parent = self, id = wx.ID_ANY, size=(250, -1))
        self.mapsellabel = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label = _('Insert existing %s') % maplabel)
        self.mapselect = gselect.Select(self, wx.ID_ANY, size = (250, -1),
                                        type = element, multiple = False)
        self.functlabel = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _('Insert mapcalc function'))
        self.function = wx.ComboBox(parent = self, id = wx.ID_ANY, 
                                    size = (250, -1), choices = self.funct_list,
                                    style = wx.CB_DROPDOWN |
                                    wx.CB_READONLY | wx.TE_PROCESS_ENTER)
        
        self.overwrite = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                     label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        
        #
        # Bindings
        #
        for btn in self.btn.keys():
            self.btn[btn].Bind(wx.EVT_BUTTON, self.AddMark)
        
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnMCalcRun)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        
        self.mapselect.Bind(wx.EVT_TEXT, self.OnSelect)
        self.function.Bind(wx.EVT_COMBOBOX, self.OnSelect)
        self.function.Bind(wx.EVT_TEXT_ENTER, self.OnSelect)
        
        self._layout()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        controlSizer = wx.BoxSizer(wx.HORIZONTAL)
        operatorSizer = wx.StaticBoxSizer(self.operatorBox, wx.HORIZONTAL)

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

        operandSizer = wx.StaticBoxSizer(self.operandBox, wx.HORIZONTAL)
        buttonSizer3 = wx.GridBagSizer(7, 1)
        buttonSizer3.Add(item = self.newmaplabel, pos = (0, 0),
                         span = (1, 2), flag = wx.ALIGN_CENTER)
        buttonSizer3.Add(item = self.newmaptxt, pos = (1,0),
                         span = (1, 2))
        buttonSizer3.Add(item = self.mapsellabel, pos = (2,0),
                         span = (1,2), flag = wx.ALIGN_CENTER)
        buttonSizer3.Add(item = self.mapselect, pos = (3,0),
                         span = (1,2))
        buttonSizer3.Add(item = self.functlabel, pos = (4,0),
                         span = (1,2), flag = wx.ALIGN_CENTER)
        buttonSizer3.Add(item = self.function, pos = (5,0),
                         span = (1,2))
        buttonSizer3.Add(item = self.btn['paren'], pos = (6, 0),
                         span = (1,1), flag = wx.ALIGN_LEFT)
        buttonSizer3.Add(item = self.btn_clear, pos = (6,1),
                         span = (1,1), flag = wx.ALIGN_RIGHT)
        
        buttonSizer4 = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer4.Add(item = self.btn_close,
                         flag = wx.ALL, border = 5)
        buttonSizer4.Add(item = self.btn_run,
                         flag = wx.ALL, border = 5)
        buttonSizer4.Add(item = self.btn_help,
                         flag = wx.ALL, border = 5)
        
        operatorSizer.Add(item = buttonSizer1, proportion = 0,
                          flag = wx.ALL, border = 5)
        operatorSizer.Add(item = buttonSizer2, proportion = 0,
                          flag = wx.TOP | wx.BOTTOM | wx.RIGHT, border = 5)
        
        operandSizer.Add(item = buttonSizer3, proportion = 0,
                         flag = wx.TOP | wx.BOTTOM | wx.RIGHT, border = 5)
        
        controlSizer.Add(item = operatorSizer, proportion = 0,
                         flag = wx.RIGHT, border = 5)
        controlSizer.Add(item = operandSizer, proportion = 0)

        expressSizer = wx.StaticBoxSizer(self.expressBox, wx.HORIZONTAL)
        expressSizer.Add(item = self.text_mcalc, proportion = 1)

        sizer.Add(item = controlSizer, flag = wx.EXPAND | wx.ALL,
                      border = 5)        
        sizer.Add(item = expressSizer, flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                      border = 5)
        sizer.Add(item = self.overwrite, flag = wx.EXPAND | wx.LEFT | wx.RIGHT,
                      border = 5)
        sizer.Add(item = buttonSizer4, proportion = 0,
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 1)
        
        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)
        
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
        elif event.GetId() == self.btn['cond'].GetId(): mark = "?:"
        elif event.GetId() == self.btn['paren'].GetId(): mark = "()"        
        self._addSomething(mark)
        
    def OnSelect(self, event):
        """!Gets raster map or function selection and send it to
        insertion method
        """
        item = event.GetString()
        self._addSomething(item)
        
    def _addSomething(self,what):
        """!Inserts operators, map names, and functions into text area
        """
        self.text_mcalc.SetFocus()
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
        
        newmcalcstr += what
        position_offset += len(what)
        newmcalcstr += ' ' + mcalcstr[position:]
        
        self.text_mcalc.SetValue(newmcalcstr)
        self.text_mcalc.SetInsertionPoint(position + position_offset)
        self.text_mcalc.Update()
        
    def OnMCalcRun(self,event):
        """!Builds and runs r.mapcalc statement
        """
        name = self.newmaptxt.GetValue().strip()
        if not name:
            gcmd.GMessage(parent = self,
                          message = _("You must enter the name of a new map to create"),
                          msgType = 'info')
            return
        
        if not self.text_mcalc.GetValue().strip():
            gcmd.GMessage(parent = self,
                          message = _("You must enter a mapcalc statement to create a new map"),
                          msgType = 'info')
            return
        
        mctxt = self.text_mcalc.GetValue().strip().replace("\n"," ")
        mctxt = mctxt.replace(" " , "")
        if self.rast3d:
            prg = 'r3.mapcalc'
        else:
            prg = 'r.mapcalc'

        if self.log:
            cmd = [prg, str('expression=%s = %s' % (name, mctxt))]
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')
            self.log.RunCmd(cmd)
            self.parent.Raise()
        else:
            if self.overwrite.IsChecked():
                overwrite = True
            else:
                overwrite = False
            gcmd.RunCommand(prg,
                            expression = "%s=%s" % (name, mctxt),
                            overwrite = overwrite)
        
    def OnClear(self, event):
        """!Clears text area
        """
        self.text_mcalc.SetValue('')
        
    def OnHelp(self, event):
        """!Launches r.mapcalc help
        """
        gcmd.RunCommand('g.manual', entry = 'r.mapcalc')
        
    def OnClose(self,event):
        """!Close window"""
        self.Destroy()

if __name__ == "__main__":
    app = wx.App(0)
    frame = MapCalcFrame(None)
    frame.Show()
    app.MainLoop()

