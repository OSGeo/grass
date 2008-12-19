"""
MODULE:    mcalc_builder.py

CLASSES:
    * MapCalcFrame

PURPOSE:   GRASS builds r.mapcalc statements

           Usage:
           mcalc_builder

AUTHOR(S): GRASS Development Team
           Original author: Michael Barton, Arizona State University

COPYRIGHT: (C) 2008 by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import wx
import os,sys
import time

import gcmd
import gselect
import subprocess

imagePath = os.path.join( os.getenv("GISBASE"), "etc", "wxpython")
sys.path.append(imagePath)
import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

class MapCalcFrame(wx.Frame):
    """
    Mapcalc Frame class. Calculator-style window to create
    and run r.mapcalc statements
    """
    def __init__(self, parent, id, title, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, style=wx.TAB_TRAVERSAL|wx.DEFAULT_FRAME_STYLE,
                 dimension=2):

        wx.Frame.__init__(self, parent, id, title, pos, size,
                          style)
        
        self.Centre(wx.BOTH)

        if dimension == 3:
            self.SetTitle(_("GRASS %s Map Calculator") % "3D" )
        else:
            self.SetTitle(_("GRASS %s Map Calculator") % "2D" )

        #
        # variables
        #
        self.parent = parent
        self.heading = 'mapcalc statement'
        self.newmap = ''
        self.dimension = dimension
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
        
        if self.dimension == 3:
            indx = self.funct_list.index('y()') +1
            self.funct_list.insert(indx, 'z()')
            indx = self.funct_list.index('nsres()') +1
            self.funct_list.insert(indx, 'tbres()')
            maplabel = 'volume'
            element = 'rast3d'
        else:
            maplabel = 'map'
            element = 'cell'

        #
        # Buttons
        #
        self.btn_clear = wx.Button(self, -1, "Clear")
        self.btn_help = wx.Button(self, -1, "Help")
        self.btn_run = wx.Button(self, -1, "Run")
        self.btn_run.SetDefault()
        self.btn_close = wx.Button(self, -1, "Close")

        self.btn_pow = wx.Button(self, -1, "^")
        self.btn_pow.SetToolTipString('exponent')
        self.btn_div = wx.Button(self, -1, "/")
        self.btn_div.SetToolTipString('divide')
        self.btn_add = wx.Button(self, -1, "+")
        self.btn_add.SetToolTipString('add')
        self.btn_minus = wx.Button(self, -1, "-")
        self.btn_minus.SetToolTipString('subtract')
        self.btn_mod = wx.Button(self, -1, "%")
        self.btn_mod.SetToolTipString('modulus')
        self.btn_mult = wx.Button(self, -1, "*")
        self.btn_mult.SetToolTipString('multiply')

        self.btn_paren = wx.Button(self, -1, "( )") 

        self.btn_lshift = wx.Button(self, -1, "<<")
        self.btn_lshift.SetToolTipString('left shift')
        self.btn_rshift = wx.Button(self, -1, ">>")
        self.btn_rshift.SetToolTipString('right shift')
        self.btn_rshiftu = wx.Button(self, -1, ">>>")
        self.btn_rshiftu.SetToolTipString('right shift (unsigned)')
        self.btn_gt = wx.Button(self, -1, ">")
        self.btn_gt.SetToolTipString('greater than')
        self.btn_gteq = wx.Button(self, -1, ">=")
        self.btn_gteq.SetToolTipString('greater than or equal to')
        self.btn_lt = wx.Button(self, -1, "<")
        self.btn_lt.SetToolTipString('less than or equal to')
        self.btn_lteq = wx.Button(self, -1, "<=")
        self.btn_lteq.SetToolTipString('less than')
        self.btn_eq = wx.Button(self, -1, "==")
        self.btn_eq.SetToolTipString('equal to')
        self.btn_noteq = wx.Button(self, -1, "!=")
        self.btn_noteq.SetToolTipString('not equal to')

        self.btn_compl = wx.Button(self, -1, "~")
        self.btn_compl.SetToolTipString("one's complement")
        self.btn_not = wx.Button(self, -1, "!")
        self.btn_not.SetToolTipString("NOT")
        self.btn_andbit = wx.Button(self, -1, '&')
        self.btn_andbit.SetToolTipString("bitwise AND")
        self.btn_orbit = wx.Button(self, -1, "|")
        self.btn_orbit.SetToolTipString("bitwise OR")
        self.btn_and = wx.Button(self, -1, "&&&")
        self.btn_and.SetToolTipString('logical AND')
        self.btn_andnull = wx.Button(self, -1, "&&&&&")
        self.btn_andnull.SetToolTipString('logical AND (ignores NULLs')
        self.btn_or = wx.Button(self, -1, "||")
        self.btn_or.SetToolTipString('logical OR')
        self.btn_ornull = wx.Button(self, -1, "|||")
        self.btn_ornull.SetToolTipString('logical OR (ignores NULLs')
        self.btn_cond = wx.Button(self, -1, "?:") 
        self.btn_cond.SetToolTipString('conditional')
        
        #
        # Text area
        #
        self.text_mcalc = wx.TextCtrl(self, -1, '', size=(-1,75),style=wx.TE_MULTILINE)
        
        #
        # Map and function insertion text and ComboBoxes
        self.newmaplabel = wx.StaticText(self, -1, 'Name of new %s to create' % maplabel)
        self.newmaptxt = wx.TextCtrl(self, wx.ID_ANY, size=(250,-1))
        self.mapsellabel = wx.StaticText(self, -1, 'Insert existing %s' % maplabel)
        self.mapselect = gselect.Select(self, wx.ID_ANY, size=(250,-1),
                                        type=element, multiple=False)
        self.functlabel = wx.StaticText(self, -1, 'Insert mapcalc function')
        self.function = wx.ComboBox(self, wx.ID_ANY, "", (-1,-1), 
                         (250, -1), self.funct_list, wx.CB_DROPDOWN
                         | wx.CB_READONLY
                         | wx.TE_PROCESS_ENTER
                         #| wx.CB_SORT
                         )
        #
        # Bindings
        #
        self.btn_compl.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_not.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_pow.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_div.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_add.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_minus.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_mod.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_mult.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_lshift.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_rshift.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_rshiftu.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_gt.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_gteq.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_lt.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_lteq.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_eq.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_noteq.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_andbit.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_orbit.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_and.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_andnull.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_or.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_ornull.Bind(wx.EVT_BUTTON, self.AddMark)        
        self.btn_cond.Bind(wx.EVT_BUTTON, self.AddMark)
        self.btn_paren.Bind(wx.EVT_BUTTON, self.AddMark)
        
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_clear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnMCalcRun)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        
        self.newmaptxt.Bind(wx.EVT_TEXT, self.OnNewmap)
        
        #self.mapselect.Bind(wx.EVT_COMBOBOX, self.OnSelect)
        self.mapselect.Bind(wx.EVT_TEXT, self.OnSelect)
        #self.mapselect.Bind(wx.EVT_TEXT_ENTER, self.OnSelect)
        self.function.Bind(wx.EVT_COMBOBOX, self.OnSelect)
        #self.function.Bind(wx.EVT_TEXT, self.OnSelect)
        self.function.Bind(wx.EVT_TEXT_ENTER, self.OnSelect)

        self.__doLayout()

    def __doLayout(self):
        pagesizer = wx.BoxSizer(wx.VERTICAL)
        
        controlsizer = wx.BoxSizer(wx.HORIZONTAL)
        btnpanelsizer = wx.GridBagSizer(1,1)

        buttonsizer1 = wx.GridBagSizer(5,1)
        buttonsizer1.Add(self.btn_add, (0,0))
        buttonsizer1.Add(self.btn_minus, (0,1))
        buttonsizer1.Add(self.btn_mod, (5,0))
        buttonsizer1.Add(self.btn_mult, (1,0))
        buttonsizer1.Add(self.btn_div, (1,1))
        buttonsizer1.Add(self.btn_pow, (5,1))
        buttonsizer1.Add(self.btn_gt, (2,0))
        buttonsizer1.Add(self.btn_gteq, (2,1))
        buttonsizer1.Add(self.btn_eq, (4,0))
        buttonsizer1.Add(self.btn_lt, (3,0))
        buttonsizer1.Add(self.btn_lteq, (3,1))
        buttonsizer1.Add(self.btn_noteq, (4,1))

        buttonsizer2 = wx.GridBagSizer(5,1)
        buttonsizer2.Add(self.btn_and, (0,0))
        buttonsizer2.Add(self.btn_andbit, (1,0))
        buttonsizer2.Add(self.btn_andnull, (2,0))
        buttonsizer2.Add(self.btn_or, (0,1))
        buttonsizer2.Add(self.btn_orbit, (1,1))
        buttonsizer2.Add(self.btn_ornull, (2,1))
        buttonsizer2.Add(self.btn_lshift, (3,0))
        buttonsizer2.Add(self.btn_rshift, (3,1))
        buttonsizer2.Add(self.btn_rshiftu, (4,0))
        buttonsizer2.Add(self.btn_cond, (5,0))
        buttonsizer2.Add(self.btn_compl, (5,1))
        buttonsizer2.Add(self.btn_not, (4,1))

        buttonsizer3 = wx.GridBagSizer(7, 1)
        buttonsizer3.Add(self.newmaplabel, (0,0), (1,2), wx.ALIGN_CENTER)
        buttonsizer3.Add(self.newmaptxt, (1,0), (1,2), wx.TOP, 4)
        buttonsizer3.Add(self.mapsellabel, (2,0), (1,2), wx.ALIGN_CENTER)
        buttonsizer3.Add(self.mapselect, (3,0), (1,2))
        buttonsizer3.Add(self.functlabel, (4,0), (1,2), wx.ALIGN_CENTER)
        buttonsizer3.Add(self.function, (5,0), (1,2))
        buttonsizer3.Add(self.btn_paren, (6,0), (1,1), wx.ALIGN_CENTER)
        buttonsizer3.Add(self.btn_clear, (6,1), (1,1), wx.ALIGN_CENTER)
        
        buttonsizer4 = wx.GridSizer(4, 3, 3, 3)
        buttonsizer4.Add(self.btn_run,0,wx.RIGHT,5)
        buttonsizer4.Add(self.btn_close,0,wx.RIGHT,5)
        buttonsizer4.Add(self.btn_help,0,wx.RIGHT,5)
        
        label = wx.StaticText(self, -1, 'Mapcalc operators')

        btnpanelsizer.Add(label, (0,0), (1,2), wx.ALIGN_CENTER)
        btnpanelsizer.Add(buttonsizer1, (1,0), (1,1), wx.RIGHT|wx.TOP, 5)
        btnpanelsizer.Add(buttonsizer2, (1,1), (1,1), wx.LEFT|wx.TOP, 5)

        controlsizer.Add(btnpanelsizer, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.RIGHT, border=20)
        controlsizer.Add(buttonsizer3, proportion=0)
        
        pagesizer.Add(controlsizer, flag=wx.EXPAND|wx.ALL,border=10)        
        pagesizer.Add(self.text_mcalc, flag=wx.EXPAND|wx.ALL,border=5)
        pagesizer.Add(buttonsizer4, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.ALL, border=5)
        self.SetAutoLayout(True)
        self.SetSizer(pagesizer)
        pagesizer.Fit(self)
        #pagesizer.SetSizeHints(self)
        self.Layout()
        self.Show(True)

    def AddMark(self,event):
        """
        Sends operators to insertion method
        """
        
        if event.GetId() == self.btn_compl.GetId(): mark = "~"
        elif event.GetId() == self.btn_not.GetId(): mark = "!"
        elif event.GetId() == self.btn_pow.GetId(): mark = "^"
        elif event.GetId() == self.btn_div.GetId(): mark = "/"
        elif event.GetId() == self.btn_add.GetId(): mark = "+"
        elif event.GetId() == self.btn_minus.GetId(): mark = "-"
        elif event.GetId() == self.btn_mod.GetId(): mark = "%"
        elif event.GetId() == self.btn_mult.GetId(): mark = "*"
        elif event.GetId() == self.btn_lshift.GetId(): mark = "<<"
        elif event.GetId() == self.btn_rshift.GetId(): mark = ">>"
        elif event.GetId() == self.btn_rshiftu.GetId(): mark = ">>>"
        elif event.GetId() == self.btn_gt.GetId(): mark = ">"
        elif event.GetId() == self.btn_gteq.GetId(): mark = ">="
        elif event.GetId() == self.btn_lt.GetId(): mark = "<"
        elif event.GetId() == self.btn_lteq.GetId(): mark = "<="
        elif event.GetId() == self.btn_eq.GetId(): mark = "=="
        elif event.GetId() == self.btn_noteq.GetId(): mark = "!="
        elif event.GetId() == self.btn_andbit.GetId(): mark = "&"
        elif event.GetId() == self.btn_orbit.GetId(): mark = "|"
        elif event.GetId() == self.btn_or.GetId(): mark =  "||"
        elif event.GetId() == self.btn_ornull.GetId(): mark = "|||"
        elif event.GetId() == self.btn_and.GetId(): mark = "&&"
        elif event.GetId() == self.btn_andnull.GetId(): mark = "&&&"
        elif event.GetId() == self.btn_cond.GetId(): mark = "?:"
        elif event.GetId() == self.btn_paren.GetId(): mark = "()"        
        self.__addSomething(mark)
        
    def OnNewmap(self, event):
        self.newmap = event.GetString()

    def OnSelect(self, event):
        """
        Gets raster map or function selection and send it to insertion method
        """
        item = event.GetString()
        self.__addSomething(item)
        self.text_mcalc.SetFocus()

    def __addSomething(self,what):
        """
        Inserts operators, map names, and functions into text area
        """
        self.text_mcalc.SetFocus()
        mcalcstr = self.text_mcalc.GetValue()
        newmcalcstr = ''
        position = self.text_mcalc.GetInsertionPoint()
        
        selection = self.text_mcalc.GetSelection()

        newmcalcstr = mcalcstr[:position]
        
        try:
            if newmcalcstr[-1] != " ":
                newmcalcstr += " "
        except:
            pass
        newmcalcstr += what
        position_offset = len(what)
        newmcalcstr += " "+mcalcstr[position:]

        self.text_mcalc.SetValue(newmcalcstr)
        self.text_mcalc.SetInsertionPoint(position+position_offset)
        self.text_mcalc.Update()

    def OnMCalcRun(self,event):
        """
        Builds and runs r.mapcalc statement
        """
        if self.newmap == '':
            wx.MessageBox("You must enter the name of a new map to create")
            return
        
        if self.text_mcalc.GetValue() == '':
            wx.MessageBox("You must enter a mapcalc statement to create a new map")
            return

        try:
            mctxt = self.text_mcalc.GetValue().strip().replace("\n"," ")
            mctxt = mctxt.replace(" ","")
            if self.dimension == 3:
                cmdlist = ["r3.mapcalc"," %s=%s" % (self.newmap,mctxt)]
            else:
                cmdlist = ["r.mapcalc"," %s=%s" % (self.newmap,mctxt)]

            p = gcmd.Command(cmdlist)
            if p.returncode == 0:
                wx.MessageBox("Map %s created successfully" % self.newmap)
        except:
            pass

    def OnClear(self, event):
        """
        Clears text area
        """
        self.text_mcalc.SetValue("")
        
    def OnHelp(self, event):
        """
        Launches r.mapcalc help
        """
        cmdlist = ['g.manual','r.mapcalc']
        gcmd.Command(cmdlist)

    def OnClose(self,event):
        self.Destroy()

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print >>sys.stderr, __doc__
        sys.exit()

    app = wx.App(0)
    sqlb = SQLFrame(None, -1, 'SQL Builder',sys.argv[1])
    app.MainLoop()


